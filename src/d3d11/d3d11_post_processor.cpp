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
	D3D11PostProcessor::D3D11PostProcessor(ComPtr<ID3D11Device> device) : device(device) {
		device->GetImmediateContext(context.GetAddressOf());
	}

	bool D3D11PostProcessor::Apply(const D3D11PostProcessInput &input, Viewport &outputViewport) {
		bool didPostprocessing = false;

		if (g_config.debugMode) {
			StartProfiling();
		}

		if (g_config.upscaling.enabled) {
			try {
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

				didPostprocessing = true;
			}
			catch (const std::exception &e) {
				LOG_ERROR << "Upscaling failed: " << e.what();
				g_config.upscaling.enabled = false;
			}
		}

		if (g_config.debugMode) {
			EndProfiling();
		}

		if (g_config.captureOutput && input.eye == 0) {
			SaveTextureToFile(didPostprocessing ? input.outputTexture : input.inputTexture);
		}

		return didPostprocessing;
	}

	bool D3D11PostProcessor::PrePSSetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState * const *ppSamplers) {
		if (!g_config.upscaling.applyMipBias) {
			passThroughSamplers.clear();
			mappedSamplers.clear();
			return false;
		}

		ID3D11SamplerState *samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
		memcpy(samplers, ppSamplers, numSamplers * sizeof(ID3D11SamplerState*));
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

		context->PSSetSamplers(startSlot, numSamplers, samplers);
		return true;
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

		HRESULT result = DirectX::SaveDDSTextureToFile( context.Get(), texture, filePath.c_str() );
		if (FAILED(result)) {
			LOG_ERROR << "Error taking screen capture: " << std::hex << result << std::dec;
		}
	}

	void D3D11PostProcessor::CreateProfileQueries() {
		for (auto &profileQuery : profileQueries) {
			D3D11_QUERY_DESC qd;
			qd.Query = D3D11_QUERY_TIMESTAMP;
			qd.MiscFlags = 0;
			device->CreateQuery(&qd, profileQuery.queryStart.ReleaseAndGetAddressOf());
			device->CreateQuery(&qd, profileQuery.queryEnd.ReleaseAndGetAddressOf());
			qd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			device->CreateQuery(&qd, profileQuery.queryDisjoint.ReleaseAndGetAddressOf());
		}
	}

	void D3D11PostProcessor::StartProfiling() {
		if (profileQueries[0].queryStart == nullptr) {
			CreateProfileQueries();
		}

		context->Begin(profileQueries[currentQuery].queryDisjoint.Get());
		context->End(profileQueries[currentQuery].queryStart.Get());
	}

	void D3D11PostProcessor::EndProfiling() {
		context->End(profileQueries[currentQuery].queryEnd.Get());
		context->End(profileQueries[currentQuery].queryDisjoint.Get());

		currentQuery = (currentQuery + 1) % QUERY_COUNT;
		while (context->GetData(profileQueries[currentQuery].queryDisjoint.Get(), nullptr, 0, 0) == S_FALSE) {
			Sleep(1);
		}
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
		HRESULT result = context->GetData(profileQueries[currentQuery].queryDisjoint.Get(), &disjoint, sizeof(disjoint), 0);
		if (result == S_OK && !disjoint.Disjoint) {
			UINT64 begin, end;
			context->GetData(profileQueries[currentQuery].queryStart.Get(), &begin, sizeof(UINT64), 0);
			context->GetData(profileQueries[currentQuery].queryEnd.Get(), &end, sizeof(UINT64), 0);
			float duration = (end - begin) / float(disjoint.Frequency);
			summedGpuTime += duration;
			++countedQueries;

			if (countedQueries >= 500) {
				float avgTimeMs = 1000.f / countedQueries * summedGpuTime;
				// queries are done per eye, but we want the average for both eyes per frame
				avgTimeMs *= 2;
				LOG_INFO << "Average GPU processing time for post-processing: " << avgTimeMs << " ms";
				countedQueries = 0;
				summedGpuTime = 0.f;
			}
		}
	}
}
