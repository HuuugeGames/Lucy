#pragma once

#include <array>
#include <iosfwd>
#include "location.hh"
#include "IROp.hpp"
#include "RValue.hpp"

namespace IR {

struct Triplet {
	template <typename T>
	Triplet(Op operation, const T &operand) : operation{operation}, operands{operand} {}

	template <typename T1, typename T2>
	Triplet(Op operation, const T1 &lhs, const T2 &rhs) : operation{operation}, operands{lhs, rhs} {}

	Op operation;
	std::array <RValue, 2> operands;
};

} //namespace IR

std::ostream & operator << (std::ostream &os, const IR::Triplet &t);
