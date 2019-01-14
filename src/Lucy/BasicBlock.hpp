#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "RValue.hpp"
#include "Serial.hpp"

namespace AST {
	class Assignment;
	class BinOp;
	class LValue;
	class NestedExpr;
	class Node;
	class UnOp;
	class Value;
}

class Scope;
class Triplet;

struct BasicBlock {
	enum class ExitType {
		Conditional,
		Fallthrough,
		Break,
		Return,
	};

	BasicBlock(UID id);
	BasicBlock(const BasicBlock &) = delete;
	BasicBlock & operator = (const BasicBlock &) = delete;
	BasicBlock & operator = (BasicBlock &&other);
	~BasicBlock();

	bool isEmpty() const;
	void generateTriplets();
	void removePredecessor(BasicBlock *block);

	std::string label;
	std::vector <const AST::Node *> insn;
	std::vector <std::unique_ptr <Triplet> > tripletCode;
	ExitType exitType = ExitType::Fallthrough;

	const AST::Node *returnExprList = nullptr;
	const AST::Node *condition = nullptr;
	std::array <BasicBlock *, 2> nextBlock = {nullptr, nullptr};
	std::vector <BasicBlock *> predecessors;
	unsigned phase = 0;

	Scope *scope = nullptr;

private:
	void process(std::vector <RValue> &stack, const AST::Assignment &assignment);
	void process(std::vector <RValue> &stack, const AST::BinOp &binOp);
	void process(std::vector <RValue> &stack, const AST::LValue &lval);
	void process(std::vector <RValue> &stack, const AST::NestedExpr &nestedExpr);
	void process(std::vector <RValue> &stack, const AST::Node &node);
	void process(std::vector <RValue> &stack, const AST::UnOp &binOp);
	void process(std::vector <RValue> &stack, const AST::Value &valueNode);
};
