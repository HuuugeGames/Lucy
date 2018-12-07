#include "AST.hpp"
#include "ControlFlowGraph.hpp"
#include "Function.hpp"

Function::Function(const AST::Function &fnNode, Scope &scope)
	: m_fnNode{fnNode}, m_fnScope{&scope, &m_fnScope}
{
	if (!fnNode.isLocal() && !fnNode.isAnonymous() && !fnNode.isMethod() && scope.functionScope() == nullptr)
		LOG(Check::GlobalFunctionDefinition, fnNode.name().location() << " : function definition in global scope: " << fnNode.fullName() << '\n');

	if (fnNode.isMethod())
		m_fnScope.addFunctionParam("self");

	for (const auto &param : fnNode.params().names())
		m_fnScope.addFunctionParam(param.first);

	m_cfg = std::make_unique<ControlFlowGraph>(fnNode.chunk(), m_fnScope);
}
