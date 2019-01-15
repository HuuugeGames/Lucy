#include <iostream>
#include "AST.hpp"
#include "Triplet.hpp"

std::ostream & operator << (std::ostream &os, const Triplet &t)
{
	if (t.operation == Triplet::Op::Assign) {
		os << t.operands[0] << " = " << t.operands[1];
		return os;
	}

	if (static_cast<unsigned>(t.operation) < static_cast<unsigned>(AST::BinOp::Type::_last)) {
		os << "tmp_0x" << &t << " = "
			<< t.operands[0]
			<< ' ' << AST::BinOp::toString(static_cast<AST::BinOp::Type>(t.operation)) << ' '
			<< t.operands[1];
	} else if (t.operation == Triplet::Op::TableIndex) {
		os << "tmp_0x" << &t << " = " << t.operands[0] << '[' << t.operands[1] << ']';
	} else if (t.operation == Triplet::Op::Push) {
		os << "push " << t.operands[0];
	} else if (t.operation == Triplet::Op::Pop) {
		os << "pop " << t.operands[0];
	} else if (t.operation == Triplet::Op::Call) {
		const long argResultCnt = std::get<long>(std::get<ValueVariant>(t.operands[1].valueRef));
		os << "call " << t.operands[0] << ' ' << ((argResultCnt >> 16) & 0xffff) << ' ' << (argResultCnt & 0xffff);
	} else {
		switch (t.operation) {
			case Triplet::Op::UnaryNegate: os << '-'; break;
			case Triplet::Op::UnaryNot: os << "not "; break;
			case Triplet::Op::UnaryLength: os << '#'; break;
			default: assert(false);
		}
		os << "tmp_0x" << &t << " = " << t.operands[0];
	}

	return os;
}
