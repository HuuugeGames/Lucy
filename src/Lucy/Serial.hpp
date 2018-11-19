#pragma once

#include <cstdint>

using UID = uint32_t;

class Serial {
public:
	static const UID EmptyUid = 0;
	static const UID MinUid = 1;

	Serial() : nextUid{MinUid} {}
	UID next()
	{
		return nextUid++;
	}

private:
	UID nextUid;
};
