#include <iostream>
#include "AST.hpp"
#include "Fold.hpp"
#include "IR.hpp"

std::ostream & operator << (std::ostream &os, const IR::Triplet &t)
{
	if (anyOf(t.operation, IR::Op::Assign, IR::Op::TableAssign)) {
		os << t.operands[0] << " = " << t.operands[1];
	} else if (t.operation == IR::Op::TableCtor) {
		os << t.operands[0] << " = {}";
	} else if (static_cast<unsigned>(t.operation) < static_cast<unsigned>(AST::BinOp::Type::_last)) {
		os << "tmp_0x" << &t << " = "
			<< t.operands[0]
			<< ' ' << AST::BinOp::toString(static_cast<AST::BinOp::Type>(t.operation)) << ' '
			<< t.operands[1];
	} else if (t.operation == IR::Op::TableIndex) {
		os << "tmp_0x" << &t << " = " << t.operands[0] << '[' << t.operands[1] << ']';
	} else if (t.operation == IR::Op::Push) {
		os << "push " << t.operands[0];
	} else if (t.operation == IR::Op::Pop) {
		os << "pop " << t.operands[0];
	} else if (t.operation == IR::Op::Call) {
		const long argResultCnt = std::get<long>(std::get<ValueVariant>(t.operands[1].valueRef));
		os << "call " << t.operands[0] << ' ' << ((argResultCnt >> 16) & 0xffff) << ' ' << (argResultCnt & 0xffff);
	} else if (t.operation == IR::Op::CallUnknownResults) {
		os << "call_varres " << t.operands[0] << ' ' << t.operands[1];
	} else if (t.operation == IR::Op::Return) {
		os << "return " << t.operands[0];
	} else if (t.operation == IR::Op::Test) {
		os << "test " << t.operands[0];
	} else if (t.operation == IR::Op::Jump) {
		os << "jump " << t.operands[0];
	} else if (t.operation == IR::Op::JumpTrue) {
		os << "jump_true " << t.operands[0];
	} else { //unary operation
		os << "tmp_0x" << &t << " = ";
		switch (t.operation) {
			case IR::Op::UnaryNegate: os << '-'; break;
			case IR::Op::UnaryNot: os << "not "; break;
			case IR::Op::UnaryLength: os << '#'; break;
			default: assert(false);
		}
		os << '(' << t.operands[0] << ')';
	}

	return os;
}
