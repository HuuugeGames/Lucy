#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AST_fwd.hpp"
#include "VarAccess.hpp"

class Function;

class Scope {
public:
	Scope(Scope *parent = nullptr, Function *function = nullptr);
	Scope(const Scope &) = delete;
	void operator = (const Scope &) = delete;
	~Scope();

	Function * function() { return m_function; }
	const std::vector <std::string> & functionParams() const { return m_fnParams; }

	Scope * functionScope();
	Scope * parent() { return m_parent; }
	Scope * push();

	bool addFunctionParam(const std::string &name);
	void addLoad(const AST::LValue &var);
	void addLocalStore(const AST::LValue &var);
	void addVarAccess(const AST::LValue &var, VarAccess::Type type);

private:
	Scope *m_parent = nullptr;
	Function *m_function = nullptr;
	std::vector <std::unique_ptr <Scope> > m_children;
	std::vector <std::unique_ptr <VarAccess> > m_rwOps;

	std::vector <std::string> m_fnParams;
};
