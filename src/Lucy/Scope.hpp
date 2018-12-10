#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Store.hpp"

namespace AST {
	class LValue;
};

class Scope {
public:
	Scope(Scope *parent = nullptr, Scope *functionScope = nullptr);
	Scope(const Scope &) = delete;
	void operator = (const Scope &) = delete;
	~Scope() = default;

	Scope * functionScope() { return m_functionScope; }
	Scope * parent() { return m_parent; }
	Scope * push(Scope *functionScope = nullptr);

	void addFunctionParam(const std::string &name);
	void addLoad(const AST::LValue &var);
	void addLocalStore(const AST::LValue &var);
	void addStore(const AST::LValue &var);

private:
	Scope *m_parent = nullptr;
	Scope *m_functionScope = nullptr;
	std::vector <std::unique_ptr <Scope> > m_children;
	std::vector <std::unique_ptr <Store> > m_stores;

	std::vector <std::string> m_fnParams;
};
