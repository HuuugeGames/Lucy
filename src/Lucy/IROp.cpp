#include <array>
#include "IROp.hpp"

const char * prettyPrint(IR::Op op)
{
	static const std::array <const char *, static_cast<size_t>(IR::Op::UnaryLength) + 1> str {
		"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%", "^", "-", "not", "#"
	};

	return str.at(op.value());
}
