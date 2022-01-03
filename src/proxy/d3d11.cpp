#include "logging.h"
#include "proxy_helpers.h"
#include "win_header_sane.h"
#include <dxgi.h>
#include <d3d11.h>

namespace {
	HMODULE g_realDll = nullptr;

	template<typename T>
	T LoadRealFunction(T, const std::string &name) {
		vrperfkit::EnsureLoadDll(g_realDll, "d3d11.dll");
		return reinterpret_cast<T>(vrperfkit::GetDllFunctionPointer(g_realDll, name));
	}
}

#define LOAD_REAL_FUNC(name) static auto realFunc = LoadRealFunction(name, #name)

extern "C" {

	HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext) {
		LOAD_REAL_FUNC(D3D11CreateDevice);
		return realFunc(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	}

	HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext) {
		LOAD_REAL_FUNC(D3D11CreateDeviceAndSwapChain);
		return realFunc(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
	}
}
