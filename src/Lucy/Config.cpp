#include <cstring>
#include <iostream>

#include "Config.hpp"
#include "Logger.hpp"

void Config::parse(unsigned argc, char **argv)
{
	unsigned idx = 1;
	const char *current = nullptr;

	auto ensureArg = [&idx, argc, current]()
	{
		++idx;
		if (idx == argc)
			FATAL("Missing argument to option: " << current << '\n');
	};

	while (idx < argc) {
		current = argv[idx];

		if (strcmp(current, "--graphviz") == 0) {
			ensureArg();
			this->graphvizOutput = argv[idx];
		} else if (strcmp(current, "--output") == 0 || strcmp(current, "-o") == 0) {
			ensureArg();
			this->logOutput = argv[idx];
		} else {
			if (current[0] == '-')
				LOG(Logger::Pedantic, "Skipping unrecognized option: " << current << '\n');
			else
				this->inputFile = current;
		}

		++idx;
	}
}
