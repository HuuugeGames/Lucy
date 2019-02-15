#include <cstring>
#include <iostream>
#include <string_view>
#include <vector>

#include "Config.hpp"
#include "Logger.hpp"

void Config::parse(unsigned argc, const char **argv)
{
	std::vector <std::string_view> vec;

	vec.reserve(argc);
	for (unsigned i = 0; i < argc; ++i)
		vec.emplace_back(argv[i]);

	parse(vec);
}

void Config::parse(const char *optString)
{
	if (!optString)
		return;

	std::vector <std::string_view> argv;
	argv.push_back(""); //argv[0]

	const char *p = optString;
	while (*p) {
		while (*p && isspace(*p))
			++p;

		if (*p) {
			const char *begin = p;
			while (*p && !isspace(*p))
				++p;
			argv.emplace_back(begin, p - begin);
		}
	}

	parse(argv);
}

void Config::parse(const std::vector <std::string_view> &argv)
{
	unsigned idx = 1; //skip argv[0]
	std::string_view current;

	auto ensureArg = [&idx, &argv, &current]()
	{
		++idx;
		if (idx == argv.size())
			FATAL("Missing argument to option: " << current << '\n');
	};

	while (idx != argv.size()) {
		current = argv[idx];

		if (current == "--disable-all") {
			for (unsigned i = 0; i != Check::_last; ++i)
				Logger::disable(Check{i});
		} else if (current == "--enable-all") {
			for (unsigned i = 0; i != Check::_last; ++i)
				Logger::enable(Check{i});
		} else if (current == "--enable" || current == "--disable") {
			ensureArg();
			auto flag = Check::fromString(argv[idx]);
			if (flag.has_value()) {
				if (current == "--enable")
					Logger::enable(*flag);
				else
					Logger::disable(*flag);
			} else {
				LOG(Logger::Pedantic, "Unknown flag: " << argv[idx] << '\n');
			}
		} else if (current == "--dump-ast") {
			boolOpts.set(DumpAST);
		} else if (current == "--dump-ir") {
			boolOpts.set(DumpIR);
		} else if (current == "--graphviz") {
			ensureArg();
			this->graphvizOutput = argv[idx];
		} else if (current == "-h" || current == "--help") {
			usage();
		} else if (current == "--output" || current == "-o") {
			if (boolOpts.get(WriteToStdout))
				FATAL("--output and --stdout are mutually exclusive\n");

			ensureArg();
			this->logOutput = argv[idx];
		} else if (current == "--stdout") {
			if (!this->logOutput.empty())
				FATAL("--output and --stdout are mutually exclusive\n");

			boolOpts.set(WriteToStdout);
		} else {
			if (current[0] == '-')
				LOG(Logger::Pedantic, "Skipping unrecognized option: " << current << '\n');
			else
				this->inputFile = current;
		}

		++idx;
	}
}

[[noreturn]] void Config::usage()
{
	std::cout <<
R"___(Lucy (a.k.a. Analua), a static analyzer for Lua code.

Usage:
  $ ./Lucy [options] [file]

If input file is not specified, read standard input.

Options:
  --graphviz <file>  write CFG description in dot language
  -h, --help         usage information (this text)
  --output <file>    write to file instead of stderr
  --stdout           write to stdout instead of stderr

Debugging and development options:
  --dump-ast         write abstract syntax tree description to stdout
  --dump-ir          write intermediate representation code to stdout

If command line is not available, you can pass options in LUCY_OPTIONS
environment variable. Example:

  $ LUCY_OPTIONS="--stdout" ./Lucy

)___";
	std::exit(1);
}
