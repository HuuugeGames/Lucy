#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "Config.hpp"

bool Config::parse(unsigned argc, char **argv)
{
	unsigned idx = 1;

	auto ensureArg = [&idx, argc]() -> bool
	{
		++idx;
		if (idx == argc) {
			fputs("Missing argument\n", stderr);
			return false;
		}
		return true;
	};

	while (idx < argc) {
		const char *s = argv[idx];

		if (strcmp(s, "-j") == 0 || strcmp(s, "--jobs") == 0) {
			if (!ensureArg())
				return false;

			char *parseEnd;
			errno = 0;
			unsigned long parallelJobs = strtoul(s, &parseEnd, 10);
			if (errno == ERANGE || *parseEnd != '\0') {
				fprintf(stderr, "Invalid number of jobs passed: %s\n", argv[idx]);
				return false;
			}

			this->maxParallelJobs = parallelJobs;
		} else if (strcmp(s, "--lucy") == 0) {
			if (!ensureArg())
				return false;

			this->lucyPath = argv[idx];
		} else {
			fprintf(stderr, "Skipping unrecognized parameter: %s\n", s);
		}

		++idx;
	}

	return true;
}
