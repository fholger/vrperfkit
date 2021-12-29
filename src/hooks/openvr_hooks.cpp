#include "openvr_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "win_header_sane.h"

namespace {
	void *Hook_VRClientCoreFactory(const char *pInterfaceName, int *pReturnCode) {
		LOG_INFO << "OpenVR: requested interface " << pInterfaceName;

		void *instance = vrperfkit::hooks::CallOriginal(Hook_VRClientCoreFactory)(pInterfaceName, pReturnCode);
		return instance;
	}
}

namespace vrperfkit {
	namespace hooks {

		void InstallOpenVrHooks() {
			static bool hooksLoaded = false;
			if (hooksLoaded) {
				return;
			}

#ifdef WIN64
			std::wstring dllName = L"vrclient_x64.dll";
#else
			std::wstring dllName = L"vrclient.dll";
#endif
			HMODULE handle;
			if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
				return;
			}

			LOG_INFO << dllName << " is loaded in the process, installing hooks...";
			InstallHookInDll("VRClientCoreFactory", handle, Hook_VRClientCoreFactory);

			hooksLoaded = true;
		}
	}
}
