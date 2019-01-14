#include <array>
#include <cassert>

#include "AST.hpp"
#include "BasicBlock.hpp"
#include "RValue.hpp"
#include "Triplet.hpp"

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
	std::vector <RValue> stack;

	for (const AST::Node *i : insn) {
		switch (i->type().value()) {
			case AST::Node::Type::Assignment: {
				auto assignmentNode = static_cast<const AST::Assignment *>(i);
				process(stack, *assignmentNode);
				break;
			}
			default:
				FATAL(i->location() << " : Unhandled instruction type: " << i->type() << '\n');
		}
	}

	std::cerr << "[triplets]\n";
	for (const auto &t : tripletCode)
		std::cerr << *t << '\n';
	std::cerr << "[/triplets]\n\n";
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::Assignment &assignment)
{
	assert(stack.empty());

	const auto &exprs = assignment.exprList().exprs();
	const auto &varList = assignment.varList().vars();

	if (exprs.size() != varList.size())
		FATAL(assignment.location() << " Assignments with differing number of left and right hand operands are currently unsupported\n");

	size_t cnt = 0;
	for (const auto &e : exprs) {
		process(stack, *e);
		auto result = stack.back();
		stack.pop_back();

		auto tmp = RValue::getTemporary(cnt++);
		tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, tmp, result});
		stack.push_back(tmp);
	}

	assert(stack.size() == exprs.size());

	for (const auto &v : varList) {
		process(stack, *v);
	}

	assert(stack.size() == exprs.size() * 2);

	for (size_t i = 0; i != exprs.size(); ++i)
		tripletCode.emplace_back(new Triplet{Triplet::Op::Assign, stack[exprs.size() + i], stack[i]});

	stack.clear();
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::BinOp &binOp)
{
	process(stack, binOp.left());
	auto lhs = stack.back();
	stack.pop_back();

	process(stack, binOp.right());
	auto rhs = stack.back();
	stack.pop_back();

	tripletCode.emplace_back(new Triplet{static_cast<Triplet::Op>(binOp.binOpType()), lhs, rhs});
	stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::LValue &lval)
{
	if (lval.lvalueType() == AST::LValue::Type::Name) {
		stack.push_back(&lval);
		return;
	}

	process(stack, lval.tableExpr());
	auto tableVar = stack.back();
	stack.pop_back();

	if (lval.lvalueType() == AST::LValue::Type::Dot) {
		tripletCode.emplace_back(new Triplet{Triplet::Op::TableIndex, tableVar, ValueVariant{lval.name()}});
	} else {
		process(stack, lval.keyExpr());
		tripletCode.emplace_back(new Triplet{Triplet::Op::TableIndex, tableVar, stack.back()});
		stack.pop_back();
	}

	stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::NestedExpr &nestedExpr)
{
	process(stack, nestedExpr.expr());
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::Node &node)
{
	#define SUBCASE(type) case AST::Node::Type::type: return process(stack, static_cast<const AST::type &>(node))

	switch (node.type().value()) {
		SUBCASE(BinOp);
		SUBCASE(LValue);
		SUBCASE(NestedExpr);
		SUBCASE(UnOp);
		SUBCASE(Value);
		default: FATAL(node.location() << " : Unhandled node type: " << node.type() << '\n');
	}

	#undef SUBCASE
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::UnOp &unOp)
{
	static const std::array <Triplet::Op, static_cast<unsigned>(AST::UnOp::Type::_last)> UnaryOpType {
		Triplet::Op::UnaryNegate,
		Triplet::Op::UnaryNot,
		Triplet::Op::UnaryLength,
	};

	process(stack, unOp.operand());
	auto operand = stack.back();
	stack.pop_back();

	tripletCode.emplace_back(new Triplet{UnaryOpType[static_cast<unsigned>(unOp.unOpType())], RValue{operand}, RValue::nil()});
	stack.push_back(tripletCode.back().get());
}

void BasicBlock::process(std::vector <RValue> &stack, const AST::Value &valueNode)
{
	stack.push_back(valueNode.toValueVariant());
}
