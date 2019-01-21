#pragma once

#include <string>
#include <variant>
#include "AST_fwd.hpp"

class RValue;
class Triplet;

using TableReference = std::pair <const RValue *, const RValue *>;
using ResultPack = void *;

using ValueVariant = std::variant <
	std::nullptr_t,
	bool,
	long,
	double,
	std::string,
	const AST::Function *,
	TableReference,
	ResultPack
>;
