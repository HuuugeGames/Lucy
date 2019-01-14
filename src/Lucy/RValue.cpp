#include <iomanip>
#include <unordered_map>
#include "AST.hpp"
#include "RValue.hpp"

std::ostream & operator << (std::ostream &os, const RValue &rval)
{
	switch (rval.valueRef.index()) {
		case 0:
			std::visit([&os](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr(std::is_same_v<T, nullptr_t>)
					os << "nil";
				else if constexpr(std::is_same_v<T, std::string>)
					os << std::quoted(arg);
				else
					os << arg;
			}, std::get<0>(rval.valueRef));
			break;
		case 1: {
			auto lval = std::get<1>(rval.valueRef);
			assert(lval->lvalueType() == AST::LValue::Type::Name);
			os << lval->name();
			break;
		}
		case 2:
			os << "tmp_0x" << std::get<2>(rval.valueRef);
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
