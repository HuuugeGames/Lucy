#include <fstream>

#include "Logger.hpp"

Logger::Logger()
{
	setFlag(Issue::Type::EmptyChunk);

	setFlag(Issue::Type::Function_DuplicateParam);
	unsetFlag(Issue::Type::Function_EmptyDefinition);
	setFlag(Issue::Type::Function_UnusedParam);
	unsetFlag(Issue::Type::Function_UnusedParamUnderscore);
	setFlag(Issue::Type::Function_VariableResultCount);

	setFlag(Issue::Type::GlobalFunctionDefinition);
	setFlag(Issue::Type::GlobalStore_FunctionScope);
	setFlag(Issue::Type::GlobalStore_GlobalScope);
	unsetFlag(Issue::Type::GlobalStore_Underscore);
	setFlag(Issue::Type::GlobalStore_UpperCase);

	setFlag(Issue::Type::ShadowingDefinition);
}

bool Logger::isEnabled(Issue::Type issue)
{
	return instance().m_flags.get(issue.value());
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
