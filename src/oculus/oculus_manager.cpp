#include "oculus_manager.h"

#include "logging.h"
#include <wrl/client.h>
#include <d3d11.h>
#include <d3d12.h>
#include <OVR_CAPI_D3D.h>

using Microsoft::WRL::ComPtr;

namespace vrperfkit {
	OculusManager g_oculus;

	void OculusManager::Init(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain) {
		this->session = session;
		submittedEyeChains[0] = leftEyeChain;
		submittedEyeChains[1] = rightEyeChain;

		LOG_INFO << "Initializing Oculus frame submission...";

		ovrTextureSwapChainDesc desc;
		ovr_GetTextureSwapChainDesc(session, leftEyeChain, &desc);
		LOG_INFO << "Submitted texture has size " << desc.Width << "x" << desc.Height;

		// determine which graphics API the swapchains are created with
		ComPtr<ID3D11Texture2D> d3d11Tex;
		ComPtr<ID3D12Resource> d3d12Tex;

		ovrResult result = ovr_GetTextureSwapChainBufferDX(session, leftEyeChain, 0, IID_PPV_ARGS(d3d12Tex.GetAddressOf()));
		if (OVR_SUCCESS(result)) {
			LOG_INFO << "Game is using D3D12 swapchains, not currently supported";
			return;
		}
		else {
			ovrErrorInfo info;
			ovr_GetLastErrorInfo(&info);
			LOG_INFO << "Getting DX12 texture failed: " << info.ErrorString;
		}

		result = ovr_GetTextureSwapChainBufferDX(session, leftEyeChain, 0, IID_PPV_ARGS(d3d11Tex.GetAddressOf()));
		if (OVR_SUCCESS(result)) {
			LOG_INFO << "Game is using D3D11 swapchains, initializing D3D11 resources";
		}
		else {
			ovrErrorInfo info;
			ovr_GetLastErrorInfo(&info);
			LOG_INFO << "Getting DX12 texture failed: " << info.ErrorString;
		}
		FlushLog();

		initialized = true;
	}

	void OculusManager::Shutdown() {
		initialized = false;
	}

	void OculusManager::EnsureInit(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain) {
		if (!initialized || session != this->session || leftEyeChain != submittedEyeChains[0] || rightEyeChain != submittedEyeChains[1]) {
			Shutdown();
			Init(session, leftEyeChain, rightEyeChain);
		}
	}

	void OculusManager::OnFrameSubmission(ovrSession session, ovrLayerEyeFovDepth &eyeLayer) {
		EnsureInit(session, eyeLayer.ColorTexture[0], eyeLayer.ColorTexture[1]);
	}
}
