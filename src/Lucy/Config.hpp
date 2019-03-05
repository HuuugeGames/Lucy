#pragma once

#include <string_view>
#include <vector>

#include "Bitfield.hpp"
#include "EnumHelpers.hpp"

struct Config {
	std::string graphvizOutput;
	std::string inputFile;
	std::string logOutput;

	EnumClass(Option, unsigned,
		DumpAST,
		DumpIR,
		WriteToStdout
	);

	Bitfield <Option::_size> boolOpts;

	bool getOpt(Option o) const { return static_cast<bool>(boolOpts.get(o.value())); }
	void parse(unsigned argc, const char **argv);
	void parse(const char *optString);
	void parse(const std::vector <std::string_view> &argv);

	[[noreturn]] void usage();
};
