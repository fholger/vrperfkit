#include "oculus_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "win_header_sane.h"
#include "OVR_CAPI.h"
#include "upscaling/upscaling_mod.h"

namespace {
	ovrSizei ovrHook_GetFovTextureSize(ovrSession session, ovrEyeType eye, ovrFovPort fov, float pixelsPerDisplayPixel) {
		ovrSizei result = vrperfkit::hooks::CallOriginal(ovrHook_GetFovTextureSize)(session, eye, fov, pixelsPerDisplayPixel);
		if (result.w > 0 && result.h > 0) {
			uint32_t width = result.w;
			uint32_t height = result.h;
			vrperfkit::g_upscalingMod.AdjustRenderResolution(width, height);
			LOG_DEBUG << "Render resolution adjusted from " << result.w << "x" << result.h << " to " << width << "x" << height;
			result.w = width;
			result.h = height;
		}
		return result;
	}
}

namespace vrperfkit {
	namespace hooks {

		void InstallOculusHooks() {
			static bool hooksLoaded = false;
			if (hooksLoaded) {
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
			InstallHookInDll("ovr_GetFovTextureSize", handle, ovrHook_GetFovTextureSize);

			hooksLoaded = true;
		}
	}
}
