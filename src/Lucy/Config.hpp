#pragma once

#include <string_view>
#include <vector>

#include "Bitfield.hpp"

struct Config {
	std::string graphvizOutput;
	std::string inputFile;
	std::string logOutput;

	enum Option {
		DumpAST,
		DumpIR,
		WriteToStdout,
		_last,
	};

	Bitfield <_last> boolOpts;

	bool getOpt(Option o) const { return static_cast<bool>(boolOpts.get(o)); }
	void parse(unsigned argc, const char **argv);
	void parse(const char *optString);
	void parse(const std::vector <std::string_view> &argv);

	[[noreturn]] void usage();
};
