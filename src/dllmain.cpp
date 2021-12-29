#include "logging.h"
#include "win_header_sane.h"

namespace fs = std::filesystem;

namespace vrperfkit {
	fs::path GetModulePath(HMODULE module) {
		WCHAR buf[4096];
		return GetModuleFileNameW(module, buf, ARRAYSIZE(buf)) ? buf : fs::path();
	}
}

namespace {
	fs::path g_dllPath;
	fs::path g_basePath;
	fs::path g_executablePath;

	void InitVrPerfkit(HMODULE module) {
		g_dllPath = vrperfkit::GetModulePath(module);
		g_basePath = g_dllPath.parent_path();
		g_executablePath = vrperfkit::GetModulePath(nullptr);

		vrperfkit::OpenLogFile(g_basePath / "vrperfkit.log");
		LOG_INFO << "======================";
		LOG_INFO << "VR Performance Toolkit";
		LOG_INFO << "======================\n";
		vrperfkit::FlushLog();
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		InitVrPerfkit(module);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
