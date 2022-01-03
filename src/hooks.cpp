#include "hooks.h"

#include "logging.h"
#include "MinHook.h"

#include <unordered_map>

namespace {
	std::unordered_map<intptr_t, intptr_t> g_hooksToOriginal;
}

namespace vrperfkit {
	namespace hooks {

		void Init() {
			if (MH_Initialize() != MH_OK) {
				LOG_ERROR << "Failed to initialize MinHook";
			}
		}

		void Shutdown() {
			MH_Uninitialize();
			g_hooksToOriginal.clear();
		}

		void InstallVirtualFunctionHook(const std::string &name, void *instance, uint32_t methodPos, void *detour) {
			LOG_INFO << "Installing virtual function hook for " << name;
			LPVOID *vtable = *((LPVOID**)instance);
			LPVOID pTarget = vtable[methodPos];

			LPVOID pOriginal = nullptr;
			if (MH_CreateHook(pTarget, detour, &pOriginal) != MH_OK || MH_EnableHook(pTarget) != MH_OK) {
				LOG_ERROR << "Failed to install hook for " << name;
				return;
			}

			g_hooksToOriginal[reinterpret_cast<intptr_t>(detour)] = reinterpret_cast<intptr_t>(pOriginal);
		}

		void InstallHook(const std::string &name, void *target, void *detour) {
			LOG_INFO << "Installing hook for " << name << " from " << target << " to " << detour;
			LPVOID pOriginal = nullptr;
			if (MH_CreateHook(target, detour, &pOriginal) != MH_OK || MH_EnableHook(target) != MH_OK) {
				LOG_ERROR << "Failed to install hook for " << name;
				return;
			}

			g_hooksToOriginal[reinterpret_cast<intptr_t>(detour)] = reinterpret_cast<intptr_t>(pOriginal);
		}

		void InstallHookInDll(const std::string &name, HMODULE module, void *detour) {
			LPVOID target = GetProcAddress(module, name.c_str());
			if (target != nullptr) {
				InstallHook(name, target, detour);
			}
		}

		intptr_t HookToOriginal(intptr_t hook) {
			return g_hooksToOriginal[hook];
		}
	}
}
