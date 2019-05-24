%glr-parser
%expect 130
%expect-rr 1

%code requires
{

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

%type <AST::Chunk *> block chunk chunk_base else function_body_block
%type <AST::Node *> expr prefix_expr statement last_statement
%type <std::pair <AST::Node *, AST::Chunk *> > else_if
%type <std::vector <std::pair <AST::Node *, AST::Chunk *> > > else_if_list
%type <AST::If *> if
%type <AST::ParamList *> name_list param_list
%type <AST::ExprList *> args expr_list
%type <AST::FunctionCall *> function_call
%type <AST::VarList *> var_list
%type <AST::LValue *> var
%type <AST::TableCtor *> field_list field_list_base table_ctor
%type <AST::Field *> field
%type <AST::Function *> function function_body
%type <AST::FunctionName *> function_name function_name_base

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
	driver.addChunk($chunk);
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
	$$ = new AST::Chunk{@$};
	$$->append($last_statement);
}
| chunk_base last_statement {
	$$ = $chunk_base;
	$$->append($last_statement);
}
| chunk_base {
	$$ = $chunk_base;
}
| %empty {
	$$ = new AST::Chunk{@$};
}
;

chunk_base :
statement opt_semicolon {
	$$ = new AST::Chunk{@$};
	$$->append($statement);
}
| chunk statement opt_semicolon {
	$$ = $chunk;
	$$->append($statement);
}
;

block :
chunk {
	$$ = $chunk;
}
;

prefix_expr :
var {
	$$ = $var;
}
| function_call {
	$$ = $function_call;
}
| LPAREN expr RPAREN {
	$$ = new AST::NestedExpr{$expr, @$};
}
;

statement :
var_list ASSIGN expr_list {
	$$ = new AST::Assignment{$var_list, $expr_list, @$};
}
| function_call {
	$$ = $function_call;
}
| DO block END {
	$$ = $block;
}
| WHILE expr DO block END {
	$$ = new AST::While{$expr, $block, @$};
}
| REPEAT block UNTIL expr {
	$$ = new AST::Repeat{$expr, $block, @$};
}
| if else_if_list else END {
	AST::If *tmp = $if;
	for (const auto &p : $else_if_list)
		tmp->addElseIf(p.first, p.second);
	tmp->setElse($else);
	$$ = tmp;
}
| FOR ID ASSIGN expr[start] COMMA expr[limit] COMMA expr[step] DO block END {
	$$ = new AST::For{$ID, $start, $limit, $step, $block, @$};
}
| FOR ID ASSIGN expr[start] COMMA expr[limit] DO block END {
	$$ = new AST::For{$ID, $start, $limit, nullptr, $block, @$};
}
| FOR name_list IN expr_list DO block END {
	$$ = new AST::ForEach{$name_list, $expr_list, $block, @$};
}
| FUNCTION function_name function_body {
	$function_body->setName($function_name);
	$$ = $function_body;
}
| LOCAL FUNCTION ID function_body {
	$function_body->setName(new AST::FunctionName{std:move($ID), @ID});
	$function_body->setLocal();
	$$ = $function_body;
}
| LOCAL name_list {
	auto tmp = new AST::Assignment{$name_list, nullptr, @$};
	tmp->setLocal(true);
	$$ = tmp;
}
| LOCAL name_list ASSIGN expr_list {
	auto tmp = new AST::Assignment{$name_list, $expr_list, @$};
	tmp->setLocal(true);
	$$ = tmp;
}
;

function_name :
function_name_base {
	$$ = $function_name_base;
}
| function_name_base COLON ID {
	$$ = $function_name_base;
	$$->appendMethodName(std::move($ID), @$);
}
;

function_name_base :
ID {
	$$ = new AST::FunctionName{std::move($ID), @$};
}
| function_name_base[base] DOT ID {
	$$ = $base;
	$$->appendNamePart(std::move($ID), @$);
}
;

if :
IF expr THEN block {
	$$ = new AST::If{$expr, $block, @$};
}
;

else :
ELSE block {
	$$ = $block;
}
| %empty {
	$$ = nullptr;
}
;

else_if :
ELSEIF expr THEN block {
	$$ = std::make_pair($expr, $block);
}
;

else_if_list :
else_if {
	$$ = std::vector <std::pair <AST::Node *, AST::Chunk *> >{};
	$$.emplace_back(std::move($else_if));
}
| else_if_list[base] else_if {
	$$ = std::move($base);
	$$.emplace_back(std::move($else_if));
}
| %empty {
	$$ = std::vector <std::pair <AST::Node *, AST::Chunk *> >{};
}
;

last_statement :
RETURN expr_list opt_semicolon {
	$$ = new AST::Return{$expr_list, @$};
}
| RETURN opt_semicolon {
	$$ = new AST::Return{nullptr, @$};
}
| BREAK opt_semicolon {
	$$ = new AST::Break{@$};
};

expr_list :
expr {
	$$ = new AST::ExprList{@$};
	$$->append($expr);
}
| expr_list[exprs] COMMA expr {
	$$ = $exprs;
	$$->append($expr);
}
;

var_list :
var {
	$$ = new AST::VarList{@$};
	$$->append($var);
}
| var_list[vars] COMMA var {
	$$ = $vars;
	$$->append($var);
}
;

var :
ID {
	$$ = new AST::LValue{$ID, @$};
}
| prefix_expr LBRACKET expr RBRACKET {
	$$ = new AST::LValue{$prefix_expr, $expr, @$};
}
| prefix_expr DOT ID {
	$$ = new AST::LValue{$prefix_expr, $ID, @$};
}
;

function_call :
prefix_expr args {
	$$ = new AST::FunctionCall{$prefix_expr, $args, @$};
}
| prefix_expr COLON ID args {
	yy::location fnExprLocation = @prefix_expr;
	fnExprLocation.end = @ID.end;
	AST::LValue *fnExpr = new AST::LValue{$prefix_expr, $ID, fnExprLocation};
	$args->prepend($prefix_expr->clone().release());
	$$ = new AST::FunctionCall{fnExpr, $args, @$};
}
;

args :
LPAREN expr_list RPAREN {
	$$ = $expr_list;
}
| LPAREN RPAREN {
	$$ = new AST::ExprList{@$};
}
| table_ctor {
	$$ = new AST::ExprList{@$};
	$$->append($table_ctor);
}
| STRING_VALUE {
	$$ = new AST::ExprList{@$};
	$$->append(new AST::StringValue{$STRING_VALUE, @$});
}
;

function :
FUNCTION function_body {
	$$ = $function_body;
}
;

function_body :
LPAREN RPAREN function_body_block[block] {
	$$ = new AST::Function{new AST::ParamList{}, $block, @$};
}
| LPAREN param_list RPAREN function_body_block[block] {
	$$ = new AST::Function{$param_list, $block, @$};
}
;

function_body_block :
block END {
	$$ = $block;
}
| END {
	$$ = new AST::Chunk{@$};
}
;

name_list :
ID {
	$$ = new AST::ParamList{@$};
	$$->append($ID, @$);
}
| name_list[names] COMMA ID {
	$$ = $names;
	$$->append($ID, @ID);
}
;

param_list :
name_list COMMA ELLIPSIS {
	$$ = $name_list;
	$$->setEllipsis();
}
| name_list {
	$$ = $name_list;
}
| ELLIPSIS {
	$$ = new AST::ParamList{@$};
	$$->setEllipsis();
}
;

expr :
NIL {
	$$ = new AST::NilValue{@$};
}
| FALSE {
	$$ = new AST::BooleanValue{false, @$};
}
| TRUE {
	$$ = new AST::BooleanValue{true, @$};
}
| INT_VALUE {
	$$ = new AST::IntValue{$INT_VALUE, @$};
}
| REAL_VALUE {
	$$ = new AST::RealValue{$REAL_VALUE, @$};
}
| STRING_VALUE {
	$$ = new AST::StringValue{$STRING_VALUE, @$};
}
| ELLIPSIS {
	$$ = new AST::Ellipsis{@$};
}
| function {
	$$ = $function;
}
| expr[left] OR expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Or, $left, $right, @$};
}
| expr[left] AND expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::And, $left, $right, @$};
}
| expr[left] LT expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Less, $left, $right, @$};
}
| expr[left] LE expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::LessEqual, $left, $right, @$};
}
| expr[left] GT expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Greater, $left, $right, @$};
}
| expr[left] GE expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::GreaterEqual, $left, $right, @$};
}
| expr[left] EQ expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Equal, $left, $right, @$};
}
| expr[left] NE expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::NotEqual, $left, $right, @$};
}
| expr[left] PLUS expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Plus, $left, $right, @$};
}
| expr[left] MINUS expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Minus, $left, $right, @$};
}
| expr[left] MUL expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Times, $left, $right, @$};
}
| expr[left] DIV expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Divide, $left, $right, @$};
}
| expr[left] MOD expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Modulo, $left, $right, @$};
}
| expr[left] POWER expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Exponentation, $left, $right, @$};
}
| expr[left] CONCAT expr[right] {
	$$ = new AST::BinOp{AST::BinOp::Type::Concat, $left, $right, @$};
}
| MINUS expr[neg] %prec NEGATE {
	if ($neg->isValue()) {
		const auto *v = static_cast<const AST::Value *>($neg);
		if (v->valueType() == ValueType::Integer) {
			$$ = new AST::IntValue{-static_cast<const AST::IntValue *>(v)->value(), @$};
		} else if (v->valueType() == ValueType::Real) {
			$$ = new AST::RealValue{-static_cast<const AST::RealValue *>(v)->value(), @$};
		} else {
			error(@$, "Invalid ValueType for unary minus operator");
			YYABORT;
		}

		delete $neg;
	} else {
		$$ = new AST::UnOp{AST::UnOp::Type::Negate, $neg, @$};
	}
}
| NOT expr[not] {
	$$ = new AST::UnOp{AST::UnOp::Type::Not, $not, @$};
}
| HASH expr[len] {
	$$ = new AST::UnOp{AST::UnOp::Type::Length, $len, @$};
}
| table_ctor {
	$$ = $table_ctor;
}
| prefix_expr {
	$$ = $prefix_expr;
}
;

table_ctor :
LBRACE RBRACE {
	$$ = new AST::TableCtor{@$};
}
| LBRACE field_list RBRACE {
	$$ = $field_list;
}
;

field_list : field_list_base opt_field_separator {
	$$ = $field_list_base;
}

field_list_base :
field {
	$$ = new AST::TableCtor{@$};
	$$->append($field);
}
| field_list_base[fields] field_separator field {
	$$ = $fields;
	$$->append($field);
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
	$$ = new AST::Field{$key, $val, @$};
}
| ID[key] ASSIGN expr[val] {
	$$ = new AST::Field{$key, $val, @$};
}
| expr[val] {
	$$ = new AST::Field{$val, @$};
}
;

%%

void yy::Parser::error(const location &loc, const std::string &msg)
{
	std::cerr << "Parse error: " << loc << " : " << msg << '\n';
}
