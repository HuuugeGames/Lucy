#pragma once

#include <string>
#include <thread>

struct Config {
	unsigned long maxParallelJobs = std::max(1u, std::thread::hardware_concurrency());
	std::string lucyPath = "/usr/bin/Lucy";

	bool parse(unsigned argc, char **argv);
};
