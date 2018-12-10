#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "VarAccess.hpp"

namespace AST {
	class LValue;
};

class Scope {
public:
	Scope(Scope *parent = nullptr, Scope *functionScope = nullptr);
	Scope(const Scope &) = delete;
	void operator = (const Scope &) = delete;
	~Scope();

	Scope * functionScope() { return m_functionScope; }
	Scope * parent() { return m_parent; }
	Scope * push(Scope *functionScope = nullptr);

	void addFunctionParam(const std::string &name);
	void addLoad(const AST::LValue &var);
	void addLocalStore(const AST::LValue &var);
	void addVarAccess(const AST::LValue &var, VarAccess::Type type);

private:
	Scope *m_parent = nullptr;
	Scope *m_functionScope = nullptr;
	std::vector <std::unique_ptr <Scope> > m_children;
	std::vector <std::unique_ptr <VarAccess> > m_rwOps;

	std::vector <std::string> m_fnParams;
};
