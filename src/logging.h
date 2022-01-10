#pragma once
#include <codecvt>
#include <filesystem>
#include <fstream>
#include "config.h"

#define LOG_INFO vrperfkit::LogMessage("", true)
#define LOG_DEBUG if (vrperfkit::g_config.debugMode) vrperfkit::LogMessage("(DEBUG) ")
#define LOG_ERROR vrperfkit::LogMessage("(!) ERROR: ", true)

namespace vrperfkit {
	extern std::ofstream g_logFile;

	void OpenLogFile(std::filesystem::path path);
	void FlushLog();

	class LogMessage {
	public:
		explicit LogMessage(const std::string &prefix = "", bool flush = false);
		~LogMessage();

		template<typename T>
		LogMessage& operator<<(const T &t) {
			g_logFile << t;
			return *this;
		}

		template<>
		LogMessage& operator<<(const std::wstring &str) {
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			return *this << conv.to_bytes(str);
		}

		LogMessage& operator<<(const wchar_t *msg) {
			return *this << std::wstring(msg);
		}

		LogMessage& operator<<(const UpscaleMethod &method) {
			return *this << MethodToString(method);
		}

	private:
		bool flush;
	};
}
