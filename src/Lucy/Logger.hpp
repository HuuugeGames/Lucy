#pragma once

#include <iostream>
#include <limits>

class Logger {
public:
	enum : unsigned {
		Fatal = 0,
		Error = 1,
		Warning = 2,
		Pedantic = 3,
	};

	static std::ostream & log() { return *instance().m_output; }
	static unsigned threshold() { return instance().m_threshold; }
	static void setOutput(const std::string &filename);
	static void setThreshold(unsigned threshold) { instance().m_threshold = threshold; }
	//TODO set output

private:
	Logger() = default;
	Logger(const Logger &) = delete;
	void operator = (const Logger &) = delete;
	~Logger() = default;

	static Logger & instance();

	std::ostream *m_output = &std::cerr;
	unsigned m_threshold = std::numeric_limits<unsigned>::max();
};

#define LOG(severity, msg) \
	do { \
		if (severity <= Logger::threshold()) \
			Logger::log() << msg; \
	} while (false)

#define FATAL(msg) \
	do { \
		Logger::log() << msg; \
		exit(1); \
	} while (false)
