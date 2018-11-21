#include <iostream>

#include "ControlFlowGraph.hpp"
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

	ControlFlowGraph cfg{*d.chunks()[0]};
	cfg.graphvizDump("cfg.dot");

	return 0;
}
