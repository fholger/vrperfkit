#pragma once
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>

#define LOG_INFO vrperfkit::LogMessage()
#define LOG_DEBUG vrperfkit::LogMessage("(DEBUG) ")
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

	private:
		bool flush;
	};
}
