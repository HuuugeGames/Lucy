#include <cstring>
#include <iostream>

#include "Config.hpp"

bool Config::parse(unsigned argc, char **argv)
{
	unsigned idx = 1;

	auto ensureArg = [&idx, argc]() -> bool
	{
		++idx;
		if (idx == argc) {
			std::cerr << "Missing argument\n";
			return false;
		}
		return true;
	};

	while (idx < argc) {
		const char *s = argv[idx];

		if (strcmp(s, "--graphviz") == 0) {
			if (!ensureArg())
				return false;

			this->graphvizOutput = argv[idx];
		} else {
			if (s[0] == '-')
				std::cerr << "Skipping unrecognized option: " << s << '\n';
			else
				inputFile = s;
		}

		++idx;
	}

	return true;
}
