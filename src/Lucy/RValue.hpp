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

	static const AST::LValue * getTemporary(unsigned idx);
	static RValue nil() { return ValueVariant{nullptr}; }

	RValue() = default;
	RValue(const ValueVariant &imm) : valueRef{imm} {}
	RValue(const AST::LValue *lval) : valueRef{lval} {}
	RValue(const IR::Triplet *temp) : valueRef{temp} {}

	Type type() const { return static_cast<Type>(valueRef.index()); }

	const ValueVariant & getImmediate() const { return std::get<Type::Immediate>(valueRef); }
	const AST::LValue * getLValue() const { return std::get<Type::LValue>(valueRef); }
	const IR::Triplet * getTemporary() const { return std::get<Type::Temporary>(valueRef); }

	std::variant <ValueVariant, const AST::LValue *, const IR::Triplet *> valueRef;
};

std::ostream & operator << (std::ostream &os, const RValue &rval);
