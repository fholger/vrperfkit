#include "config.h"
#include "logging.h"
#include "win_header_sane.h"
#include "hooks.h"
#include "hotkeys.h"
#include "oculus/oculus_hooks.h"
#include "oculus/oculus_manager.h"
#include "openvr/openvr_hooks.h"

namespace fs = std::filesystem;

namespace vrperfkit {
	HMODULE g_moduleSelf;
	fs::path g_dllPath;
	fs::path g_basePath;
	fs::path g_executablePath;

	fs::path GetModulePath(HMODULE module) {
		WCHAR buf[4096];
		return GetModuleFileNameW(module, buf, ARRAYSIZE(buf)) ? buf : fs::path();
	}

	void InstallD3D11Hooks();
	void InstallDXGIHooks();
}

namespace {

	void InstallVrHooks() {
		vrperfkit::InstallOpenVrHooks();
		vrperfkit::InstallOculusHooks();
		vrperfkit::InstallD3D11Hooks();
		vrperfkit::InstallDXGIHooks();
	}

	HMODULE WINAPI Hook_LoadLibraryA(LPCSTR lpFileName) {
		HMODULE handle = vrperfkit::hooks::CallOriginal(Hook_LoadLibraryA)(lpFileName);

		if (handle != nullptr && handle != vrperfkit::g_moduleSelf) {
			InstallVrHooks();
		}

		return handle;
	}

	HMODULE WINAPI Hook_LoadLibraryExA(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags) {
		HMODULE handle = vrperfkit::hooks::CallOriginal(Hook_LoadLibraryExA)(lpFileName, hFile, dwFlags);

		if (handle != nullptr && handle != vrperfkit::g_moduleSelf && (dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)) == 0) {
			InstallVrHooks();
		}

		return handle;
	}

	HMODULE WINAPI Hook_LoadLibraryW(LPCWSTR lpFileName) {
		// necessary, or else VS merges this function with Hook_LoadLibraryA and assigns them the same address
		std::wstring fileName (lpFileName);
		HMODULE handle = vrperfkit::hooks::CallOriginal(Hook_LoadLibraryW)(fileName.c_str());

		if (handle != nullptr && handle != vrperfkit::g_moduleSelf) {
			InstallVrHooks();
		}

		return handle;
	}

	HMODULE WINAPI Hook_LoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags) {
		std::wstring fileName (lpFileName);
		HMODULE handle = vrperfkit::hooks::CallOriginal(Hook_LoadLibraryExW)(fileName.c_str(), hFile, dwFlags);

		if (handle != nullptr && handle != vrperfkit::g_moduleSelf && (dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)) == 0) {
			InstallVrHooks();
		}

		return handle;
	}

	void InitVrPerfkit(HMODULE module) {
		vrperfkit::g_moduleSelf = module;
		vrperfkit::g_dllPath = vrperfkit::GetModulePath(module);
		vrperfkit::g_basePath = vrperfkit::g_dllPath.parent_path();
		vrperfkit::g_executablePath = vrperfkit::GetModulePath(nullptr);

		vrperfkit::OpenLogFile(vrperfkit::g_basePath / "vrperfkit.log");
		LOG_INFO << "======================";
		LOG_INFO << "VR Performance Toolkit";
		LOG_INFO << "======================\n";

		vrperfkit::LoadConfig(vrperfkit::g_basePath / "vrperfkit.yml");
		vrperfkit::LoadHotkeys(vrperfkit::g_basePath / "vrperfkit.yml");
		vrperfkit::PrintCurrentConfig();
		vrperfkit::PrintHotkeys();

		vrperfkit::hooks::Init();
		vrperfkit::hooks::InstallHook("LoadLibraryA", (void*)&LoadLibraryA, (void*)&Hook_LoadLibraryA);
		vrperfkit::hooks::InstallHook("LoadLibraryExA", (void*)&LoadLibraryExA, (void*)&Hook_LoadLibraryExA);
		vrperfkit::hooks::InstallHook("LoadLibraryW", (void*)LoadLibraryW, (void*)Hook_LoadLibraryW);
		vrperfkit::hooks::InstallHook("LoadLibraryExW", (void*)&LoadLibraryExW, (void*)&Hook_LoadLibraryExW);
		InstallVrHooks();
	}

	void ShutdownVrPerfkit() {
		LOG_INFO << "Shutting down\n";
		vrperfkit::g_oculus.Shutdown();
		vrperfkit::hooks::Shutdown();
		vrperfkit::FlushLog();
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		InitVrPerfkit(module);
		break;

	case DLL_PROCESS_DETACH:
		ShutdownVrPerfkit();
		break;
	}

	return TRUE;
}
