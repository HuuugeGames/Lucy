#include <iomanip>
#include <unordered_map>
#include "AST.hpp"
#include "RValue.hpp"

std::ostream & operator << (std::ostream &os, const RValue &rval)
{
	switch (rval.valueRef.index()) {
		case RValue::Type::Immediate:
			std::visit([&os](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr(std::is_same_v<T, nullptr_t>)
					os << "nil";
				else if constexpr(std::is_same_v<T, std::string>)
					os << std::quoted(arg);
				else
					os << arg;
			}, std::get<RValue::Type::Immediate>(rval.valueRef));
			break;
		case RValue::Type::LValue: {
			auto lval = std::get<RValue::Type::LValue>(rval.valueRef);
			assert(lval->lvalueType() == AST::LValue::Type::Name);
			os << lval->name();
			break;
		}
		case RValue::Type::Temporary:
			os << "tmp_0x" << std::get<RValue::Type::Temporary>(rval.valueRef);
			break;
	}

	return os;
}

const AST::LValue * RValue::getTemporary(unsigned idx)
{
	static std::unordered_map <unsigned, std::unique_ptr <const AST::LValue> > temporaries;

	auto iter = temporaries.find(idx);
	if (iter == temporaries.end())
		iter = temporaries.emplace(idx, new AST::LValue{"__tmp_rval_" + std::to_string(idx)}).first;

	return iter->second.get();
}
