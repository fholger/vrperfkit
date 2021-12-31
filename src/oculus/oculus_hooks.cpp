#include "oculus_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "oculus_manager.h"
#include "win_header_sane.h"
#include "OVR_CAPI.h"
#include "post_processor.h"

namespace {
	HMODULE g_oculusDll = nullptr;

	ovrSizei ovrHook_GetFovTextureSize(ovrSession session, ovrEyeType eye, ovrFovPort fov, float pixelsPerDisplayPixel) {
		ovrSizei result = vrperfkit::hooks::CallOriginal(ovrHook_GetFovTextureSize)(session, eye, fov, pixelsPerDisplayPixel);
		if (result.w > 0 && result.h > 0) {
			uint32_t width = result.w;
			uint32_t height = result.h;
			vrperfkit::g_postprocess.AdjustInputResolution(width, height);
			//LOG_DEBUG << "Render resolution adjusted from " << result.w << "x" << result.h << " to " << width << "x" << height;
			result.w = width;
			result.h = height;
		}
		return result;
	}

	ovrResult ovrHook_EndFrame(ovrSession session, long long frameIndex, const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const* const* layerPtrList, unsigned int layerCount) {
		unsigned int eyeLayerIndex;
		for (eyeLayerIndex = 0; eyeLayerIndex < layerCount; ++eyeLayerIndex) {
			if (layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFov || layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth) {
				break;
			}
		}

		std::vector modifiedLayers (layerPtrList, layerPtrList + layerCount);
		ovrLayerEyeFovDepth eyeLayer;
		if (eyeLayerIndex != layerCount) {
			memcpy(&eyeLayer, layerPtrList[eyeLayerIndex], layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth ? sizeof(ovrLayerEyeFovDepth) : sizeof(ovrLayerEyeFov));
			modifiedLayers[eyeLayerIndex] = &eyeLayer.Header;

			vrperfkit::g_oculus.OnFrameSubmission(session, eyeLayer);
		} 

		ovrResult result = vrperfkit::hooks::CallOriginal(ovrHook_EndFrame)(session, frameIndex, viewScaleDesc, modifiedLayers.data(), layerCount);
		return result;
	}
}

namespace vrperfkit {
	void InstallOculusHooks() {
		if (g_oculusDll != nullptr) {
			return;
		}

#ifdef WIN64
		std::wstring dllName = L"LibOVRRT64_1.dll";
#else
		std::wstring dllName = L"LibOVRRT32_1.dll";
#endif
		HMODULE handle;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
			return;
		}

		LOG_INFO << dllName << " is loaded in the process, installing hooks...";
		hooks::InstallHookInDll("ovr_GetFovTextureSize", handle, ovrHook_GetFovTextureSize);
		hooks::InstallHookInDll("ovr_EndFrame", handle, ovrHook_EndFrame);

		g_oculusDll = handle;
	}
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainDesc(ovrSession session, ovrTextureSwapChain chain, ovrTextureSwapChainDesc* out_Desc) {
	IMPL_ORIG(g_oculusDll, ovr_GetTextureSwapChainDesc);
	return orig_ovr_GetTextureSwapChainDesc(session, chain, out_Desc);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainBufferDX(ovrSession session, ovrTextureSwapChain chain, int index, IID iid, void** out_Buffer) {
	IMPL_ORIG(g_oculusDll, ovr_GetTextureSwapChainBufferDX);
	return orig_ovr_GetTextureSwapChainBufferDX(session, chain, index, iid, out_Buffer);
}

OVR_PUBLIC_FUNCTION(void) ovr_GetLastErrorInfo(ovrErrorInfo* errorInfo) {
	IMPL_ORIG(g_oculusDll, ovr_GetLastErrorInfo);
	return orig_ovr_GetLastErrorInfo(errorInfo);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CreateTextureSwapChainDX(ovrSession session, IUnknown* d3dPtr, const ovrTextureSwapChainDesc* desc, ovrTextureSwapChain* out_TextureSwapChain) {
	IMPL_ORIG(g_oculusDll, ovr_CreateTextureSwapChainDX);
	return orig_ovr_CreateTextureSwapChainDX(session, d3dPtr, desc, out_TextureSwapChain);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainLength(ovrSession session, ovrTextureSwapChain chain, int* out_Length) {
	IMPL_ORIG(g_oculusDll, ovr_GetTextureSwapChainLength);
	return orig_ovr_GetTextureSwapChainLength(session, chain, out_Length);
}

OVR_PUBLIC_FUNCTION(void) ovr_DestroyTextureSwapChain(ovrSession session, ovrTextureSwapChain chain) {
	IMPL_ORIG(g_oculusDll, ovr_DestroyTextureSwapChain);
	return orig_ovr_DestroyTextureSwapChain(session, chain);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovr_GetTextureSwapChainCurrentIndex(ovrSession session, ovrTextureSwapChain chain, int* out_Index) {
	IMPL_ORIG(g_oculusDll, ovr_GetTextureSwapChainCurrentIndex);
	return orig_ovr_GetTextureSwapChainCurrentIndex(session, chain, out_Index);
}
