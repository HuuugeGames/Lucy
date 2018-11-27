#include <iostream>

#include "Config.hpp"
#include "ControlFlowGraph.hpp"
#include "Driver.hpp"
#include "Serial.hpp"

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);
	Config conf;
	if (!conf.parse(argc, argv))
		return 1;

	Driver d;

	if (!conf.inputFile.empty()) {
		if (!d.setInputFile(conf.inputFile))
			return 1;
	}

	if (d.parse() != 0)
		return 1;

	ControlFlowGraph cfg{*d.chunks()[0]};
	if (!conf.graphvizOutput.empty())
		cfg.graphvizDump(conf.graphvizOutput);

	return 0;
}
