#pragma once

namespace IR {

enum class Op : unsigned {
	//keep this in the same order as AST::BinOp::Type
	Or,
	And,
	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,
	Concat,
	Plus,
	Minus,
	Times,
	Divide,
	Modulo,
	Exponentation,
	//AST::BinOp::Type ends here

	UnaryNegate,
	UnaryNot,
	UnaryLength,

	Assign,
	TableAssign,
	TableCtor,
	TableIndex,
	Push,
	Pop,
	Call,
	CallUnknownResults,
	Return,

	Test,
	Jump,
	JumpTrue,
};

} //namespace IR
