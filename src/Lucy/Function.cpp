#include <sstream>
#include "AST.hpp"
#include "ControlFlowGraph.hpp"
#include "Function.hpp"
#include "Issue.hpp"

Function::Function(const AST::Function &fnNode, Scope &scope)
	: m_fnNode{fnNode}, m_fnScope{&scope, this}
{
	if (!fnNode.isLocal() && !fnNode.isAnonymous() && !fnNode.isMethod() && !fnNode.isNested() && scope.functionScope() == nullptr)
		Logger::logIssue<Issue::GlobalFunctionDefinition>(fnNode.name().location(), fnNode.fullName());

	if (fnNode.isMethod())
		m_fnScope.addFunctionParam("self", yy::location{});

	for (const auto &param : fnNode.params().names()) {
		if (!m_fnScope.addFunctionParam(param.first, param.second)) {
			if (param.first != "self")
				Logger::logIssue<Issue::Function::DuplicateParam>(param.second, param.first);
			else
				Logger::logIssue<Issue::Function::DuplicateParamSelf>(param.second);
		}
	}

	if (fnNode.isVariadic()) {
		if (!m_fnScope.addFunctionParam("arg", yy::location{})) {
			Logger::logIssue<Issue::Function::EllipsisShadowsParam>(fnNode.paramList().location());
		}
	}

	m_cfg = std::make_unique<ControlFlowGraph>(fnNode.chunk(), m_fnScope);

	if (m_resultCnt.value_or(0)) {
		for (BasicBlock *pred : m_cfg->exit()->predecessors) {
			if (pred->exitType() != BasicBlock::ExitType::Return) {
				Logger::logIssue<Issue::Function::FallthroughNoResult>(fnNode.paramList().location());
				break;
			}
		}
	}
}

bool Function::isVariadic() const
{
	return m_fnNode.isVariadic();
}

void Function::irDump(unsigned indent)
{
	std::ostringstream ss;
	ss << m_fnNode.fullName();
	if (m_fnNode.isAnonymous())
		ss << " 0x" << &m_fnNode;
	ss << " (" << m_fnNode.location() << ')';

	m_cfg->irDump(indent, ss.str().c_str());
}

void Function::setResultCount(const AST::Return &returnNode)
{
	unsigned resultCnt = 0;
	if (!returnNode.empty())
		resultCnt = returnNode.exprList().size();

	if (!m_resultCnt) {
		m_resultCnt = resultCnt;
		return;
	}

	if (*m_resultCnt != resultCnt)
		Logger::logIssue<Issue::Function::VariableResultCount>(returnNode.location(), *m_resultCnt, resultCnt);
}
