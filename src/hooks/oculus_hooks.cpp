#include "oculus_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "win_header_sane.h"
#include "OVR_CAPI.h"

namespace {
	ovrHmdDesc ovrHook_GetHmdDesc(ovrSession session) {
		ovrHmdDesc result = vrperfkit::hooks::CallOriginal(ovrHook_GetHmdDesc)(session);
		LOG_DEBUG << "ovr_GetHmdDesc called. Headset resolution: " << result.Resolution.w << "x" << result.Resolution.h;
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
			InstallHookInDll("ovr_GetHmdDesc", handle, ovrHook_GetHmdDesc);

			hooksLoaded = true;
		}
	}
}
