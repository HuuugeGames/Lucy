#include <iostream>

#include "AST.hpp"
#include "Scope.hpp"
#include "Store.hpp"

Scope::Scope(Scope *parent, Scope *functionScope)
	: m_parent{parent}, m_functionScope{functionScope}
{
}

Scope * Scope::push(Scope *functionScope)
{
	if (!functionScope)
		m_children.emplace_back(new Scope{this, this->m_functionScope});
	else
		m_children.emplace_back(new Scope{this, functionScope});

	return m_children.back().get();
}

void Scope::addFunctionParam(const std::string &name)
{
	m_fnParams.push_back(name);
}

void Scope::addLocalStore(const AST::LValue &var)
{
	m_stores.emplace_back(var, Store::Type::Local);
}

void Scope::addStore(const AST::LValue &var)
{
	//TODO
	if (var.lvalueType() != AST::LValue::Type::Name)
		return;

	Store::Type storeType = Store::Type::Global;
	Scope *currentScope = this;
	bool found = false;
	bool closure = false;

	while (!found && currentScope) {
		for (auto iter = currentScope->m_stores.crbegin(); iter != currentScope->m_stores.crend(); ++iter) {
			if (iter->var().lvalueType() == AST::LValue::Type::Name && iter->var().name() == var.name()) {
				storeType = iter->type();
				if (closure && storeType == Store::Type::Local)
					storeType = Store::Type::Upvalue;

				found = true;
				break;
			}
		}

		if (!found) {
			for (const auto &param : currentScope->m_fnParams) {
				if (var.name() == param) {
					if (!closure)
						storeType = Store::Type::Local;
					else
						storeType = Store::Type::Upvalue;

					found = true;
					break;
				}
			}
		}

		Scope *next = currentScope->parent();
		if (next && next->functionScope() != currentScope->functionScope())
			closure = true;
		currentScope = next;
	}

	if (!found) {
		Check type = Check::GlobalStore_FunctionScope;
		if (var.name() == "_")
			type = Check::GlobalStore_Underscore;

		REPORT(type, var.location() << " : assignment to global name: " << var.name() << '\n');
	}
	m_stores.emplace_back(var, storeType);
}
