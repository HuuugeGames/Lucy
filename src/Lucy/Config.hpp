#pragma once

#include <string_view>
#include <vector>

struct Config {
	std::string graphvizOutput;
	std::string inputFile;
	std::string logOutput;

	void parse(unsigned argc, const char **argv);
	void parse(const char *optString);
	void parse(const std::vector <std::string_view> &argv);
};
