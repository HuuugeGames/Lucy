#pragma once

#include <variant>
#include "location.hh"
#include "EnumHelpers.hpp"

namespace Issue {

EnumClass(Type, uint32_t,
	EmptyChunk,
	GlobalFunctionDefinition,
	ShadowingDefinition,

	Function_DuplicateParam,
	Function_DuplicateParamSelf,
	Function_EllipsisShadowsParam,
	Function_EmptyDefinition,
	Function_FallthroughNoResult,
	Function_UnusedParam,
	Function_UnusedParamUnderscore,
	Function_VariableResultCount,

	GlobalStore_FunctionScope,
	GlobalStore_GlobalScope,
	GlobalStore_Underscore,
	GlobalStore_UpperCase
);

class BaseIssue {
public:
	Type type() const { return m_type; }
	const yy::location & location() const { return m_location; }

	bool operator < (const BaseIssue &other) const
	{
		if (m_type.value() != other.m_type.value())
			return m_type.value() < other.m_type.value();

		auto positionCmp = [](const yy::position &a, const yy::position &b) -> int
		{
			const std::string &filenameA = a.filename ? *a.filename : std::string{""};
			const std::string &filenameB = b.filename ? *b.filename : std::string{""};

			const int filenameCmp = filenameA.compare(filenameB);
			if (filenameCmp != 0)
				return filenameCmp;
			if (a.line != b.line)
				return a.line - b.line;
			if (a.column != b.column)
				return a.column - b.column;

			return 0;
		};

		const int cmp = positionCmp(m_location.begin, other.m_location.begin);
		if (cmp != 0)
			return cmp < 0;

		return positionCmp(m_location.end, other.m_location.end) < 0;
	}

protected:
	BaseIssue(Type t, const yy::location &location) : m_type{t}, m_location{location} {}

	virtual explicit operator std::string() const = 0;

private:
	Type m_type;
	yy::location m_location;
};

class EmptyChunk : public BaseIssue {
public:
	EmptyChunk(const yy::location &location) : BaseIssue{Type::EmptyChunk, location} {}

	explicit operator std::string() const override;
};

class GlobalFunctionDefinition : public BaseIssue {
public:
	GlobalFunctionDefinition(const yy::location &location, const std::string &fnName)
		: BaseIssue{Type::GlobalFunctionDefinition, location}, m_fnName{fnName} {}

	explicit operator std::string() const override;

private:
	std::string m_fnName;
};

class ShadowingDefinition : public BaseIssue {
public:
	ShadowingDefinition(const yy::location &location, const std::string &varName, const yy::location &varLocation)
		: BaseIssue{Type::ShadowingDefinition, location}, m_varName{varName}, m_varLocation{varLocation} {}

	explicit operator std::string() const override;

private:
	std::string m_varName;
	yy::location m_varLocation;
};

} //namespace Issue

namespace Issue::Function {

class DuplicateParam : public BaseIssue {
public:
	DuplicateParam(const yy::location &location, const std::string &paramName)
		: BaseIssue{Type::Function_DuplicateParam, location}, m_paramName{paramName} {}

	explicit operator std::string() const override;

private:
	std::string m_paramName;
};

class DuplicateParamSelf : public BaseIssue {
public:
	DuplicateParamSelf(const yy::location &location) : BaseIssue{Type::Function_DuplicateParamSelf, location} {}

	explicit operator std::string() const override;
};

class EllipsisShadowsParam : public BaseIssue {
public:
	EllipsisShadowsParam(const yy::location &location) : BaseIssue{Type::Function_EllipsisShadowsParam, location} {}

	explicit operator std::string() const override;
};

class EmptyDefinition : public BaseIssue {
public:
	EmptyDefinition(const yy::location &location) : BaseIssue{Type::Function_EmptyDefinition, location} {}

	explicit operator std::string() const override;
};

class FallthroughNoResult : public BaseIssue {
public:
	FallthroughNoResult(const yy::location &location) : BaseIssue{Type::Function_FallthroughNoResult, location} {}

	explicit operator std::string() const override;
};

class UnusedParam : public BaseIssue {
public:
	UnusedParam(const yy::location &location, const std::string &paramName)
		: BaseIssue{Type::Function_UnusedParam, location}, m_paramName{paramName} {}

	explicit operator std::string() const override;

private:
	std::string m_paramName;
};

class UnusedParamUnderscore : public BaseIssue {
public:
	UnusedParamUnderscore(const yy::location &location) : BaseIssue{Type::Function_UnusedParamUnderscore, location} {}

	explicit operator std::string() const override;
};

class VariableResultCount : public BaseIssue {
public:
	VariableResultCount(const yy::location &location, unsigned retValCnt, unsigned retValAltCnt)
		: BaseIssue{Type::Function_VariableResultCount, location}, m_retValCnt{retValCnt}, m_retValAltCnt{retValAltCnt} {}

	explicit operator std::string() const override;

private:
	unsigned m_retValCnt;
	unsigned m_retValAltCnt;
};

} //namespace Issue::Function

namespace Issue::GlobalStore {

class FunctionScope : public BaseIssue {
public:
	FunctionScope(const yy::location &location, const std::string &varName)
		: BaseIssue{Type::GlobalStore_FunctionScope, location}, m_varName{varName} {}

	explicit operator std::string() const override;

private:
	std::string m_varName;
};

class GlobalScope : public BaseIssue {
public:
	GlobalScope(const yy::location &location, const std::string &varName)
		: BaseIssue{Type::GlobalStore_GlobalScope, location}, m_varName{varName} {}

	explicit operator std::string() const override;

private:
	std::string m_varName;
};

class Underscore : public BaseIssue {
public:
	Underscore(const yy::location &location) : BaseIssue{Type::GlobalStore_Underscore, location} {}

	explicit operator std::string() const override;
};

class UpperCase : public BaseIssue {
public:
	UpperCase(const yy::location &location, const std::string &varName)
		: BaseIssue{Type::GlobalStore_UpperCase, location}, m_varName{varName} {}

	explicit operator std::string() const override;

private:
	std::string m_varName;
};

} //namespace Issue::GlobalStore

using IssueVariant = std::variant <
	Issue::EmptyChunk,
	Issue::GlobalFunctionDefinition,
	Issue::ShadowingDefinition,
	Issue::Function::DuplicateParam,
	Issue::Function::DuplicateParamSelf,
	Issue::Function::EllipsisShadowsParam,
	Issue::Function::EmptyDefinition,
	Issue::Function::FallthroughNoResult,
	Issue::Function::UnusedParam,
	Issue::Function::UnusedParamUnderscore,
	Issue::Function::VariableResultCount,
	Issue::GlobalStore::FunctionScope,
	Issue::GlobalStore::GlobalScope,
	Issue::GlobalStore::Underscore,
	Issue::GlobalStore::UpperCase>;
