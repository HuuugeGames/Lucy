#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AST_fwd.hpp"
#include "IR.hpp"
#include "RValue.hpp"
#include "Serial.hpp"

class ControlFlowGraph;
class Scope;

struct BasicBlock {
	enum class ExitType {
		Conditional,
		Fallthrough,
		Break,
		Return,
	};

	BasicBlock(UID id);
	BasicBlock(const std::string &label);
	BasicBlock(std::string &&label);
	BasicBlock(const BasicBlock &) = delete;
	BasicBlock & operator = (const BasicBlock &) = delete;
	BasicBlock & operator = (BasicBlock &&other);
	~BasicBlock();

	void irDump(unsigned indent = 0) const;
	bool isEmpty() const;
	void generateIR();
	void removePredecessor(BasicBlock *block);

	std::string label;
	std::vector <const AST::Node *> insn;
	std::vector <std::unique_ptr <IR::Triplet> > irCode;

	ExitType exitType = ExitType::Fallthrough;
	const AST::Node *returnExprList = nullptr;
	const AST::Node *condition = nullptr;
	std::array <BasicBlock *, 2> nextBlock = {nullptr, nullptr};
	std::vector <BasicBlock *> predecessors;
	unsigned phase = 0;

	Scope *scope = nullptr;

private:
	class BBContext;

	static void finalize(BBContext &ctx);

	static void process(BBContext &ctx, const AST::Assignment &assignment);
	static void process(BBContext &ctx, const AST::BinOp &binOp);
	static void process(BBContext &ctx, const AST::Ellipsis &ellipsis);
	static void process(BBContext &ctx, const AST::ExprList &exprList);
	static void process(BBContext &ctx, const AST::Function &fnNode);
	static void process(BBContext &ctx, const AST::FunctionCall &fnCallNode);
	static void process(BBContext &ctx, const AST::LValue &lval);
	static void process(BBContext &ctx, const AST::NestedExpr &nestedExpr);
	static void process(BBContext &ctx, const AST::Node &node);
	static void process(BBContext &ctx, const AST::TableCtor &tableCtor);
	static void process(BBContext &ctx, const AST::UnOp &binOp);
	static void process(BBContext &ctx, const AST::Value &valueNode);

	static void splitBlock(BBContext &ctx, const AST::LValue *tmpDst, const AST::BinOp &binOp);

	std::unique_ptr <const AST::If> m_condNode;
	std::array <std::unique_ptr <BasicBlock>, 2> m_subBlocks;
};
