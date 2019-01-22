#include <array>
#include <cassert>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "RValue.hpp"
#include "Triplet.hpp"

struct BasicBlock::BBContext {
	std::vector <RValue> stack;
	std::vector <unsigned> requiredResults;
	unsigned tempCnt = 0;
};

BasicBlock::BasicBlock(UID id)
	: label{std::string{"BB_"} + std::to_string(id)}
{
}

BasicBlock & BasicBlock::operator = (BasicBlock &&other) = default;

BasicBlock::~BasicBlock() = default;

void BasicBlock::irDump(unsigned indent) const
{
	const std::string indentStr(indent, '\t');

	std::cout << indentStr << '[' << label << "]\n";
	for (const auto &t : tripletCode)
		std::cout << '\t' << indentStr << *t << '\n';
	std::cout << indentStr << "[/" << label << "]\n";
}

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

void BasicBlock::generateTriplets()
{
	BBContext ctx;

	for (const AST::Node *i : insn) {
		ctx.stack.clear();
		ctx.tempCnt = 0;

		ctx.requiredResults.push_back(0);
		process(ctx, *i);
		ctx.requiredResults.pop_back();
	}

	if (returnExprList) {
		ctx.tempCnt = 0;
		const auto &exprList = static_cast<const AST::ExprList &>(*returnExprList);
		process(ctx, exprList);

		for (unsigned i = 0; i != exprList.exprs().size(); ++i) {
			tripletCode.emplace_back(new Triplet{Triplet::Op::Push, ctx.stack.back()});
			ctx.stack.pop_back();
		}
		const long resultCnt = exprList.exprs().size();
		tripletCode.emplace_back(new Triplet{Triplet::Op::Return, ValueVariant{resultCnt}});
	}
}

void BasicBlock::process(BBContext &ctx, const AST::Assignment &assignment)
{
	assert(ctx.stack.empty());

	const auto &exprList = assignment.exprList();
	const auto &varList = assignment.varList().vars();

	ctx.tempCnt = 0;
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

			auto tmp = RValue::getTemporary(ctx.tempCnt++);
			tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, tmp, result});
			ctx.stack.push_back(tmp);
		}
	}

	while (ctx.stack.size() > varList.size())
		ctx.stack.pop_back();
	while (ctx.stack.size() < varList.size())
		ctx.stack.push_back(RValue::nil());

	for (const auto &v : varList) {
		ctx.requiredResults.push_back(1);
		process(ctx, *v);
		ctx.requiredResults.pop_back();
	}

	assert(ctx.stack.size() == varList.size() * 2);

	for (size_t i = 0; i != varList.size(); ++i) {
		const auto &lhs = ctx.stack[varList.size() + i];
		const auto &rhs = ctx.stack[i];

		if (auto t = std::get_if<RValue::Type::Temporary>(&lhs.valueRef); t && (*t)->operation == Triplet::Op::TableIndex) {
			auto tableRef = new Triplet{Triplet::Op::TableAssign, ValueVariant{TableReference{&(*t)->operands[0], &(*t)->operands[1]}}, rhs};
			tripletCode.emplace_back(tableRef);
		} else {
			tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, lhs, rhs});
		}
	}

	ctx.stack.clear();
}

void BasicBlock::process(BBContext &ctx, const AST::BinOp &binOp)
{
	ctx.requiredResults.push_back(1);
	process(ctx, binOp.left());
	auto lhs = ctx.stack.back();
	ctx.stack.pop_back();

	process(ctx, binOp.right());
	ctx.requiredResults.pop_back();
	auto rhs = ctx.stack.back();
	ctx.stack.pop_back();

	tripletCode.emplace_back(new Triplet{static_cast<Triplet::Op>(binOp.binOpType()), lhs, rhs});
	ctx.stack.push_back(tripletCode.back().get());
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

		auto tmp = RValue::getTemporary(ctx.tempCnt++);
		tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, tmp, result});
		ctx.stack.push_back(tmp);
	}
}

void BasicBlock::process(BBContext &ctx, const AST::Function &fnNode)
{
	assert(fnNode.isAnonymous());

	auto tmp = RValue::getTemporary(ctx.tempCnt++);
	tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, tmp, ValueVariant{&fnNode}});
	ctx.stack.push_back(tmp);
}

void BasicBlock::process(BBContext &ctx, const AST::FunctionCall &fnCallNode)
{
	const size_t stackBase = ctx.stack.size();

	process(ctx, fnCallNode.args());
	while (ctx.stack.size() != stackBase) {
		tripletCode.emplace_back(new Triplet{Triplet::Op::Push, ctx.stack.back()});
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

		tripletCode.emplace_back(new Triplet{Triplet::Op::Call, ctx.stack.back(), ValueVariant{fnCallMeta}});
		ctx.stack.pop_back();

		for (long i = 0; i != requiredResults; ++i) {
			auto tmp = RValue::getTemporary(ctx.tempCnt++);
			tripletCode.emplace_back(new Triplet{Triplet::Op::Pop, tmp});
			ctx.stack.push_back(tmp);
		}
	} else {
		tripletCode.emplace_back(new Triplet{Triplet::Op::CallUnknownResults, ctx.stack.back(), ValueVariant{requiredArgs}});
		ctx.stack.pop_back();

		auto tmp = RValue::getTemporary(ctx.tempCnt++);
		tripletCode.emplace_back(new Triplet{Triplet::Op::Pop, tmp});
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
		tripletCode.emplace_back(new Triplet{Triplet::Op::TableIndex, tableVar, ValueVariant{lval.name()}});
	} else {
		process(ctx, lval.keyExpr());
		tripletCode.emplace_back(new Triplet{Triplet::Op::TableIndex, tableVar, ctx.stack.back()});
		ctx.stack.pop_back();
	}
	ctx.requiredResults.pop_back();

	ctx.stack.push_back(tripletCode.back().get());
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
	auto table = RValue::getTemporary(ctx.tempCnt++);
	tripletCode.emplace_back(new Triplet{Triplet::Op::TableCtor, table});
	const RValue *tableRval = &tripletCode.back()->operands[0];

	long idxCnt = 0;
	for (const auto &f : tableCtor.fields()) {
		auto k = RValue::getTemporary(ctx.tempCnt++);

		switch (f->fieldType()) {
			case AST::Field::Type::Brackets:
				ctx.requiredResults.push_back(1);
				process(ctx, f->keyExpr());
				ctx.requiredResults.pop_back();

				tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, k, ctx.stack.back()});
				ctx.stack.pop_back();
				break;
			case AST::Field::Type::Literal:
				tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, k, ValueVariant{f->fieldName()}});
				break;
			case AST::Field::Type::NoIndex:
				tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, k, ValueVariant{++idxCnt}});
				break;
		}

		const RValue *keyRval = &tripletCode.back()->operands[0];

		ctx.requiredResults.push_back(1);
		process(ctx, f->valueExpr());
		ctx.requiredResults.pop_back();

		RValue v = ctx.stack.back();
		ctx.stack.pop_back();

		TableReference ref{tableRval, keyRval};
		tripletCode.emplace_back(new Triplet{Triplet::Op::TableAssign, ValueVariant{ref}, v});
	}

	ctx.stack.push_back(table);
}

void BasicBlock::process(BBContext &ctx, const AST::UnOp &unOp)
{
	static const std::array <Triplet::Op, static_cast<unsigned>(AST::UnOp::Type::_last)> UnaryOpType {
		Triplet::Op::UnaryNegate,
		Triplet::Op::UnaryNot,
		Triplet::Op::UnaryLength,
	};

	ctx.requiredResults.push_back(1);
	process(ctx, unOp.operand());
	ctx.requiredResults.pop_back();

	auto operand = ctx.stack.back();
	ctx.stack.pop_back();

	tripletCode.emplace_back(new Triplet{UnaryOpType[static_cast<unsigned>(unOp.unOpType())], RValue{operand}});
	ctx.stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(BBContext &ctx, const AST::Value &valueNode)
{
	ctx.stack.push_back(valueNode.toValueVariant());
}
