#include "oculus_manager.h"

#include "hotkeys.h"
#include "logging.h"
#include "resolution_scaling.h"
#include "d3d11/d3d11_helper.h"
#include "d3d11/d3d11_post_processor.h"

#include <wrl/client.h>
#include <d3d11.h>
#include <OVR_CAPI_D3D.h>

using Microsoft::WRL::ComPtr;

namespace vrperfkit {
	namespace {
		void Check(const std::string &action, ovrResult result) {
			if (OVR_FAILURE(result)) {
				ovrErrorInfo info;
				ovr_GetLastErrorInfo(&info);
				std::string message = "Failed " + action + ": " + info.ErrorString;
				throw std::exception(message.c_str());
			}
		}

	}

	OculusManager g_oculus;

	struct OculusD3D11Resources {
		std::unique_ptr<D3D11PostProcessor> postProcessor;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		std::vector<ComPtr<ID3D11Texture2D>> submittedTextures[2];
		ComPtr<ID3D11Texture2D> resolveTexture[2];
		std::vector<ComPtr<ID3D11ShaderResourceView>> submittedViews[2];
		std::vector<ComPtr<ID3D11Texture2D>> outputTextures[2];
		std::vector<ComPtr<ID3D11ShaderResourceView>> outputViews[2];
		std::vector<ComPtr<ID3D11UnorderedAccessView>> outputUavs[2];
		bool multisampled[2];
		bool usingArrayTex;
	};

	void OculusManager::Init(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain) {
		this->session = session;
		submittedEyeChains[0] = leftEyeChain;
		submittedEyeChains[1] = rightEyeChain;

		LOG_INFO << "Initializing Oculus frame submission...";

		try {
			// determine which graphics API the swapchains are created with
			ComPtr<ID3D11Texture2D> d3d11Tex;

			ovrResult result = ovr_GetTextureSwapChainBufferDX(session, leftEyeChain, 0, IID_PPV_ARGS(d3d11Tex.GetAddressOf()));
			if (OVR_SUCCESS(result)) {
				InitD3D11();
			}
		}
		catch (const std::exception &e) {
			LOG_ERROR << "Failed to create graphics resources: " << e.what();
		}

		if (!initialized) {
			LOG_ERROR << "Could not initialize graphics resources; game may be using an unsupported graphics API";
			Shutdown();
			failed = true;
		}

		FlushLog();
	}

	void OculusManager::Shutdown() {
		initialized = false;
		failed = false;
		graphicsApi = GraphicsApi::UNKNOWN;
		d3d11Res.reset();
		for (int i = 0; i < 2; ++i) {
			if (outputEyeChains[i] != nullptr) {
				ovr_DestroyTextureSwapChain(session, outputEyeChains[i]);
			}
			submittedEyeChains[i] = nullptr;
			outputEyeChains[i] = nullptr;
		}
		session = nullptr;
	}

	void OculusManager::EnsureInit(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain) {
		if (!initialized || session != this->session || leftEyeChain != submittedEyeChains[0] || rightEyeChain != submittedEyeChains[1]) {
			Shutdown();
			Init(session, leftEyeChain, rightEyeChain);
		}
	}

	void OculusManager::OnFrameSubmission(ovrSession session, ovrLayerEyeFovDepth &eyeLayer) {
		if (failed || session == nullptr || eyeLayer.ColorTexture[0] == nullptr) {
			return;
		}
		EnsureInit(session, eyeLayer.ColorTexture[0], eyeLayer.ColorTexture[1]);
		if (failed) {
			return;
		}

		try {
			if (graphicsApi == GraphicsApi::D3D11) {
				PostProcessD3D11(eyeLayer);
			}

			CheckHotkeys();
		}
		catch (const std::exception &e) {
			LOG_ERROR << "Failed during post processing: " << e.what();
			Shutdown();
			failed = true;
		}
	}

	ProjectionCenters OculusManager::CalculateProjectionCenter(const ovrFovPort *fov) {
		ProjectionCenters projCenters;
		for (int eye = 0; eye < 2; ++eye) {
			projCenters.eyeCenter[eye].x = 0.5f * (1.f + (fov[eye].LeftTan - fov[eye].RightTan) / (fov[eye].RightTan + fov[eye].LeftTan));
			projCenters.eyeCenter[eye].y = 0.5f * (1.f + (fov[eye].DownTan - fov[eye].UpTan) / (fov[eye].DownTan + fov[eye].UpTan));
		}
		return projCenters;
	}

	void OculusManager::InitD3D11() {
		LOG_INFO << "Game is using D3D11 swapchains, initializing D3D11 resources";
		graphicsApi = GraphicsApi::D3D11;
		d3d11Res.reset(new OculusD3D11Resources);

		for (int eye = 0; eye < 2; ++eye) {
			d3d11Res->multisampled[eye] = false;
			if (submittedEyeChains[eye] == nullptr)
				continue;

			int length = 0;
			Check("getting texture swapchain length", ovr_GetTextureSwapChainLength(session, submittedEyeChains[eye], &length));
			for (int i = 0; i < length; ++i) {
				ComPtr<ID3D11Texture2D> texture;
				Check("getting swapchain texture", ovr_GetTextureSwapChainBufferDX(session, submittedEyeChains[eye], i, IID_PPV_ARGS(texture.GetAddressOf())));
				d3d11Res->submittedTextures[eye].push_back(texture);
			}
			d3d11Res->submittedTextures[eye][0]->GetDevice(d3d11Res->device.ReleaseAndGetAddressOf());
			d3d11Res->device->GetImmediateContext(d3d11Res->context.ReleaseAndGetAddressOf());

			ovrTextureSwapChainDesc chainDesc;
			Check("getting swapchain description", ovr_GetTextureSwapChainDesc(session, submittedEyeChains[eye], &chainDesc));
			if (chainDesc.SampleCount > 1) {
				LOG_INFO << "Submitted textures are multi-sampled, creating resolve texture";
				d3d11Res->resolveTexture[eye] = CreateResolveTexture(d3d11Res->device.Get(), d3d11Res->submittedTextures[0][0].Get());
				d3d11Res->multisampled[eye] = true;
			}

			for (int i = 0; i < length; ++i) {
				auto view = CreateShaderResourceView(d3d11Res->device.Get(), 
					chainDesc.SampleCount > 1 
						? d3d11Res->resolveTexture[eye].Get()
						: d3d11Res->submittedTextures[eye][i].Get());
				d3d11Res->submittedViews[eye].push_back(view);
			}

			chainDesc.SampleCount = 1;
			chainDesc.MipLevels = 1;
			chainDesc.BindFlags = ovrTextureBind_DX_UnorderedAccess;
			// fixme: may need some adjustments from actual types
			chainDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
			chainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM;
			chainDesc.StaticImage = false;
			LOG_INFO << "Eye " << eye << ": submitted textures have resolution " << chainDesc.Width << "x" << chainDesc.Height;
			AdjustOutputResolution(chainDesc.Width, chainDesc.Height);
			LOG_INFO << "Eye " << eye << ": output resolution is " << chainDesc.Width << "x" << chainDesc.Height;
			Check("creating output swapchain", ovr_CreateTextureSwapChainDX(session, d3d11Res->device.Get(), &chainDesc, &outputEyeChains[eye]));

			Check("getting texture swapchain length", ovr_GetTextureSwapChainLength(session, outputEyeChains[eye], &length));
			for (int i = 0; i < length; ++i) {
				ComPtr<ID3D11Texture2D> texture;
				Check("getting swapchain texture", ovr_GetTextureSwapChainBufferDX(session, outputEyeChains[eye], i, IID_PPV_ARGS(texture.GetAddressOf())));
				d3d11Res->outputTextures[eye].push_back(texture);

				auto view = CreateShaderResourceView(d3d11Res->device.Get(), texture.Get());
				d3d11Res->outputViews[eye].push_back(view);

				auto uav = CreateUnorderedAccessView(d3d11Res->device.Get(), texture.Get());
				d3d11Res->outputUavs[eye].push_back(uav);
			}
		}

		d3d11Res->usingArrayTex = false;

		if (submittedEyeChains[1] == nullptr) {
			LOG_INFO << "Game is using a single texture for both eyes";
			d3d11Res->submittedTextures[1] = d3d11Res->submittedTextures[0];
			d3d11Res->resolveTexture[1] = d3d11Res->resolveTexture[0];
			d3d11Res->outputTextures[1] = d3d11Res->outputTextures[0];
			ovrTextureSwapChainDesc chainDesc;
			Check("getting swapchain description", ovr_GetTextureSwapChainDesc(session, submittedEyeChains[0], &chainDesc));
			if (chainDesc.ArraySize == 1) {
				d3d11Res->submittedViews[1] = d3d11Res->submittedViews[0];
				d3d11Res->outputViews[1] = d3d11Res->outputViews[0];
				d3d11Res->outputUavs[1] = d3d11Res->outputUavs[0];
			}
			else {
				LOG_INFO << "Game is using an array texture";
				d3d11Res->usingArrayTex = true;
				for (auto tex : d3d11Res->submittedTextures[0]) {
					auto view = CreateShaderResourceView(d3d11Res->device.Get(), tex.Get(), 1);
					d3d11Res->submittedViews[1].push_back(view);
				}
				for (auto tex : d3d11Res->outputTextures[0]) {
					auto resolvedTex = d3d11Res->resolveTexture[1] != nullptr ? d3d11Res->resolveTexture[1] : tex;
					auto view = CreateShaderResourceView(d3d11Res->device.Get(), resolvedTex.Get(), 1);
					d3d11Res->outputViews[1].push_back(view);
					auto uav = CreateUnorderedAccessView(d3d11Res->device.Get(), resolvedTex.Get(), 1);
					d3d11Res->outputUavs[1].push_back(uav);
				}
			}
		}

		d3d11Res->postProcessor.reset(new D3D11PostProcessor(d3d11Res->device));

		LOG_INFO << "D3D11 resource creation complete";
		initialized = true;
	}

	void OculusManager::PostProcessD3D11(ovrLayerEyeFovDepth &eyeLayer) {
		auto projCenters = CalculateProjectionCenter(eyeLayer.Fov);

		for (int eye = 0; eye < 2; ++eye) {
			int index;
			Check("getting current swapchain index", ovr_GetTextureSwapChainCurrentIndex(session, submittedEyeChains[eye], &index));
			// since the current submitted texture has already been committed, the index will point past the current texture
			index = (index - 1 + d3d11Res->submittedTextures[eye].size()) % d3d11Res->submittedTextures[eye].size();

			// if the incoming texture is multi-sampled, we need to resolve it before we can post-process it
			if (d3d11Res->multisampled[eye]) {
				if (d3d11Res->usingArrayTex || submittedEyeChains[eye] != nullptr) {
					D3D11_TEXTURE2D_DESC td;
					d3d11Res->submittedTextures[eye][index]->GetDesc(&td);
					d3d11Res->context->ResolveSubresource(
						d3d11Res->resolveTexture[eye].Get(),
						D3D11CalcSubresource(0, d3d11Res->usingArrayTex ? eye : 0, 1),
						d3d11Res->submittedTextures[eye][index].Get(),
						D3D11CalcSubresource(0, d3d11Res->usingArrayTex ? eye : 0, td.MipLevels),
						TranslateTypelessFormats(td.Format));
				}
			}

			int outIndex = 0;
			ovr_GetTextureSwapChainCurrentIndex(session, outputEyeChains[eye], &outIndex);

			D3D11PostProcessInput input;
			input.inputTexture = d3d11Res->submittedTextures[eye][index].Get();
			input.inputView = d3d11Res->submittedViews[eye][index].Get();
			input.outputTexture = d3d11Res->outputTextures[eye][outIndex].Get();
			input.outputView = d3d11Res->outputViews[eye][outIndex].Get();
			input.outputUav = d3d11Res->outputUavs[eye][outIndex].Get();
			input.inputViewport.x = eyeLayer.Viewport[eye].Pos.x;
			input.inputViewport.y = eyeLayer.Viewport[eye].Pos.y;
			input.inputViewport.width = eyeLayer.Viewport[eye].Size.w;
			input.inputViewport.height = eyeLayer.Viewport[eye].Size.h;
			input.eye = eye;
			input.projectionCenter = projCenters.eyeCenter[eye];
			if (submittedEyeChains[1] == nullptr || submittedEyeChains[1] == submittedEyeChains[0]) {
				if (d3d11Res->usingArrayTex) {
					input.mode = TextureMode::ARRAY;
				} else {
					input.mode = TextureMode::COMBINED;
				}
			} else {
				input.mode = TextureMode::SINGLE;
			}

			Viewport outputViewport;
			if (d3d11Res->postProcessor->Apply(input, outputViewport)) {
				ovr_CommitTextureSwapChain(session, outputEyeChains[eye]);
				eyeLayer.ColorTexture[eye] = outputEyeChains[eye];
				eyeLayer.Viewport[eye].Pos.x = outputViewport.x;
				eyeLayer.Viewport[eye].Pos.y = outputViewport.y;
				eyeLayer.Viewport[eye].Size.w = outputViewport.width;
				eyeLayer.Viewport[eye].Size.h = outputViewport.height;
			}
		}
	}
}
