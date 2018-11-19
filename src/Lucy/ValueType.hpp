#pragma once

enum class ValueType : unsigned int {
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
};
