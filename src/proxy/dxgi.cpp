#include "logging.h"
#include "proxy_helpers.h"
#include "win_header_sane.h"
#include "hooks.h"
#include <dxgi.h>

namespace fs = std::filesystem;

namespace {
	HMODULE g_realDll = nullptr;
	HMODULE g_dxvkDll = nullptr;
	bool isHooked = false;

	template<typename T>
	T* LoadRealFunction(T* fn, const std::string &name) {
		vrperfkit::EnsureLoadDll(g_realDll, vrperfkit::GetSystemPath() / "dxgi.dll");
		if (isHooked) {
			return vrperfkit::hooks::CallOriginal(fn);
		}
		return static_cast<T*>(vrperfkit::GetDllFunctionPointer(g_realDll, name));
	}

	template<typename T>
	T* LoadDxvkFunction(T*, const std::string &name) {
		if (vrperfkit::g_config.dxvk.enabled) {
			vrperfkit::EnsureLoadDll(g_dxvkDll, vrperfkit::g_config.dxvk.dxgiDllPath);
			return static_cast<T*>(vrperfkit::GetDllFunctionPointer(g_dxvkDll, name));
		}
		return nullptr;
	}

	template<typename T>
	T* Switch(T* system, T* dxvk) {
		if (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk && dxvk != nullptr) {
			return dxvk;
		}
		return system;
	}
}

#define LOAD_REAL_FUNC(name) static auto realFunc = LoadRealFunction(name, #name)
#define LOAD_DXVK_FUNC(name) static auto dxvkFunc = LoadDxvkFunction(name, #name)

extern "C" {

	BOOL WINAPI CompatString(LPCSTR szName, ULONG *pSize, LPSTR lpData, bool Flag) {
		LOAD_REAL_FUNC(CompatString);
		return realFunc(szName, pSize, lpData, Flag);
	}

	BOOL WINAPI CompatValue(LPCSTR szName, UINT64 *pValue) {
		LOAD_REAL_FUNC(CompatValue);
		return realFunc(szName, pValue);
	}

	HRESULT WINAPI CreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(CreateDXGIFactory);
		LOAD_DXVK_FUNC(CreateDXGIFactory);
		return Switch(realFunc, dxvkFunc)(riid, ppFactory);
	}

	HRESULT WINAPI CreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(CreateDXGIFactory1);
		LOAD_DXVK_FUNC(CreateDXGIFactory1);
		return Switch(realFunc, dxvkFunc)(riid, ppFactory);
	}

	HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, _Out_ void **ppFactory) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(CreateDXGIFactory2);
		LOAD_DXVK_FUNC(CreateDXGIFactory2);
		return Switch(realFunc, dxvkFunc)(Flags, riid, ppFactory);
	}

	HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter, UINT flags, void *unknown, void **device) {
		LOAD_REAL_FUNC(DXGID3D10CreateDevice);
		return realFunc(d3d10core, factory, adapter, flags, unknown, device);
	}

	HRESULT WINAPI DXGID3D10CreateLayeredDevice(int a, int b, int c, int d, int e) {
		LOAD_REAL_FUNC(DXGID3D10CreateLayeredDevice);
		return realFunc(a, b, c, d, e);
	}

	HRESULT WINAPI DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers) {
		LOAD_REAL_FUNC(DXGID3D10GetLayeredDeviceSize);
		return realFunc(pLayers, NumLayers);
	}

	HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count) {
		LOAD_REAL_FUNC(DXGID3D10RegisterLayers);
		return realFunc(layers, layer_count);
	}

	HRESULT WINAPI DXGIDeclareAdapterRemovalSupport() {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(DXGIDeclareAdapterRemovalSupport);
		LOAD_DXVK_FUNC(DXGIDeclareAdapterRemovalSupport);
		return Switch(realFunc, dxvkFunc)();
	}

	HRESULT WINAPI DXGIDumpJournal(void *pfnCallback) {
		LOAD_REAL_FUNC(DXGIDumpJournal);
		return realFunc(pfnCallback);
	}

	HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void **pDebug) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(DXGIGetDebugInterface1);
		LOAD_DXVK_FUNC(DXGIGetDebugInterface1);
		return Switch(realFunc, dxvkFunc)(Flags, riid, pDebug);
	}

	HRESULT WINAPI DXGIReportAdapterConfiguration(int a) {
		LOAD_REAL_FUNC(DXGIReportAdapterConfiguration);
		return realFunc(a);
	}

	void WINAPI DXGID3D10ETWRundown() {}
}

namespace vrperfkit {
	void InstallDXGIHooks() {
		if (g_realDll != nullptr) {
			return;
		}

		std::wstring dllName = L"dxgi.dll";
		HMODULE handle;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
			return;
		}

		if (handle == g_moduleSelf) {
			return;
		}

		LOG_INFO << dllName << " is loaded in the process, installing hooks...";
		hooks::InstallHookInDll("CreateDXGIFactory", handle, (void*)CreateDXGIFactory);
		hooks::InstallHookInDll("CreateDXGIFactory1", handle, (void*)CreateDXGIFactory1);
		hooks::InstallHookInDll("CreateDXGIFactory2", handle, (void*)CreateDXGIFactory2);
		hooks::InstallHookInDll("DXGIGetDebugInterface1", handle, (void*)DXGIGetDebugInterface1);
		hooks::InstallHookInDll("DXGIDeclareAdapterRemovalSupport", handle, (void*)DXGIDeclareAdapterRemovalSupport);

		g_realDll = handle;
		isHooked = true;
	}
}
