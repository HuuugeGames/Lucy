#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Scope.hpp"

namespace AST {
	class Function;
}

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
