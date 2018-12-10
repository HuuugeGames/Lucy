#include "Store.hpp"

Store::Store(const AST::LValue &var, Type type, Store *origin)
	: m_var{var}, m_type{type}, m_origin{origin}
{
	if (m_origin)
		m_origin->m_dependent.push_back(this);
}
