#include "d3d11_post_processor.h"

#include "config.h"
#include "d3d11_cas_upscaler.h"
#include "d3d11_fsr_upscaler.h"
#include "d3d11_nis_upscaler.h"
#include "logging.h"
#include "hooks.h"
#include "ScreenGrab11.h"

#include <sstream>

namespace vrperfkit {
	namespace {
		void D3D11ContextHook_PSSetSamplers(ID3D11DeviceContext *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState * const *ppSamplers) {
			ID3D11SamplerState *samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
			memcpy(samplers, ppSamplers, NumSamplers * sizeof(ID3D11SamplerState*));

			D3D11PostProcessor *postProcessor = nullptr;
			UINT size = sizeof(postProcessor);
			if (SUCCEEDED(self->GetPrivateData(__uuidof(D3D11PostProcessor), &size, &postProcessor)) && postProcessor != nullptr) {
				postProcessor->OnPSSetSamplers(samplers, NumSamplers);
			}

			hooks::CallOriginal(D3D11ContextHook_PSSetSamplers)(self, StartSlot, NumSamplers, samplers);
		}
	}

	D3D11PostProcessor::D3D11PostProcessor(ComPtr<ID3D11Device> device) : device(device) {}

	D3D11PostProcessor::~D3D11PostProcessor() {
		ComPtr<ID3D11DeviceContext> context;
		device->GetImmediateContext(context.GetAddressOf());
		context->SetPrivateData(__uuidof(D3D11PostProcessor), 0, nullptr);
	}

	bool D3D11PostProcessor::Apply(const D3D11PostProcessInput &input, Viewport &outputViewport) {
		if (g_config.upscaling.enabled) {
			try {
				ComPtr<ID3D11DeviceContext> context;
				device->GetImmediateContext(context.GetAddressOf());
				D3D11State previousState;
				StoreD3D11State(context.Get(), previousState);

				// disable any RTs in case our input texture is still bound; otherwise using it as a view will fail
				context->OMSetRenderTargets(0, nullptr, nullptr);

				PrepareUpscaler(input.outputTexture);
				D3D11_TEXTURE2D_DESC td;
				input.outputTexture->GetDesc(&td);
				outputViewport.x = outputViewport.y = 0;
				outputViewport.width = td.Width;
				outputViewport.height = td.Height;
				if (input.mode == TextureMode::COMBINED) {
					outputViewport.width /= 2;
					if (input.eye == RIGHT_EYE) {
						outputViewport.x += outputViewport.width;
					}
				}
				upscaler->Upscale(input, outputViewport);

				float newLodBias = -log2f(outputViewport.width / (float)input.inputViewport.width);
				if (newLodBias != mipLodBias) {
					LOG_DEBUG << "MIP LOD Bias changed from " << mipLodBias << " to " << newLodBias << ", recreating samplers";
					passThroughSamplers.clear();
					mappedSamplers.clear();
					mipLodBias = newLodBias;
				}

				RestoreD3D11State(context.Get(), previousState);

				if (g_config.captureOutput && input.eye == 0) {
					SaveTextureToFile(input.outputTexture);
				}

				return true;
			}
			catch (const std::exception &e) {
				LOG_ERROR << "Upscaling failed: " << e.what();
				g_config.upscaling.enabled = false;
			}
		}
		if (g_config.captureOutput && input.eye == 0) {
			SaveTextureToFile(input.inputTexture);
		}
		return false;
	}

	void D3D11PostProcessor::OnPSSetSamplers(ID3D11SamplerState **samplers, UINT numSamplers) {
		if (!g_config.upscaling.applyMipBias) {
			passThroughSamplers.clear();
			mappedSamplers.clear();
			return;
		}

		for (UINT i = 0; i < numSamplers; ++i) {
			ID3D11SamplerState *orig = samplers[i];
			if (orig == nullptr || passThroughSamplers.find(orig) != passThroughSamplers.end()) {
				continue;
			}

			if (mappedSamplers.find(orig) == mappedSamplers.end()) {
				D3D11_SAMPLER_DESC sd;
				orig->GetDesc(&sd);
				if (sd.MipLODBias != 0 || sd.MaxAnisotropy == 1) {
					// do not mess with samplers that already have a bias or are not doing anisotropic filtering.
					// should hopefully reduce the chance of causing rendering errors.
					passThroughSamplers.insert(orig);
					continue;
				}
				sd.MipLODBias = mipLodBias;
				LOG_INFO << "Creating replacement sampler for " << orig << " with MIP LOD bias " << sd.MipLODBias;
				device->CreateSamplerState(&sd, mappedSamplers[orig].GetAddressOf());
				passThroughSamplers.insert(mappedSamplers[orig].Get());
			}

			samplers[i] = mappedSamplers[orig].Get();
		}
	}

	void D3D11PostProcessor::PrepareUpscaler(ID3D11Texture2D *outputTexture) {
		if (upscaler == nullptr || upscaleMethod != g_config.upscaling.method) {
			D3D11_TEXTURE2D_DESC td;
			outputTexture->GetDesc(&td);
			upscaleMethod = g_config.upscaling.method;
			switch (upscaleMethod) {
			case UpscaleMethod::FSR:
				upscaler.reset(new D3D11FsrUpscaler(device.Get(), td.Width, td.Height, td.Format));
				break;
			case UpscaleMethod::NIS:
				upscaler.reset(new D3D11NisUpscaler(device.Get()));
				break;
			case UpscaleMethod::CAS:
				upscaler.reset(new D3D11CasUpscaler(device.Get()));
				break;
			}

			passThroughSamplers.clear();
			mappedSamplers.clear();
			ComPtr<ID3D11DeviceContext> context;
			device->GetImmediateContext(context.GetAddressOf());
			D3D11PostProcessor *instance = this;
			UINT size = sizeof(instance);
			context->SetPrivateData(__uuidof(D3D11PostProcessor), size, &instance);
			hooks::InstallVirtualFunctionHook("PSSetSamplers", context.Get(), 10, (void*)&D3D11ContextHook_PSSetSamplers);
		}
	}

	extern std::filesystem::path g_basePath;
	void D3D11PostProcessor::SaveTextureToFile(ID3D11Texture2D *texture) {
		g_config.captureOutput = false;

		static char timeBuf[16];
		std::time_t now = std::time(nullptr);
		std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", std::localtime(&now));

		std::wostringstream filename;
		filename << "capture_" << timeBuf
				 << "_" << MethodToString(g_config.upscaling.method).c_str()
				 << "_s" << int(roundf(g_config.upscaling.sharpness * 100))
				 << "_r" << int(roundf(g_config.upscaling.radius * 100))
				 << ".dds";
		std::filesystem::path filePath = g_basePath / filename.str();

		ComPtr<ID3D11DeviceContext> context;
		device->GetImmediateContext(context.GetAddressOf());
		HRESULT result = DirectX::SaveDDSTextureToFile( context.Get(), texture, filePath.c_str() );
		if (FAILED(result)) {
			LOG_ERROR << "Error taking screen capture: " << std::hex << result << std::dec;
		}
	}
}
