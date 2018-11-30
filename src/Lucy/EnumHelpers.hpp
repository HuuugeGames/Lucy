#pragma once

#include <type_traits>
#include "StringPack.hpp"

template <typename ET>
constexpr typename std::underlying_type <ET>::type toUnderlying(ET et)
{
	return static_cast<typename std::underlying_type<ET>::type>(et);
}

template <typename ET>
constexpr typename std::underlying_type <ET>::type & toUnderlyingRef(ET &et)
{
	return reinterpret_cast<typename std::underlying_type<ET>::type &>(et);
}

#define EnumClass(EnumName, EnumType, ...) \
	class EnumName { \
	public: \
		enum EnumName ## __Internal : EnumType { \
			__VA_ARGS__ \
		}; \
		constexpr EnumName(EnumName ## __Internal value) : m_value{value} {} \
		constexpr bool operator == (EnumName other) { return this->m_value == other.m_value; } \
		constexpr bool operator != (EnumName other) { return this->m_value != other.m_value; } \
		operator const char *() { return m_strPack[m_value]; } \
		constexpr EnumType value() const { return m_value; } \
	private: \
		EnumName ## __Internal m_value; \
		static constexpr auto m_strPack = \
			makeStringPack<meta::countTokensTotalLength(#__VA_ARGS__)>( \
				meta::splitTokens<meta::countTokens(#__VA_ARGS__)>(#__VA_ARGS__)); \
	}
