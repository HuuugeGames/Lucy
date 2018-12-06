#include <fstream>

#include "Logger.hpp"

Logger::Logger()
{
	setFlag(Check::EmptyChunk);
	unsetFlag(Check::EmptyFunction);

	setFlag(Check::GlobalStore_FunctionScope);
	setFlag(Check::GlobalStore_GlobalScope);
	unsetFlag(Check::GlobalStore_Underscore);
	setFlag(Check::GlobalStore_UpperCase);
}

bool Logger::dispatch(Check c)
{
	return instance().m_flags.get(c.value());
}

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
