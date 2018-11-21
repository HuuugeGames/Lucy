#include <fstream>
#include <string_view>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "ControlFlowGraph.hpp"
#include "Serial.hpp"

ControlFlowGraph::ControlFlowGraph(const Chunk &chunk)
{
	std::tie(m_entry, m_exit) = processChunk(chunk);
}

std::pair <BasicBlock *, BasicBlock *> ControlFlowGraph::processChunk(const Chunk &chunk)
{
	auto makeBB = [this]
	{
		m_blocks.emplace_back(std::make_unique<BasicBlock>(m_blockSerial.next()));
		return m_blocks.back().get();
	};

	auto entry = makeBB();
	auto current = entry;

	for (const auto &insn : chunk.children()) {
		if (current->exitType != BasicBlock::ExitType::Fallthrough) {
			std::cerr << "Dangling statements after block exit\n"; //TODO location info
			break;
		}

		switch (insn->type()) {
			case Node::Type::Break:
				//TODO break from loop
				current->exitType = BasicBlock::ExitType::Break;
				[[fallthrough]]
			case Node::Type::Assignment:
			case Node::Type::FunctionCall:
			case Node::Type::Function:
				current->insn.push_back(insn.get());
				break;
			case Node::Type::Chunk: {
				const Chunk *subChunk = static_cast<const Chunk *>(insn.get());
				auto [entry, exit] = processChunk(*subChunk);
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				break;
			}
			case Node::Type::Return: {
				const Return *returnNode = static_cast<const Return *>(insn.get());
				current->exitType = BasicBlock::ExitType::Return;

				if (returnNode->empty())
					current->returnExprList = nullptr;
				else
					current->returnExprList = &returnNode->exprList();

				break;
			}
			case Node::Type::If: {
				const If *ifNode = static_cast<const If *>(insn.get());
				const auto &cond = ifNode->conditions();
				const auto &chunks = ifNode->chunks();
				std::vector <BasicBlock *> exits;

				BasicBlock *previous;
				for (size_t i = 0; i < cond.size(); ++i) {
					current->exitType = BasicBlock::ExitType::Conditional;
					current->condition = cond[i].get();

					auto [entry, exit] = processChunk(*chunks[i]);
					exits.emplace_back(exit);

					previous = current;
					previous->nextBlock[0] = entry;

					if (i + 1 < cond.size()) {
						current = makeBB();
						previous->nextBlock[1] = current;
					}
				}

				current = makeBB();
				if (ifNode->hasElse()) {
					auto [entry, exit] = processChunk(ifNode->elseNode());
					exits.emplace_back(exit);
					previous->nextBlock[1] = entry;
				} else {
					previous->nextBlock[1] = current;
				}

				for (auto *exit : exits)
					exit->nextBlock[0] = current;
				break;
			}
			case Node::Type::While: {
				const While *whileNode = static_cast<const While *>(insn.get());
				auto previous = current;
				current = makeBB();
				current->exitType = BasicBlock::ExitType::Conditional;
				current->condition = &whileNode->condition();
				previous->nextBlock[0] = current;
				previous = current;

				auto [entry, exit] = processChunk(whileNode->chunk());
				current = makeBB();

				previous->nextBlock[0] = entry;
				previous->nextBlock[1] = current;
				exit->nextBlock[0] = previous;
				break;
			}
			case Node::Type::Repeat: {
				const Repeat *repeatNode = static_cast<const Repeat *>(insn.get());
				auto [entry, exit] = processChunk(repeatNode->chunk());
				current->nextBlock[0] = entry;

				current = makeBB();
				current->exitType = BasicBlock::ExitType::Conditional;
				current->condition = &repeatNode->condition();
				current->nextBlock[1] = entry;
				exit->nextBlock[0] = current;

				auto previous = current;
				current = makeBB();
				previous->nextBlock[0] = current;
				break;
			}
			case Node::Type::For: {
				const For *forNode = static_cast<const For *>(insn.get());
				m_additionalNodes.emplace_back(new Assignment{forNode->iterator(), forNode->startExpr().clone()});
				current->insn.push_back(m_additionalNodes.back().get());

				auto previous = current;
				current = makeBB();
				previous->nextBlock[0] = current;

				auto condition = forNode->cloneCondition();
				current->exitType = BasicBlock::ExitType::Conditional;
				current->condition = condition.get();
				m_additionalNodes.emplace_back(std::move(condition));

				previous = current;
				auto [entry, exit] = processChunk(forNode->chunk());
				previous->nextBlock[0] = entry;
				exit->nextBlock[0] = previous;

				auto step = forNode->cloneStepExpr();
				exit->insn.push_back(step.get());
				m_additionalNodes.emplace_back(std::move(step));

				current = makeBB();
				previous->nextBlock[1] = current;
				break;
			}
			case Node::Type::ForEach: {
				const ForEach *forEachNode = static_cast<const ForEach *>(insn.get());
				auto [entry, exit] = processChunk(generateClosure(*forEachNode));
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				break;
			}
			default:
				std::cerr << "Unhandled instruction type: " << toUnderlying(insn->type()) << '\n';
				abort();
		}
	}

	return {entry, current};
}

const Chunk & ControlFlowGraph::generateClosure(const ForEach &forEach)
{
	const char IteratorFn[] = "__f";
	const char InvariantState[] = "__s";
	const char ControlVar[] = "__c";

	Chunk *result = new Chunk{};
	m_additionalNodes.emplace_back(result);

	VarList *initVars = new VarList{IteratorFn, InvariantState, ControlVar};
	Assignment *initAssignment = new Assignment{initVars, forEach.cloneExprList().release()};
	initAssignment->setLocal(true);
	result->append(initAssignment);

	Chunk *loopChunk = new Chunk{};
	While *loop = new While{new BooleanValue{true}, loopChunk};
	result->append(loop);

	//Is it JSON yet?
	Assignment *loopAssignment = new Assignment
	{
		forEach.cloneVariables().release(),
		new ExprList
		{
			new FunctionCall
			{
				new LValue{IteratorFn},
				new ExprList
				{
					new LValue{InvariantState},
					new LValue{ControlVar}
				}
			}
		}
	};
	loopAssignment->setLocal(true);
	loopChunk->append(loopAssignment);

	loopChunk->append(new Assignment{ControlVar, forEach.variables().names()[0]});

	If *endLoopCondition = new If
	{
		new BinOp
		{
			BinOp::Type::Equal,
			new LValue{ControlVar},
			new NilValue{},
		},
		new Chunk
		{
			new Break{}
		}
	};
	loopChunk->append(endLoopCondition);

	loopChunk->append(forEach.cloneChunk().release());

	return *result;
}

void ControlFlowGraph::graphvizDump(const char *filename)
{
	std::ofstream file{filename};

	file << "digraph BB {\n";
	file << "\tnode [fontname=\"Monospace\"];\n"
		"\tedge [fontname=\"Monospace\"];\n\n";

	for (const auto &bb : m_blocks) {
		file << '\t';
		file << bb->label << " [shape=box";
		if (!bb->insn.empty() || bb->exitType != BasicBlock::ExitType::Fallthrough) {
			file << ",label=<<table border=\"0\" cellborder=\"0\" cellspacing=\"0\"><tr><td align=\"left\">//" << bb->label << "</td></tr><hr/>";
			for (const auto &i : bb->insn) {
				file << "<tr><td align=\"left\">";
				i->printCode(file);
				file << "</td></tr>";
			}

			if (bb->exitType == BasicBlock::ExitType::Conditional) {
				if (!bb->insn.empty())
					file << "<hr/>";
				file << "<tr><td><b>if</b> ";
				bb->condition->printCode(file);
				file << "</td></tr>";
			}

			file << "</table>>";
		} else {
			file << ",color=red";
		}
		file << "];\n";

		if (bb->exitType == BasicBlock::ExitType::Conditional) {
			auto [trueBlock, falseBlock] = std::make_tuple(bb->nextBlock[0], bb->nextBlock[1]);
			file << '\t' << bb->label << " -> " << trueBlock->label << " [label=\"true\",labelangle=45];\n";
			file << '\t' << bb->label << " -> " << falseBlock->label << " [label=\"false\",labeldistance=2.0];\n";
		} else {
			assert(!bb->nextBlock[1]);
			for (unsigned i = 0; i < 2; ++i) {
				if (auto b = bb->nextBlock[i]; b)
					file << '\t' << bb->label << " -> " << b->label << ";\n";
			}
		}
		file << '\n';

	}
	file << "}\n";
}
