#pragma once

#include <memory>

#include "AST_fwd.hpp"
#include "Scope.hpp"

class ControlFlowGraph;

class Function {
public:
	Function(const AST::Function &fnNode, Scope &scope);

	const ControlFlowGraph & cfg() const { return *m_cfg; }

private:
	const AST::Function &m_fnNode;
	Scope m_fnScope;
	std::unique_ptr <ControlFlowGraph> m_cfg;
};
