#include "logging.h"

#include <mutex>
#include <ctime>

namespace fs = std::filesystem;

namespace {
	std::mutex g_logMutex;
}

namespace vrperfkit {
	std::ofstream g_logFile;

	void OpenLogFile(fs::path path) {
		g_logFile.open(path, std::ios_base::trunc | std::ios_base::out);
		g_logFile.setf(std::ios::showbase);
	}

	void FlushLog() {
		g_logFile << std::flush;
	}

	LogMessage::LogMessage(const std::string &prefix, bool flush) : flush(flush) {
		char timeBuf[16];
		std::time_t now = std::time(nullptr);
		tm localTime;
		localtime_s(&localTime, &now);
		std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &localTime);

		g_logMutex.lock();
		g_logFile << timeBuf << " [" << std::this_thread::get_id() << "] " << prefix;
	}

	LogMessage::~LogMessage() {
		g_logFile << "\n";
		if (flush)
			g_logFile << std::flush;
		g_logMutex.unlock();
	}
}
