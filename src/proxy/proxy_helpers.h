#pragma once
#include <string>
#include "win_header_sane.h"

namespace vrperfkit {
	void * GetDllFunctionPointer(HMODULE module, const std::string &name);

	void EnsureLoadDll(HMODULE &pModule, const std::string &name);
}
