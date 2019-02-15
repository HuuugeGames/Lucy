#include <sstream>
#include "AST.hpp"
#include "ControlFlowGraph.hpp"
#include "Function.hpp"

Function::Function(const AST::Function &fnNode, Scope &scope)
	: m_fnNode{fnNode}, m_fnScope{&scope, &m_fnScope}
{
	if (!fnNode.isLocal() && !fnNode.isAnonymous() && !fnNode.isMethod() && !fnNode.isNested() && scope.functionScope() == nullptr)
		REPORT(Check::GlobalFunctionDefinition, fnNode.name().location() << " : function definition in global scope: " << fnNode.fullName() << '\n');

	if (fnNode.isMethod())
		m_fnScope.addFunctionParam("self");

	for (const auto &param : fnNode.params().names()) {
		if (!m_fnScope.addFunctionParam(param.first))
			REPORT(Check::Function_DuplicateParam, fnNode.params().location() << " : duplicate function parameter: " << param.first << '\n');
	}

	m_cfg = std::make_unique<ControlFlowGraph>(fnNode.chunk(), m_fnScope);
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
