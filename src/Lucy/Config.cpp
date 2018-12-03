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

		if (current == "--graphviz") {
			ensureArg();
			this->graphvizOutput = argv[idx];
		} else if (current == "--output" || current == "-o") {
			ensureArg();
			this->logOutput = argv[idx];
		} else {
			if (current[0] == '-')
				LOG(Logger::Pedantic, "Skipping unrecognized option: " << current << '\n');
			else
				this->inputFile = current;
		}

		++idx;
	}
}
