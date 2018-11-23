#include <cassert>

#include "BasicBlock.hpp"

bool BasicBlock::isEmpty() const
{
	return exitType == BasicBlock::ExitType::Fallthrough && insn.empty();
}

void BasicBlock::removePredecessor(BasicBlock *block)
{
	size_t i = 0;
	while (i < predecessors.size()) {
		if (predecessors[i] == block) {
			predecessors[i] = predecessors.back();
			predecessors.pop_back();
			return;
		}
		++i;
	}

	assert(false);
}
