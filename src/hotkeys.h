#pragma once
#include <filesystem>

namespace vrperfkit {
	void LoadHotkeys(const std::filesystem::path &configPath);
	void CheckHotkeys();
	void PrintHotkeys();
}
