#pragma once

#include <memory>

#include "AST_fwd.hpp"
#include "Scope.hpp"

class ControlFlowGraph;

class Function {
public:
	Function(const AST::Function &fnNode, Scope &scope);

	bool isVariadic() const;

	const ControlFlowGraph & cfg() const { return *m_cfg; }
	void irDump(unsigned indent = 0);
	Scope & scope() { return m_fnScope; }

	std::optional <unsigned> resultCount() const { return m_resultCnt; }
	void setResultCount(const AST::Return &returnNode);

private:
	const AST::Function &m_fnNode;
	Scope m_fnScope;
	std::unique_ptr <ControlFlowGraph> m_cfg;
	std::optional <unsigned> m_resultCnt;
};
