#include <array>
#include <cassert>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "Fold.hpp"
#include "RValue.hpp"
#include "Triplet.hpp"

struct BasicBlock::BBContext {
	std::vector <RValue> stack;
	unsigned tempCnt = 0;
};

BasicBlock::BasicBlock(UID id)
	: label{std::string{"BB_"} + std::to_string(id)}
{
}

BasicBlock & BasicBlock::operator = (BasicBlock &&other) = default;

BasicBlock::~BasicBlock() = default;

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

		process(ctx, *i);
	}

	std::cerr << "[triplets]\n";
	for (const auto &t : tripletCode)
		std::cerr << *t << '\n';
	std::cerr << "[/triplets]\n\n";
}

void BasicBlock::process(BBContext &ctx, const AST::Assignment &assignment)
{
	assert(ctx.stack.empty());

	const auto &exprList = assignment.exprList();
	const auto &varList = assignment.varList().vars();

	ctx.tempCnt = 0;
	for (size_t exprIdx = 0; exprIdx != exprList.size(); ++exprIdx) {
		const auto &e = exprList.exprs()[exprIdx];

		process(ctx, *e);

		if (any_of(e->type(), AST::Node::Type::FunctionCall, AST::Node::Type::MethodCall)) {
			auto &fnCall = tripletCode.back();
			long &fnMeta = std::get<long>(std::get<ValueVariant>(fnCall->operands[1].valueRef));

			fnMeta = (fnMeta & ~0xffff);
			if (exprIdx < exprList.size() - 1 && exprIdx < varList.size())
				fnMeta |= 1;
			else if (exprIdx == exprList.size() - 1 && exprIdx < varList.size())
				fnMeta |= varList.size() - exprList.size() + 1;

			const long resultsNeeded = fnMeta & 0xffff;
			for (long i = 0; i != resultsNeeded; ++i) {
				auto tmp = RValue::getTemporary(ctx.tempCnt++);
				tripletCode.emplace_back(new Triplet{Triplet::Op::Pop, tmp});
				ctx.stack.push_back(tmp);
			}
		} else {
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

	for (const auto &v : varList)
		process(ctx, *v);

	assert(ctx.stack.size() == varList.size() * 2);

	for (size_t i = 0; i != varList.size(); ++i)
		tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, ctx.stack[varList.size() + i], ctx.stack[i]});

	ctx.stack.clear();
}

void BasicBlock::process(BBContext &ctx, const AST::BinOp &binOp)
{
	process(ctx, binOp.left());
	auto lhs = ctx.stack.back();
	ctx.stack.pop_back();

	process(ctx, binOp.right());
	auto rhs = ctx.stack.back();
	ctx.stack.pop_back();

	tripletCode.emplace_back(new Triplet{static_cast<Triplet::Op>(binOp.binOpType()), lhs, rhs});
	ctx.stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(BBContext &ctx, const AST::ExprList &exprList)
{
	for (const auto &e : exprList.exprs()) {
		process(ctx, *e);
		auto result = ctx.stack.back();
		ctx.stack.pop_back();

		auto tmp = RValue::getTemporary(ctx.tempCnt++);
		tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, tmp, result});
		ctx.stack.push_back(tmp);
	}
}

void BasicBlock::process(BBContext &ctx, const AST::FunctionCall &fnCallNode)
{
	const size_t stackBase = ctx.stack.size();

	process(ctx, fnCallNode.args());
	while (ctx.stack.size() != stackBase) {
		tripletCode.emplace_back(new Triplet{Triplet::Op::Push, ctx.stack.back()});
		ctx.stack.pop_back();
	}

	process(ctx, fnCallNode.functionExpr());
	tripletCode.emplace_back(new Triplet{Triplet::Op::Call, ctx.stack.back(), ValueVariant{static_cast<long>(fnCallNode.args().size() << 16)}});
	ctx.stack.pop_back();
}

void BasicBlock::process(BBContext &ctx, const AST::LValue &lval)
{
	if (lval.lvalueType() == AST::LValue::Type::Name) {
		ctx.stack.push_back(&lval);
		return;
	}

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

	ctx.stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(BBContext &ctx, const AST::MethodCall &methodCallNode)
{
	const size_t stackBase = ctx.stack.size();
	//TODO
}

void BasicBlock::process(BBContext &ctx, const AST::NestedExpr &nestedExpr)
{
	process(ctx, nestedExpr.expr());
}

void BasicBlock::process(BBContext &ctx, const AST::Node &node)
{
	#define SUBCASE(type) case AST::Node::Type::type: return process(ctx, static_cast<const AST::type &>(node))

	switch (node.type().value()) {
		SUBCASE(Assignment);
		SUBCASE(BinOp);
		SUBCASE(ExprList);
		SUBCASE(FunctionCall);
		SUBCASE(LValue);
		SUBCASE(NestedExpr);
		SUBCASE(UnOp);
		SUBCASE(Value);
		default: FATAL(node.location() << " : Unhandled node type: " << node.type() << '\n');
	}

	#undef SUBCASE
}

void BasicBlock::process(BBContext &ctx, const AST::UnOp &unOp)
{
	static const std::array <Triplet::Op, static_cast<unsigned>(AST::UnOp::Type::_last)> UnaryOpType {
		Triplet::Op::UnaryNegate,
		Triplet::Op::UnaryNot,
		Triplet::Op::UnaryLength,
	};

	process(ctx, unOp.operand());
	auto operand = ctx.stack.back();
	ctx.stack.pop_back();

	tripletCode.emplace_back(new Triplet{UnaryOpType[static_cast<unsigned>(unOp.unOpType())], RValue{operand}});
	ctx.stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(BBContext &ctx, const AST::Value &valueNode)
{
	ctx.stack.push_back(valueNode.toValueVariant());
}
