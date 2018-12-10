#include "AST.hpp"

namespace {
	const std::string EmptyString;
}

namespace AST {

const Chunk Chunk::Empty;
const ParamList ParamList::Empty;

const std::string & LValue::resolveName() const
{
	if (m_type == LValue::Type::Name)
		return name();

	assert(m_tableExpr->type() == Node::Type::LValue);
	if (m_tableExpr->type() != Node::Type::LValue) {
		LOG(Logger::Warning, location() << " : nontrivial (" << toUnderlying(m_tableExpr->type()) << ") expression for table name\n");
		return EmptyString;
	}

	return static_cast<const LValue *>(m_tableExpr.get())->resolveName();
}

}
