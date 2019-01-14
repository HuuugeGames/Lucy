#include <iostream>
#include "AST.hpp"
#include "Triplet.hpp"

std::ostream & operator << (std::ostream &os, const Triplet &t)
{
	if (t.operation == Triplet::Op::Assign) {
		os << t.operands[0] << " = " << t.operands[1];
		return os;
	}

	os << "tmp_0x" << &t << " = ";

	if (static_cast<unsigned>(t.operation) < static_cast<unsigned>(AST::BinOp::Type::_last)) {
		os << t.operands[0]
			<< ' ' << AST::BinOp::toString(static_cast<AST::BinOp::Type>(t.operation)) << ' '
			<< t.operands[1];
	} else if (t.operation == Triplet::Op::TableIndex) {
		os << t.operands[0] << '[' << t.operands[1] << ']';
	} else {
		switch (t.operation) {
			case Triplet::Op::UnaryNegate: os << '-'; break;
			case Triplet::Op::UnaryNot: os << "not "; break;
			case Triplet::Op::UnaryLength: os << '#'; break;
			default: assert(false);
		}
		os << t.operands[0];
	}

	return os;
}
