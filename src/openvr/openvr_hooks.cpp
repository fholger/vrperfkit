#include "openvr_hooks.h"
#include "hooks.h"
#include "logging.h"
#include "win_header_sane.h"
#include "openvr.h"

namespace vrperfkit {
	extern HMODULE g_module;

	namespace {
		vr::EVRCompositorError IVRCompositor012Hook_Submit(vr::EVREye eEye, const vr::Texture_t *pTexture, const vr::VRTextureBounds_t *pBounds, vr::EVRSubmitFlags nSubmitFlags) {
			auto error = hooks::CallOriginal(IVRCompositor012Hook_Submit)(eEye, pTexture, pBounds, nSubmitFlags);
			return error;
		}

		void *Hook_VRClientCoreFactory(const char *pInterfaceName, int *pReturnCode) {
			void *instance = hooks::CallOriginal(Hook_VRClientCoreFactory)(pInterfaceName, pReturnCode);
			HookOpenVrInterface(pInterfaceName, instance);
			return instance;
		}

		void *IVRClientCoreHook_GetGenericInterface(const char *interfaceName, vr::EVRInitError *error) {
			void *instance = hooks::CallOriginal(IVRClientCoreHook_GetGenericInterface)(interfaceName, error);
			HookOpenVrInterface(interfaceName, instance);
			return instance;
		}

		void IVRClientCoreHook_Cleanup() {
			hooks::CallOriginal(IVRClientCoreHook_Cleanup);
			LOG_INFO << "IVRClientCore::Cleanup was called, deleting hooks...";
			hooks::RemoveHook((void*)&IVRClientCoreHook_GetGenericInterface);
			hooks::RemoveHook((void*)&IVRClientCoreHook_Cleanup);
			hooks::RemoveHook((void*)IVRCompositor012Hook_Submit);
		}
	}

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
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle) || handle == g_module) {
			return;
		}

		LOG_INFO << dllName << " is loaded in the process, installing hooks...";
		hooks::InstallHookInDll("VRClientCoreFactory", handle, (void*)Hook_VRClientCoreFactory);

		hooksLoaded = true;
	}

	void HookOpenVrInterface(const char *interfaceName, void *instance) {
		LOG_INFO << "OpenVR: requested interface " << interfaceName;
		if (instance == nullptr) {
			return;
		}

		unsigned int version = 0;
		if (std::sscanf(interfaceName, "IVRClientCore_%u", &version)) {
			hooks::RemoveHook((void*)&IVRClientCoreHook_Cleanup);
			hooks::RemoveHook((void*)&IVRClientCoreHook_GetGenericInterface);
			if (version <= 3) {
				hooks::InstallVirtualFunctionHook("IVRClientCore::GetGenericInterface", instance, 3, (void*)&IVRClientCoreHook_GetGenericInterface);
				hooks::InstallVirtualFunctionHook("IVRClientCore::Cleanup", instance, 1, (void*)&IVRClientCoreHook_Cleanup);
			}
			else {
				LOG_ERROR << "Don't know how to inject into version " << version << " of IVRClientCore";
			}
		}

		if (std::sscanf(interfaceName, "IVRCompositor_%u", &version)) {
			hooks::RemoveHook((void*)&IVRCompositor012Hook_Submit);
			if (version >= 12) {
				hooks::InstallVirtualFunctionHook("IVRCompositor::Submit", instance, 6, (void*)&IVRCompositor012Hook_Submit);
			}
			else {
				LOG_ERROR << "Don't know how to inject into version " << version << " of IVRCompositor";
			}
		}
	}
}
