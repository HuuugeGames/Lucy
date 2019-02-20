#pragma once

#include <iosfwd>
#include <string_view>
#include <variant>

#include "AST_fwd.hpp"
#include "EnumHelpers.hpp"
#include "ValueVariant.hpp"

namespace IR {
	class Triplet;
}

struct RValue {
	EnumClass(Type, unsigned, Immediate, LValue, Temporary);

	RValue() = default;
	RValue(const ValueVariant &imm) : valueRef{imm} {}
	RValue(const AST::LValue *lval) : valueRef{lval} {}
	RValue(const IR::Triplet *temp) : valueRef{temp} {}

	static const AST::LValue * getTemporary(unsigned idx);
	static RValue nil() { return ValueVariant{nullptr}; }

	std::variant <ValueVariant, const AST::LValue *, const IR::Triplet *> valueRef;
};

std::ostream & operator << (std::ostream &os, const RValue &rval);
