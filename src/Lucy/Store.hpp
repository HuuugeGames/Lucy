#pragma once

namespace AST {
	class LValue;
}

class Scope;

class Store {
public:
	enum class Type {
		Global,
		Upvalue,
		Local,
	};

	Store(const AST::LValue &var, Type type);
	Store(const Store &) = delete;
	Store(Store &&) = default;
	void operator = (const Store &) = delete;
	~Store() = default;

	const AST::LValue & var() const { return m_var; }
	Type type() const { return m_type; }
private:
	const AST::LValue &m_var;
	Type m_type;
};
