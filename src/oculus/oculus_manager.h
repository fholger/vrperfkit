#pragma once
#include "OVR_CAPI.h"
#include "types.h"

#include <memory>

namespace vrperfkit {
	struct OculusD3D11Resources;

	class OculusManager {
	public:
		void Init(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain);
		void Shutdown();
		void EnsureInit(ovrSession session, ovrTextureSwapChain leftEyeChain, ovrTextureSwapChain rightEyeChain);

		void OnFrameSubmission(ovrSession session, ovrLayerEyeFovDepth &eyeLayer);

	private:
		bool failed = false;
		bool initialized = false;
		GraphicsApi graphicsApi = GraphicsApi::UNKNOWN;
		ovrSession session = nullptr;
		ovrTextureSwapChain submittedEyeChains[2] = { nullptr, nullptr };
		ovrTextureSwapChain outputEyeChains[2] = { nullptr, nullptr };

		ProjectionCenters CalculateProjectionCenter(const ovrFovPort *fov);

		std::unique_ptr<OculusD3D11Resources> d3d11Res;
		void InitD3D11();

		void PostProcessD3D11(ovrLayerEyeFovDepth &eyeLayer);
	};

	extern OculusManager g_oculus;
}
