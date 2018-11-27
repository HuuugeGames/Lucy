#pragma once

#include <string>

struct Config {
	std::string graphvizOutput;
	std::string inputFile;

	bool parse(unsigned argc, char **argv);
};
