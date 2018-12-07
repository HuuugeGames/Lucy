#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "Bitfield.hpp"
#include "EnumHelpers.hpp"

EnumClass(Check, uint32_t,
	EmptyChunk,
	EmptyFunction,
	GlobalFunctionDefinition,
	GlobalStore_FunctionScope,
	GlobalStore_GlobalScope,
	GlobalStore_Underscore,
	GlobalStore_UpperCase,
	_last
);

class Logger {
public:
	enum : unsigned {
		Fatal = 0,
		Error = 1,
		Warning = 2,
		Pedantic = 3,
	};

	static void enable(Check c) { instance().setFlag(c); }
	static void disable(Check c) { instance().unsetFlag(c); }
	static bool dispatch(Check c);

	static std::ostream & log() { return *instance().m_output; }
	static unsigned threshold() { return instance().m_threshold; }
	static void setOutput(const std::string &filename);
	static void setOutput(std::ostream &os) { instance().m_output = &os; }
	static void setThreshold(unsigned threshold) { instance().m_threshold = threshold; }

private:
	Logger();
	Logger(const Logger &) = delete;
	void operator = (const Logger &) = delete;
	~Logger() = default;

	static Logger & instance();

	void setFlag(Check c) { m_flags.set(c.value()); }
	void unsetFlag(Check c) { m_flags.unset(c.value()); }

	Bitfield <Check::_last> m_flags;
	std::ostream *m_output = &std::cerr;
	unsigned m_threshold = std::numeric_limits<unsigned>::max();
};

#define LOG(severity, msg) \
	do { \
		if (severity <= Logger::threshold()) \
			Logger::log() << msg; \
	} while (false)

#define REPORT(check, msg) \
	do { \
		if (Logger::dispatch(check)) \
			Logger::log() << '[' << Check{check} << "] " << msg; \
	} while (false)

#define FATAL(msg) \
	do { \
		Logger::log() << msg; \
		exit(1); \
	} while (false)
