#include "hooks.h"

#include "logging.h"
#include "MinHook.h"

#include <unordered_map>

namespace {
	struct HookInfo {
		intptr_t target;
		intptr_t original;
		intptr_t hook;
	};
	std::unordered_map<intptr_t, HookInfo> g_hooksToOriginal;
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
			MH_STATUS result = MH_CreateHook(pTarget, detour, &pOriginal);
			if (result != MH_OK || MH_EnableHook(pTarget) != MH_OK) {
				if (result == MH_ERROR_ALREADY_CREATED) {
					LOG_INFO << "  Hook already installed.";
				} else {
					LOG_ERROR << "Failed to install hook for " << name;
				}
				return;
			}

			g_hooksToOriginal[reinterpret_cast<intptr_t>(detour)] = HookInfo {
				reinterpret_cast<intptr_t>(pTarget),
				reinterpret_cast<intptr_t>(pOriginal),
				reinterpret_cast<intptr_t>(detour),
			};
		}

		void RemoveHook(void *detour) {
			auto entry = g_hooksToOriginal.find(reinterpret_cast<intptr_t>(detour));
			if (entry != g_hooksToOriginal.end()) {
				void *target = reinterpret_cast<void *>(entry->second.target);
				LOG_INFO << "Removing hook to " << target;
				if (MH_STATUS status; (status = MH_DisableHook(target)) != MH_OK) {
					LOG_ERROR << "Error when disabling hook to " << target << ": " << status;
				}
				if (MH_STATUS status; (status = MH_RemoveHook(target)) != MH_OK) {
					LOG_ERROR << "Error when removing hook to " << target << ": " << status;
				}
				g_hooksToOriginal.erase(entry);
			}
		}

		void InstallHook(const std::string &name, void *target, void *detour) {
			LOG_INFO << "Installing hook for " << name << " from " << target << " to " << detour;
			LPVOID pOriginal = nullptr;
			if (MH_CreateHook(target, detour, &pOriginal) != MH_OK || MH_EnableHook(target) != MH_OK) {
				LOG_ERROR << "Failed to install hook for " << name;
				return;
			}

			g_hooksToOriginal[reinterpret_cast<intptr_t>(detour)] = HookInfo {
				reinterpret_cast<intptr_t>(target),
				reinterpret_cast<intptr_t>(pOriginal),
				reinterpret_cast<intptr_t>(detour),
			};
		}

		void InstallHookInDll(const std::string &name, HMODULE module, void *detour) {
			LPVOID target = GetProcAddress(module, name.c_str());
			if (target != nullptr) {
				InstallHook(name, target, detour);
			}
		}

		intptr_t HookToOriginal(intptr_t hook) {
			return g_hooksToOriginal[hook].original;
		}
	}
}
