#pragma once

#include <cstdint>
#include "EnumHelpers.hpp"

EnumClass(ValueType, uint8_t,
	Invalid,
	Unknown,
	Nil,
	Boolean,
	Integer,
	Real,
	String,
	Table,
	Function,
	_last
);
