#include <fstream>
#include <functional>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "ControlFlowGraph.hpp"
#include "Fold.hpp"
#include "Function.hpp"
#include "Scope.hpp"
#include "Serial.hpp"

struct ControlFlowGraph::CFGContext {
	std::vector <std::pair <BasicBlock *, const AST::Break &> > breakBlocks;
	std::vector <BasicBlock *> returnBlocks;
	Scope *currentScope;

	void popScope()
	{
		currentScope = currentScope->parent();
	}

	void pushScope()
	{
		currentScope = currentScope->push();
	}
};

ControlFlowGraph::ControlFlowGraph(const AST::Chunk &chunk, Scope &scope)
{
	CFGContext ctx;
	ctx.currentScope = &scope;
	std::tie(m_entry, m_exit) = process(ctx, chunk);

	for (const auto &brk : ctx.breakBlocks)
		LOG(Logger::Error, brk.second.location() << " : no loop to break from\n");

	if (!m_exit->isEmpty()) {
		m_blocks.emplace_back(std::make_unique<BasicBlock>(m_blockSerial.next()));
		auto commonExit = m_blocks.back().get();
		commonExit->phase = m_walkPhase;
		commonExit->scope = m_exit->scope;

		m_exit->nextBlock[0] = commonExit;
		m_exit = commonExit;
	}

	for (auto ret : ctx.returnBlocks)
		ret->nextBlock[0] = m_exit;

	calcPredecessors();
	generateIR();
	prune();

	scope.reportUnusedFnParams();
}

ControlFlowGraph::~ControlFlowGraph() = default;

void ControlFlowGraph::graphvizDump(const char *filename) const
{
	std::ofstream file{filename};

	file << "digraph BB {\n";
	file << "\tnode [fontname=\"Monospace\"];\n"
		"\tedge [fontname=\"Monospace\"];\n\n";

	graphvizDump(file);

	file << "}\n";
}

void ControlFlowGraph::irDump(unsigned indent, const char *label) const
{
	if (!label)
		label = "global scope";

	Logger::indent(std::cout, indent) << '[' << label << "]\n";
	for (unsigned i = 0; i != m_blocks.size(); ++i) {
		m_blocks[i]->irDump(indent + 1);
		if (i + 1 != m_blocks.size())
			std::cout << '\n';
	}

	if (!m_functions.empty())
		std::cout << '\n';

	for (unsigned i = 0; i != m_functions.size(); ++i) {
		m_functions[i]->irDump(indent + 1);
		if (i + 1 != m_functions.size())
			std::cout << '\n';
	}

	Logger::indent(std::cout, indent) << "[/" << label << "]\n";
}

std::pair <BasicBlock *, BasicBlock *> ControlFlowGraph::process(CFGContext &ctx, const AST::Chunk &chunk)
{
	auto makeBB = [this, &ctx]
	{
		m_blocks.emplace_back(std::make_unique<BasicBlock>(m_blockSerial.next()));
		auto newBlock = m_blocks.back().get();
		newBlock->phase = m_walkPhase;
		newBlock->scope = ctx.currentScope;
		return newBlock;
	};

	auto redirectBreaks = [this, &ctx](BasicBlock *dst)
	{
		for (const auto &brk : ctx.breakBlocks)
			brk.first->nextBlock[0] = dst;
		ctx.breakBlocks.clear();
	};

	ctx.pushScope();
	auto entry = makeBB();
	auto current = entry;

	if (chunk.isEmpty()) {
		if (ctx.currentScope->parent() == ctx.currentScope->functionScope())
			Logger::logIssue<Issue::Function::EmptyDefinition>(chunk.location());
		else
			Logger::logIssue<Issue::EmptyChunk>(chunk.location());
	}

	for (const auto &insn : chunk.children()) {
		if (current->exitType() != BasicBlock::ExitType::Fallthrough) {
			LOG(Logger::Error, insn->location() << " : dangling statements after block exit\n");
			break;
		}

		switch (insn->type().value()) {
			case AST::Node::Type::Break: {
				auto breakNode = static_cast<const AST::Break *>(insn.get());
				current->setExitType(BasicBlock::ExitType::Break);
				ctx.breakBlocks.emplace_back(current, *breakNode);
				break;
			}
			case AST::Node::Type::Function: {
				auto fnNode = static_cast<const AST::Function *>(insn.get());
				process(ctx, *fnNode);
				if (!fnNode->isAnonymous()) {
					const auto &assignFn = rewrite(ctx, *fnNode);
					current->insn.push_back(&assignFn);
				} else {
					current->insn.push_back(insn.get());
				}
				break;
			}
			case AST::Node::Type::Assignment: {
				auto assignmentNode = static_cast<const AST::Assignment *>(insn.get());
				process(ctx, *assignmentNode);
				current->insn.push_back(insn.get());
				break;
			}
			case AST::Node::Type::FunctionCall: {
				auto prev = current;
				current = makeBB();
				prev->nextBlock[0] = current;

				auto fnCallNode = static_cast<const AST::FunctionCall *>(insn.get());
				process(ctx, *fnCallNode);
				current->insn.push_back(insn.get());
				break;
			}
			case AST::Node::Type::Chunk: {
				auto subChunk = static_cast<const AST::Chunk *>(insn.get());
				auto [entry, exit] = process(ctx, *subChunk);
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				break;
			}
			case AST::Node::Type::Return: {
				auto returnNode = static_cast<const AST::Return *>(insn.get());
				current->setExitType(BasicBlock::ExitType::Return);
				ctx.returnBlocks.push_back(current);

				if (!returnNode->empty()) {
					current->returnExprList = &returnNode->exprList();
					process(ctx, returnNode->exprList());
				} else {
					current->returnExprList = nullptr;
				}

				if (auto fn = ctx.currentScope->function(); fn)
					fn->setResultCount(*returnNode);

				break;
			}
			case AST::Node::Type::If: {
				auto ifNode = static_cast<const AST::If *>(insn.get());
				const auto &cond = ifNode->conditions();
				const auto &chunks = ifNode->chunks();
				std::vector <BasicBlock *> exits;

				BasicBlock *previous;
				for (size_t i = 0; i < cond.size(); ++i) {
					current->setExitType(BasicBlock::ExitType::Conditional);
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
			case AST::Node::Type::While: {
				auto whileNode = static_cast<const AST::While *>(insn.get());
				process(ctx, whileNode->condition());

				auto previous = current;
				current = makeBB();
				current->setExitType(BasicBlock::ExitType::Conditional);
				current->condition = &whileNode->condition();
				previous->nextBlock[0] = current;
				previous = current;

				auto [entry, exit] = process(ctx, whileNode->chunk());
				current = makeBB();

				previous->nextBlock[0] = entry;
				previous->nextBlock[1] = current;
				exit->nextBlock[0] = previous;
				exit->setLoopFooter(current);
				redirectBreaks(current);
				break;
			}
			case AST::Node::Type::Repeat: {
				auto repeatNode = static_cast<const AST::Repeat *>(insn.get());
				process(ctx, repeatNode->condition());

				auto [entry, exit] = process(ctx, repeatNode->chunk());
				current->nextBlock[0] = entry;

				current = makeBB();
				current->setExitType(BasicBlock::ExitType::Conditional);
				current->condition = &repeatNode->condition();

				auto helperBlock = makeBB();
				current->nextBlock[1] = helperBlock;
				helperBlock->nextBlock[0] = entry;
				helperBlock->setAttribute(BasicBlock::Attribute::BackEdge);
				exit->nextBlock[0] = current;

				auto previous = current;
				current = makeBB();
				previous->nextBlock[0] = current;
				helperBlock->setLoopFooter(current);
				redirectBreaks(current);
				break;
			}
			case AST::Node::Type::For: {
				auto forNode = static_cast<const AST::For *>(insn.get());
				auto assignmentNode = std::make_unique<AST::Assignment>(forNode->iterator(), forNode->startExpr().clone());
				assignmentNode->setLocal(true);
				current->insn.push_back(assignmentNode.get());
				m_additionalNodes.emplace_back(std::move(assignmentNode));

				auto previous = current;
				current = makeBB();
				previous->nextBlock[0] = current;

				auto condition = forNode->cloneCondition();
				current->setExitType(BasicBlock::ExitType::Conditional);
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
				exit->setLoopFooter(current);
				previous->nextBlock[1] = current;
				redirectBreaks(current);
				break;
			}
			case AST::Node::Type::ForEach: {
				auto forEachNode = static_cast<const AST::ForEach *>(insn.get());
				auto [entry, exit] = process(ctx, rewrite(ctx, *forEachNode));
				current->nextBlock[0] = entry;
				current = makeBB();
				exit->nextBlock[0] = current;
				redirectBreaks(current);
				break;
			}
			default:
				FATAL(insn->location() << " : Unhandled instruction type: " << insn->type() << '\n');
		}
	}

	ctx.popScope();
	return {entry, current};
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::Assignment &assignment)
{
	process(ctx, assignment.exprList());

	for (const auto &lval : assignment.varList().vars()) {
		if (lval->lvalueType() != AST::LValue::Type::Name)
			process(ctx, *lval);
	}

	if (assignment.isLocal()) {
		for (const auto &lval : assignment.varList().vars())
			ctx.currentScope->addLocalStore(*lval);
	} else {
		for (const auto &lval : assignment.varList().vars())
			ctx.currentScope->addVarAccess(*lval, VarAccess::Type::Write);
	}
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::ExprList &exprList)
{
	for (const auto &e : exprList.exprs())
		process(ctx, *e);
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::Field &field)
{
	if (field.fieldType() == AST::Field::Type::Brackets)
		process(ctx, field.keyExpr());
	process(ctx, field.valueExpr());
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::Function &fnNode)
{
	m_functions.emplace_back(std::make_unique<Function>(fnNode, *ctx.currentScope));
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::FunctionCall &fnCallNode)
{
	process(ctx, fnCallNode.functionExpr());
	process(ctx, fnCallNode.args());
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::LValue &lval)
{
	switch (lval.lvalueType()) {
		case AST::LValue::Type::Name:
			ctx.currentScope->addVarAccess(lval, VarAccess::Type::Read);
			break;
		case AST::LValue::Type::Bracket:
			process(ctx, lval.keyExpr());
			[[fallthrough]];
		case AST::LValue::Type::Dot:
			process(ctx, lval.tableExpr());
			break;
	}
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::Node &node)
{
	if (node.isValue() || node.type() == AST::Node::Type::Ellipsis)
		return;

	switch (node.type().value()) {
		case AST::Node::Type::Function:
			process(ctx, static_cast<const AST::Function &>(node));
			break;
		case AST::Node::Type::FunctionCall:
		case AST::Node::Type::MethodCall:
			process(ctx, static_cast<const AST::FunctionCall &>(node));
			break;
		case AST::Node::Type::TableCtor:
			process(ctx, static_cast<const AST::TableCtor &>(node));
			break;
		case AST::Node::Type::NestedExpr:
			process(ctx, static_cast<const AST::NestedExpr &>(node).expr());
			break;
		case AST::Node::Type::LValue:
			process(ctx, static_cast<const AST::LValue &>(node));
			break;
		case AST::Node::Type::BinOp: {
			const AST::BinOp &boNode = static_cast<const AST::BinOp &>(node);
			process(ctx, boNode.left());
			process(ctx, boNode.right());
			break;
		}
		case AST::Node::Type::UnOp: {
			process(ctx, static_cast<const AST::UnOp &>(node).operand());
			break;
		}
		default:
			FATAL(node.location() << " : Unhandled node type: " << node.type() << '\n');
	}
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::TableCtor &table)
{
	for (const auto &f : table.fields())
		process(ctx, *f);
}

void ControlFlowGraph::process(CFGContext &ctx, const AST::VarList &varList)
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

		for (BasicBlock *next : block->nextBlock) {
			if (next) {
				next->predecessors.push_back(block);
				doCalcPredecessors(next);
			}
		}
	};

	doCalcPredecessors(m_entry);
}

void ControlFlowGraph::generateIR()
{
	++m_walkPhase;

	std::function <void (BasicBlock *)> doGenerateIR = [this, &doGenerateIR](BasicBlock *block)
	{
		if (!block || block->phase == m_walkPhase)
			return;
		block->phase = m_walkPhase;

		block->generateIR();
		for (BasicBlock *next : block->nextBlock)
			doGenerateIR(next);
	};

	doGenerateIR(m_entry);
}

void ControlFlowGraph::prune()
{
	++m_walkPhase;

	auto canPrune = [this](const BasicBlock *block)
	{
		return block && block != m_exit && block != m_entry && block->canPrune();
	};

	std::function <void (BasicBlock *)> doPrune = [this, &canPrune, &doPrune](BasicBlock *block)
	{
		if (!block || block->phase == m_walkPhase)
			return;
		block->phase = m_walkPhase;

		for (unsigned int i = 0; i < 2; ++i) {
			BasicBlock *next = block->nextBlock[i];
			while (canPrune(next)) {
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
		if (canPrune(m_blocks[idx].get())) {
			m_blocks[idx] = std::move(m_blocks.back());
			m_blocks.pop_back();
		} else {
			++idx;
		}
	}
}

const AST::Chunk & ControlFlowGraph::rewrite(CFGContext &ctx, const AST::ForEach &forEach)
{
	const char IteratorFn[] = "__f";
	const char InvariantState[] = "__s";
	const char ControlVar[] = "__c";

	auto node = std::make_unique<AST::Chunk>();

	{
		auto initAssignment = std::make_unique<AST::Assignment>(
			std::make_unique<AST::VarList>(std::initializer_list<const char *>{IteratorFn, InvariantState, ControlVar}),
			forEach.cloneExprList()
		);
		initAssignment->setLocal(true);
		node->append(std::move(initAssignment));
	}

	auto loopChunk = std::make_unique<AST::Chunk>();
	{
		auto iteratorFnArgs = std::make_unique<AST::ExprList>();
		iteratorFnArgs->append(std::make_unique<AST::LValue>(InvariantState));
		iteratorFnArgs->append(std::make_unique<AST::LValue>(ControlVar));

		auto loopExprList = std::make_unique<AST::ExprList>();
		loopExprList->append(std::make_unique<AST::FunctionCall>(std::make_unique<AST::LValue>(IteratorFn), std::move(iteratorFnArgs)));

		auto loopAssignment = std::make_unique<AST::Assignment>(forEach.cloneVariables(), std::move(loopExprList));
		loopAssignment->setLocal(true);
		loopChunk->append(std::move(loopAssignment));
		loopChunk->append(std::make_unique<AST::Assignment>(ControlVar, forEach.variables().names()[0].first));
	}

	{
		auto endLoopChunk = std::make_unique<AST::Chunk>();
		endLoopChunk->append(std::make_unique<AST::Break>());
		auto endLoopCondition = std::make_unique<AST::If>
		(
			std::make_unique<AST::BinOp>
			(
				AST::BinOp::Type::Equal,
				std::make_unique<AST::LValue>(ControlVar),
				std::make_unique<AST::NilValue>()
			),
			std::move(endLoopChunk)
		);
		loopChunk->append(std::move(endLoopCondition));
		loopChunk->append(forEach.cloneChunk());
	}

	node->append(std::make_unique<AST::While>(std::make_unique<AST::BooleanValue>(true), std::move(loopChunk)));
	auto result = node.get();
	m_additionalNodes.emplace_back(std::move(node));
	return *result;
}

const AST::Assignment & ControlFlowGraph::rewrite(CFGContext &ctx, const AST::Function &fnNode)
{
	assert(!fnNode.isAnonymous());

	const auto &fnName = fnNode.name();
	const auto &nameParts = fnName.nameParts();
	auto nameLval = std::make_unique<AST::LValue>(nameParts[0]);

	for (unsigned i = 1; i != nameParts.size(); ++i)
		nameLval = std::make_unique<AST::LValue>(std::move(nameLval), nameParts[i]);
	if (fnName.isMethod())
		nameLval = std::make_unique<AST::LValue>(std::move(nameLval), fnName.method());

	auto anonymizedFn = fnNode.clone<AST::Function>();
	anonymizedFn->clearName();

	auto node = std::make_unique<AST::Assignment>(std::move(nameLval), std::move(anonymizedFn));
	if (fnNode.isLocal())
		node->setLocal(true);

	auto result = node.get();
	m_additionalNodes.emplace_back(std::move(node));
	return *result;
}

const AST::FunctionCall & ControlFlowGraph::rewrite(CFGContext &ctx, const AST::MethodCall &callNode)
{
	auto node = callNode.cloneAsFunctionCall();
	auto result = node.get();
	m_additionalNodes.emplace_back(std::move(node));
	return *result;
}

void ControlFlowGraph::graphvizDump(std::ostream &os) const
{
	const std::string CFGPrefix = "CFG_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + '_';
	auto uniqueLabel = [&CFGPrefix](auto &&block) -> std::string
	{
		return CFGPrefix + block->label();
	};

	std::unordered_set <const BasicBlock *> visited;

	std::function <void (const BasicBlock *)> printGraph = [this, &os, &printGraph, &uniqueLabel, &visited](const BasicBlock *bb)
	{
		if (!bb || !visited.insert(bb).second)
			return;

		os << '\t' << uniqueLabel(bb) << " [shape=box";
		if (!bb->isEmpty()) {
			os << ",label=<<table border=\"0\" cellborder=\"0\" cellspacing=\"0\"><tr><td align=\"left\">//"
				<< bb->label() << " [" << bb->exitType() << "]</td></tr><hr/>";
			for (const auto &i : bb->irCode) {
				os << "<tr><td align=\"left\">" << *i << "</td></tr>";
			}

			switch (bb->exitType().value()) {
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

		if (bb->exitType() == BasicBlock::ExitType::Conditional) {
			auto [trueBlock, falseBlock] = std::make_tuple(bb->nextBlock[0], bb->nextBlock[1]);
			os << '\t' << uniqueLabel(bb) << " -> " << uniqueLabel(trueBlock) << " [label=\"true\",labelangle=45];\n";
			os << '\t' << uniqueLabel(bb) << " -> " << uniqueLabel(falseBlock) << " [label=\"false\",labeldistance=2.0];\n";
		} else {
			assert(!bb->nextBlock[1]);
			if (auto next = bb->nextBlock[0]; next) {
				os << '\t' << uniqueLabel(bb) << " -> " << uniqueLabel(next);
				if (bb->attribute(BasicBlock::Attribute::BackEdge)) {
					assert(bb->loopFooter());
					os << " [color=blue];\n";
					os << '\t' << uniqueLabel(bb) << " -> " << uniqueLabel(bb->loopFooter()) << " [style=dashed,color=blue]";
				}
				os << ";\n";
			}
		}

		os << '\n';

		for (const auto &next : bb->nextBlock)
			printGraph(next);
	};

	printGraph(m_entry);

	for (const auto &f : m_functions)
		f->cfg().graphvizDump(os);
}
