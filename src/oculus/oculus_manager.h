#pragma once
#include "OVR_CAPI.h"

namespace vrperfkit {
	class OculusManager {
	public:
		void Init(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain);
		void Shutdown();
		void EnsureInit(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain);

		void OnFrameSubmission(ovrSession session, ovrLayerEyeFovDepth &eyeLayer);

	private:
		bool initialized = false;
		ovrSession session = nullptr;
		ovrTextureSwapChain submittedEyeChains[2] = { nullptr, nullptr };
	};

	extern OculusManager g_oculus;
}
