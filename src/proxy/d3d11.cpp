#include "logging.h"
#include "win_header_sane.h"
#include <dxgi.h>
#include <d3d11.h>

namespace fs = std::filesystem;

namespace {
	HMODULE g_realDll = nullptr;

	fs::path GetSystemPath() {
		WCHAR buf[4096] = L"";
		GetSystemDirectoryW(buf, ARRAYSIZE(buf));
		return buf;
	}

	void EnsureLoadRealDll() {
		if (g_realDll != nullptr) {
			return;
		}

		fs::path realDllPath = GetSystemPath() / "d3d11.dll";
		LOG_INFO << "Loading real DLL at " << realDllPath;
		g_realDll = LoadLibraryW(realDllPath.c_str());
		if (g_realDll == nullptr) {
			LOG_ERROR << "Failed to load original DLL";
		}
	}

	template<typename T>
	T LoadRealFunction(T, const std::string &name) {
		EnsureLoadRealDll();
		return reinterpret_cast<T>(GetProcAddress(g_realDll, name.c_str()));
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
