#pragma once

#include <string>

struct Config {
	std::string graphvizOutput;
	std::string inputFile;
	std::string logOutput;

	void parse(unsigned argc, char **argv);
};
