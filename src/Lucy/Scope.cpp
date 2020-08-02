#include <iostream>

#include "AST.hpp"
#include "Function.hpp"
#include "Logger.hpp"
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

bool Scope::addFunctionParam(const std::string &name, const yy::location &location)
{
	if (std::find_if(m_fnParams.begin(), m_fnParams.end(), [&name](const auto &param) { return param.name == name; }) != m_fnParams.end())
		return false;

	m_fnParams.emplace_back(name, location);
	if (location == yy::location{})
		m_fnParams.back().synthetic = true;
	return true;
}

void Scope::addLocalStore(const AST::LValue &var)
{
	const std::string &varName = var.resolveName();

	if (Logger::isEnabled(Issue::Type::ShadowingDefinition)) {
		for (auto iter = m_rwOps.crbegin(); iter != m_rwOps.crend(); ++iter) {
			if (!(*iter)->isFunctionParameter()) {
				const std::string &resolvedName = (*iter)->var().resolveName();
				if (resolvedName != "_" && resolvedName == varName) {
					Logger::logIssue<Issue::ShadowingDefinition>(var.location(), varName, (*iter)->var().location());
				}
			}
		}
	}

	m_rwOps.emplace_back(std::make_unique<VarAccess>(var, VarAccess::Type::Write, VarAccess::Storage::Local));
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

		if (originScope)
			break;

		Scope *next = currentScope->parent();
		if (next && next->functionScope() != currentScope->functionScope()) {
			for (auto &param : currentScope->m_fnParams) {
				if (resolvedName == param.name) {
					if (!closure)
						storage = VarAccess::Storage::Local;
					else
						storage = VarAccess::Storage::Upvalue;

					param.used = true;
					originScope = currentScope;
					break;
				}
			}

			closure = true;
		}

		currentScope = next;
	}

	if (type == VarAccess::Type::Write) {
		if (!originScope || (originScope != this && storage == VarAccess::Storage::Global)) {
			if (resolvedName == "_")
				Logger::logIssue<Issue::GlobalStore::Underscore>(var.location());
			else if (isupper(resolvedName[0]))
				Logger::logIssue<Issue::GlobalStore::UpperCase>(var.location(), resolvedName);
			else if (!m_function)
				Logger::logIssue<Issue::GlobalStore::GlobalScope>(var.location(), resolvedName);
			else
				Logger::logIssue<Issue::GlobalStore::FunctionScope>(var.location(), resolvedName);
		}
	}

	m_rwOps.emplace_back(std::make_unique<VarAccess>(var, type, storage, origin));
}

void Scope::reportUnusedFnParams() const
{
	for (const auto &param : m_fnParams) {
		if (!param.used && !param.synthetic) {
			if (param.name == "_")
				Logger::logIssue<Issue::Function::UnusedParamUnderscore>(param.location);
			else
				Logger::logIssue<Issue::Function::UnusedParam>(param.location, param.name);
		}
	}

	for (const auto &subScope : m_children)
		subScope->reportUnusedFnParams();
}
