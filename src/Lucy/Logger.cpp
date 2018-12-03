#include <fstream>

#include "Logger.hpp"

void Logger::setOutput(const std::string &filename)
{
	static std::ofstream output;

	std::ofstream tmp{filename};
	if (tmp.fail())
		FATAL("Unable to open file for writing: " << filename << '\n');

	output = std::move(tmp);
	instance().m_output = &output;
}

Logger & Logger::instance()
{
	static Logger obj;
	return obj;
}
