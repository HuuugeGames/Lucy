#pragma once

#include <array>
#include <map>
#include <vector>
#include "IROp.hpp"
#include "Serial.hpp"
#include "ValueVariant.hpp"

class BasicBlock;

namespace SSA {

struct Triplet {
	IR::Op operation;
	std::array <UID, 2> ssaRef;
};

using Phi = std::vector <UID>;

using Entry = std::variant <ValueVariant, std::string, Triplet, Phi>;

class Context {
public:

private:
	Serial m_opCnt;
	std::map <std::pair <BasicBlock *, std::string>, UID> m_currentState;
};

} //namespace SSA
