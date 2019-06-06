#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AST_fwd.hpp"
#include "Bitfield.hpp"
#include "EnumHelpers.hpp"
#include "IR.hpp"
#include "RValue.hpp"
#include "Serial.hpp"

class ControlFlowGraph;
class Scope;

struct BasicBlock {
	EnumClass(ExitType, uint8_t,
		Conditional,
		Fallthrough,
		Break,
		Return
	);

	EnumClass(Attribute, uint8_t,
		BackEdge,
		LoopFooter,
		SubBlock
	);

	BasicBlock(UID id);
	BasicBlock(const std::string &label);
	BasicBlock(std::string &&label);
	BasicBlock(const BasicBlock &) = delete;
	BasicBlock & operator = (const BasicBlock &) = delete;
	BasicBlock & operator = (BasicBlock &&other);
	~BasicBlock();

	void irDump(unsigned indent = 0) const;

	const std::string & label() const { return m_label; }
	void setLabel(const std::string &label) { m_label = label; }
	void setLabel(std::string &&label) { m_label = std::move(label); }

	ExitType exitType() const { return m_exitType; }
	void setExitType(ExitType et) { m_exitType = et; }

	bool attribute(Attribute attrib) const { return static_cast<bool>(m_attrib.get(attrib.value())); }
	void setAttribute(Attribute attrib) { m_attrib.set(attrib.value()); }

	bool canPrune() const;
	bool isEmpty() const;
	void generateIR();
	void removePredecessor(BasicBlock *block);

	const BasicBlock * loopFooter() const { return m_loopFooterEdge; }
	void setLoopFooter(BasicBlock *dst);

	std::vector <const AST::Node *> insn;
	std::vector <std::unique_ptr <IR::Triplet> > irCode;

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

	std::string m_label;
	ExitType m_exitType = ExitType::Fallthrough;
	Bitfield <Attribute::_size> m_attrib;
	std::unique_ptr <const AST::If> m_condNode;
	std::array <std::unique_ptr <BasicBlock>, 2> m_subBlocks;
	BasicBlock *m_loopFooterEdge = nullptr;
};
