#include "Store.hpp"

Store::Store(const AST::LValue &var, Type type)
	: m_var{var}, m_type{type}
{
}
