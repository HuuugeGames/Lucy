#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ControlFlowGraph.hpp"

namespace AST {
	class Function;
}

class Function {
public:
	Function(const AST::Function &fnNode);

private:
	void findUpvalues();

	const AST::Function &m_fnNode;
	std::unique_ptr <ControlFlowGraph> m_cfg;
	std::vector <std::string> m_upvalues;
};
