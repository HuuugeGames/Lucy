#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "EnumHelpers.hpp"
#include "Logger.hpp"
#include "ValueType.hpp"
#include "ValueVariant.hpp"

namespace AST {

class Node {
public:
	EnumClass(Type, uint32_t,
		Chunk,
		ExprList,
		NestedExpr,
		VarList,
		ParamList,
		Ellipsis,
		LValue,
		FunctionCall,
		MethodCall,
		Assignment,
		Value,
		TableCtor,
		Field,
		BinOp,
		UnOp,
		Break,
		Return,
		FunctionName,
		Function,
		If,
		While,
		Repeat,
		For,
		ForEach
	);

	virtual void print(unsigned indent = 0) const
	{
		do_indent(indent);
		std::cout << "Node\n";
	}

	virtual void printCode(std::ostream &os) const
	{
		os << "-- INVALID (" << type().value() << ')';
	}

	constexpr Node(const yy::location &location = yy::location{}) : m_location{location} {}
	virtual ~Node() = default;
	Node(Node &&) = default;

	virtual bool isValue() const { return false; }

	Node(const Node &) = delete;
	Node & operator = (const Node &) = delete;
	Node & operator = (Node &&) = default;

	virtual Type type() const = 0;
	virtual std::unique_ptr <Node> clone() const = 0;

	template <typename T>
	std::unique_ptr <T> clone() const
	{
		static_assert(std::is_base_of_v<Node, T>);
		return std::unique_ptr <T>{static_cast<T *>(clone().release())};
	}

	const yy::location & location() const
	{
		return m_location;
	}

protected:
	void do_indent(unsigned indent) const
	{
		do_indent(std::cout, indent);
	}

	void do_indent(std::ostream &os, unsigned indent) const
	{
		for (unsigned i = 0; i < indent; ++i)
			os << '\t';
	}

	void extendLocation(const yy::location &location)
	{
		if (m_location.begin.filename == nullptr)
			m_location.begin.filename = location.begin.filename;
		m_location.end = location.end;
	}

	void printLocation(unsigned indent) const
	{
		do_indent(indent);
		std::cout << m_location << '\n';
	}

private:
	yy::location m_location;
};

class Chunk : public Node {
	friend std::unique_ptr <Chunk> std::make_unique<Chunk>(const Chunk &);
public:
	static const Chunk Empty;

	Chunk(const yy::location &location = yy::location{}) : Node{location} {}

	bool isEmpty() const { return m_children.empty(); }

	void append(std::unique_ptr <Node> &&n)
	{
		extendLocation(n->location());
		m_children.emplace_back(std::move(n));
	}

	const std::vector <std::unique_ptr <Node> > & children() const { return m_children; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Chunk:\n";
		for (const auto &n : m_children)
			n->print(indent + 1);
	}

	Node::Type type() const override { return Type::Chunk; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Chunk>(*this);
	}

	using Node::clone;

private:
	Chunk(const Chunk &other)
		: Node{other.location()}
	{
		for (const auto &n : other.m_children)
			m_children.emplace_back(n->clone());
	}

	std::vector <std::unique_ptr <Node> > m_children;
};

class ParamList : public Node {
	friend class Function;
	friend std::unique_ptr <ParamList> std::make_unique<ParamList>(const ParamList &);
public:
	ParamList(const yy::location &location = yy::location{}) : Node{location}, m_ellipsis{false} {}

	void append(const std::string &name, const yy::location &location = yy::location{})
	{
		m_names.emplace_back(name, location);
		extendLocation(location);
	}

	void append(std::string &&name, const yy::location &location = yy::location{})
	{
		m_names.emplace_back(std::move(name), location);
		extendLocation(location);
	}

	bool hasEllipsis() const { return m_ellipsis; }
	void setEllipsis() { m_ellipsis = true; };

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Name list: ";
		if (!m_names.empty()) {
			std::cout << '<' << m_names[0].second << "> " << m_names[0].first;
		}

		for (size_t i = 1; i < m_names.size(); ++i) {
			std::cout << ", <" << m_names[i].second << "> " << m_names[i].first;
		}

		if (m_ellipsis)
			std::cout << "...";
		std::cout << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		if (m_names.empty()) {
			if (m_ellipsis)
				os << "( ... )";
			else
				os << "()";

			return;
		}

		os << "( " << m_names[0].first;
		for (size_t i = 1; i < m_names.size(); ++i)
			os << ", " << m_names[i].first;
		os << " )";
	}

	const std::vector <std::pair <std::string, yy::location> > & names() const { return m_names; }

	Node::Type type() const override { return Type::ParamList; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<ParamList>(*this);
	}

	using Node::clone;

private:
	ParamList(const ParamList &other)
		: Node{other.location()}, m_names{other.m_names}, m_ellipsis{other.m_ellipsis}
	{
	}

	std::vector <std::pair <std::string, yy::location> > m_names;
	bool m_ellipsis;
};

class ExprList : public Node {
	friend std::unique_ptr <ExprList> std::make_unique<ExprList>(const ExprList &);
public:
	ExprList(const yy::location &location = yy::location{}) : Node{location} {}

	void append(std::unique_ptr <Node> &&n)
	{
		extendLocation(n->location());
		m_exprs.emplace_back(std::move(n));
	}

	void prepend(std::unique_ptr <Node> &&n)
	{
		m_exprs.emplace(m_exprs.begin(), std::move(n));
	}

	bool empty() const { return m_exprs.empty(); }
	size_t size() const { return m_exprs.size(); }

	const std::vector <std::unique_ptr <Node> > & exprs() const { return m_exprs; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Expression list: [\n";
		for (const auto &n : m_exprs)
			n->print(indent + 1);
		do_indent(indent);
		std::cout << "]\n";
	}

	void printCode(std::ostream &os) const override
	{
		if (m_exprs.empty())
			return;
		m_exprs[0]->printCode(os);

		for (size_t i = 1; i < m_exprs.size(); ++i) {
			os << ", ";
			m_exprs[i]->printCode(os);
		}
	}

	Node::Type type() const override { return Type::ExprList; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<ExprList>(*this);
	}

	using Node::clone;

private:
	ExprList(const ExprList &other)
		: Node{other.location()}
	{
		for (const auto &n : other.m_exprs)
			m_exprs.emplace_back(n->clone());
	}

	std::vector <std::unique_ptr <Node> > m_exprs;
};

class NestedExpr : public Node {
	friend std::unique_ptr <NestedExpr> std::make_unique<NestedExpr>(const NestedExpr &);
public:
	NestedExpr(std::unique_ptr <Node> &&expr, const yy::location &location = yy::location{})
		: Node{location}, m_expr{std::move(expr)}
	{
	}

	const Node & expr() const { return *m_expr; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Nested expression:\n";
		m_expr->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		os << "( ";
		m_expr->printCode(os);
		os << " )";
	}

	Node::Type type() const override { return Type::NestedExpr; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<NestedExpr>(*this);
	}

private:
	NestedExpr(const NestedExpr &other)
		: Node{other.location()}, m_expr{other.m_expr->clone()}
	{
	}

	std::unique_ptr <Node> m_expr;
};

class LValue : public Node {
	friend std::unique_ptr <LValue> std::make_unique<LValue>(const LValue &);
public:
	enum class Type {
		Bracket,
		Dot,
		Name,
	};

	LValue(std::unique_ptr <Node> &&tableExpr, std::unique_ptr <Node> &&keyExpr, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Bracket}, m_tableExpr{std::move(tableExpr)}, m_keyExpr{std::move(keyExpr)}
	{
	}

	LValue(std::unique_ptr <Node> &&tableExpr, const std::string &fieldName, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Dot}, m_tableExpr{std::move(tableExpr)}, m_name{fieldName}
	{
	}

	LValue(std::unique_ptr <Node> &&tableExpr, std::string &&fieldName, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Dot}, m_tableExpr{std::move(tableExpr)}, m_name{std::move(fieldName)}
	{
	}

	LValue(const std::string &varName, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Name}, m_name{varName}
	{
	}

	LValue(std::string &&varName, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Name}, m_name{std::move(varName)}
	{
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "LValue";
		switch (m_type) {
			case Type::Bracket:
				std::cout << " bracket operator:\n";
				m_tableExpr->print(indent + 1);
				m_keyExpr->print(indent + 1);
				break;
			case Type::Dot:
				std::cout << " dot operator:\n";
				m_tableExpr->print(indent + 1);
				do_indent(indent + 1);
				std::cout << "Field name: " << m_name << '\n';
				break;
			case Type::Name:
				std::cout << '\n';
				do_indent(indent + 1);
				std::cout << m_name << '\n';
				break;
		}
	}

	void printCode(std::ostream &os) const override
	{
		switch (m_type) {
			case Type::Bracket:
				m_tableExpr->printCode(os);
				os << '[';
				m_keyExpr->printCode(os);
				os << ']';
				break;
			case Type::Dot:
				m_tableExpr->printCode(os);
				os << '.' << m_name;
				break;
			case Type::Name:
				os << m_name;
				break;
			default:
				os << "<invalid LValue type = " << static_cast<uint32_t>(m_type) << '>';
		}
	}

	const std::string & name() const { return m_name; }
	const Node & tableExpr() const { return *m_tableExpr; }
	const Node & keyExpr() const { return *m_keyExpr; }

	const std::string & resolveName() const;

	Type lvalueType() const { return m_type; }
	Node::Type type() const override { return Node::Type::LValue; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<LValue>(*this);
	}

	using Node::clone;

private:
	LValue(const LValue &other)
		: Node{other.location()}, m_type{other.m_type}
	{
		switch (m_type) {
			case Type::Bracket:
				m_tableExpr = other.m_tableExpr->clone();
				m_keyExpr = other.m_keyExpr->clone();
				break;
			case Type::Dot:
				m_tableExpr = other.m_tableExpr->clone();
				m_name = other.m_name;
				break;
			case Type::Name:
				m_name = other.m_name;
				break;
			default:
				assert(false);
		}
	}

	Type m_type;
	std::unique_ptr <Node> m_tableExpr;
	std::unique_ptr <Node> m_keyExpr;
	std::string m_name;
};

class VarList : public Node {
	friend std::unique_ptr <VarList> std::make_unique<VarList>(const VarList &);
public:
	VarList(const yy::location &location = yy::location{}) : Node{location} {}

	VarList(std::initializer_list <const char *> varNames, const yy::location &location = yy::location{}) : Node{location}
	{
		for (auto varName : varNames)
			append(std::make_unique<LValue>(varName));
	}

	void append(std::unique_ptr <LValue> &&lval)
	{
		extendLocation(lval->location());
		m_vars.emplace_back(std::move(lval));
	}

	const std::vector <std::unique_ptr <LValue> > & vars() const
	{
		return m_vars;
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Variable list: [\n";
		for (const auto &lv : m_vars)
			lv->print(indent + 1);

		do_indent(indent);
		std::cout << "]\n";
	}

	void printCode(std::ostream &os) const override
	{
		bool first = true;
		for (const auto &lvalue : m_vars) {
			if (!first)
				os << ", ";
			first = false;
			lvalue->printCode(os);
		}
	}

	Node::Type type() const override { return Type::VarList; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<VarList>(*this);
	}

	using Node::clone;

private:
	VarList(const VarList &other)
		: Node{other.location()}
	{
		for (const auto &n : other.m_vars)
			m_vars.emplace_back(n->clone<LValue>());
	}

	std::vector <std::unique_ptr <LValue> > m_vars;
};

class Ellipsis : public Node {
public:
	constexpr Ellipsis(const yy::location &location) : Node{location} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Ellipsis (...)\n";
	}

	Node::Type type() const override { return Type::Ellipsis; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Ellipsis>(location());
	}
};

class Assignment : public Node {
	friend std::unique_ptr <Assignment> std::make_unique<Assignment>(const Assignment &);
public:
	Assignment(std::unique_ptr <LValue> &&lval, std::unique_ptr <Node> &&expr, const yy::location &location = yy::location{})
		: Node{location}, m_varList{std::make_unique<VarList>()}, m_exprList{std::make_unique<ExprList>()}, m_local{false}
	{
		m_varList->append(std::move(lval));
		m_exprList->append(std::move(expr));
	}

	Assignment(const std::string &name, std::unique_ptr <Node> &&expr, const yy::location &location = yy::location{})
		: Node{location}, m_varList{std::make_unique<VarList>()}, m_exprList{std::make_unique<ExprList>()}, m_local{false}
	{
		m_varList->append(std::make_unique<LValue>(name)); //TODO name location
		m_exprList->append(std::move(expr));
	}

	Assignment(const std::string &dstVar, const std::string &srcVar, const yy::location &location = yy::location{})
		: Assignment{dstVar, std::make_unique<LValue>(srcVar), location} //TODO dstVar, srcVar location
	{
	}

	Assignment(std::unique_ptr <VarList> &&vl, std::unique_ptr <ExprList> &&el, const yy::location &location = yy::location{})
		: Node{location}, m_varList{std::move(vl)}, m_exprList{std::move(el)}, m_local{false}
	{
		if (!m_exprList)
			m_exprList = std::make_unique<ExprList>();
	}

	Assignment(std::unique_ptr <ParamList> &&pl, std::unique_ptr <ExprList> &&el, const yy::location &location = yy::location{})
		: Node{location}, m_varList{std::make_unique<VarList>()}, m_exprList{std::move(el)}, m_local{true}
	{
		for (const auto &name : pl->names())
			m_varList->append(std::make_unique<LValue>(name.first, name.second));

		if (!m_exprList)
			m_exprList = std::make_unique<ExprList>();
	}

	bool isLocal() const { return m_local; }
	void setLocal(bool local) { m_local = local; }

	const VarList & varList() const { return *m_varList; }
	const ExprList & exprList() const { return *m_exprList; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		if (m_local)
			std::cout << "local ";
		std::cout << "assignment:\n";
		m_varList->print(indent + 1);
		if (!m_exprList->exprs().empty()) {
			m_exprList->print(indent + 1);
		} else {
			do_indent(indent + 1);
			std::cout << "nil\n";
		}
	}

	void printCode(std::ostream &os) const override
	{
		if (m_local)
			os << "<b>local</b> ";
		m_varList->printCode(os);
		if (!m_exprList->empty()) {
			os << " = ";
			m_exprList->printCode(os);
		}
	}

	Node::Type type() const override { return Type::Assignment; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Assignment>(*this);
	}

private:
	Assignment(const Assignment &other)
		: Node{other.location()},
		  m_varList{other.m_varList->clone<VarList>()},
		  m_exprList{other.m_exprList->clone<ExprList>()},
		  m_local{other.m_local}
	{
	}

	std::unique_ptr <VarList> m_varList;
	std::unique_ptr <ExprList> m_exprList;
	bool m_local;
};

class Value : public Node {
public:
	constexpr Value(const yy::location &location) : Node{location} {}

	bool isValue() const override { return true; }

	Node::Type type() const override { return Type::Value; }

	virtual ValueType valueType() const = 0;
	virtual ValueVariant toValueVariant() const = 0;
};

class NilValue : public Value {
public:
	constexpr NilValue(const yy::location &location = yy::location{}) : Value{location} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "nil\n";
	}

	void printCode(std::ostream &os) const override
	{
		os << "nil";
	}

	ValueType valueType() const override { return ValueType::Nil; }
	ValueVariant toValueVariant() const override { return nullptr; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<NilValue>(location());
	}
};

class BooleanValue : public Value {
public:
	constexpr BooleanValue(bool v, const yy::location &location = yy::location{}) : Value{location}, m_value{v} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << std::boolalpha << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		if (m_value)
			os << "true";
		else
			os << "false";
	}

	ValueType valueType() const override { return ValueType::Boolean; }
	ValueVariant toValueVariant() const override { return m_value; }

	bool value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<BooleanValue>(m_value, location());
	}

private:
	bool m_value;
};

class StringValue : public Value {
public:
	StringValue(const std::string &v, const yy::location &location = yy::location{}) : Value{location}, m_value{v} {}
	StringValue(std::string &&v, const yy::location &location = yy::location{}) : Value{location}, m_value{std::move(v)} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "String: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		os << '"';
		for (char c : m_value) {
			switch (c) {
				case '<': os << "&lt;"; break;
				case '>': os << "&gt;"; break;
				case '&': os << "&amp;"; break;
				default: os << c;
			}
		}
		os << '"';
	}

	ValueType valueType() const override { return ValueType::String; }
	ValueVariant toValueVariant() const override { return m_value; }

	const std::string & value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<StringValue>(m_value, location());
	}

private:
	std::string m_value;
};

class IntValue : public Value {
public:
	constexpr IntValue(long v, const yy::location &location = yy::location{}) : Value{location}, m_value{v} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Int: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		os << m_value;
	}

	ValueType valueType() const override { return ValueType::Integer; }

	long value() const { return m_value; }
	ValueVariant toValueVariant() const override { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<IntValue>(m_value, location());
	}

private:
	long m_value;
};

class RealValue : public Value {
public:
	constexpr RealValue(double v, const yy::location &location = yy::location{}) : Value{location}, m_value{v} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Real: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		os << m_value;
	}

	ValueType valueType() const override { return ValueType::Real; }

	double value() const { return m_value; }
	ValueVariant toValueVariant() const override { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<RealValue>(m_value, location());
	}

private:
	double m_value;
};

class FunctionCall : public Node {
	friend std::unique_ptr <FunctionCall> std::make_unique<FunctionCall>(const FunctionCall &);
public:
	FunctionCall(std::unique_ptr <Node> &&funcExpr, std::unique_ptr <ExprList> &&args, const yy::location &location = yy::location{})
		: Node{location}, m_functionExpr{std::move(funcExpr)}, m_args{std::move(args)}
	{
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Function call:\n";
		m_functionExpr->print(indent + 1);
		do_indent(indent + 1);
		std::cout << "Args:\n";
		m_args->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		m_functionExpr->printCode(os);
		if (m_args->empty()) {
			os << "()";
			return;
		}

		os << "( ";
		m_args->printCode(os);
		os << " )";
	}

	const Node & functionExpr() const { return *m_functionExpr; }

	const ExprList & args() const { return *m_args; }

	Node::Type type() const override { return Type::FunctionCall; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<FunctionCall>(*this);
	}

protected:
	FunctionCall(const FunctionCall &other)
		: Node{other.location()},
		  m_functionExpr{other.m_functionExpr->clone()},
		  m_args{other.m_args->clone<ExprList>()}
	{
	}

	std::unique_ptr <Node> cloneCallExpr() const
	{
		return m_functionExpr->clone();
	}

private:
	std::unique_ptr <Node> m_functionExpr;
	std::unique_ptr <ExprList> m_args;
};

class MethodCall : public FunctionCall {
	friend std::unique_ptr <MethodCall> std::make_unique<MethodCall>(const MethodCall &);
public:
	MethodCall(std::unique_ptr <Node> &&funcExpr, std::unique_ptr <ExprList> &&args, const std::string &methodName, const yy::location &location = yy::location{})
		: FunctionCall{std::move(funcExpr), std::move(args), location}, m_methodName{methodName}
	{
	}

	MethodCall(std::unique_ptr <Node> &&funcExpr, std::unique_ptr <ExprList> &&args, std::string &&methodName, const yy::location &location = yy::location{})
		: FunctionCall{std::move(funcExpr), std::move(args), location}, m_methodName{std::move(methodName)}
	{
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Method call:\n";
		functionExpr().print(indent + 1);
		do_indent(indent + 1);
		std::cout << "Method name: " << m_methodName << '\n';
		args().print(indent + 2);
	}

	void printCode(std::ostream &os) const override
	{
		functionExpr().printCode(os);
		os << ':' << m_methodName;

		if (args().empty()) {
			os << "()";
			return;
		}

		os << "( ";
		args().printCode(os);
		os << " )";
	}

	Node::Type type() const override { return Type::MethodCall; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<MethodCall>(*this);
	}

	std::unique_ptr <FunctionCall> cloneAsFunctionCall() const
	{
		auto fnCallExpr = std::make_unique<LValue>(cloneCallExpr(), m_methodName);
		auto methodArgs = std::make_unique<ExprList>();
		methodArgs->append(cloneCallExpr());
		for (const auto &e : args().exprs())
			methodArgs->append(e->clone());
		return std::make_unique<FunctionCall>(std::move(fnCallExpr), std::move(methodArgs), location());
	}

private:
	MethodCall(const MethodCall &other)
		: FunctionCall{other},
		  m_methodName{other.m_methodName}
	{
	}

	std::string m_methodName;
};

class Field : public Node {
	friend std::unique_ptr <Field> std::make_unique<Field>(const Field &);
public:
	enum class Type {
		Brackets,
		Literal,
		NoIndex,
	};

	Field(std::unique_ptr <Node> &&expr, std::unique_ptr <Node> &&val, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Brackets}, m_keyExpr{std::move(expr)}, m_valueExpr{std::move(val)}
	{
	}

	Field(const std::string &s, std::unique_ptr <Node> &&val, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::Literal}, m_fieldName{s}, m_valueExpr{std::move(val)}
	{
	}

	Field(std::unique_ptr <Node> &&val, const yy::location &location = yy::location{})
		: Node{location}, m_type{Type::NoIndex}, m_keyExpr{nullptr}, m_valueExpr{std::move(val)}
	{
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		switch (m_type) {
			case Type::Brackets:
				std::cout << "Expr to expr:\n";
				m_keyExpr->print(indent + 1);
				break;
			case Type::Literal:
				std::cout << "Name to expr:\n";
				do_indent(indent + 1);
				std::cout << m_fieldName << '\n';
				break;
			case Type::NoIndex:
				std::cout << "Expr:\n";
				break;
		}

		m_valueExpr->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		switch (m_type) {
			case Type::Brackets:
				os << '[';
				m_keyExpr->printCode(os);
				os << "] = ";
				break;
			case Type::Literal:
				os << m_fieldName << " = ";
				break;
			case Type::NoIndex:
				break;
		}

		m_valueExpr->printCode(os);
	}

	Type fieldType() const { return m_type; }

	Node::Type type() const override { return Node::Type::Field; }

	const std::string & fieldName() const { return m_fieldName; }
	const Node & keyExpr() const { return *m_keyExpr; }
	const Node & valueExpr() const { return *m_valueExpr; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Field>(*this);
	}

	using Node::clone;

private:
	Field(const Field &other)
		: Node{other.location()},
		  m_type{other.m_type}
	{
		m_valueExpr = other.m_valueExpr->clone();

		switch (m_type) {
			case Type::Brackets:
				m_keyExpr = other.m_keyExpr->clone();
				break;
			case Type::Literal:
				m_fieldName = other.m_fieldName;
				break;
			case Type::NoIndex:
				break;
			default:
				assert(false);
		}
	}

	Type m_type;
	std::string m_fieldName;
	std::unique_ptr <Node> m_keyExpr;
	std::unique_ptr <Node> m_valueExpr;
};

class TableCtor : public Node {
	friend std::unique_ptr <TableCtor> std::make_unique<TableCtor>(const TableCtor &);
public:
	TableCtor(const yy::location &location = yy::location{}) : Node{location} {}

	void append(std::unique_ptr <Field> &&f) { m_fields.emplace_back(std::move(f)); }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "Table:\n";
		for (const auto &p : m_fields)
			p->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		if (m_fields.empty()) {
			os << "{}";
			return;
		}

		os << "{ ";
		m_fields[0]->printCode(os);
		for (size_t i = 1; i < m_fields.size(); ++i) {
			os << ", ";
			m_fields[i]->printCode(os);
		}
		os << " }";
	}

	const std::vector <std::unique_ptr <Field> > & fields() const { return m_fields; }

	Node::Type type() const override { return Node::Type::TableCtor; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<TableCtor>(*this);
	}
private:
	TableCtor(const TableCtor &other)
		: Node{other.location()}
	{
		for (const auto &f : other.m_fields)
			m_fields.emplace_back(f->clone<Field>());
	}

	std::vector <std::unique_ptr <Field> > m_fields;
};

class BinOp : public Node {
	friend std::unique_ptr <BinOp> std::make_unique<BinOp>(const BinOp &);
public:
	EnumClass(Type, unsigned,
		Or,
		And,
		Equal,
		NotEqual,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Concat,
		Plus,
		Minus,
		Times,
		Divide,
		Modulo,
		Exponentation
	);

	BinOp(Type t, std::unique_ptr <Node> &&left, std::unique_ptr <Node> &&right, const yy::location &location = yy::location{})
		: Node{location}, m_type{t}, m_left{std::move(left)}, m_right{std::move(right)}
	{
	}

	static const std::vector <ValueType> & applicableTypes(Type t)
	{
		static auto ApplicableTypes = []{
			std::array <std::vector <ValueType>, Type::_size> result;

			for (auto v : {Type::Plus, Type::Minus, Type::Times, Type::Divide})
				result[v] = {ValueType::Integer, ValueType::Real};

			result[Type::Modulo] = {ValueType::Integer};

			return result;
		}();

		return ApplicableTypes[t.value()];
	}

	static bool isApplicable(Type t, ValueType vt)
	{
		auto types = applicableTypes(t);
		return std::find(types.begin(), types.end(), vt) != types.end();
	}

	static const char * toHtmlString(Type t)
	{
		static const char *s[] = {"or", "and", "==", "~=", "&lt;", "&lt;=", "&gt;", "&gt;=", "..", "+", "-", "*", "/", "%", "^"};
		return s[t.value()];
	}

	static const char * toString(Type t)
	{
		static const char *s[] = {"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%", "^"};
		return s[t.value()];
	}

	Type binOpType() const { return m_type; }

	const Node & left() const { return *m_left; }
	const Node & right() const { return *m_right; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "BinOp: " << toString() << '\n';
		m_left->print(indent + 1);
		m_right->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		m_left->printCode(os);
		os << ' ' << toHtmlString() << ' ';
		m_right->printCode(os);
	}

	Node::Type type() const override { return Node::Type::BinOp; }

	const char * toHtmlString() const { return toHtmlString(m_type); }
	const char * toString() const { return toString(m_type); }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<BinOp>(*this);
	}

private:
	BinOp(const BinOp &other)
		: Node{other.location()},
		  m_type{other.m_type},
		  m_left{other.m_left->clone()},
		  m_right{other.m_right->clone()}
	{
	}

	Type m_type;
	std::unique_ptr <Node> m_left;
	std::unique_ptr <Node> m_right;
};

class UnOp : public Node {
	friend std::unique_ptr <UnOp> std::make_unique<UnOp>(const UnOp &);
public:
	EnumClass(Type, unsigned,
		Negate,
		Not,
		Length
	);

	UnOp(Type t, std::unique_ptr <Node> &&op, const yy::location &location = yy::location{})
		: Node{location}, m_type{t}, m_operand{std::move(op)}
	{
	}

	static const char * toString(Type t)
	{
		static const char *s[] = {"-", "not", "#"};
		return s[t.value()];
	}

	Type unOpType() const { return m_type; }

	const Node & operand() const { return *m_operand; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "UnOp: " << toString() << '\n';
		m_operand->print(indent + 1);
	}

	void printCode(std::ostream &os) const override
	{
		os << toString();
		m_operand->printCode(os);
	}

	Node::Type type() const override { return Node::Type::UnOp; }

	const char * toString() const { return toString(m_type); }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<UnOp>(*this);
	}

private:
	UnOp(const UnOp &other)
		: Node{other.location()},
		  m_type{other.m_type},
		  m_operand{other.m_operand->clone()}
	{
	}

	Type m_type;
	std::unique_ptr <Node> m_operand;
};

class Break : public Node {
public:
	Break(const yy::location &location = yy::location{}) : Node{location} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "break\n";
	}

	void printCode(std::ostream &os) const override
	{
		os << "<b>break</b>";
	}

	Node::Type type() const override { return Node::Type::Break; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Break>();
	}
};

class Return : public Node {
	friend std::unique_ptr <Return> std::make_unique<Return>(const Return &);
public:
	Return(std::unique_ptr <ExprList> &&exprList, const yy::location &location = yy::location{})
		: Node{location}, m_exprList{std::move(exprList)}
	{
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "return\n";
		if (m_exprList)
			m_exprList->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::Return; }

	bool empty() const { return m_exprList.get() == nullptr; }
	const ExprList & exprList() const { return *m_exprList; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Return>(*this);
	}

private:
	Return(const Return &other)
		: Node{other.location()},
		  m_exprList{other.m_exprList ? other.m_exprList->clone<ExprList>() : nullptr}
	{
	}

	std::unique_ptr <ExprList> m_exprList;
};

class FunctionName : public Node {
	friend std::unique_ptr <FunctionName> std::make_unique<FunctionName>(const FunctionName &);
public:
	FunctionName(std::string &&base, const yy::location &location = yy::location{})
		: Node{location}
	{
		m_name.emplace_back(std::move(base));
	}

	bool isMethod() const { return !m_method.empty(); }
	bool isNested() const { return m_name.size() > 1; }
	const std::vector <std::string> & nameParts() const { return m_name; }
	const std::string & method() const { return m_method; }

	void appendMethodName(std::string &&methodName, const yy::location &location)
	{
		m_method = std::move(methodName);
		extendLocation(location);
	}

	void appendNamePart(std::string &&namePart, const yy::location &location)
	{
		m_name.emplace_back(std::move(namePart));
		extendLocation(location);
	}

	std::string fullName() const
	{
		std::string result = m_name[0];
		for (auto iter = m_name.cbegin() + 1; iter != m_name.cend(); ++iter) {
			result.push_back('.');
			result += *iter;
		}

		if (!m_method.empty()) {
			result.push_back(':');
			result += m_method;
		}

		return result;
	}

	Node::Type type() const override { return Node::Type::FunctionName; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<FunctionName>(*this);
	}

	using Node::clone;

private:
	FunctionName(const FunctionName &other)
		: Node{other.location()},
		  m_name{other.m_name},
		  m_method{other.m_method}
	{
	}

	std::vector <std::string> m_name;
	std::string m_method;
};

class Function : public Node {
	friend std::unique_ptr <Function> std::make_unique<Function>(const Function &);
public:
	Function(std::unique_ptr <ParamList> &&params, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}, m_params{std::move(params)}, m_chunk{std::move(chunk)}, m_local{false}
	{
	}

	const Chunk & chunk() const
	{
		if (!m_chunk)
			return Chunk::Empty;
		return *m_chunk;
	}

	bool isAnonymous() const { return !m_name; }
	bool isLocal() const { return m_local; }
	bool isMethod() const { return m_name && m_name->isMethod(); }
	bool isNested() const { return m_name && m_name->isNested(); }
	bool isVariadic() const { return m_params->hasEllipsis(); }
	const FunctionName & name() const { return *m_name; }
	const ParamList & paramList() const { return *m_params; }

	void setLocal() { m_local = true; }

	std::string fullName() const
	{
		if (!m_name)
			return "<anonymous>";
		return m_name->fullName();
	}

	void clearName()
	{
		if (m_name->isMethod()) {
			m_params->m_names.insert(m_params->m_names.begin(), std::make_pair("self", yy::location{}));
		}
		m_name.reset();
	}

	void setName(std::string &&name)
	{
		m_name = std::make_unique<FunctionName>(std::move(name));
	}

	void setName(std::unique_ptr <FunctionName> &&name)
	{
		m_name = std::move(name);
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		if (m_local)
			std::cout << "local ";
		std::cout << "function ";

		if (m_name)
			std::cout << m_name->fullName() << '\n';
		else
			std::cout << "<anonymous>\n";

		do_indent(indent);
		std::cout << "params:\n";
		m_params->print(indent + 1);

		do_indent(indent);
		std::cout << "body:\n";
		if (m_chunk) {
			m_chunk->print(indent + 1);
		} else {
			do_indent(indent + 1);
			std::cout << "<empty>\n";
		}
	}

	void printCode(std::ostream &os) const override
	{
		if (m_local)
			os << "<b>local</b> ";
		os << "<b>function</b> ";

		if (m_name)
			os << m_name->fullName();
		else
			os << "<i>&lt;anonymous&gt;</i>";
		params().printCode(os);
	}

	const ParamList & params() const
	{
		return *m_params;
	}

	Node::Type type() const override { return Node::Type::Function; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Function>(*this);
	}

	using Node::clone;

private:
	Function(const Function &other)
		: Node{other.location()},
		  m_name{other.m_name ? other.m_name->clone<FunctionName>() : nullptr},
		  m_params{other.m_params ? other.m_params->clone<ParamList>() : nullptr},
		  m_chunk{other.m_chunk ? other.m_chunk->clone<Chunk>() : nullptr},
		  m_local{other.m_local}
	{
	}

	std::unique_ptr <FunctionName> m_name;
	std::unique_ptr <ParamList> m_params;
	std::unique_ptr <Chunk> m_chunk;
	bool m_local;
};

class If : public Node {
	friend std::unique_ptr <If> std::make_unique<If>(const If &);
public:
	If(std::unique_ptr <Node> &&condition, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}
	{
		m_conditions.emplace_back(std::move(condition));
		appendChunk(std::move(chunk));
	}

	bool hasElse() const { return m_else.get() != nullptr; }

	const std::vector <std::unique_ptr <Chunk> > & chunks() const { return m_chunks; }
	const std::vector <std::unique_ptr <Node> > & conditions() const { return m_conditions; }
	const Chunk & elseNode() const { return *m_else; }
	void setElse(std::unique_ptr <Chunk> &&chunk) { m_else = std::move(chunk); }

	void addElseIf(std::unique_ptr <Node> &&condition, std::unique_ptr <Chunk> &&chunk)
	{
		m_conditions.emplace_back(std::move(condition));
		appendChunk(std::move(chunk));
	}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "if:\n";
		m_conditions[0]->print(indent + 1);
		m_chunks[0]->print(indent + 1);

		for (size_t i = 1; i < m_conditions.size(); ++i) {
			do_indent(indent);
			std::cout << "elseif:\n";
			m_conditions[1]->print(indent + 1);
			m_chunks[1]->print(indent + 1);
		}

		if (m_else) {
			do_indent(indent);
			std::cout << "else:\n";
			m_else->print(indent + 1);
		}
	}

	Node::Type type() const override { return Node::Type::If; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<If>(*this);
	}

private:
	void appendChunk(std::unique_ptr <Chunk> &&chunk)
	{
		if (chunk)
			m_chunks.emplace_back(std::move(chunk));
		else
			m_chunks.emplace_back(Chunk::Empty.clone<Chunk>());
	}

	If(const If &other)
		: Node{other.location()}
	{
		for (const auto &n : other.m_conditions)
			m_conditions.emplace_back(n->clone());
		for (const auto &c : other.m_chunks)
			m_chunks.emplace_back(c->clone<Chunk>());
		if (other.m_else)
			m_else = other.m_else->clone<Chunk>();
	}

	std::vector <std::unique_ptr <Node> > m_conditions;
	std::vector <std::unique_ptr <Chunk> > m_chunks;
	std::unique_ptr <Chunk> m_else;
};

class While : public Node {
	friend std::unique_ptr <While> std::make_unique<While>(const While &);
public:
	While(std::unique_ptr <Node> &&condition, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}, m_condition{std::move(condition)}, m_chunk{std::move(chunk)}
	{
	}

	const Chunk & chunk() const { return *m_chunk; }
	const Node & condition() const { return *m_condition; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "while:\n";
		m_condition->print(indent + 1);
		m_chunk->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::While; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<While>(*this);
	}

private:
	While(const While &other)
		: Node{other.location()},
		  m_condition{other.m_condition->clone()},
		  m_chunk{other.m_chunk->clone<Chunk>()}
	{
	}

	std::unique_ptr <Node> m_condition;
	std::unique_ptr <Chunk> m_chunk;
};

class Repeat : public Node {
	friend std::unique_ptr <Repeat> std::make_unique<Repeat>(const Repeat &);
public:
	Repeat(std::unique_ptr <Node> &&condition, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}, m_condition{std::move(condition)}, m_chunk{std::move(chunk)}
	{
	}

	const Chunk & chunk() const { return *m_chunk; }
	const Node & condition() const { return *m_condition; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "repeat:\n";
		m_chunk->print(indent + 1);
		m_condition->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::Repeat; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Repeat>(*this);
	}

private:
	Repeat(const Repeat &other)
		: Node{other.location()},
		  m_condition{other.m_condition->clone()},
		  m_chunk{other.m_chunk->clone<Chunk>()}
	{
	}

	std::unique_ptr <Node> m_condition;
	std::unique_ptr <Chunk> m_chunk;
};

class For : public Node {
	friend std::unique_ptr <For> std::make_unique<For>(const For &);
public:
	For(const std::string &iterator, std::unique_ptr <Node> &&start, std::unique_ptr <Node> &&limit, std::unique_ptr <Node> &&step, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}, m_iterator{iterator}, m_start{std::move(start)}, m_limit{std::move(limit)}, m_step{std::move(step)}, m_chunk{std::move(chunk)}
	{
		if (!m_step)
			return;

		if (!m_step->isValue())
			FATAL(location << " : Nontrival step expressions in for-loop not supported yet\n");

		const Value *v = static_cast<Value *>(m_step.get());
		if (v->valueType() != ValueType::Integer && v->valueType() != ValueType::Real)
			FATAL(location << " : Step expression in for loops need to be of numeric type\n");
	}

	const Chunk & chunk() const { return *m_chunk; }

	const std::string & iterator() const { return m_iterator; }
	const Node & startExpr() const { return *m_start; }
	const Node & limitExpr() const { return *m_limit; }

	bool hasStepExpression() const { return m_step.get() != nullptr; }
	const Node & stepExpr() const { return *m_step; }

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "for:\n";

		do_indent(indent);
		std::cout << "iterator: " << m_iterator << '\n';

		do_indent(indent);
		std::cout << "start:\n";
		m_start->print(indent + 1);

		do_indent(indent);
		std::cout << "limit:\n";
		m_limit->print(indent + 1);

		do_indent(indent);
		std::cout << "step:\n";

		if (m_step) {
			m_step->print(indent + 1);
		} else {
			do_indent(indent + 1);
			std::cout << "1 (default)\n";
		}

		do_indent(indent);
		std::cout << "do:\n";
		m_chunk->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::For; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<For>(*this);
	}

	std::unique_ptr <BinOp> cloneCondition() const
	{
		if (!m_step)
			return std::make_unique<BinOp>(BinOp::Type::LessEqual, std::make_unique<LValue>(iterator()), limitExpr().clone());

		auto binopType = [](auto &&v) {
			if (v < 0)
				return BinOp::Type::GreaterEqual;
			return BinOp::Type::LessEqual;
		};

		const Value *v = static_cast<const Value *>(m_step.get());
		BinOp::Type op = BinOp::Type::_size;

		if (v->valueType() == ValueType::Integer) {
			op = binopType(static_cast<const IntValue *>(v)->value());
		} else if (v->valueType() == ValueType::Real) {
			op = binopType(static_cast<const RealValue *>(v)->value());
		}

		assert(op != BinOp::Type::_size);
		return std::make_unique<BinOp>(op, std::make_unique<LValue>(iterator()), limitExpr().clone());
	}

	std::unique_ptr <Assignment> cloneStepExpr() const
	{
		if (!m_step)
			return std::make_unique<Assignment>(m_iterator, std::make_unique<BinOp>(BinOp::Type::Plus, std::make_unique<LValue>(iterator()), std::make_unique<IntValue>(1)));

		return std::make_unique<Assignment>(m_iterator, std::make_unique<BinOp>(BinOp::Type::Plus, std::make_unique<LValue>(iterator()), stepExpr().clone()));
	}

private:
	For(const For &other)
		: Node{other.location()},
		  m_iterator{other.m_iterator},
		  m_start{other.m_start->clone()},
		  m_limit{other.m_limit->clone()},
		  m_step{other.m_step ? other.m_step->clone() : nullptr},
		  m_chunk{other.m_chunk->clone<Chunk>()}
	{
	}

	std::string m_iterator;
	std::unique_ptr <Node> m_start, m_limit, m_step;
	std::unique_ptr <Chunk> m_chunk;
};

class ForEach : public Node {
	friend std::unique_ptr <ForEach> std::make_unique<ForEach>(const ForEach &);
public:
	ForEach(std::unique_ptr <ParamList> &&variables, std::unique_ptr <ExprList> &&exprs, std::unique_ptr <Chunk> &&chunk, const yy::location &location = yy::location{})
		: Node{location}, m_variables{std::move(variables)}, m_exprs{std::move(exprs)}, m_chunk{std::move(chunk)} {}

	void print(unsigned indent = 0) const override
	{
		printLocation(indent);
		do_indent(indent);
		std::cout << "for_each:\n";
		m_variables->print(indent + 1);

		do_indent(indent);
		std::cout << "in:\n";
		m_exprs->print(indent + 1);

		do_indent(indent);
		std::cout << "do:\n";
		m_chunk->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::ForEach; }

	const ParamList & variables() const { return *m_variables; }
	const ExprList & exprList() const { return *m_exprs; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<ForEach>(*this);
	}

	std::unique_ptr <ParamList> cloneVariables() const
	{
		return m_variables->clone<ParamList>();
	}

	std::unique_ptr <ExprList> cloneExprList() const
	{
		return m_exprs->clone<ExprList>();
	}

	std::unique_ptr <Chunk> cloneChunk() const
	{
		return m_chunk->clone<Chunk>();
	}

private:
	ForEach(const ForEach &other)
		: Node{other.location()},
		  m_variables{other.m_variables->clone<ParamList>()},
		  m_exprs{other.m_exprs->clone<ExprList>()},
		  m_chunk{other.m_chunk->clone<Chunk>()}
	{
	}

	std::unique_ptr <ParamList> m_variables;
	std::unique_ptr <ExprList> m_exprs;
	std::unique_ptr <Chunk> m_chunk;
};

} //namespace AST
