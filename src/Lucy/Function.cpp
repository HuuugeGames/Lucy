#include "AST.hpp"
#include "Function.hpp"

Function::Function(const AST::Function &fnNode)
	: m_fnNode{fnNode}
{
	m_cfg = std::make_unique<ControlFlowGraph>(fnNode.chunk());
}

void Function::findUpvalues()
{

}
