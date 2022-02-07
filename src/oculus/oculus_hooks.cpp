#include "oculus_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "oculus_manager.h"
#include "win_header_sane.h"
#include "OVR_CAPI.h"
#include "resolution_scaling.h"

namespace {
	HMODULE g_oculusDll = nullptr;
	uint32_t g_oculusVersion = 0;

	struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrOldLayerHeader {
	  ovrLayerType Type;
	  unsigned Flags;
	};

	struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrOldLayerEyeFov {
	  ovrOldLayerHeader Header;
	  ovrTextureSwapChain ColorTexture[ovrEye_Count];
	  ovrRecti Viewport[ovrEye_Count];
	  ovrFovPort Fov[ovrEye_Count];
	  ovrPosef RenderPose[ovrEye_Count];
	  double SensorSampleTime;
	};

	struct OVR_ALIGNAS(OVR_PTR_SIZE) ovrOldLayerEyeFovDepth : public ovrOldLayerEyeFov {
	  ovrTextureSwapChain DepthTexture[ovrEye_Count];
	  ovrTimewarpProjectionDesc ProjectionDesc;
	};

	ovrSizei ovrHook_GetFovTextureSize(ovrSession session, ovrEyeType eye, ovrFovPort fov, float pixelsPerDisplayPixel) {
		ovrSizei result = vrperfkit::hooks::CallOriginal(ovrHook_GetFovTextureSize)(session, eye, fov, pixelsPerDisplayPixel);
		if (result.w > 0 && result.h > 0) {
			vrperfkit::AdjustRenderResolution(result.w, result.h);
		}
		return result;
	}

	void CopyEyeLayer(const ovrLayerHeader *inputLayer, ovrLayerEyeFovDepth &outputLayer) {
		outputLayer.Header.Type = inputLayer->Type;
		outputLayer.Header.Flags = inputLayer->Flags;

		// work around the Reserved field added to the header in v25
		const ovrLayerEyeFov *eyeLayer = (g_oculusVersion < 25)
			? (const ovrLayerEyeFov *)((uint8_t*)inputLayer - sizeof(ovrLayerHeader::Reserved))
			: (const ovrLayerEyeFov *)inputLayer;

		memcpy(&outputLayer.ColorTexture[0], eyeLayer->ColorTexture[0], inputLayer->Type == ovrLayerType_EyeFovDepth ? sizeof(ovrLayerEyeFovDepth) : sizeof(ovrLayerEyeFov));
	}

	void HandleOldFrameSubmission(ovrSession session, ovrOldLayerHeader const * const *layerPtrList, uint32_t layerCount, std::vector<const ovrOldLayerHeader*> &modifiedLayers, ovrOldLayerEyeFovDepth &eyeLayer) {
		unsigned int eyeLayerIndex;
		for (eyeLayerIndex = 0; eyeLayerIndex < layerCount; ++eyeLayerIndex) {
			if (layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFov || layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth) {
				break;
			}
		}

		modifiedLayers.assign(layerPtrList, layerPtrList + layerCount);
		if (eyeLayerIndex != layerCount) {
			memcpy(&eyeLayer, layerPtrList[eyeLayerIndex], layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth ? sizeof(ovrOldLayerEyeFovDepth) : sizeof(ovrOldLayerEyeFov));
			modifiedLayers[eyeLayerIndex] = &eyeLayer.Header;

			ovrLayerEyeFovDepth newLayer;
			newLayer.Header.Type = eyeLayer.Header.Type;
			newLayer.Header.Flags = eyeLayer.Header.Flags;
			memcpy(&newLayer.ColorTexture[0], &eyeLayer.ColorTexture[0], sizeof(eyeLayer) - sizeof(eyeLayer.Header));

			vrperfkit::g_oculus.OnFrameSubmission(session, newLayer);

			memcpy(&eyeLayer.ColorTexture[0], &newLayer.ColorTexture[0], sizeof(eyeLayer) - sizeof(eyeLayer.Header));
		} 
	}

	void HandleFrameSubmission(ovrSession session, ovrLayerHeader const * const *layerPtrList, uint32_t layerCount, std::vector<const ovrLayerHeader*> &modifiedLayers, ovrLayerEyeFovDepth &eyeLayer) {
		unsigned int eyeLayerIndex;
		for (eyeLayerIndex = 0; eyeLayerIndex < layerCount; ++eyeLayerIndex) {
			if (layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFov || layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth) {
				break;
			}
		}

		modifiedLayers.assign(layerPtrList, layerPtrList + layerCount);
		if (eyeLayerIndex != layerCount) {
			memcpy(&eyeLayer, layerPtrList[eyeLayerIndex], layerPtrList[eyeLayerIndex]->Type == ovrLayerType_EyeFovDepth ? sizeof(ovrLayerEyeFovDepth) : sizeof(ovrLayerEyeFov));
			modifiedLayers[eyeLayerIndex] = &eyeLayer.Header;

			vrperfkit::g_oculus.OnFrameSubmission(session, eyeLayer);
		} 
	}

	ovrResult ovrHook_EndFrame(ovrSession session, long long frameIndex, const ovrViewScaleDesc* viewScaleDesc, void const* const* layerPtrList, unsigned int layerCount) {
		if (g_oculusVersion < 25) {
			ovrOldLayerEyeFovDepth eyeLayer;
			std::vector<const ovrOldLayerHeader*> modifiedLayers;
			HandleOldFrameSubmission(session, (ovrOldLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_EndFrame)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
		else {
			ovrLayerEyeFovDepth eyeLayer;
			std::vector<const ovrLayerHeader*> modifiedLayers;
			HandleFrameSubmission(session, (ovrLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_EndFrame)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
	}

	ovrResult ovrHook_SubmitFrame2(ovrSession session, long long frameIndex, const ovrViewScaleDesc* viewScaleDesc, void const* const* layerPtrList, unsigned int layerCount) {
		if (g_oculusVersion < 25) {
			ovrOldLayerEyeFovDepth eyeLayer;
			std::vector<const ovrOldLayerHeader*> modifiedLayers;
			HandleOldFrameSubmission(session, (ovrOldLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_SubmitFrame2)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
		else {
			ovrLayerEyeFovDepth eyeLayer;
			std::vector<const ovrLayerHeader*> modifiedLayers;
			HandleFrameSubmission(session, (ovrLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_SubmitFrame2)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
	}

	ovrResult ovrHook_SubmitFrame(ovrSession session, long long frameIndex, const void* viewScaleDesc, void const* const* layerPtrList, unsigned int layerCount) {
		if (g_oculusVersion < 25) {
			ovrOldLayerEyeFovDepth eyeLayer;
			std::vector<const ovrOldLayerHeader*> modifiedLayers;
			HandleOldFrameSubmission(session, (ovrOldLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_SubmitFrame)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
		else {
			ovrLayerEyeFovDepth eyeLayer;
			std::vector<const ovrLayerHeader*> modifiedLayers;
			HandleFrameSubmission(session, (ovrLayerHeader const * const *)layerPtrList, layerCount, modifiedLayers, eyeLayer);
			return vrperfkit::hooks::CallOriginal(ovrHook_SubmitFrame)(session, frameIndex, viewScaleDesc, (const void**)modifiedLayers.data(), layerCount);
		}
	}

	ovrResult ovrHook_Initialize(const ovrInitParams* params) {
		g_oculusVersion = params->RequestedMinorVersion;
		LOG_INFO << "Oculus runtime initialization for version " << g_oculusVersion;
		return vrperfkit::hooks::CallOriginal(ovrHook_Initialize)(params);
	}
}

namespace vrperfkit {
	void InstallOculusHooks() {
		if (g_oculusDll != nullptr) {
			return;
		}

#ifdef WIN64
		std::wstring dllNames[] = { L"LibOVRRT64_1.dll", L"VirtualDesktop.LibOVRRT64_1.dll", L"LibPVRRT64_1_X.dll" };
#else
		std::wstring dllNames[] = { L"LibOVRRT32_1.dll", L"VirtualDesktop.LibOVRRT32_1.dll", L"LibPVRRT32_1_X.dll" };
#endif
		HMODULE handle = nullptr;
		for (auto dllName : dllNames) {
			if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
				continue;
			}

			LOG_INFO << dllName << " is loaded in the process, installing hooks...";
			hooks::InstallHookInDll("ovr_Initialize", handle, (void*)ovrHook_Initialize);
			hooks::InstallHookInDll("ovr_GetFovTextureSize", handle, (void*)ovrHook_GetFovTextureSize);
			hooks::InstallHookInDll("ovr_EndFrame", handle, (void*)ovrHook_EndFrame);
			hooks::InstallHookInDll("ovr_SubmitFrame", handle, (void*)ovrHook_SubmitFrame);
			hooks::InstallHookInDll("ovr_SubmitFrame2", handle, (void*)ovrHook_SubmitFrame2);

			g_oculusDll = handle;
			break;
		}
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

OVR_PUBLIC_FUNCTION(ovrResult) ovr_CommitTextureSwapChain(ovrSession session, ovrTextureSwapChain chain) {
	IMPL_ORIG(g_oculusDll, ovr_CommitTextureSwapChain);
	return orig_ovr_CommitTextureSwapChain(session, chain);
}
