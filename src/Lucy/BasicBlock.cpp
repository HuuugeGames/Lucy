#include <array>
#include <cassert>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "Fold.hpp"

struct BasicBlock::BBContext {
	BasicBlock *current = nullptr;
	std::vector <BasicBlock *> blockStack;
	std::vector <RValue> stack;
	std::vector <unsigned> requiredResults;
	Serial tempCnt;

	void emplaceTriplet(IR::Triplet *t)
	{
		current->irCode.emplace_back(t);
	}

	IR::Triplet * lastTriplet()
	{
		return current->irCode.back().get();
	}

	const AST::LValue * getTemporary()
	{
		return RValue::getTemporary(tempCnt.next());
	}

	void popBlock()
	{
		assert(!blockStack.empty());
		current->finalize(*this);
		blockStack.pop_back();
		current = blockStack.empty() ? nullptr : blockStack.back();
	}

	void pushBlock(BasicBlock *b)
	{
		current = b;
		blockStack.push_back(b);
	}
};

BasicBlock::BasicBlock(UID id)
	: m_label{std::string{"BB_"} + std::to_string(id)}
{
}

BasicBlock::BasicBlock(const std::string &label)
	: m_label{label}
{
}

BasicBlock::BasicBlock(std::string &&label)
	: m_label{std::move(label)}
{
}

BasicBlock & BasicBlock::operator = (BasicBlock &&other) = default;

BasicBlock::~BasicBlock() = default;

void BasicBlock::irDump(unsigned indent) const
{
	Logger::indent(std::cout, indent) << '[' << label() << "]\n";
	for (const auto &t : irCode)
		Logger::indent(std::cout, indent + 1) << *t << '\n';

	Logger::indent(std::cout, indent) << "[/" << label() << "]\n";

	if (m_condNode) {
		std::cout << '\n';
		m_subBlocks[0]->irDump(indent + 1);
		std::cout << '\n';
		m_subBlocks[1]->irDump(indent);
	}
}

bool BasicBlock::canPrune() const
{
	return isEmpty() && !attribute(Attribute::BackEdge) && !attribute(Attribute::LoopFooter);
}

bool BasicBlock::isEmpty() const
{
	return exitType() == BasicBlock::ExitType::Fallthrough && irCode.empty();
}

void BasicBlock::generateIR()
{
	BBContext ctx;
	ctx.pushBlock(this);

	for (unsigned idx = 0; idx != insn.size(); ++idx) {
		const AST::Node *insnNode = insn[idx];

		ctx.requiredResults.push_back(0);
		ctx.tempCnt.reset();
		ctx.current->process(ctx, *insnNode);
		ctx.requiredResults.pop_back();
		assert(ctx.stack.empty());
	}

	ctx.popBlock();
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

void BasicBlock::setLoopFooter(BasicBlock *dst)
{
	this->setAttribute(BasicBlock::Attribute::BackEdge);
	m_loopFooterEdge = dst;
	dst->setAttribute(BasicBlock::Attribute::LoopFooter);
}

void BasicBlock::finalize(BBContext &ctx)
{
	switch (ctx.current->exitType().value()) {
		case ExitType::Conditional: {
			assert(ctx.current->condition);
			assert(!ctx.current->returnExprList);

			ctx.requiredResults.push_back(1);
			process(ctx, *ctx.current->condition);
			ctx.requiredResults.pop_back();

			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Test, ctx.stack.back()});
			ctx.stack.pop_back();
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::JumpTrue, ValueVariant{ctx.current->nextBlock[0]->label()}});
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Jump, ValueVariant{ctx.current->nextBlock[1]->label()}});
			break;
		}

		case ExitType::Return: {
			assert(!ctx.current->condition);

			if (!ctx.current->returnExprList)
				break;

			const auto &exprList = static_cast<const AST::ExprList &>(*ctx.current->returnExprList);
			ctx.tempCnt.reset();
			process(ctx, exprList);

			for (unsigned i = 0; i != exprList.exprs().size(); ++i) {
				ctx.emplaceTriplet(new IR::Triplet{IR::Op::Push, ctx.stack.back()});
				ctx.stack.pop_back();
			}
			const long resultCnt = exprList.exprs().size();
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Return, ValueVariant{resultCnt}});

			break;
		}

		default: {
			if (ctx.current->nextBlock[0])
				ctx.emplaceTriplet(new IR::Triplet{IR::Op::Jump, ValueVariant{ctx.current->nextBlock[0]->label()}});
		}
	}
}

void BasicBlock::process(BBContext &ctx, const AST::Assignment &assignment)
{
	const auto baseStackSize = ctx.stack.size();
	const auto &exprList = assignment.exprList();
	const auto &varList = assignment.varList().vars();

	for (size_t exprIdx = 0; exprIdx != exprList.size(); ++exprIdx) {
		const auto &e = exprList.exprs()[exprIdx];

		long resultsNeeded = 0;
		if (exprIdx < exprList.size() - 1 && exprIdx < varList.size())
			resultsNeeded = 1;
		else if (exprIdx == exprList.size() - 1 && exprIdx < varList.size())
			resultsNeeded = varList.size() - exprList.size() + 1;

		ctx.requiredResults.push_back(resultsNeeded);
		process(ctx, *e);
		ctx.requiredResults.pop_back();

		if (e->type() != AST::Node::Type::FunctionCall) {
			auto result = ctx.stack.back();
			ctx.stack.pop_back();

			auto tmp = ctx.getTemporary();
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, tmp, result});
			ctx.stack.push_back(tmp);
		}
	}

	{
		const auto dstStackSize = baseStackSize + varList.size();
		while (ctx.stack.size() > dstStackSize)
			ctx.stack.pop_back();
		while (ctx.stack.size() < dstStackSize)
			ctx.stack.push_back(RValue::nil());
	}

	for (const auto &v : varList) {
		ctx.requiredResults.push_back(1);
		process(ctx, *v);
		ctx.requiredResults.pop_back();
	}

	assert(ctx.stack.size() == baseStackSize + 2 * varList.size());

	for (size_t i = 0; i != varList.size(); ++i) {
		const auto &lhs = ctx.stack[baseStackSize + varList.size() + i];
		const auto &rhs = ctx.stack[baseStackSize + i];

		if (auto t = std::get_if<RValue::Type::Temporary>(&lhs.valueRef); t && (*t)->operation == IR::Op::TableIndex) {
			auto tableRef = new IR::Triplet{IR::Op::TableAssign, ValueVariant{TableReference{&(*t)->operands[0], &(*t)->operands[1]}}, rhs};
			ctx.emplaceTriplet(tableRef);
		} else {
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, lhs, rhs});
		}
	}

	ctx.stack.resize(baseStackSize);
}

void BasicBlock::process(BBContext &ctx, const AST::BinOp &binOp)
{
	ctx.requiredResults.push_back(1);
	process(ctx, binOp.left());
	auto lhs = ctx.stack.back();
	ctx.stack.pop_back();

	if (anyOf(binOp.binOpType(), AST::BinOp::Type::And, AST::BinOp::Type::Or)) {
		auto tmp = ctx.getTemporary();
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, tmp, lhs});

		splitBlock(ctx, tmp, binOp);
		ctx.stack.push_back(tmp);
		ctx.requiredResults.pop_back();
		return;
	}

	process(ctx, binOp.right());
	ctx.requiredResults.pop_back();
	auto rhs = ctx.stack.back();
	ctx.stack.pop_back();

	ctx.emplaceTriplet(new IR::Triplet{IR::Op{binOp.binOpType().value()}, lhs, rhs});
	ctx.stack.push_back(ctx.lastTriplet());
}

void BasicBlock::process(BBContext &ctx, const AST::Ellipsis &ellipsis)
{
	auto tmp = ctx.getTemporary();
	ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, tmp, ValueVariant{std::string{"arg"}}});
	ctx.stack.push_back(tmp);
}

void BasicBlock::process(BBContext &ctx, const AST::ExprList &exprList)
{
	const auto &exprs = exprList.exprs();

	for (unsigned i = 0; i != exprs.size(); ++i) {
		const auto &e = exprs[i];

		if (i != exprs.size() - 1)
			ctx.requiredResults.push_back(1);
		else
			ctx.requiredResults.push_back(std::numeric_limits<unsigned>::max());
		process(ctx, *e);
		ctx.requiredResults.pop_back();

		auto result = ctx.stack.back();
		ctx.stack.pop_back();

		auto tmp = ctx.getTemporary();
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, tmp, result});
		ctx.stack.push_back(tmp);
	}
}

void BasicBlock::process(BBContext &ctx, const AST::Function &fnNode)
{
	assert(fnNode.isAnonymous());

	auto tmp = ctx.getTemporary();
	ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, tmp, ValueVariant{&fnNode}});
	ctx.stack.push_back(tmp);
}

void BasicBlock::process(BBContext &ctx, const AST::FunctionCall &fnCallNode)
{
	const size_t stackBase = ctx.stack.size();

	process(ctx, fnCallNode.args());
	while (ctx.stack.size() != stackBase) {
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::Push, ctx.stack.back()});
		ctx.stack.pop_back();
	}

	ctx.requiredResults.push_back(1);
	process(ctx, fnCallNode.functionExpr());
	ctx.requiredResults.pop_back();

	const long requiredArgs = fnCallNode.args().size();

	bool knownResultCnt = true;
	long requiredResults = 0;
	if (ctx.requiredResults.back() != std::numeric_limits<unsigned>::max())
		requiredResults = ctx.requiredResults.back();
	else
		knownResultCnt = false;

	if (knownResultCnt) {
		const long fnCallMeta = (requiredArgs << 16) | requiredResults;

		ctx.emplaceTriplet(new IR::Triplet{IR::Op::Call, ctx.stack.back(), ValueVariant{fnCallMeta}});
		ctx.stack.pop_back();

		for (long i = 0; i != requiredResults; ++i) {
			auto tmp = ctx.getTemporary();
			ctx.emplaceTriplet(new IR::Triplet{IR::Op::Pop, tmp});
			ctx.stack.push_back(tmp);
		}
	} else {
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::CallUnknownResults, ctx.stack.back(), ValueVariant{requiredArgs}});
		ctx.stack.pop_back();

		auto tmp = ctx.getTemporary();
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::Pop, tmp});
		ctx.stack.push_back(tmp);
	}
}

void BasicBlock::process(BBContext &ctx, const AST::LValue &lval)
{
	if (lval.lvalueType() == AST::LValue::Type::Name) {
		ctx.stack.push_back(&lval);
		return;
	}

	ctx.requiredResults.push_back(1);
	process(ctx, lval.tableExpr());
	auto tableVar = ctx.stack.back();
	ctx.stack.pop_back();

	if (lval.lvalueType() == AST::LValue::Type::Dot) {
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::TableIndex, tableVar, ValueVariant{lval.name()}});
	} else {
		process(ctx, lval.keyExpr());
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::TableIndex, tableVar, ctx.stack.back()});
		ctx.stack.pop_back();
	}
	ctx.requiredResults.pop_back();

	ctx.stack.push_back(ctx.lastTriplet());
}

void BasicBlock::process(BBContext &ctx, const AST::NestedExpr &nestedExpr)
{
	ctx.requiredResults.push_back(1);
	process(ctx, nestedExpr.expr());
	ctx.requiredResults.pop_back();
}

void BasicBlock::process(BBContext &ctx, const AST::Node &node)
{
	#define SUBCASE(type) case AST::Node::Type::type: return process(ctx, static_cast<const AST::type &>(node))

	switch (node.type().value()) {
		SUBCASE(Assignment);
		SUBCASE(BinOp);
		SUBCASE(Ellipsis);
		SUBCASE(ExprList);
		SUBCASE(Function);
		SUBCASE(FunctionCall);
		SUBCASE(LValue);
		SUBCASE(NestedExpr);
		SUBCASE(TableCtor);
		SUBCASE(UnOp);
		SUBCASE(Value);
		default: FATAL(node.location() << " : Unhandled node type: " << node.type() << '\n');
	}

	#undef SUBCASE
}

void BasicBlock::process(BBContext &ctx, const AST::TableCtor &tableCtor)
{
	auto table = ctx.getTemporary();
	ctx.emplaceTriplet(new IR::Triplet{IR::Op::TableCtor, table});
	const RValue *tableRval = &ctx.lastTriplet()->operands[0];

	long idxCnt = 0;
	for (const auto &f : tableCtor.fields()) {
		auto k = ctx.getTemporary();

		switch (f->fieldType()) {
			case AST::Field::Type::Brackets:
				ctx.requiredResults.push_back(1);
				process(ctx, f->keyExpr());
				ctx.requiredResults.pop_back();

				ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, k, ctx.stack.back()});
				ctx.stack.pop_back();
				break;
			case AST::Field::Type::Literal:
				ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, k, ValueVariant{f->fieldName()}});
				break;
			case AST::Field::Type::NoIndex:
				ctx.emplaceTriplet(new IR::Triplet{IR::Op::Assign, k, ValueVariant{++idxCnt}});
				break;
		}

		const RValue *keyRval = &ctx.lastTriplet()->operands[0];

		ctx.requiredResults.push_back(1);
		process(ctx, f->valueExpr());
		ctx.requiredResults.pop_back();

		RValue v = ctx.stack.back();
		ctx.stack.pop_back();

		TableReference ref{tableRval, keyRval};
		ctx.emplaceTriplet(new IR::Triplet{IR::Op::TableAssign, ValueVariant{ref}, v});
	}

	ctx.stack.push_back(table);
}

void BasicBlock::process(BBContext &ctx, const AST::UnOp &unOp)
{
	static const std::array <IR::Op, AST::UnOp::Type::_size> UnaryOpType {
		IR::Op::UnaryNegate,
		IR::Op::UnaryNot,
		IR::Op::UnaryLength,
	};

	ctx.requiredResults.push_back(1);
	process(ctx, unOp.operand());
	ctx.requiredResults.pop_back();

	auto operand = ctx.stack.back();
	ctx.stack.pop_back();

	ctx.emplaceTriplet(new IR::Triplet{UnaryOpType[static_cast<unsigned>(unOp.unOpType())], RValue{operand}});
	ctx.stack.push_back(ctx.lastTriplet());
}

void BasicBlock::process(BBContext &ctx, const AST::Value &valueNode)
{
	ctx.stack.push_back(valueNode.toValueVariant());
}

void BasicBlock::splitBlock(BBContext &ctx, const AST::LValue *tmpDst, const AST::BinOp &binOp)
{
	assert(anyOf(binOp.binOpType(), AST::BinOp::Type::And, AST::BinOp::Type::Or));

	BasicBlock *trueBlock = new BasicBlock{ctx.current->label() + "_T"};
	BasicBlock *falseBlock = new BasicBlock{ctx.current->label() + "_F"};

	falseBlock->setAttribute(BasicBlock::Attribute::SubBlock);
	falseBlock->setExitType(ctx.current->exitType());
	falseBlock->returnExprList = ctx.current->returnExprList;
	falseBlock->condition = ctx.current->condition;
	falseBlock->nextBlock = ctx.current->nextBlock;
	falseBlock->predecessors.push_back(ctx.current);
	falseBlock->predecessors.push_back(trueBlock);

	falseBlock->phase = trueBlock->phase = ctx.current->phase;
	falseBlock->scope = trueBlock->scope = ctx.current->scope;

	trueBlock->setAttribute(BasicBlock::Attribute::SubBlock);
	trueBlock->nextBlock[0] = falseBlock;
	trueBlock->predecessors.push_back(ctx.current);

	ctx.current->m_subBlocks[0].reset(trueBlock);
	ctx.current->m_subBlocks[1].reset(falseBlock);

	for (auto nextBlock : ctx.current->nextBlock) {
		if (nextBlock) {
			nextBlock->removePredecessor(ctx.current);
			nextBlock->predecessors.push_back(falseBlock);
		}
	}

	ctx.current->nextBlock[0] = trueBlock;
	ctx.current->nextBlock[1] = falseBlock;

	AST::Node *conditionalExpr = tmpDst->clone().release();
	if (binOp.binOpType() == AST::BinOp::Type::Or)
		conditionalExpr = new AST::UnOp{AST::UnOp::Type::Not, conditionalExpr};

	AST::Assignment *assignRightSide = new AST::Assignment{static_cast<AST::LValue *>(tmpDst->clone().release()), binOp.right().clone().release()};

	AST::If *condNode = new AST::If
	{
		conditionalExpr,
		new AST::Chunk
		{
			{assignRightSide}
		}
	};

	ctx.current->setExitType(ExitType::Conditional);
	ctx.current->returnExprList = nullptr;
	ctx.current->condition = condNode->conditions()[0].get();
	ctx.current->m_condNode.reset(condNode);
	ctx.popBlock();

	ctx.pushBlock(trueBlock);
	process(ctx, *assignRightSide);
	ctx.popBlock();

	ctx.pushBlock(falseBlock);
}
