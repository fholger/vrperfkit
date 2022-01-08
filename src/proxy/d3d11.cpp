#include "hooks.h"
#include "logging.h"
#include "proxy_helpers.h"
#include <dxgi.h>
#include <d3d11.h>

namespace {
	HMODULE g_realDll = nullptr;
	HMODULE g_dxvkDll = nullptr;
	bool isHooked = false;

	template<typename T>
	T* LoadRealFunction(T* fn, const std::string &name) {
		vrperfkit::EnsureLoadDll(g_realDll, vrperfkit::GetSystemPath() / "d3d11.dll");
		if (isHooked) {
			return vrperfkit::hooks::CallOriginal(fn);
		}
		return static_cast<T*>(vrperfkit::GetDllFunctionPointer(g_realDll, name));
	}

	template<typename T>
	T* LoadDxvkFunction(T*, const std::string &name) {
		if (vrperfkit::g_config.dxvk.enabled) {
			vrperfkit::EnsureLoadDll(g_dxvkDll, vrperfkit::g_config.dxvk.d3d11DllPath);
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

	HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(D3D11CreateDevice);
		LOAD_DXVK_FUNC(D3D11CreateDevice);
		return Switch(realFunc, dxvkFunc)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	}

	HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext) {
		LOG_DEBUG << "Redirecting " << __FUNCTION__ << " to " << (vrperfkit::g_config.dxvk.enabled && vrperfkit::g_config.dxvk.shouldUseDxvk ? "dxvk" : "system");
		LOAD_REAL_FUNC(D3D11CreateDeviceAndSwapChain);
		LOAD_DXVK_FUNC(D3D11CreateDeviceAndSwapChain);
		return Switch(realFunc, dxvkFunc)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
	}
}

namespace vrperfkit {
	void InstallD3D11Hooks() {
		if (g_realDll != nullptr) {
			return;
		}

		std::wstring dllName = L"d3d11.dll";
		HMODULE handle;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, dllName.c_str(), &handle)) {
			return;
		}

		if (handle == g_moduleSelf) {
			return;
		}

		LOG_INFO << dllName << " is loaded in the process, installing hooks...";
		hooks::InstallHookInDll("D3D11CreateDevice", handle, (void*)D3D11CreateDevice);
		hooks::InstallHookInDll("D3D11CreateDeviceAndSwapChain", handle, (void*)D3D11CreateDeviceAndSwapChain);

		g_realDll = handle;
		isHooked = true;
	}
}
