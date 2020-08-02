#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AST_fwd.hpp"
#include "VarAccess.hpp"

#include "location.hh"

class Function;

class Scope {
public:
	Scope(Scope *parent = nullptr, Function *function = nullptr);
	Scope(const Scope &) = delete;
	void operator = (const Scope &) = delete;
	~Scope();

	Function * function() { return m_function; }
	const auto & functionParams() const { return m_fnParams; }

	Scope * functionScope();
	Scope * parent() { return m_parent; }
	Scope * push();

	//TODO deprecate most of these
	bool addFunctionParam(const std::string &name, const yy::location &location);
	void addLoad(const AST::LValue &var);
	void addLocalStore(const AST::LValue &var);
	void addVarAccess(const AST::LValue &var, VarAccess::Type type);

	void reportUnusedFnParams() const;

private:
	Scope *m_parent = nullptr;
	Function *m_function = nullptr;
	std::vector <std::unique_ptr <Scope> > m_children;
	std::vector <std::unique_ptr <VarAccess> > m_rwOps;

	struct FnParam {
		FnParam(const std::string &name, const yy::location &location) : name{name}, location{location} {}

		std::string name;
		yy::location location;
		bool used = false;
		bool synthetic = false;
	};

	std::vector <FnParam> m_fnParams;
};
