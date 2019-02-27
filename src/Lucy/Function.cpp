#include <sstream>
#include "AST.hpp"
#include "ControlFlowGraph.hpp"
#include "Function.hpp"

Function::Function(const AST::Function &fnNode, Scope &scope)
	: m_fnNode{fnNode}, m_fnScope{&scope, this}
{
	if (!fnNode.isLocal() && !fnNode.isAnonymous() && !fnNode.isMethod() && !fnNode.isNested() && scope.functionScope() == nullptr)
		REPORT(Check::GlobalFunctionDefinition, fnNode.name().location() << " : function definition in global scope: " << fnNode.fullName() << '\n');

	if (fnNode.isMethod())
		m_fnScope.addFunctionParam("self");

	for (const auto &param : fnNode.params().names()) {
		std::cout << "param: <" << param.first << ", " << param.second << ">\n";
	}

	for (const auto &param : fnNode.params().names()) {
		if (!m_fnScope.addFunctionParam(param.first)) {
			if (param.first != "self")
				REPORT(Check::Function_DuplicateParam, param.second << " : duplicate function parameter: " << param.first << '\n');
			else
				REPORT(Check::Function_DuplicateParam, param.second << " : parameter \"self\" clashes with implicit parameter of the same name\n");
		}
	}

	m_cfg = std::make_unique<ControlFlowGraph>(fnNode.chunk(), m_fnScope);

	if (m_resultCnt.value_or(0)) {
		for (BasicBlock *pred : m_cfg->exit()->predecessors) {
			if (pred->exitType != BasicBlock::ExitType::Return) {
				REPORT(Check::Function_VariableResultCount, fnNode.name().location() << " : function " << fnNode.fullName() << " might fall-through without returning any result\n");
				break;
			}
		}
	}
}

void Function::irDump(unsigned indent)
{
	const std::string indentStr(indent, '\t');
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
		REPORT(Check::Function_VariableResultCount, returnNode.location() << " : returning other number of results (" << resultCnt << ") than in other return statement (" << *m_resultCnt << ")\n");
}
