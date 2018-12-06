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
	const std::string &storedName = var.resolveName();

	Store::Type storeType = Store::Type::Global;
	Scope *currentScope = this, *prevStoreScope = nullptr;
	bool closure = false;

	while (!prevStoreScope && currentScope) {
		for (auto iter = currentScope->m_stores.crbegin(); iter != currentScope->m_stores.crend(); ++iter) {
			if (iter->var().resolveName() == storedName) {
				storeType = iter->type();
				if (closure && storeType == Store::Type::Local)
					storeType = Store::Type::Upvalue;

				prevStoreScope = currentScope;
				break;
			}
		}

		if (!prevStoreScope) {
			for (const auto &param : currentScope->m_fnParams) {
				if (storedName == param) {
					if (!closure)
						storeType = Store::Type::Local;
					else
						storeType = Store::Type::Upvalue;

					prevStoreScope = currentScope;
					break;
				}
			}
		}

		Scope *next = currentScope->parent();
		if (next && next->functionScope() != currentScope->functionScope())
			closure = true;
		currentScope = next;
	}

	if (!prevStoreScope || (prevStoreScope != this && storeType == Store::Type::Global)) {
		Check type = Check::GlobalStore_FunctionScope;
		if (!m_functionScope)
			type = Check::GlobalStore_GlobalScope;

		if (storedName == "_")
			type = Check::GlobalStore_Underscore;
		else if (isupper(storedName[0]))
			type = Check::GlobalStore_UpperCase;

		REPORT(type, var.location() << " : assignment to global name: " << storedName << '\n');
	}

	m_stores.emplace_back(var, storeType);
}
