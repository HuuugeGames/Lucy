#include <sstream>

#include "Issue.hpp"

namespace Issue {

std::ostream & operator << (std::ostream &os, const BaseIssue &bi)
{
	return os << '[' << bi.type() << "] " << bi.location();
}

EmptyChunk::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : empty chunk of code";
	return ss.str();
}

GlobalFunctionDefinition::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : function definition in global scope: \"" << m_fnName << '"';
	return ss.str();
}

ShadowingDefinition::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : definition of a local variable \"" << m_varName << "\" shadows earlier uses of a variable with the same name (" << m_varLocation << ')';
	return ss.str();
}

} //namespace Issue

namespace Issue::Function {

DuplicateParam::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : duplicate function parameter: \"" << m_paramName << '"';
	return ss.str();
}

DuplicateParamSelf::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : parameter \"self\" clashes with implicit parameter of the same name";
	return ss.str();
}

EllipsisShadowsParam::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : parameter \"arg\" is shadowed by \"...\" (ellipsis)";
	return ss.str();
}

EmptyDefinition::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : empty function definition";
	return ss.str();
}

FallthroughNoResult::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : function might fall-through without returning any result";
	return ss.str();
}

UnusedParam::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : unused parameter: \"" << m_paramName << '"';
	return ss.str();
}

UnusedParamUnderscore::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : unused parameter: \"_\" (underscore)";
	return ss.str();
}

VariableResultCount::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : returning different number of results (" << m_retValAltCnt << ") than in other return statement (" << m_retValCnt << ')';
	return ss.str();
}

} //namespace Issue::Function

namespace Issue::GlobalStore {

FunctionScope::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : assignment to global name \"" << m_varName << "\" in function scope";
	return ss.str();
}

GlobalScope::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : assignment to global name \"" << m_varName << "\" in global scope";
	return ss.str();
}

Underscore::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : assignment to global name \"_\" (underscore)";
	return ss.str();
}

UpperCase::operator std::string() const
{
	std::ostringstream ss;
	ss << *this << " : assignment to global name \"" << m_varName << '"';
	return ss.str();
}

} //namespace Issue::GlobalStore
