#include <iostream>

#include "AST.hpp"
#include "Function.hpp"
#include "Scope.hpp"

Scope::Scope(Scope *parent, Function *function)
	: m_parent{parent}, m_function{function}
{
}

Scope::~Scope()
{
}

Scope * Scope::functionScope()
{
	return m_function ? &m_function->scope() : nullptr;
}

Scope * Scope::push()
{
	m_children.emplace_back(new Scope{this, m_function});
	return m_children.back().get();
}

bool Scope::addFunctionParam(const std::string &name)
{
	if (std::find(m_fnParams.begin(), m_fnParams.end(), name) != m_fnParams.end())
		return false;

	m_fnParams.push_back(name);
	return true;
}

void Scope::addLocalStore(const AST::LValue &var)
{
	m_rwOps.emplace_back(new VarAccess{var, VarAccess::Type::Write, VarAccess::Storage::Local});
}

void Scope::addVarAccess(const AST::LValue &var, VarAccess::Type type)
{
	const std::string &resolvedName = var.resolveName();

	VarAccess::Storage storage = VarAccess::Storage::Global;
	VarAccess *origin = nullptr;
	Scope *currentScope = this, *originScope = nullptr;
	bool closure = false;

	while (!originScope && currentScope) {
		for (auto iter = currentScope->m_rwOps.crbegin(); iter != currentScope->m_rwOps.crend(); ++iter) {
			if ((*iter)->type() == VarAccess::Type::Write && (*iter)->var().resolveName() == resolvedName) {
				storage = (*iter)->storage();
				if (closure && storage == VarAccess::Storage::Local)
					storage = VarAccess::Storage::Upvalue;

				originScope = currentScope;
				origin = iter->get();
				break;
			}
		}

		if (!originScope) {
			for (const auto &param : currentScope->m_fnParams) {
				if (resolvedName == param) {
					if (!closure)
						storage = VarAccess::Storage::Local;
					else
						storage = VarAccess::Storage::Upvalue;

					originScope = currentScope;
					break;
				}
			}
		}

		Scope *next = currentScope->parent();
		if (next && next->functionScope() != currentScope->functionScope())
			closure = true;
		currentScope = next;
	}

	if (type == VarAccess::Type::Write) {
		if (!originScope || (originScope != this && storage == VarAccess::Storage::Global)) {
			Check check = Check::GlobalStore_FunctionScope;
			if (!m_function)
				check = Check::GlobalStore_GlobalScope;

			if (resolvedName == "_")
				check = Check::GlobalStore_Underscore;
			else if (isupper(resolvedName[0]))
				check = Check::GlobalStore_UpperCase;

			REPORT(check, var.location() << " : assignment to global name: " << resolvedName << '\n');
		}
	}

	m_rwOps.emplace_back(new VarAccess{var, type, storage, origin});
}
