#include "config.h"

#include "logging.h"
#include "yaml-cpp/yaml.h"

#include <fstream>

namespace fs = std::filesystem;

namespace vrperfkit {
	UpscaleMethod MethodFromString(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		if (s == "fsr") {
			return UpscaleMethod::FSR;
		}
		if (s == "nis") {
			return UpscaleMethod::NIS;
		}
		if (s == "cas") {
			return UpscaleMethod::CAS;
		}
		LOG_INFO << "Unknown upscaling method " << s << ", defaulting to FSR";
		return UpscaleMethod::FSR;
	}

	std::string MethodToString(UpscaleMethod method) {
		switch (method) {
		case UpscaleMethod::FSR:
			return "FSR";
		case UpscaleMethod::NIS:
			return "NIS";
		case UpscaleMethod::CAS:
			return "CAS";
		}
	}

	FixedFoveatedMethod FFRMethodFromString(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		if (s == "vrs") {
			return FixedFoveatedMethod::VRS;
		}
		LOG_INFO << "Unknown fixed foveated method " << s << ", defaulting to VRS";
		return FixedFoveatedMethod::VRS;
	}

	std::string FFRMethodToString(FixedFoveatedMethod method) {
		switch (method) {
		case FixedFoveatedMethod::VRS:
			return "VRS";
		}
	}

	std::string PrintToggle(bool toggle) {
		return toggle ? "enabled" : "disabled";
	}

	Config g_config;

	void LoadConfig(const fs::path &configPath) {
		g_config = Config();

		if (!exists(configPath)) {
			LOG_ERROR << "Config file not found, falling back to defaults";
			return;
		}

		try {
			std::ifstream cfgFile (configPath);
			YAML::Node cfg = YAML::Load(cfgFile);

			YAML::Node upscaleCfg = cfg["upscaling"];
			UpscaleConfig &upscaling= g_config.upscaling;
			upscaling.enabled = upscaleCfg["enabled"].as<bool>(upscaling.enabled);
			upscaling.method = MethodFromString(upscaleCfg["method"].as<std::string>(MethodToString(upscaling.method)));
			upscaling.renderScale = upscaleCfg["renderScale"].as<float>(upscaling.renderScale);
			if (upscaling.renderScale < 0.5f) {
				LOG_INFO << "Setting render scale to minimum value of 0.5";
				upscaling.renderScale = 0.5f;
			}
			upscaling.sharpness = std::max(0.f, upscaleCfg["sharpness"].as<float>(upscaling.sharpness));
			upscaling.radius = std::max(0.f, upscaleCfg["radius"].as<float>(upscaling.radius));
			upscaling.applyMipBias = upscaleCfg["applyMipBias"].as<bool>(upscaling.applyMipBias);

			YAML::Node dxvkCfg = cfg["dxvk"];
			DxvkConfig &dxvk = g_config.dxvk;
			dxvk.enabled = dxvkCfg["enabled"].as<bool>(dxvk.enabled);
			dxvk.dxgiDllPath = dxvkCfg["dxgiDllPath"].as<std::string>(dxvk.dxgiDllPath);
			dxvk.d3d11DllPath = dxvkCfg["d3d11DllPath"].as<std::string>(dxvk.d3d11DllPath);

			YAML::Node ffrCfg = cfg["fixedFoveated"];
			FixedFoveatedConfig &ffr = g_config.ffr;
			ffr.enabled = ffrCfg["enabled"].as<bool>(ffr.enabled);
			ffr.favorHorizontal = ffrCfg["favorHorizontal"].as<bool>(ffr.favorHorizontal);
			ffr.innerRadius = ffrCfg["innerRadius"].as<float>(ffr.innerRadius);
			ffr.midRadius = ffrCfg["midRadius"].as<float>(ffr.midRadius);
			ffr.outerRadius = ffrCfg["outerRadius"].as<float>(ffr.outerRadius);
			ffr.overrideSingleEyeOrder = ffrCfg["overrideSingleEyeOrder"].as<std::string>(ffr.overrideSingleEyeOrder);

			g_config.debugMode = cfg["debugMode"].as<bool>(g_config.debugMode);

			g_config.dllLoadPath = cfg["dllLoadPath"].as<std::string>(g_config.dllLoadPath);
		}
		catch (const YAML::Exception &e) {
			LOG_ERROR << "Failed to load configuration file: " << e.msg;
		}
	}

	void PrintCurrentConfig() {
		LOG_INFO << "Current configuration:";
		LOG_INFO << "  Upscaling (" << MethodToString(g_config.upscaling.method) << ") is " << PrintToggle(g_config.upscaling.enabled);
		if (g_config.upscaling.enabled) {
			LOG_INFO << "    * Render scale: " << std::setprecision(2) << g_config.upscaling.renderScale;
			LOG_INFO << "    * Sharpness:    " << std::setprecision(2) << g_config.upscaling.sharpness;
			LOG_INFO << "    * Radius:       " << std::setprecision(2) << g_config.upscaling.radius;
			LOG_INFO << "    * MIP bias:     " << PrintToggle(g_config.upscaling.applyMipBias);
		}
		LOG_INFO << "  Fixed foveated rendering (" << FFRMethodToString(g_config.ffr.method) << ") is " << PrintToggle(g_config.ffr.enabled);
		if (g_config.ffr.enabled) {
			LOG_INFO << "    * Inner radius: " << std::setprecision(2) << g_config.ffr.innerRadius;
			LOG_INFO << "    * Mid radius:   " << std::setprecision(2) << g_config.ffr.midRadius;
			LOG_INFO << "    * Outer radius: " << std::setprecision(2) << g_config.ffr.outerRadius;
			if (!g_config.ffr.overrideSingleEyeOrder.empty()) {
				LOG_INFO << "    * Eye order:    " << g_config.ffr.overrideSingleEyeOrder;
			}
		}
		LOG_INFO << "  Debug mode is " << PrintToggle(g_config.debugMode);
		FlushLog();
	}
}
