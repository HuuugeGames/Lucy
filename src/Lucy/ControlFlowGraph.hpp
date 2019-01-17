#pragma once

#include <memory>
#include <vector>

#include "AST_fwd.hpp"
#include "BasicBlock.hpp"
#include "Serial.hpp"

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
	class CFGContext;

	std::pair <BasicBlock *, BasicBlock *> process(CFGContext &ctx, const AST::Chunk &chunk);
	void process(CFGContext &ctx, const AST::Assignment &assignment);
	void process(CFGContext &ctx, const AST::ExprList &exprList);
	void process(CFGContext &ctx, const AST::Field &field);
	void process(CFGContext &ctx, const AST::Function &fnNode);
	void process(CFGContext &ctx, const AST::FunctionCall &fnCallNode);
	void process(CFGContext &ctx, const AST::LValue &lval);
	void process(CFGContext &ctx, const AST::Node &node);
	void process(CFGContext &ctx, const AST::TableCtor &table);
	void process(CFGContext &ctx, const AST::VarList &varList);

	void calcPredecessors();
	void generateTriplets();
	void prune();
	void graphvizDump(std::ostream &os) const;

	const AST::Chunk & rewrite(CFGContext &ctx, const AST::ForEach &forEach);
	const AST::Assignment & rewrite(CFGContext &ctx, const AST::Function &fnNode);
	const AST::FunctionCall & rewrite(CFGContext &ctx, const AST::MethodCall &callNode);

	Serial m_blockSerial;
	std::vector <std::unique_ptr <BasicBlock> > m_blocks;
	std::vector <std::unique_ptr <const AST::Node> > m_additionalNodes;
	BasicBlock *m_entry, *m_exit;
	unsigned m_walkPhase = 0;

	std::vector <std::unique_ptr <Function> > m_functions;
};
