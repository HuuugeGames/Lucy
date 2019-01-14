#pragma once

#include <iosfwd>
#include <string>
#include <variant>

namespace AST {
	class Function;
}

//TODO Table
using ValueVariant = std::variant <std::nullptr_t, bool, long, double, std::string, const AST::Function *>;
