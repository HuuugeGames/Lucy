#pragma once

#include <array>
#include <iosfwd>
#include "IROp.hpp"
#include "RValue.hpp"

namespace IR {

struct Triplet {
	Op operation;
	std::array <RValue, 2> operands;
};

} //namespace IR

std::ostream & operator << (std::ostream &os, const IR::Triplet &t);
