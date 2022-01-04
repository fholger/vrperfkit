#include "logging.h"
#include "proxy_helpers.h"
#include "openvr/openvr_hooks.h"

#include <dxgi.h>

namespace {
	HMODULE g_realDll = nullptr;

	template<typename T>
	T LoadRealFunction(T, const std::string &name) {
#ifdef WIN64
		std::string dllName = "vrclient_x64.dll";
#else
		std::string dllName = "vrclient.dll";
#endif
		vrperfkit::EnsureLoadDll(g_realDll, dllName);
		return reinterpret_cast<T>(vrperfkit::GetDllFunctionPointer(g_realDll, name));
	}
}

#define LOAD_REAL_FUNC(name) static auto realFunc = LoadRealFunction(name, #name)

extern "C" {

	void * __cdecl VRClientCoreFactory(const char *pInterfaceName, int *pReturnCode) {
		LOAD_REAL_FUNC(VRClientCoreFactory);
		void *instance = realFunc(pInterfaceName, pReturnCode);
		vrperfkit::HookOpenVrInterface(pInterfaceName, instance);
		return instance;
	}
}
