#pragma once

#include <memory>
#include <vector>

#include "BasicBlock.hpp"
#include "Serial.hpp"

class CFGContext;
class Chunk;
class ForEach;
class Node;

class ControlFlowGraph {
public:
	ControlFlowGraph(const Chunk &chunk);

	ControlFlowGraph(const ControlFlowGraph &) = delete;
	void operator = (const ControlFlowGraph &) = delete;
	~ControlFlowGraph() = default;

	void graphvizDump(const char *filename);

private:
	std::pair <BasicBlock *, BasicBlock *> processChunk(CFGContext &ctx, const Chunk &chunk);
	const Chunk & rewrite(CFGContext &ctx, const ForEach &forEach);

	Serial m_blockSerial;
	std::vector <std::unique_ptr <BasicBlock> > m_blocks;
	std::vector <std::unique_ptr <const Node> > m_additionalNodes;
	BasicBlock *m_entry, *m_exit;
};
