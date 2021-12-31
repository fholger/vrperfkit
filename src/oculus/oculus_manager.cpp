#include "oculus_manager.h"

#include "logging.h"
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
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		std::vector<ComPtr<ID3D11Texture2D>> submittedTextures[2];
		std::vector<ComPtr<ID3D11Texture2D>> outputTextures[2];
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
	}

	void OculusManager::InitD3D11() {
		LOG_INFO << "Game is using D3D11 swapchains, initializing D3D11 resources";
		graphicsApi = GraphicsApi::D3D11;
		d3d11Res.reset(new OculusD3D11Resources);

		for (int eye = 0; eye < 2; ++eye) {
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
			chainDesc.SampleCount = 1;
			chainDesc.MipLevels = 1;
			chainDesc.BindFlags = ovrTextureBind_DX_UnorderedAccess;
			chainDesc.MiscFlags = ovrTextureMisc_None;
			Check("creating output swapchain", ovr_CreateTextureSwapChainDX(session, d3d11Res->device.Get(), &chainDesc, &outputEyeChains[eye]));

			Check("getting texture swapchain length", ovr_GetTextureSwapChainLength(session, outputEyeChains[eye], &length));
			for (int i = 0; i < length; ++i) {
				ComPtr<ID3D11Texture2D> texture;
				Check("getting swapchain texture", ovr_GetTextureSwapChainBufferDX(session, outputEyeChains[eye], i, IID_PPV_ARGS(texture.GetAddressOf())));
				d3d11Res->outputTextures[eye].push_back(texture);
			}
		}

		LOG_INFO << "D3D11 resource creation complete";
		initialized = true;
	}
}
