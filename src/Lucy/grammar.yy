%glr-parser
%expect 130
%expect-rr 1

%code requires
{

#include <sstream>
#include <string>
#include <variant>

class Driver;

#include "AST.hpp"
}

%{

#include "Driver.hpp"
#include "Logger.hpp"
#include "Scanner.hpp"

#undef yylex
#define yylex driver.m_scanner.token

%}

%skeleton "lalr1.cc"

%defines
%define api.parser.class {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.error verbose
%parse-param {Driver &driver}

%locations

%start root

%type <std::unique_ptr <AST::Chunk> > block chunk chunk_base else function_body_block
%type <std::unique_ptr <AST::Node> > expr prefix_expr statement last_statement
%type <std::pair <std::unique_ptr<AST::Node>, std::unique_ptr<AST::Chunk> > > else_if
%type <std::vector <std::pair <std::unique_ptr<AST::Node>, std::unique_ptr<AST::Chunk> > > > else_if_list
%type <std::unique_ptr <AST::If> > if
%type <std::unique_ptr <AST::ParamList> > name_list param_list
%type <std::unique_ptr <AST::ExprList> > args expr_list
%type <std::unique_ptr <AST::FunctionCall> > function_call
%type <std::unique_ptr <AST::VarList> > var_list
%type <std::unique_ptr <AST::LValue> > var
%type <std::unique_ptr <AST::TableCtor> > field_list field_list_base table_ctor
%type <std::unique_ptr <AST::Field> > field
%type <std::unique_ptr <AST::Function> > function function_body
%type <std::unique_ptr <AST::FunctionName> > function_name function_name_base

%token <long> INT_VALUE
%token <double> REAL_VALUE
%token <std::string> ID STRING_VALUE
%token NIL TRUE FALSE ELLIPSIS
%token BREAK RETURN FUNCTION DO WHILE END REPEAT UNTIL FOR IF THEN ELSE ELSEIF IN LOCAL
%token HASH NOT
%token LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE DOT COMMA SEMICOLON COLON
%token END_OF_INPUT 0 "eof"

%right ASSIGN
%left COMMA SEMICOLON
%left OR
%left AND
%left EQ NE LT LE GT GE
%left CONCAT
%left PLUS MINUS
%left MUL DIV MOD
%right NOT LENGTH
%precedence NEGATE
%right POWER

%%

root :
chunk {
	driver.addChunk(std::move($chunk));
}
;

opt_semicolon :
SEMICOLON {
}
| %empty {
}
;

chunk :
last_statement {
	$$ = std::make_unique<AST::Chunk>(std::move(@$));
	$$->append(std::move($last_statement));
}
| chunk_base last_statement {
	$$ = std::move($chunk_base);
	$$->append(std::move($last_statement));
}
| chunk_base {
	$$ = std::move($chunk_base);
}
| %empty {
	$$ = std::make_unique<AST::Chunk>(@$);
}
;

chunk_base :
statement opt_semicolon {
	$$ = std::make_unique<AST::Chunk>(@$);
	$$->append(std::move($statement));
}
| chunk statement opt_semicolon {
	$$ = std::move($chunk);
	$$->append(std::move($statement));
}
;

block :
chunk {
	$$ = std::move($chunk);
}
;

prefix_expr :
var {
	$$ = std::move($var);
}
| function_call {
	$$ = std::move($function_call);
}
| LPAREN expr RPAREN {
	$$ = std::make_unique<AST::NestedExpr>(std::move($expr), @$);
}
;

statement :
var_list ASSIGN expr_list {
	$$ = std::make_unique<AST::Assignment>(std::move($var_list), std::move($expr_list), @$);
}
| function_call {
	$$ = std::move($function_call);
}
| DO block END {
	$$ = std::move($block);
}
| WHILE expr DO block END {
	$$ = std::make_unique<AST::While>(std::move($expr), std::move($block), @$);
}
| REPEAT block UNTIL expr {
	$$ = std::make_unique<AST::Repeat>(std::move($expr), std::move($block), @$);
}
| if else_if_list else END {
	for (auto &&p : $else_if_list)
		$if->addElseIf(std::move(p.first), std::move(p.second));
	$if->setElse(std::move($else));
	$$ = std::move($if);
}
| FOR ID ASSIGN expr[start] COMMA expr[limit] COMMA expr[step] DO block END {
	$$ = std::make_unique<AST::For>($ID, std::move($start), std::move($limit), std::move($step), std::move($block), @$);
}
| FOR ID ASSIGN expr[start] COMMA expr[limit] DO block END {
	$$ = std::make_unique<AST::For>($ID, std::move($start), std::move($limit), nullptr, std::move($block), @$);
}
| FOR name_list IN expr_list DO block END {
	$$ = std::make_unique<AST::ForEach>(std::move($name_list), std::move($expr_list), std::move($block), @$);
}
| FUNCTION function_name function_body {
	$function_body->setName(std::move($function_name));
	$$ = std::move($function_body);
}
| LOCAL FUNCTION ID function_body {
	$function_body->setName(std::make_unique<AST::FunctionName>(std::move($ID), @ID));
	$function_body->setLocal();
	$$ = std::move($function_body);
}
| LOCAL name_list {
	auto tmp = std::make_unique<AST::Assignment>(std::move($name_list), nullptr, @$);
	tmp->setLocal(true);
	$$ = std::move(tmp);
}
| LOCAL name_list ASSIGN expr_list {
	auto tmp = std::make_unique<AST::Assignment>(std::move($name_list), std::move($expr_list), @$);
	tmp->setLocal(true);
	$$ = std::move(tmp);
}
;

function_name :
function_name_base {
	$$ = std::move($function_name_base);
}
| function_name_base COLON ID {
	$$ = std::move($function_name_base);
	$$->appendMethodName(std::move($ID), @$);
}
;

function_name_base :
ID {
	$$ = std::make_unique<AST::FunctionName>(std::move($ID), @$);
}
| function_name_base[base] DOT ID {
	$$ = std::move($base);
	$$->appendNamePart(std::move($ID), @$);
}
;

if :
IF expr THEN block {
	$$ = std::make_unique<AST::If>(std::move($expr), std::move($block), @$);
}
;

else :
ELSE block {
	$$ = std::move($block);
}
| %empty {
	$$ = nullptr;
}
;

else_if :
ELSEIF expr THEN block {
	$$ = std::make_pair(std::move($expr), std::move($block));
}
;

else_if_list :
else_if {
	$$ = std::vector <std::pair <std::unique_ptr <AST::Node>, std::unique_ptr<AST::Chunk> > >{};
	$$.emplace_back(std::move($else_if));
}
| else_if_list[base] else_if {
	$$ = std::move($base);
	$$.emplace_back(std::move($else_if));
}
| %empty {
	$$ = std::vector <std::pair <std::unique_ptr <AST::Node>, std::unique_ptr <AST::Chunk> > >{};
}
;

last_statement :
RETURN expr_list opt_semicolon {
	$$ = std::make_unique<AST::Return>(std::move($expr_list), @$);
}
| RETURN opt_semicolon {
	$$ = std::make_unique<AST::Return>(nullptr, @$);
}
| BREAK opt_semicolon {
	$$ = std::make_unique<AST::Break>(@$);
};

expr_list :
expr {
	$$ = std::make_unique<AST::ExprList>(@$);
	$$->append(std::move($expr));
}
| expr_list[exprs] COMMA expr {
	$$ = std::move($exprs);
	$$->append(std::move($expr));
}
;

var_list :
var {
	$$ = std::make_unique<AST::VarList>(@$);
	$$->append(std::move($var));
}
| var_list[vars] COMMA var {
	$$ = std::move($vars);
	$$->append(std::move($var));
}
;

var :
ID {
	$$ = std::make_unique<AST::LValue>($ID, @$);
}
| prefix_expr LBRACKET expr RBRACKET {
	$$ = std::make_unique<AST::LValue>(std::move($prefix_expr), std::move($expr), @$);
}
| prefix_expr DOT ID {
	$$ = std::make_unique<AST::LValue>(std::move($prefix_expr), $ID, @$);
}
;

function_call :
prefix_expr args {
	$$ = std::make_unique<AST::FunctionCall>(std::move($prefix_expr), std::move($args), @$);
}
| prefix_expr COLON ID args {
	yy::location fnExprLocation = @prefix_expr;
	fnExprLocation.end = @ID.end;
	$args->prepend($prefix_expr->clone());
	auto fnExpr = std::make_unique<AST::LValue>(std::move($prefix_expr), $ID, fnExprLocation);
	$$ = std::make_unique<AST::FunctionCall>(std::move(fnExpr), std::move($args), @$);
}
;

args :
LPAREN expr_list RPAREN {
	$$ = std::move($expr_list);
}
| LPAREN RPAREN {
	$$ = std::make_unique<AST::ExprList>(@$);
}
| table_ctor {
	$$ = std::make_unique<AST::ExprList>(@$);
	$$->append(std::move($table_ctor));
}
| STRING_VALUE {
	$$ = std::make_unique<AST::ExprList>(@$);
	$$->append(std::make_unique<AST::StringValue>($STRING_VALUE, @$));
}
;

function :
FUNCTION function_body {
	$$ = std::move($function_body);
}
;

function_body :
LPAREN RPAREN function_body_block[block] {
	$$ = std::make_unique<AST::Function>(std::make_unique<AST::ParamList>(), std::move($block), @$);
}
| LPAREN param_list RPAREN function_body_block[block] {
	$$ = std::make_unique<AST::Function>(std::move($param_list), std::move($block), @$);
}
;

function_body_block :
block END {
	$$ = std::move($block);
}
| END {
	$$ = std::make_unique<AST::Chunk>(@$);
}
;

name_list :
ID {
	$$ = std::make_unique<AST::ParamList>(@$);
	$$->append($ID, @$);
}
| name_list[names] COMMA ID {
	$$ = std::move($names);
	$$->append($ID, @ID);
}
;

param_list :
name_list COMMA ELLIPSIS {
	$$ = std::move($name_list);
	$$->setEllipsis();
}
| name_list {
	$$ = std::move($name_list);
}
| ELLIPSIS {
	$$ = std::make_unique<AST::ParamList>(@$);
	$$->setEllipsis();
}
;

expr :
NIL {
	$$ = std::make_unique<AST::NilValue>(@$);
}
| FALSE {
	$$ = std::make_unique<AST::BooleanValue>(false, @$);
}
| TRUE {
	$$ = std::make_unique<AST::BooleanValue>(true, @$);
}
| INT_VALUE {
	$$ = std::make_unique<AST::IntValue>($INT_VALUE, @$);
}
| REAL_VALUE {
	$$ = std::make_unique<AST::RealValue>($REAL_VALUE, @$);
}
| STRING_VALUE {
	$$ = std::make_unique<AST::StringValue>($STRING_VALUE, @$);
}
| ELLIPSIS {
	$$ = std::make_unique<AST::Ellipsis>(@$);
}
| function {
	$$ = std::move($function);
}
| expr[left] OR expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Or, std::move($left), std::move($right), @$);
}
| expr[left] AND expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::And, std::move($left), std::move($right), @$);
}
| expr[left] LT expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Less, std::move($left), std::move($right), @$);
}
| expr[left] LE expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::LessEqual, std::move($left), std::move($right), @$);
}
| expr[left] GT expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Greater, std::move($left), std::move($right), @$);
}
| expr[left] GE expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::GreaterEqual, std::move($left), std::move($right), @$);
}
| expr[left] EQ expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Equal, std::move($left), std::move($right), @$);
}
| expr[left] NE expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::NotEqual, std::move($left), std::move($right), @$);
}
| expr[left] PLUS expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Plus, std::move($left), std::move($right), @$);
}
| expr[left] MINUS expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Minus, std::move($left), std::move($right), @$);
}
| expr[left] MUL expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Times, std::move($left), std::move($right), @$);
}
| expr[left] DIV expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Divide, std::move($left), std::move($right), @$);
}
| expr[left] MOD expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Modulo, std::move($left), std::move($right), @$);
}
| expr[left] POWER expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Exponentation, std::move($left), std::move($right), @$);
}
| expr[left] CONCAT expr[right] {
	$$ = std::make_unique<AST::BinOp>(AST::BinOp::Type::Concat, std::move($left), std::move($right), @$);
}
| MINUS expr[neg] %prec NEGATE {
	if ($neg->isValue()) {
		const auto *v = static_cast<const AST::Value *>($neg.get());
		if (v->valueType() == ValueType::Integer) {
			$$ = std::make_unique<AST::IntValue>(-static_cast<const AST::IntValue *>(v)->value(), @$);
		} else if (v->valueType() == ValueType::Real) {
			$$ = std::make_unique<AST::RealValue>(-static_cast<const AST::RealValue *>(v)->value(), @$);
		} else {
			error(@$, "Invalid ValueType for unary minus operator");
			YYABORT;
		}
	} else {
		$$ = std::make_unique<AST::UnOp>(AST::UnOp::Type::Negate, std::move($neg), @$);
	}
}
| NOT expr[not] {
	$$ = std::make_unique<AST::UnOp>(AST::UnOp::Type::Not, std::move($not), @$);
}
| HASH expr[len] {
	$$ = std::make_unique<AST::UnOp>(AST::UnOp::Type::Length, std::move($len), @$);
}
| table_ctor {
	$$ = std::move($table_ctor);
}
| prefix_expr {
	$$ = std::move($prefix_expr);
}
;

table_ctor :
LBRACE RBRACE {
	$$ = std::make_unique<AST::TableCtor>(@$);
}
| LBRACE field_list RBRACE {
	$$ = std::move($field_list);
}
;

field_list : field_list_base opt_field_separator {
	$$ = std::move($field_list_base);
}

field_list_base :
field {
	$$ = std::make_unique<AST::TableCtor>(@$);
	$$->append(std::move($field));
}
| field_list_base[fields] field_separator field {
	$$ = std::move($fields);
	$$->append(std::move($field));
}
;

field_separator :
COMMA {
}
| SEMICOLON {
}
;

opt_field_separator :
field_separator {
}
| %empty {
}
;

field :
LBRACKET expr[key] RBRACKET ASSIGN expr[val] {
	$$ = std::make_unique<AST::Field>(std::move($key), std::move($val), @$);
}
| ID[key] ASSIGN expr[val] {
	$$ = std::make_unique<AST::Field>(std::move($key), std::move($val), @$);
}
| expr[val] {
	$$ = std::make_unique<AST::Field>(std::move($val), @$);
}
;

%%

void yy::Parser::error(const location &loc, const std::string &msg)
{
	std::ostringstream ss;
	ss << "Parse error: " << loc << " : " << msg << '\n';
	driver.logError(ss.str());
}
