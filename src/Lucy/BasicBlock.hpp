#pragma once

#include <array>
#include <string>
#include <vector>

#include "Serial.hpp"

namespace AST {
	class Node;
}

struct BasicBlock {
	enum class ExitType {
		Conditional,
		Fallthrough,
		Break,
		Return,
	};

	bool isEmpty() const;
	void removePredecessor(BasicBlock *block);

	BasicBlock(UID id) : label{std::string{"BB_"} + std::to_string(id)} {}
	BasicBlock(const BasicBlock &) = delete;
	void operator = (const BasicBlock &) = delete;
	~BasicBlock() = default;

	std::string label;
	std::vector <const AST::Node *> insn;
	ExitType exitType = ExitType::Fallthrough;

	const AST::Node *returnExprList = nullptr;
	const AST::Node *condition = nullptr;
	std::array <BasicBlock *, 2> nextBlock = {nullptr, nullptr};
	std::vector <BasicBlock *> predecessors;
	unsigned phase;
};
