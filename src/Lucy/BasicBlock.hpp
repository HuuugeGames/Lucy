#pragma once

#include <array>
#include <string>
#include <vector>

#include "Serial.hpp"

class Node;

struct BasicBlock {
	enum class ExitType {
		Conditional,
		Fallthrough,
		Break,
		Return,
	};

	BasicBlock(UID id) : label{std::string{"BB_"} + std::to_string(id)} {}
	BasicBlock(const BasicBlock &) = delete;
	void operator = (const BasicBlock &) = delete;
	~BasicBlock() = default;

	std::string label;
	std::vector <const Node *> insn;
	ExitType exitType = ExitType::Fallthrough;

	const Node *returnExprList = nullptr;
	const Node *condition = nullptr;
	std::array <BasicBlock *, 2> nextBlock = {nullptr, nullptr};
};
