#pragma once
#include "MinHook.h"

#include <cstdint>
#include <string>

namespace vrperfkit {
	namespace hooks {

		void Init();
		void Shutdown();

		void InstallHook(const std::string &name, void *target, void *detour);
		void InstallHookInDll(const std::string &name, HMODULE module, void *detour);
		void InstallVirtualFunctionHook(const std::string &name, void *instance, uint32_t methodPos, void *detour);

		intptr_t HookToOriginal(intptr_t hook);

		template<typename T>
		T CallOriginal(T hookFunction) {
			return reinterpret_cast<T>(HookToOriginal(reinterpret_cast<intptr_t>(hookFunction)));
		}
	}
}
