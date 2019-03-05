#include <array>
#include <iostream>
#include "IROp.hpp"

std::ostream & operator << (std::ostream &os, IR::Op op)
{
	static const std::array <const char *, static_cast<size_t>(IR::Op::UnaryLength) + 1> str {
		"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%", "^", "-", "not", "#"
	};

	os << str[static_cast<size_t>(op)];
	return os;
}
