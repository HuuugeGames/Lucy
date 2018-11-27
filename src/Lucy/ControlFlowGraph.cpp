#include <functional>
#include <fstream>
#include <string_view>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "ControlFlowGraph.hpp"
#include "Serial.hpp"

struct CFGContext {
	std::vector <BasicBlock *> breakBlocks;
};

ControlFlowGraph::ControlFlowGraph(const Chunk &chunk)
{
	CFGContext ctx;
	std::tie(m_entry, m_exit) = process(ctx, chunk);

	calcPredecessors();
	prune();
}

std::pair <BasicBlock *, BasicBlock *> ControlFlowGraph::process(CFGContext &ctx, const Chunk &chunk)
{
	auto makeBB = [this]
	{
		m_blocks.emplace_back(std::make_unique<BasicBlock>(m_blockSerial.next()));
		m_blocks.back()->phase = m_walkPhase;
		return m_blocks.back().get();
	};

	auto redirectBreaks = [this, &ctx](BasicBlock *dst)
	{
		for (auto bb : ctx.breakBlocks)
			bb->nextBlock[0] = dst;
		ctx.breakBlocks.clear();
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
				current->exitType = BasicBlock::ExitType::Break;
				ctx.breakBlocks.push_back(current);
				break;
			case Node::Type::Function: {
				const Function *fnNode = static_cast<const Function *>(insn.get());
				process(ctx, *fnNode);
				current->insn.push_back(insn.get());
				break;
			}
			case Node::Type::Assignment: {
				const Assignment *assignmentNode = static_cast<const Assignment *>(insn.get());
				process(ctx, *assignmentNode);
				current->insn.push_back(insn.get());
				break;
			}
			case Node::Type::FunctionCall:
			case Node::Type::MethodCall: {
				const FunctionCall *fnCallNode = static_cast<const FunctionCall *>(insn.get());
				process(ctx, *fnCallNode);
				current->insn.push_back(insn.get());
				break;
			}
			case Node::Type::Chunk: {
				const Chunk *subChunk = static_cast<const Chunk *>(insn.get());
				auto [entry, exit] = process(ctx, *subChunk);
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				break;
			}
			case Node::Type::Return: {
				const Return *returnNode = static_cast<const Return *>(insn.get());
				current->exitType = BasicBlock::ExitType::Return;

				if (!returnNode->empty()) {
					current->returnExprList = &returnNode->exprList();
					process(ctx, returnNode->exprList());
				} else {
					current->returnExprList = nullptr;
				}

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

					process(ctx, *cond[i]);
					auto [entry, exit] = process(ctx, *chunks[i]);
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
					auto [entry, exit] = process(ctx, ifNode->elseNode());
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
				process(ctx, whileNode->condition());

				auto previous = current;
				current = makeBB();
				current->exitType = BasicBlock::ExitType::Conditional;
				current->condition = &whileNode->condition();
				previous->nextBlock[0] = current;
				previous = current;

				auto [entry, exit] = process(ctx, whileNode->chunk());
				current = makeBB();

				previous->nextBlock[0] = entry;
				previous->nextBlock[1] = current;
				exit->nextBlock[0] = previous;
				redirectBreaks(current);
				break;
			}
			case Node::Type::Repeat: {
				const Repeat *repeatNode = static_cast<const Repeat *>(insn.get());
				process(ctx, repeatNode->condition());

				auto [entry, exit] = process(ctx, repeatNode->chunk());
				current->nextBlock[0] = entry;

				current = makeBB();
				current->exitType = BasicBlock::ExitType::Conditional;
				current->condition = &repeatNode->condition();
				current->nextBlock[1] = entry;
				exit->nextBlock[0] = current;

				auto previous = current;
				current = makeBB();
				previous->nextBlock[0] = current;
				redirectBreaks(current);
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
				auto [entry, exit] = process(ctx, forNode->chunk());
				previous->nextBlock[0] = entry;
				exit->nextBlock[0] = previous;

				auto step = forNode->cloneStepExpr();
				exit->insn.push_back(step.get());
				m_additionalNodes.emplace_back(std::move(step));

				current = makeBB();
				previous->nextBlock[1] = current;
				redirectBreaks(current);
				break;
			}
			case Node::Type::ForEach: {
				const ForEach *forEachNode = static_cast<const ForEach *>(insn.get());
				auto [entry, exit] = process(ctx, rewrite(ctx, *forEachNode));
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				redirectBreaks(current);
				break;
			}
			default:
				std::cerr << "Unhandled instruction type: " << toUnderlying(insn->type()) << '\n';
				abort();
		}
	}

	return {entry, current};
}

void ControlFlowGraph::process(CFGContext &ctx, const Assignment &assignment)
{
	process(ctx, assignment.varList());
	process(ctx, assignment.exprList());
}

void ControlFlowGraph::process(CFGContext &ctx, const ExprList &exprList)
{
	for (const auto &e : exprList.exprs())
		process(ctx, *e);
}

void ControlFlowGraph::process(CFGContext &ctx, const Field &field)
{
	if (field.fieldType() == Field::Type::Brackets)
		process(ctx, field.keyExpr());
	process(ctx, field.valueExpr());
}

void ControlFlowGraph::process(CFGContext &ctx, const Function &fnNode)
{
	m_subgraphs.emplace_back(new ControlFlowGraph{fnNode.chunk()});
}

void ControlFlowGraph::process(CFGContext &ctx, const FunctionCall &fnCallNode)
{
	process(ctx, fnCallNode.functionExpr());
	process(ctx, fnCallNode.args());
}

void ControlFlowGraph::process(CFGContext &ctx, const LValue &lv)
{
	if (lv.lvalueType() != LValue::Type::Name)
		process(ctx, *lv.tableExpr());

	if (lv.lvalueType() == LValue::Type::Bracket)
		process(ctx, *lv.keyExpr());
}

void ControlFlowGraph::process(CFGContext &ctx, const Node &node)
{
	if (node.isValue() || node.type() == Node::Type::Ellipsis)
		return;

	switch (node.type()) {
		case Node::Type::Function:
			process(ctx, static_cast<const Function &>(node));
			break;
		case Node::Type::FunctionCall:
		case Node::Type::MethodCall:
			process(ctx, static_cast<const FunctionCall &>(node));
			break;
		case Node::Type::TableCtor:
			process(ctx, static_cast<const TableCtor &>(node));
			break;
		case Node::Type::NestedExpr:
			process(ctx, static_cast<const NestedExpr &>(node).expr());
			break;
		case Node::Type::LValue:
			process(ctx, static_cast<const LValue &>(node));
			break;
		case Node::Type::BinOp: {
			const BinOp &boNode = static_cast<const BinOp &>(node);
			process(ctx, boNode.left());
			process(ctx, boNode.right());
			break;
		}
		case Node::Type::UnOp: {
			process(ctx, static_cast<const UnOp &>(node).operand());
			break;
		}
		default:
			std::cerr << "Unhandled node type: " << toUnderlying(node.type()) << '\n';
			abort();
	}
}

void ControlFlowGraph::process(CFGContext &ctx, const TableCtor &table)
{
	for (const auto &f : table.fields())
		process(ctx, *f);
}

void ControlFlowGraph::process(CFGContext &ctx, const VarList &varList)
{
	for (const auto &v : varList.vars())
		process(ctx, *v);
}

void ControlFlowGraph::calcPredecessors()
{
	++m_walkPhase;

	std::function <void (BasicBlock *)> doCalcPredecessors = [this, &doCalcPredecessors](BasicBlock *block)
	{
		if (block->phase == m_walkPhase)
			return;
		block->phase = m_walkPhase;

		for (unsigned i = 0; i < 2; ++i) {
			if (block->nextBlock[i]) {
				block->nextBlock[i]->predecessors.push_back(block);
				doCalcPredecessors(block->nextBlock[i]);
			}
		}
	};

	doCalcPredecessors(m_entry);
}

void ControlFlowGraph::prune()
{
	++m_walkPhase;

	std::function <void (BasicBlock *)> doPrune = [this, &doPrune](BasicBlock *block)
	{
		if (!block)
			return;

		if (block->phase == m_walkPhase)
			return;
		block->phase = m_walkPhase;

		for (unsigned int i = 0; i < 2; ++i) {
			BasicBlock *next = block->nextBlock[i];
			while (next && next->isEmpty() && next != m_exit) {
				assert(!next->nextBlock[1]);

				BasicBlock *subNext = next->nextBlock[0];

				for (auto p : next->predecessors) {
					for (unsigned i = 0; i < 2; ++i) {
						if (p->nextBlock[i] == next)
							p->nextBlock[i] = subNext;
					}
				}

				if (subNext) {
					subNext->removePredecessor(next);
					std::copy(next->predecessors.begin(), next->predecessors.end(), std::back_inserter(subNext->predecessors));
				}
				next->nextBlock[0] = nullptr;
				next->predecessors.clear();
				next = subNext;
			}

			doPrune(block->nextBlock[i]);
		}
	};

	doPrune(m_entry);

	size_t idx = 0;
	while (idx < m_blocks.size()) {
		if (m_blocks[idx].get() != m_entry && m_blocks[idx].get() != m_exit && m_blocks[idx]->isEmpty()) {
			m_blocks[idx] = std::move(m_blocks.back());
			m_blocks.pop_back();
		} else {
			++idx;
		}
	}
}

const Chunk & ControlFlowGraph::rewrite(CFGContext &ctx, const ForEach &forEach)
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

void ControlFlowGraph::graphvizDump(std::ostream &os)
{
	const std::string CFGPrefix = "CFG_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + '_';
	auto unique_label = [&CFGPrefix](auto &&block) -> std::string
	{
		return CFGPrefix + block->label;
	};

	for (const auto &bb : m_blocks) {
		os << '\t' << unique_label(bb) << " [shape=box";
		if (!bb->insn.empty() || bb->exitType != BasicBlock::ExitType::Fallthrough) {
			os << ",label=<<table border=\"0\" cellborder=\"0\" cellspacing=\"0\"><tr><td align=\"left\">//" << bb->label << "</td></tr><hr/>";
			for (const auto &i : bb->insn) {
				os << "<tr><td align=\"left\">";
				i->printCode(os);
				os << "</td></tr>";
			}

			switch (bb->exitType) {
				case BasicBlock::ExitType::Conditional: {
					if (!bb->insn.empty())
						os << "<hr/>";
					os << "<tr><td align=\"left\"><b>if</b> ";
					bb->condition->printCode(os);
					os << "</td></tr>";
					break;
				}
				case BasicBlock::ExitType::Break: {
					os << "<tr><td align=\"left\"><b>break</b></td></tr>";
					break;
				}
				case BasicBlock::ExitType::Return: {
					os << "<tr><td align=\"left\"><b>return</b> ";
					if (bb->returnExprList)
						bb->returnExprList->printCode(os);
					os << "</td></tr>";
					break;
				}
				default:
					break;
			}

			os << "</table>>";
		} else {
			os << ",color=red";
		}
		os << "];\n";

		if (bb->exitType == BasicBlock::ExitType::Conditional) {
			auto [trueBlock, falseBlock] = std::make_tuple(bb->nextBlock[0], bb->nextBlock[1]);
			os << '\t' << unique_label(bb) << " -> " << unique_label(trueBlock) << " [label=\"true\",labelangle=45];\n";
			os << '\t' << unique_label(bb) << " -> " << unique_label(falseBlock) << " [label=\"false\",labeldistance=2.0];\n";
		} else {
			assert(!bb->nextBlock[1]);
			for (unsigned i = 0; i < 2; ++i) {
				if (auto b = bb->nextBlock[i]; b)
					os << '\t' << unique_label(bb) << " -> " << unique_label(b) << ";\n";
			}
		}

		os << '\n';
	}

	for (const auto &subgraph : m_subgraphs)
		subgraph->graphvizDump(os);
}

void ControlFlowGraph::graphvizDump(const char *filename)
{
	std::ofstream file{filename};

	file << "digraph BB {\n";
	file << "\tnode [fontname=\"Monospace\"];\n"
		"\tedge [fontname=\"Monospace\"];\n\n";

	graphvizDump(file);

	file << "}\n";
}
