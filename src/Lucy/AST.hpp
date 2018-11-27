#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <variant>
#include <vector>

#include "EnumHelpers.hpp"
#include "ValueType.hpp"

class Node {
public:
	enum class Type {
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
		Function,
		If,
		While,
		Repeat,
		For,
		ForEach,
		_last,
	};

	virtual void print(int indent = 0) const
	{
		do_indent(indent);
		std::cout << "Node\n";
	}

	virtual void printCode(std::ostream &os) const
	{
		os << "-- INVALID (" << static_cast<uint32_t>(type()) << ')';
	}

	Node() = default;
	virtual ~Node() = default;
	Node(Node &&) = default;

	virtual bool isValue() const { return false; }

	Node(const Node &) = delete;
	Node & operator = (const Node &) = delete;
	Node & operator = (Node &&) = default;

	virtual Type type() const = 0;
	virtual std::unique_ptr <Node> clone() const = 0;

protected:
	void do_indent(int indent) const
	{
		do_indent(std::cout, indent);
	}

	void do_indent(std::ostream &os, int indent) const
	{
		for (int i = 0; i < indent; ++i)
			os << '\t';
	}
};

class Chunk : public Node {
public:
	static const Chunk Empty;

	Chunk() = default;

	Chunk(std::initializer_list <Node *> statements)
	{
		for (auto s : statements)
			append(s);
	}

	void append(Node *n) { m_children.emplace_back(n); }

	const std::vector <std::unique_ptr <Node> > & children() const { return m_children; }

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Chunk:\n";
		for (const auto &n : m_children)
			n->print(indent + 1);
	}

	Node::Type type() const override { return Type::Chunk; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <Chunk>{new Chunk{*this}};
	}

private:
	Chunk(const Chunk &other)
	{
		for (const auto &n : other.m_children)
			m_children.emplace_back(n->clone());
	}

	std::vector <std::unique_ptr <Node> > m_children;
};

class ParamList : public Node {
public:
	static const ParamList Empty;

	ParamList() : m_ellipsis{false} {}

	void append(const std::string &name) { m_names.push_back(name); }
	void append(std::string &&name) { m_names.push_back(std::move(name)); }

	bool hasEllipsis() const { return m_ellipsis; }
	void setEllipsis() { m_ellipsis = true; };

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Name list: ";
		if (!m_names.empty())
			std::cout << m_names.front();
		if (m_names.size() > 1) {
			for (auto name = m_names.cbegin() + 1; name != m_names.cend(); ++name)
				std::cout << ", " << *name;
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

		os << "( " << m_names[0];
		for (size_t i = 1; i < m_names.size(); ++i)
			os << ", " << m_names[i];
		os << " )";
	}

	const std::vector <std::string> & names() const { return m_names; }

	Node::Type type() const override { return Type::ParamList; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <ParamList>{new ParamList{*this}};
	}

private:
	ParamList(const ParamList &other)
		: m_names{other.m_names}, m_ellipsis{other.m_ellipsis}
	{
	}

	std::vector <std::string> m_names;
	bool m_ellipsis;
};

class ExprList : public Node {
public:
	ExprList() = default;

	ExprList(std::initializer_list <Node *> exprs)
	{
		for (auto expr : exprs)
			append(expr);
	}

	void append(Node *n) { m_exprs.emplace_back(n); }

	bool empty() const { return m_exprs.empty(); }

	const std::vector <std::unique_ptr <Node> > & exprs() const { return m_exprs; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <ExprList>{new ExprList{*this}};
	}

private:
	ExprList(const ExprList &other)
	{
		for (const auto &n : other.m_exprs)
			m_exprs.emplace_back(n->clone());
	}

	std::vector <std::unique_ptr <Node> > m_exprs;
};

class NestedExpr : public Node {
public:
	NestedExpr(Node *expr) : m_expr{expr} {}

	const Node & expr() const { return *m_expr; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <Node>{new NestedExpr{*this}};
	}

private:
	NestedExpr(const NestedExpr &other)
		: m_expr{other.m_expr->clone().release()}
	{
	}

	std::unique_ptr <Node> m_expr;
};

class LValue : public Node {
public:
	enum class Type {
		Bracket,
		Dot,
		Name,
	};

	LValue(Node *tableExpr, Node *keyExpr) : m_type{Type::Bracket}, m_tableExpr{tableExpr}, m_keyExpr{keyExpr} {}
	LValue(Node *tableExpr, const std::string &fieldName) : m_type{Type::Dot}, m_tableExpr{tableExpr}, m_name{fieldName} {}
	LValue(Node *tableExpr, std::string &&fieldName) : m_type{Type::Dot}, m_tableExpr{tableExpr}, m_name{std::move(fieldName)} {}
	LValue(const std::string &varName) : m_type{Type::Name}, m_name{varName} {}
	LValue(std::string &&varName) : m_type{Type::Name}, m_name{std::move(varName)} {}

	void print(int indent = 0) const override
	{
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
	const Node * tableExpr() const { return m_tableExpr.get(); }
	const Node * keyExpr() const { return m_keyExpr.get(); }

	Type lvalueType() const { return m_type; }
	Node::Type type() const override { return Node::Type::LValue; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <LValue>{new LValue{*this}};
	}

private:
	LValue(const LValue &other) : m_type{other.m_type}
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
public:
	VarList() = default;

	VarList(std::initializer_list <const char *> varNames)
	{
		for (auto varName : varNames)
			append(new LValue{varName});
	}

	void append(LValue *lval)
	{
		m_vars.emplace_back(lval);
	}

	const std::vector <std::unique_ptr <LValue> > & vars() const
	{
		return m_vars;
	}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <VarList>{new VarList{*this}};
	}

private:
	VarList(const VarList &other)
	{
		for (const auto &n : other.m_vars)
			m_vars.emplace_back(static_cast<LValue *>(n->clone().release()));
	}

	std::vector <std::unique_ptr <LValue> > m_vars;
};

class Ellipsis : public Node {
public:
	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Ellipsis (...)\n";
	}

	Node::Type type() const override { return Type::Ellipsis; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<Ellipsis>();
	}
};

class Assignment : public Node {
public:
	Assignment(const std::string &name, Node *expr)
		: m_varList{new VarList{}}, m_exprList{new ExprList{}}, m_local{false}
	{
		m_varList->append(new LValue{name});
		m_exprList->append(expr);
	}

	Assignment(const std::string &dstVar, const std::string &srcVar)
		: Assignment{dstVar, new LValue{srcVar}}
	{
	}

	Assignment(const std::string &name, std::unique_ptr <Node> &&expr)
		: Assignment{name, expr.release()}
	{
	}

	Assignment(VarList *vl, ExprList *el)
		: m_varList{vl}, m_exprList{el}, m_local{false}
	{
		if (!m_exprList)
			m_exprList.reset(new ExprList{});
	}

	Assignment(ParamList *pl, ExprList *el)
		: m_varList{new VarList{}}, m_exprList{el}, m_local{true}
	{
		for (const auto &name : pl->names())
			m_varList->append(new LValue{name});
		delete pl;

		if (!m_exprList)
			m_exprList.reset(new ExprList{});
	}

	void setLocal(bool local) { m_local = local; }

	const VarList & varList() const { return *m_varList; }
	const ExprList & exprList() const { return *m_exprList; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <Assignment>{new Assignment{*this}};
	}

private:
	Assignment(const Assignment &other)
		: m_varList{static_cast<VarList *>(other.m_varList->clone().release())},
		  m_exprList{static_cast<ExprList *>(other.m_exprList->clone().release())},
		  m_local{other.m_local}
	{
	}

	std::unique_ptr <VarList> m_varList;
	std::unique_ptr <ExprList> m_exprList;
	bool m_local;
};

class Value : public Node {
public:
	bool isValue() const override { return true; }

	Node::Type type() const override { return Type::Value; }

	virtual ValueType valueType() const = 0;
};

class NilValue : public Value {
public:
	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "nil\n";
	}

	void printCode(std::ostream &os) const override
	{
		os << "nil";
	}

	ValueType valueType() const override { return ValueType::Nil; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<NilValue>();
	}
};

class BooleanValue : public Value {
public:
	BooleanValue(bool v) : m_value{v} {}

	void print(int indent = 0) const override
	{
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

	bool value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<BooleanValue>(m_value);
	}

private:
	bool m_value;
};

class StringValue : public Value {
public:
	StringValue(const std::string &v) : m_value{v} {}
	StringValue(std::string &&v) : m_value{std::move(v)} {}

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "String: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		for (char c : m_value) {
			switch (c) {
				case '<': os << "&lt;"; break;
				case '>': os << "&gt;"; break;
				case '&': os << "&amp;"; break;
				default: os << c;
			}
		}
	}

	ValueType valueType() const override { return ValueType::String; }

	const std::string & value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<StringValue>(m_value);
	}

private:
	std::string m_value;
};

class IntValue : public Value {
public:
	constexpr IntValue(long v) : m_value{v} {}

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Int: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		os << m_value;
	}

	ValueType valueType() const override { return ValueType::Integer; }

	long value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<IntValue>(m_value);
	}

private:
	long m_value;
};

class RealValue : public Value {
public:
	constexpr RealValue(double v) : m_value{v} {}

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Real: " << m_value << '\n';
	}

	void printCode(std::ostream &os) const override
	{
		os << m_value;
	}

	ValueType valueType() const override { return ValueType::Real; }

	double value() const { return m_value; }

	std::unique_ptr <Node> clone() const override
	{
		return std::make_unique<RealValue>(m_value);
	}

private:
	double m_value;
};

class FunctionCall : public Node {
public:
	FunctionCall(Node *funcExpr, ExprList *args) : m_functionExpr{funcExpr}, m_args{args} {}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <FunctionCall>(new FunctionCall{*this});
	}

protected:
	FunctionCall(const FunctionCall &other)
		: m_functionExpr{other.m_functionExpr->clone()},
		  m_args{static_cast<ExprList *>(other.m_args->clone().release())}
	{
	}

private:
	std::unique_ptr <Node> m_functionExpr;
	std::unique_ptr <ExprList> m_args;
};

class MethodCall : public FunctionCall {
public:
	MethodCall(Node *funcExpr, ExprList *args, const std::string &methodName) : FunctionCall{funcExpr, args}, m_methodName{methodName} {}
	MethodCall(Node *funcExpr, ExprList *args, std::string &&methodName) : FunctionCall{funcExpr, args}, m_methodName{std::move(methodName)} {}

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "Method call:\n";
		functionExpr().print(indent + 1);
		do_indent(indent);
		std::cout << "Method name: " << m_methodName << '\n';
		args().print(indent + 1);
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
		return std::unique_ptr <MethodCall>{new MethodCall{*this}};
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
public:
	enum class Type {
		Brackets,
		Literal,
		NoIndex,
	};

	Field(Node *expr, Node *val) : m_type{Type::Brackets}, m_keyExpr{expr}, m_valueExpr{val} {}
	Field(const std::string &s, Node *val) : m_type{Type::Literal}, m_fieldName{s}, m_valueExpr{val} {}
	Field(Node *val) : m_type{Type::NoIndex}, m_keyExpr{nullptr}, m_valueExpr{val} {}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <Field>{new Field{*this}};
	}

private:
	Field(const Field &other)
		: m_type{other.m_type}
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
public:
	TableCtor() = default;

	void append(Field *f) { m_fields.emplace_back(f); }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <TableCtor>{new TableCtor{*this}};
	}
private:
	TableCtor(const TableCtor &other)
	{
		for (const auto &f : other.m_fields)
			m_fields.emplace_back(static_cast<Field *>(f->clone().release()));
	}

	std::vector <std::unique_ptr <Field> > m_fields;
};

class BinOp : public Node {
public:
	enum class Type {
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
		Exponentation,
		_last
	};

	BinOp(Type t, Node *left, Node *right) : m_type{t}, m_left{left}, m_right{right} {}

	static const std::vector <ValueType> & applicableTypes(Type t)
	{
		static auto ApplicableTypes = []{
			std::array <std::vector <ValueType>, toUnderlying(Type::_last)> result;

			for (auto v : {Type::Plus, Type::Minus, Type::Times, Type::Divide})
				result[toUnderlying(v)] = {ValueType::Integer, ValueType::Real};

			result[toUnderlying(Type::Modulo)] = {ValueType::Integer};

			return result;
		}();

		return ApplicableTypes[toUnderlying(t)];
	}

	static bool isApplicable(Type t, ValueType vt)
	{
		auto types = applicableTypes(t);
		return std::find(types.begin(), types.end(), vt) != types.end();
	}

	static const char * toHtmlString(Type t)
	{
		static const char *s[] = {"or", "and", "==", "~=", "&lt;", "&lt;=", "&gt;", "&gt;=", "..", "+", "-", "*", "/", "%", "^"};
		return s[toUnderlying(t)];
	}

	static const char * toString(Type t)
	{
		static const char *s[] = {"or", "and", "==", "~=", "<", "<=", ">", ">=", "..", "+", "-", "*", "/", "%", "^"};
		return s[toUnderlying(t)];
	}

	Type binOpType() const { return m_type; }

	const Node & left() const { return *m_left; }
	const Node & right() const { return *m_right; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <BinOp>{new BinOp{*this}};
	}

private:
	BinOp(const BinOp &other)
		: m_type{other.m_type},
		  m_left{other.m_left->clone()},
		  m_right{other.m_right->clone()}
	{
	}

	Type m_type;
	std::unique_ptr <Node> m_left;
	std::unique_ptr <Node> m_right;
};

class UnOp : public Node {
public:
	enum class Type {
		Negate,
		Not,
		Length,
	};

	UnOp(Type t, Node *op) : m_type{t}, m_operand{op} {}

	static const char * toString(Type t)
	{
		static const char *s[] = {"-", "not", "#"};
		return s[toUnderlying(t)];
	}

	Type unOpType() const { return m_type; }

	const Node & operand() const { return *m_operand; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <UnOp>{new UnOp{*this}};
	}

private:
	UnOp(const UnOp &other)
		: m_type{other.m_type},
		  m_operand{other.m_operand->clone()}
	{
	}

	Type m_type;
	std::unique_ptr <Node> m_operand;
};

class Break : public Node {
public:
	void print(int indent = 0) const override
	{
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
public:
	Return(ExprList *exprList) : m_exprList{exprList} {}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <Return>{new Return{*this}};
	}

private:
	Return(const Return &other)
		: m_exprList{other.m_exprList ? static_cast<ExprList *>(other.m_exprList->clone().release()) : nullptr}
	{
	}

	std::unique_ptr <ExprList> m_exprList;
};

using FunctionName = std::pair <std::vector <std::string>, std::string>;

class Function : public Node {
public:
	Function(ParamList *params, Chunk *chunk) : m_params{params}, m_chunk{chunk}, m_local{false} {}

	const Chunk & chunk() const
	{
		if (!m_chunk)
			return Chunk::Empty;
		return *m_chunk;
	}

	bool isLocal() const { return m_local; }
	void setLocal() { m_local = true; }

	std::string fullName() const
	{
		if (m_name.empty())
			return "<anonymous>";

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

	void setName(const FunctionName &name)
	{
		m_name = name.first;
		m_method = name.second;
	}

	void setName(const std::string &name)
	{
		m_name.clear();
		m_name.push_back(name);
	}

	void print(int indent = 0) const override
	{
		do_indent(indent);
		if (m_local)
			std::cout << "local ";
		std::cout << "function ";

		if (!m_name.empty()) {
			std::cout << m_name[0];
			for (auto iter = m_name.cbegin() + 1; iter != m_name.cend(); ++iter)
				std::cout << "." << *iter;

			if (!m_method.empty())
				std::cout << ":" << m_method;
			std::cout << '\n';
		} else {
			std::cout << "<anonymous>\n";
		}

		do_indent(indent);
		std::cout << "params:\n";
		if (m_params) {
			m_params->print(indent + 1);
		} else {
			do_indent(indent + 1);
			std::cout << "<no params>\n";
		}

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

		if (m_name.empty())
			os << "<i>&lt;anonymous&gt;</i>";
		else
			os << fullName();
		params().printCode(os);
	}

	const ParamList & params() const
	{
		if (!m_params)
			return ParamList::Empty;
		return *m_params;
	}

	Node::Type type() const override { return Node::Type::Function; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <Function>{new Function{*this}};
	}

private:
	Function(const Function &other)
		: m_name{other.m_name},
		  m_method{other.m_method},
		  m_params{other.m_params ? static_cast<ParamList *>(other.m_params->clone().release()) : nullptr},
		  m_chunk{other.m_chunk ? static_cast<Chunk *>(other.m_chunk->clone().release()) : nullptr},
		  m_local{other.m_local}
	{
	}

	std::vector <std::string> m_name;
	std::string m_method;
	std::unique_ptr <ParamList> m_params;
	std::unique_ptr <Chunk> m_chunk;
	bool m_local;
};

class If : public Node {
public:
	If(Node *condition, Chunk *chunk)
	{
		m_conditions.emplace_back(condition);
		appendChunk(chunk);
	}

	bool hasElse() const { return m_else.get() != nullptr; }

	const std::vector <std::unique_ptr <Chunk> > & chunks() const { return m_chunks; }
	const std::vector <std::unique_ptr <Node> > & conditions() const { return m_conditions; }
	const Chunk & elseNode() const { return *m_else; }
	void setElse(Chunk *chunk) { m_else.reset(chunk); }

	void addElseIf(Node *condition, Chunk *chunk)
	{
		m_conditions.emplace_back(condition);
		appendChunk(chunk);
	}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <If>{new If{*this}};
	}

private:
	void appendChunk(Chunk *chunk)
	{
		if (chunk)
			m_chunks.emplace_back(chunk);
		else
			m_chunks.emplace_back(static_cast<Chunk *>(Chunk::Empty.clone().release()));
	}

	If(const If &other)
	{
		for (const auto &n : other.m_conditions)
			m_conditions.emplace_back(n->clone());
		for (const auto &c : other.m_chunks)
			m_chunks.emplace_back(static_cast<Chunk *>(c->clone().release()));
		if (other.m_else)
			m_else = std::unique_ptr <Chunk>(static_cast<Chunk *>(other.m_else->clone().release()));
	}

	std::vector <std::unique_ptr <Node> > m_conditions;
	std::vector <std::unique_ptr <Chunk> > m_chunks;
	std::unique_ptr <Chunk> m_else;
};

class While : public Node {
public:
	While(Node *condition, Chunk *chunk) : m_condition{condition}, m_chunk{chunk} {}

	const Chunk & chunk() const { return *m_chunk; }
	const Node & condition() const { return *m_condition; }

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "while:\n";
		m_condition->print(indent + 1);
		m_chunk->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::While; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <While>{new While{*this}};
	}

private:
	While(const While &other)
		: m_condition{other.m_condition->clone()},
		  m_chunk{static_cast<Chunk *>(other.m_chunk->clone().release())}
	{
	}

	std::unique_ptr <Node> m_condition;
	std::unique_ptr <Chunk> m_chunk;
};

class Repeat : public Node {
public:
	Repeat(Node *condition, Chunk *chunk) : m_condition{condition}, m_chunk{chunk} {}

	const Chunk & chunk() const { return *m_chunk; }
	const Node & condition() const { return *m_condition; }

	void print(int indent = 0) const override
	{
		do_indent(indent);
		std::cout << "repeat:\n";
		m_chunk->print(indent + 1);
		m_condition->print(indent + 1);
	}

	Node::Type type() const override { return Node::Type::Repeat; }

	std::unique_ptr <Node> clone() const override
	{
		return std::unique_ptr <Repeat>{new Repeat{*this}};
	}

private:
	Repeat(const Repeat &other)
		: m_condition{other.m_condition->clone()},
		  m_chunk{static_cast<Chunk *>(other.m_chunk->clone().release())}
	{
	}

	std::unique_ptr <Node> m_condition;
	std::unique_ptr <Chunk> m_chunk;
};

class For : public Node {
public:
	For(const std::string &iterator, Node *start, Node *limit, Node *step, Chunk *chunk)
		: m_iterator{iterator}, m_start{start}, m_limit{limit}, m_step{step}, m_chunk{chunk}
	{
		if (!m_step)
			return;

		if (!m_step->isValue()) {
			std::cerr << "Nontrival step expressions in for-loop not supported yet\n";
			m_step->print(); std::cout << std::flush;
			abort();
		}

		const Value *v = static_cast<Value *>(m_step.get());
		if (v->valueType() != ValueType::Integer && v->valueType() != ValueType::Real) {
			std::cerr << "Step expression in for loops need to be of numeric type\n";
			abort();
		}
	}

	const Chunk & chunk() const { return *m_chunk; }

	const std::string & iterator() const { return m_iterator; }
	const Node & startExpr() const { return *m_start; }
	const Node & limitExpr() const { return *m_limit; }

	bool hasStepExpression() const { return m_step.get() != nullptr; }
	const Node & stepExpr() const { return *m_step; }

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <For>{new For{*this}};
	}

	std::unique_ptr <BinOp> cloneCondition() const
	{
		if (!m_step)
			return std::unique_ptr<BinOp>{new BinOp{BinOp::Type::LessEqual, new LValue{iterator()}, limitExpr().clone().release()}};

		auto binopType = [](auto &&v) {
			if (v < 0)
				return BinOp::Type::GreaterEqual;
			return BinOp::Type::LessEqual;
		};

		const Value *v = static_cast<const Value *>(m_step.get());
		BinOp::Type op = BinOp::Type::_last;

		if (v->valueType() == ValueType::Integer) {
			op = binopType(static_cast<const IntValue *>(v)->value());
		} else if (v->valueType() == ValueType::Real) {
			op = binopType(static_cast<const RealValue *>(v)->value());
		}

		assert(op != BinOp::Type::_last);
		return std::unique_ptr<BinOp>{new BinOp{op, new LValue{iterator()}, limitExpr().clone().release()}};
	}

	std::unique_ptr <Assignment> cloneStepExpr() const
	{
		if (!m_step)
			return std::make_unique<Assignment>(m_iterator, new BinOp{BinOp::Type::Plus, new LValue{iterator()}, new IntValue{1}});

		return std::make_unique<Assignment>(m_iterator, new BinOp{BinOp::Type::Plus, new LValue{iterator()}, stepExpr().clone().release()});
	}

private:
	For(const For &other)
		: m_iterator{other.m_iterator},
		  m_start{other.m_start->clone()},
		  m_limit{other.m_limit->clone()},
		  m_step{other.m_step ? other.m_step->clone() : nullptr},
		  m_chunk{static_cast<Chunk *>(other.m_chunk->clone().release())}
	{
	}

	std::string m_iterator;
	std::unique_ptr <Node> m_start, m_limit, m_step;
	std::unique_ptr <Chunk> m_chunk;
};

class ForEach : public Node {
public:
	ForEach(ParamList *variables, ExprList *exprs, Chunk *chunk)
		: m_variables{variables}, m_exprs{exprs}, m_chunk{chunk} {}

	void print(int indent = 0) const override
	{
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
		return std::unique_ptr <ForEach>{new ForEach{*this}};
	}

	std::unique_ptr <ParamList> cloneVariables() const
	{
		return std::unique_ptr <ParamList>{static_cast<ParamList *>(m_variables->clone().release())};
	}

	std::unique_ptr <ExprList> cloneExprList() const
	{
		return std::unique_ptr <ExprList>{static_cast<ExprList *>(m_exprs->clone().release())};
	}

	std::unique_ptr <Chunk> cloneChunk() const
	{
		return std::unique_ptr <Chunk>{static_cast<Chunk *>(m_chunk->clone().release())};
	}

private:
	ForEach(const ForEach &other)
		: m_variables{static_cast<ParamList *>(other.m_variables->clone().release())},
		  m_exprs{static_cast<ExprList *>(other.m_exprs->clone().release())},
		  m_chunk{static_cast<Chunk *>(other.m_chunk->clone().release())}
	{
	}

	std::unique_ptr <ParamList> m_variables;
	std::unique_ptr <ExprList> m_exprs;
	std::unique_ptr <Chunk> m_chunk;
};
