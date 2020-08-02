#include "Config.hpp"
#include "ControlFlowGraph.hpp"
#include "Driver.hpp"
#include "Scope.hpp"

int main(int argc, const char **argv)
{
	std::ios_base::sync_with_stdio(false);

	Driver d;
	Config conf;

	conf.parse(getenv("LUCY_OPTIONS"));
	conf.parse(argc, argv);

	if (!conf.inputFile.empty())
		d.setInputFile(conf.inputFile);

	if (!conf.logOutput.empty())
		Logger::setOutput(conf.logOutput);

	if (conf.getOpt(Config::Option::WriteToStdout))
		Logger::setOutput(std::cout);

	if (d.parse() != 0)
		return 1;

	assert(d.chunks().size() == 1);
	if (conf.getOpt(Config::Option::DumpAST)) {
		d.chunks()[0]->print();
		std::cout << std::flush;
	}

	Scope globalScope;
	ControlFlowGraph cfg{*d.chunks()[0], globalScope};

	if (conf.getOpt(Config::Option::DumpIR)) {
		cfg.irDump();
		std::cout << std::flush;
	}

	if (!conf.graphvizOutput.empty())
		cfg.graphvizDump(conf.graphvizOutput);

	if (!conf.issuesOutput.empty()) {
		std::ofstream output{conf.issuesOutput};
		auto printer = [&output](auto &&issue)
		{
			output << std::string{issue} << '\n';
		};

		auto issueVec = Logger::foundIssues();
		std::sort(issueVec.begin(), issueVec.end());
		for (const auto &issue : issueVec)
			std::visit(printer, issue);
	}

	return 0;
}
