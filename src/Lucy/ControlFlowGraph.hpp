#pragma once

#include <memory>
#include <vector>

#include "BasicBlock.hpp"
#include "Serial.hpp"

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
	const Chunk & generateClosure(const ForEach &forEach);
	std::pair <BasicBlock *, BasicBlock *> processChunk(const Chunk &chunk);

	Serial m_blockSerial;
	std::vector <std::unique_ptr <BasicBlock> > m_blocks;
	std::vector <std::unique_ptr <const Node> > m_additionalNodes;
	BasicBlock *m_entry, *m_exit;
};
