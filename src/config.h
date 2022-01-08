#pragma once
#include "types.h"

#include <filesystem>

namespace vrperfkit {
	struct UpscaleConfig {
		bool enabled = false;
		UpscaleMethod method = UpscaleMethod::FSR;
		float renderScale = 1.0f;
		float sharpness = 0.7f;
		float radius = 0.6f;
		bool applyMipBias = true;
	};

	struct DxvkConfig {
		bool enabled = false;
		std::string dxgiDllPath = "dxvk\\dxgi.dll";
		std::string d3d11DllPath = "dxvk\\d3d11.dll";

		// not actually a config option, but a real-time toggle hack...
		bool shouldUseDxvk = true;
	};

	struct Config {
		UpscaleConfig upscaling;
		DxvkConfig dxvk;
		bool debugMode = false;
		std::string dllLoadPath = "";
	};

	extern Config g_config;

	void LoadConfig(const std::filesystem::path &configPath);
	void PrintCurrentConfig();
}
