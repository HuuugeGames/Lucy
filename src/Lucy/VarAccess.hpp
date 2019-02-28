#pragma once

#include <vector>

#include "AST_fwd.hpp"
#include "EnumHelpers.hpp"

class Scope;

class VarAccess {
public:
	EnumClass(Type, uint8_t,
		Read,
		Write
	);

	EnumClass(Storage, uint8_t,
		Global,
		Upvalue,
		Local
	);

	VarAccess(const AST::LValue &var, Type type, Storage storage, VarAccess *origin = nullptr);
	VarAccess(const VarAccess &) = delete;
	VarAccess(VarAccess &&) = default;
	void operator = (const VarAccess &) = delete;
	~VarAccess() = default;

	const AST::LValue & var() const { return m_var; }
	Storage storage() const { return m_storage; }
	Type type() const { return m_type; }
	bool isFunctionParameter() const { return m_type == Type::Read && m_storage == Storage::Local && !m_origin; }

private:
	const AST::LValue &m_var;
	Type m_type;
	Storage m_storage;

	VarAccess *m_origin;
	std::vector <VarAccess *> m_dependent;
};
