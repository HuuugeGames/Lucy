#include <iostream>
#include "AST.hpp"
#include "Fold.hpp"
#include "IR.hpp"
#include "IROp.hpp"

std::ostream & operator << (std::ostream &os, const IR::Triplet &t)
{
	if (IR::isTemporary(t.operation)) {
		os << "tmp_0x" << &t << " = ";

		if (IR::isBinaryOp(t.operation)) {
			os << t.operands[0]
				<< ' ' << AST::BinOp::toString(AST::BinOp::Type{t.operation.value()}) << ' '
				<< t.operands[1];
		} else if (IR::isUnaryOp(t.operation)) {
			switch (t.operation.value()) {
				case IR::Op::UnaryNegate: os << '-'; break;
				case IR::Op::UnaryNot: os << "not "; break;
				case IR::Op::UnaryLength: os << '#'; break;
				default: assert(false);
			}

			os << '(' << t.operands[0] << ')';
		} else { // IR::Op::TableIndex
			assert(t.operation == IR::Op::TableIndex);
			os << t.operands[0] << '[' << t.operands[1] << ']';
		}

		return os;
	}

	if (anyOf(t.operation, IR::Op::Assign, IR::Op::TableAssign)) {
		os << t.operands[0] << " = " << t.operands[1];

		return os;
	}

	switch (t.operation.value()) {
		case IR::Op::TableCtor: os << t.operands[0] << " = {}"; break;
		case IR::Op::Push: os << "push " << t.operands[0]; break;
		case IR::Op::Pop: os << "pop " << t.operands[0]; break;
		case IR::Op::CallUnknownResults: os << "call_varres " << t.operands[0] << ' ' << t.operands[1]; break;
		case IR::Op::Return: os << "return " << t.operands[0]; break;
		case IR::Op::Jump: os << "jump " << t.operands[0]; break;
		case IR::Op::JumpCond: os << "jump_true " << t.operands[0] << ' ' << t.operands[1]; break;
		case IR::Op::Call: {
			const long argResultCnt = std::get<long>(std::get<ValueVariant>(t.operands[1].valueRef));
			os << "call " << t.operands[0] << ' ' << ((argResultCnt >> 16) & 0xffff) << ' ' << (argResultCnt & 0xffff);
			break;
		}
		default:
			assert(false);
	}

	return os;
}
