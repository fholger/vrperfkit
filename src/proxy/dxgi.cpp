#include "logging.h"
#include "win_header_sane.h"
#include <dxgi.h>

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

		fs::path realDllPath = GetSystemPath() / "dxgi.dll";
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

	BOOL WINAPI CompatString(LPCSTR szName, ULONG *pSize, LPSTR lpData, bool Flag) {
		LOAD_REAL_FUNC(CompatString);
		return realFunc(szName, pSize, lpData, Flag);
	}

	BOOL WINAPI CompatValue(LPCSTR szName, UINT64 *pValue) {
		LOAD_REAL_FUNC(CompatValue);
		return realFunc(szName, pValue);
	}

	HRESULT WINAPI CreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory) {
		LOAD_REAL_FUNC(CreateDXGIFactory);
		return realFunc(riid, ppFactory);
	}

	HRESULT WINAPI CreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory) {
		LOAD_REAL_FUNC(CreateDXGIFactory1);
		return realFunc(riid, ppFactory);
	}

	HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, _Out_ void **ppFactory) {
		LOAD_REAL_FUNC(CreateDXGIFactory2);
		return realFunc(Flags, riid, ppFactory);
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
		LOAD_REAL_FUNC(DXGIDeclareAdapterRemovalSupport);
		return realFunc();
	}

	HRESULT WINAPI DXGIDumpJournal(void *pfnCallback) {
		LOAD_REAL_FUNC(DXGIDumpJournal);
		return realFunc(pfnCallback);
	}

	HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void **pDebug) {
		LOAD_REAL_FUNC(DXGIGetDebugInterface1);
		return realFunc(Flags, riid, pDebug);
	}

	HRESULT WINAPI DXGIReportAdapterConfiguration(int a) {
		LOAD_REAL_FUNC(DXGIReportAdapterConfiguration);
		return realFunc(a);
	}
}
