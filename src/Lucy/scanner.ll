%{

#include "Driver.hpp"

#undef YY_DECL
#define YY_DECL yy::Parser::symbol_type Scanner::token()

static yy::position longStringStart;

%}

%x LongString

%option c++
%option noyywrap
%option yyclass="Scanner"
%option yylineno

DIGIT [0-9]
ID [a-zA-Z_][a-zA-Z0-9_]*

%%

break {
	return yy::Parser::make_BREAK(m_driver.location(YYText()));
}

return {
	return yy::Parser::make_RETURN(m_driver.location(YYText()));
}

nil {
	return yy::Parser::make_NIL(m_driver.location(YYText()));
}

true {
	return yy::Parser::make_TRUE(m_driver.location(YYText()));
}

false {
	return yy::Parser::make_FALSE(m_driver.location(YYText()));
}

function {
	return yy::Parser::make_FUNCTION(m_driver.location(YYText()));
}

do {
	return yy::Parser::make_DO(m_driver.location(YYText()));
}

while {
	return yy::Parser::make_WHILE(m_driver.location(YYText()));
}

repeat {
	return yy::Parser::make_REPEAT(m_driver.location(YYText()));
}

until {
	return yy::Parser::make_UNTIL(m_driver.location(YYText()));
}

end {
	return yy::Parser::make_END(m_driver.location(YYText()));
}

for {
	return yy::Parser::make_FOR(m_driver.location(YYText()));
}

in {
	return yy::Parser::make_IN(m_driver.location(YYText()));
}

if {
	return yy::Parser::make_IF(m_driver.location(YYText()));
}

then {
	return yy::Parser::make_THEN(m_driver.location(YYText()));
}

elseif {
	return yy::Parser::make_ELSEIF(m_driver.location(YYText()));
}

else {
	return yy::Parser::make_ELSE(m_driver.location(YYText()));
}

local {
	return yy::Parser::make_LOCAL(m_driver.location(YYText()));
}

0[xX]({DIGIT}|[A-Fa-f])+ {
	return yy::Parser::make_INT_VALUE(std::strtol(YYText(), nullptr, 16), m_driver.location(YYText()));
}

{DIGIT}+ {
	return yy::Parser::make_INT_VALUE(std::strtol(YYText(), nullptr, 10), m_driver.location(YYText()));
}

{DIGIT}*[.]{DIGIT}+|{DIGIT}+[.]{DIGIT}* {
	return yy::Parser::make_REAL_VALUE(std::strtod(YYText(), nullptr), m_driver.location(YYText()));
}

\"(\\.|[^\\"])*\"|\'(\\.|[^\\'])*\' {
	return yy::Parser::make_STRING_VALUE(std::string{YYText() + 1, static_cast<unsigned>(YYLeng() - 2)}, m_driver.location(YYText()));
}

\[{2} {
	longStringStart = m_driver.position();
	m_driver.location("[[");
	BEGIN(LongString);
}

<LongString>{
	\n {
		m_driver.nextLine();
		yymore();
	}
	[^\]] {
		m_driver.step();
		yymore();
	}
	\][^\]] {
		m_driver.step();
		yymore();
	}
	\]{2} {
		BEGIN(INITIAL);
		m_driver.location("]]");
		const auto longStringEnd = m_driver.position();
		return yy::Parser::make_STRING_VALUE(std::string{YYText(), static_cast<unsigned>(YYLeng() - 2)}, yy::location{longStringStart, longStringEnd});
	}
}

"..." {
	return yy::Parser::make_ELLIPSIS(m_driver.location(YYText()));
}

"=" {
	return yy::Parser::make_ASSIGN(m_driver.location(YYText()));
}

"or" {
	return yy::Parser::make_OR(m_driver.location(YYText()));
}

"and" {
	return yy::Parser::make_AND(m_driver.location(YYText()));
}

"not" {
	return yy::Parser::make_NOT(m_driver.location(YYText()));
}

"==" {
	return yy::Parser::make_EQ(m_driver.location(YYText()));
}

"~=" {
	return yy::Parser::make_NE(m_driver.location(YYText()));
}

"<" {
	return yy::Parser::make_LT(m_driver.location(YYText()));
}

"<=" {
	return yy::Parser::make_LE(m_driver.location(YYText()));
}

">" {
	return yy::Parser::make_GT(m_driver.location(YYText()));
}

">=" {
	return yy::Parser::make_GE(m_driver.location(YYText()));
}

".." {
	return yy::Parser::make_CONCAT(m_driver.location(YYText()));
}

"+" {
	return yy::Parser::make_PLUS(m_driver.location(YYText()));
}

"-" {
	return yy::Parser::make_MINUS(m_driver.location(YYText()));
}

"*" {
	return yy::Parser::make_MUL(m_driver.location(YYText()));
}

"/" {
	return yy::Parser::make_DIV(m_driver.location(YYText()));
}

"%" {
	return yy::Parser::make_MOD(m_driver.location(YYText()));
}

"^" {
	return yy::Parser::make_POWER(m_driver.location(YYText()));
}

"#" {
	return yy::Parser::make_HASH(m_driver.location(YYText()));
}

"(" {
	return yy::Parser::make_LPAREN(m_driver.location(YYText()));
}

")" {
	return yy::Parser::make_RPAREN(m_driver.location(YYText()));
}

"[" {
	return yy::Parser::make_LBRACKET(m_driver.location(YYText()));
}

"]" {
	return yy::Parser::make_RBRACKET(m_driver.location(YYText()));
}

"{" {
	return yy::Parser::make_LBRACE(m_driver.location(YYText()));
}

"}" {
	return yy::Parser::make_RBRACE(m_driver.location(YYText()));
}

"," {
	return yy::Parser::make_COMMA(m_driver.location(YYText()));
}

"." {
	return yy::Parser::make_DOT(m_driver.location(YYText()));
}

";" {
	return yy::Parser::make_SEMICOLON(m_driver.location(YYText()));
}

":" {
	return yy::Parser::make_COLON(m_driver.location(YYText()));
}

{ID} {
	return yy::Parser::make_ID(YYText(), m_driver.location(YYText()));
}

[ \t] {
	m_driver.step();
}

[\n] {
	m_driver.nextLine();
}

[\r] ;

<<EOF>> {
	return yy::Parser::make_END_OF_INPUT(m_driver.location(YYText()));
}

. {
	std::ostringstream ss;
	ss << '[' << m_driver.location() << "] Unrecognized input token: '" << YYText() << "'\n";
	m_driver.logError(ss.str());
	exit(1);
}

%%
