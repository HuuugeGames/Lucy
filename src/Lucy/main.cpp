#include <iostream>

#include "Driver.hpp"
#include "Serial.hpp"

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);
	Driver d;

	if (argc > 1) {
		if (!d.setInputFile(argv[1]))
			return 1;
	}

	if (d.parse() != 0)
		return 1;

	return 0;
}
