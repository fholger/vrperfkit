#include "hotkeys.h"
#include "logging.h"

#include <functional>
#include <yaml-cpp/yaml.h>

#include "win_header_sane.h"

namespace {
	using vrperfkit::g_config;

	void CycleUpscalingMethod() {}

	void IncreaseUpscalingRadius() {
		g_config.upscaling.radius += 0.05f;
		LOG_INFO << "New radius: " << g_config.upscaling.radius;
	}

	void DecreaseUpscalingRadius() {
		g_config.upscaling.radius = std::max(0.f, g_config.upscaling.radius - 0.05f);
		LOG_INFO << "New radius: " << g_config.upscaling.radius;
	}

	void IncreaseUpscalingSharpness() {
		g_config.upscaling.sharpness = std::min(1.f, g_config.upscaling.sharpness + 0.05f);
		LOG_INFO << "New sharpness: " << g_config.upscaling.sharpness;
	}

	void DecreaseUpscalingSharpness() {
		g_config.upscaling.sharpness = std::max(0.f, g_config.upscaling.sharpness - 0.05f);
		LOG_INFO << "New sharpness: " << g_config.upscaling.sharpness;
	}

	void ToggleDebugMode() {
		g_config.debugMode = !g_config.debugMode;
		LOG_INFO << "Debug mode is now " << (g_config.debugMode ? "enabled" : "disabled");
	}

	struct HotkeyDefinition {
		std::string name;
		std::function<void()> action;
	};
	std::vector<HotkeyDefinition> g_hotkeyDefinitions;

	void InitDefinitions() {
		g_hotkeyDefinitions = {
			{"cycleUpscalingMethod", CycleUpscalingMethod},
			{"increaseUpscalingRadius", IncreaseUpscalingRadius},
			{"decreaseUpscalingRadius", DecreaseUpscalingRadius},
			{"increaseUpscalingSharpness", IncreaseUpscalingSharpness},
			{"decreaseUpscalingSharpness", DecreaseUpscalingSharpness},
			{"toggleDebugMode", ToggleDebugMode},
		};
	}

	struct HotkeyState {
		std::vector<int> keys;
		std::function<void()> action;
		bool wasActive = false;
	};
	std::vector<HotkeyState> g_hotkeyStates;

	std::unordered_map<std::string, int> g_virtualKeyMap = {
		{ "backspace", VK_BACK },
		{ "tab", VK_TAB },
		{ "enter", VK_RETURN },
		{ "return", VK_RETURN },
		{ "shift", VK_SHIFT },
		{ "ctrl", VK_CONTROL },
		{ "alt", VK_MENU },
		{ "pause", VK_PAUSE },
		{ "esc", VK_ESCAPE },
		{ "space", VK_SPACE },
		{ "pgup", VK_PRIOR },
		{ "pgdown", VK_NEXT },
		{ "end", VK_END },
		{ "home", VK_HOME },
		{ "left", VK_LEFT },
		{ "up", VK_UP },
		{ "right", VK_RIGHT },
		{ "down", VK_DOWN },
		{ "print", VK_PRINT },
		{ "insert", VK_INSERT },
		{ "delete", VK_DELETE },
		{ "num0", VK_NUMPAD0 },
		{ "num1", VK_NUMPAD1 },
		{ "num2", VK_NUMPAD2 },
		{ "num3", VK_NUMPAD3 },
		{ "num4", VK_NUMPAD4 },
		{ "num5", VK_NUMPAD5 },
		{ "num6", VK_NUMPAD6 },
		{ "num7", VK_NUMPAD7 },
		{ "num8", VK_NUMPAD8 },
		{ "num9", VK_NUMPAD9 },
		{ "f1", VK_F1 },
		{ "f2", VK_F2 },
		{ "f3", VK_F3 },
		{ "f4", VK_F4 },
		{ "f5", VK_F5 },
		{ "f6", VK_F6 },
		{ "f7", VK_F7 },
		{ "f8", VK_F8 },
		{ "f9", VK_F9 },
		{ "f10", VK_F10 },
		{ "f11", VK_F11 },
		{ "f12", VK_F12 },
	};

	int ToVirtualKey(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		if (s.size() == 1 && std::isalnum(s[0])) {
			return s[0];
		}

		auto it = g_virtualKeyMap.find(s);
		if (it != g_virtualKeyMap.end()) {
			return it->second;
		}

		if (!s.empty()) {
			LOG_INFO << "Unknown hotkey value: " << s;
		}
		return -1;
	}

	bool g_hotkeysEnabled = false;
}

namespace vrperfkit {

	void LoadHotkeys(const std::filesystem::path &configPath) {
		InitDefinitions();
		g_hotkeyStates.clear();
		g_hotkeysEnabled = false;

		if (!exists(configPath)) {
			LOG_ERROR << "Config file not found, hotkeys will be disabled";
			return;
		}

		try {
			std::ifstream cfgFile (configPath);
			YAML::Node cfg = YAML::Load(cfgFile);

			YAML::Node hotkeysCfg = cfg["hotkeys"];
			g_hotkeysEnabled = hotkeysCfg["enabled"].as<bool>(g_hotkeysEnabled);
			for (const auto &def : g_hotkeyDefinitions) {
				auto &state = g_hotkeyStates.emplace_back();
				state.action = def.action;
				auto node = hotkeysCfg[def.name];
				if (node.IsSequence()) {
					for (auto key : node) {
						state.keys.push_back(ToVirtualKey(key.as<std::string>("")));
					}
				}
				else {
					state.keys.push_back(ToVirtualKey(node.as<std::string>("")));
				}
			}
		}
		catch (const YAML::Exception &e) {
			LOG_ERROR << "Failed to load configuration file: " << e.msg;
		}
	}

	void CheckHotkeys() {
		if (!g_hotkeysEnabled) {
			return;
		}

		for (auto &state : g_hotkeyStates) {
			if (state.keys.empty())
				continue;

			bool isActive = true;
			for (int key : state.keys) {
				isActive = isActive && GetAsyncKeyState(key);
			}

			if (isActive && !state.wasActive) {
				state.action();
			}

			state.wasActive = isActive;
		}
	}

	void PrintHotkeys() {
		if (!g_hotkeysEnabled)
			return;

		LOG_INFO << "Currently active hotkeys:";
		for (size_t i = 0; i < g_hotkeyDefinitions.size(); ++i) {
			bool hasValidConfig = !g_hotkeyStates[i].keys.empty();
			std::ostringstream keys;
			for (int key : g_hotkeyStates[i].keys) {
				hasValidConfig = hasValidConfig && key != -1;
				keys << " " << key;
			}
			if (hasValidConfig) {
				LOG_INFO << "  * " << g_hotkeyDefinitions[i].name << keys.str();
			}
		}
	}
}
