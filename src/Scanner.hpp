#pragma once

#undef yyFlexLexer
#include <FlexLexer.h>

#include "Parser.hpp"

class Driver;

class Scanner : public yyFlexLexer {
public:
	Scanner(Driver &driver) : m_driver{driver} {}
	~Scanner() = default;

	yy::Parser::symbol_type token();
private:
	Driver &m_driver;
};
