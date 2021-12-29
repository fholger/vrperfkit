#pragma once
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

		LogMessage& operator<<(std::basic_ostream &(*fn)(std::basic_ostream&)) {
			
		}

	private:
		bool flush;
	};
}
