#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "Bitfield.hpp"
#include "Issue.hpp"

class Logger {
public:
	enum : unsigned {
		Fatal = 0,
		Error = 1,
		Warning = 2,
		Pedantic = 3,
	};

	static void enable(Issue::Type issue) { instance().setFlag(issue); }
	static void disable(Issue::Type issue) { instance().unsetFlag(issue); }
	static bool isEnabled(Issue::Type issue);

	template <typename IssueType, typename ...Ts>
	static void logIssue(Ts... args);
	static std::ostream & log() { return *instance().m_output; }
	static unsigned threshold() { return instance().m_threshold; }
	static void setOutput(const std::string &filename);
	static void setOutput(std::ostream &os) { instance().m_output = &os; }
	static void setThreshold(unsigned threshold) { instance().m_threshold = threshold; }

	static std::ostream & indent(std::ostream &os, unsigned level)
	{
		for (unsigned i = 0; i != level; ++i)
			os << '\t';
		return os;
	}

private:
	Logger();
	Logger(const Logger &) = delete;
	void operator = (const Logger &) = delete;
	~Logger() = default;

	static Logger & instance();

	void setFlag(Issue::Type issue) { m_flags.set(issue.value()); }
	void unsetFlag(Issue::Type issue) { m_flags.unset(issue.value()); }

	Bitfield <Issue::Type::_size> m_flags;
	std::ostream *m_output = &std::cerr;
	unsigned m_threshold = std::numeric_limits<unsigned>::max();
	std::vector <IssueVariant> m_issues;
};

#define LOG(severity, msg) \
	do { \
		if (severity <= Logger::threshold()) \
			Logger::log() << msg; \
	} while (false)

#define FATAL(msg) \
	do { \
		Logger::log() << '[' << __FILE__ << ':' << __LINE__ << "] " << msg; \
		::exit(1); \
	} while (false)

template <typename IssueType, typename ...Ts>
void Logger::logIssue(Ts... args)
{
	auto &logger = instance();
	auto &issueVec = logger.m_issues;
	const auto &issue = std::get<IssueType>(issueVec.emplace_back(IssueType{args...}));

	if (logger.isEnabled(issue.type()))
		logger.log() << std::string{issue} << '\n';
}
