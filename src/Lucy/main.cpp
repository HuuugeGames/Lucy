#include <iostream>

#include "Config.hpp"
#include "ControlFlowGraph.hpp"
#include "Driver.hpp"
#include "Serial.hpp"

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);

	Driver d;
	Config conf;

	conf.parse(argc, argv);

	if (!conf.inputFile.empty())
		d.setInputFile(conf.inputFile);

	if (!conf.logOutput.empty())
		Logger::setOutput(conf.logOutput);

	if (d.parse() != 0)
		return 1;

	ControlFlowGraph cfg{*d.chunks()[0]};
	if (!conf.graphvizOutput.empty())
		cfg.graphvizDump(conf.graphvizOutput);

	return 0;
}
