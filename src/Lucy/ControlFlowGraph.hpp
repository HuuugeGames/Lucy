#pragma once

#include <memory>
#include <vector>

#include "BasicBlock.hpp"
#include "Serial.hpp"

namespace AST {
	class Assignment;
	class Chunk;
	class ExprList;
	class Field;
	class Function;
	class FunctionCall;
	class ForEach;
	class LValue;
	class Node;
	class TableCtor;
	class VarList;
}

class CFGContext;
class Function;

class ControlFlowGraph {
public:
	ControlFlowGraph(const AST::Chunk &chunk, Scope &scope);

	ControlFlowGraph(const ControlFlowGraph &) = delete;
	void operator = (const ControlFlowGraph &) = delete;
	~ControlFlowGraph();

	const BasicBlock * entry() const { return m_entry; }
	void graphvizDump(const char *filename) const;
	void graphvizDump(const std::string &filename) const { graphvizDump(filename.c_str()); }

private:
	std::pair <BasicBlock *, BasicBlock *> process(CFGContext &ctx, const AST::Chunk &chunk);
	void process(CFGContext &ctx, const AST::Assignment &assignment);
	void process(CFGContext &ctx, const AST::ExprList &exprList);
	void process(CFGContext &ctx, const AST::Field &field);
	void process(CFGContext &ctx, const AST::Function &fnNode);
	void process(CFGContext &ctx, const AST::FunctionCall &fnCallNode);
	void process(CFGContext &ctx, const AST::LValue &lv);
	void process(CFGContext &ctx, const AST::Node &node);
	void process(CFGContext &ctx, const AST::TableCtor &table);
	void process(CFGContext &ctx, const AST::VarList &varList);

	void calcPredecessors();
	void prune();
	void graphvizDump(std::ostream &os) const;

	const AST::Chunk & rewrite(CFGContext &ctx, const AST::ForEach &forEach);

	Serial m_blockSerial;
	std::vector <std::unique_ptr <BasicBlock> > m_blocks;
	std::vector <std::unique_ptr <const AST::Node> > m_additionalNodes;
	BasicBlock *m_entry, *m_exit;
	unsigned m_walkPhase = 0;

	std::vector <std::unique_ptr <Function> > m_functions;
};
