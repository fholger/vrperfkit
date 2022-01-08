#pragma once
#include <string>
#include "win_header_sane.h"
#include <filesystem>

namespace vrperfkit {
	extern HMODULE g_moduleSelf;

	std::filesystem::path GetSystemPath();

	void * GetDllFunctionPointer(HMODULE module, const std::string &name);

	void EnsureLoadDll(HMODULE &pModule, const std::filesystem::path &path);
}
