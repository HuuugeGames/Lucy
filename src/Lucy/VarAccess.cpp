#include "VarAccess.hpp"

VarAccess::VarAccess(const AST::LValue &var, Type type, Storage storage, VarAccess *origin)
	: m_var{var}, m_type{type}, m_storage{storage}, m_origin{origin}
{
	if (m_origin)
		m_origin->m_dependent.push_back(this);
}
