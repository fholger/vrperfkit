#include "oculus_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "win_header_sane.h"

namespace {
}

namespace vrperfkit {
	namespace hooks {

		void InstallOculusHooks() {
			static bool hooksLoaded = false;
			if (hooksLoaded) {
				return;
			}

			std::wstring dllName = L"OVRPlugin.dll";
			HMODULE handle;
			if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
				return;
			}

			LOG_INFO << dllName << " is loaded in the process, installing hooks...";
			// TODO...

			hooksLoaded = true;
		}
	}
}
