#include <array>
#include "AST.hpp"
#include "Fold.hpp"
#include "IROp.hpp"

namespace IR {

bool isArithmeticBinaryOp(Op op)
{
	return anyOf(op, Op::Plus, Op::Minus, Op::Times, Op::Divide, Op::Modulo, Op::Exponentation);
}

bool isBinaryOp(Op op)
{
	return op.value() < AST::BinOp::Type::_size;
}

bool isBooleanBinaryOp(Op op)
{
	return anyOf(op, Op::Or, Op::And, Op::Equal, Op::NotEqual,
		Op::Less, Op::LessEqual, Op::Greater, Op::GreaterEqual);
}

bool isTemporary(Op op)
{
	return isBinaryOp(op) || isUnaryOp(op) || op == Op::TableIndex;
}

bool isUnaryOp(Op op)
{
	return anyOf(op, Op::UnaryNegate, Op::UnaryNot, Op::UnaryLength);
}

const char * prettyPrint(IR::Op op)
{
	static const std::array <const char *, static_cast<size_t>(IR::Op::UnaryLength) + 1> str {
		"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%", "^", "-", "not", "#"
	};

	return str.at(op.value());
}

} //namespace IR
