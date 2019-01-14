#pragma once

#include <iosfwd>
#include <string_view>
#include <variant>
#include "ValueVariant.hpp"

namespace AST {
	class LValue;
}

class Triplet;

struct RValue {
	RValue() = default;
	RValue(const ValueVariant &imm) : valueRef{imm} {}
	RValue(const AST::LValue *lval) : valueRef{lval} {}
	RValue(const Triplet *temp) : valueRef{temp} {}

	enum class Type {
		Immediate,
		LValue,
		Temporary,
	};

	static RValue nil() { return ValueVariant{nullptr}; }

	std::variant <ValueVariant, const AST::LValue *, const Triplet *> valueRef;

	static const AST::LValue * getTemporary(unsigned idx);
};

std::ostream & operator << (std::ostream &os, const RValue &rval);
