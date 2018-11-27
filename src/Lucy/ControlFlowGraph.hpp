#pragma once

#include <memory>
#include <vector>

#include "BasicBlock.hpp"
#include "Serial.hpp"

class CFGContext;
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

class ControlFlowGraph {
public:
	ControlFlowGraph(const Chunk &chunk);

	ControlFlowGraph(const ControlFlowGraph &) = delete;
	void operator = (const ControlFlowGraph &) = delete;
	~ControlFlowGraph() = default;

	void graphvizDump(const char *filename);
	void graphvizDump(const std::string &filename) { graphvizDump(filename.c_str()); }

private:
	std::pair <BasicBlock *, BasicBlock *> process(CFGContext &ctx, const Chunk &chunk);
	void process(CFGContext &ctx, const Assignment &assignment);
	void process(CFGContext &ctx, const ExprList &exprList);
	void process(CFGContext &ctx, const Field &field);
	void process(CFGContext &ctx, const Function &fnNode);
	void process(CFGContext &ctx, const FunctionCall &fnCallNode);
	void process(CFGContext &ctx, const LValue &lv);
	void process(CFGContext &ctx, const Node &node);
	void process(CFGContext &ctx, const TableCtor &table);
	void process(CFGContext &ctx, const VarList &varList);

	void calcPredecessors();
	void prune();
	void graphvizDump(std::ostream &os);

	const Chunk & rewrite(CFGContext &ctx, const ForEach &forEach);

	Serial m_blockSerial;
	std::vector <std::unique_ptr <BasicBlock> > m_blocks;
	std::vector <std::unique_ptr <const Node> > m_additionalNodes;
	BasicBlock *m_entry, *m_exit;
	unsigned m_walkPhase = 0;

	std::vector <std::unique_ptr <ControlFlowGraph> > m_subgraphs;
};
