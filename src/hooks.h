#pragma once
#include "logging.h"
#include "MinHook.h"

#include <cstdint>
#include <string>

#define IMPL_ORIG(hModule, fnName) static auto orig_##fnName = vrperfkit::hooks::LoadFunction(hModule, #fnName, fnName)

namespace vrperfkit {
	namespace hooks {

		void Init();
		void Shutdown();

		template<typename T>
		T LoadFunction(HMODULE module, const std::string &name, T) {
			auto fn = GetProcAddress(module, name.c_str());
			if (fn == nullptr) {
				LOG_ERROR << "Could not load function " << name;
			}
			return reinterpret_cast<T>(fn);
		}

		void InstallHook(const std::string &name, void *target, void *detour);
		void InstallHookInDll(const std::string &name, HMODULE module, void *detour);
		void InstallVirtualFunctionHook(const std::string &name, void *instance, uint32_t methodPos, void *detour);
		void RemoveHook(void *detour);

		intptr_t HookToOriginal(intptr_t hook);

		template<typename T>
		T CallOriginal(T hookFunction) {
			return reinterpret_cast<T>(HookToOriginal(reinterpret_cast<intptr_t>(hookFunction)));
		}
	}
}
