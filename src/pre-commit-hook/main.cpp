#include <sys/select.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <unistd.h>
#include <vector>

#include "Config.hpp"
#include "Git.hpp"
#include "Job.hpp"

std::vector <std::string> collectFilenames(git_diff *diff)
{
	std::vector <std::string> result;
	auto fileCallback = [](const git_diff_delta *delta, float, void *payload) -> int
	{
		if (delta->status != GIT_DELTA_ADDED && delta->status != GIT_DELTA_MODIFIED)
			return 0;

		std::string_view filename{delta->new_file.path};
		const char extension[] = ".lua";
		if (filename.size() < sizeof(extension) || strcmp(extension, &filename[filename.size() - sizeof(extension) + 1]) != 0)
			return 0;

		auto fileList = reinterpret_cast<std::vector <std::string> *>(payload);
		fileList->emplace_back(filename);
		return 0;
	};

	git_diff_foreach(diff, fileCallback, nullptr, nullptr, nullptr, &result);
	return result;
}

int processFiles(const Config &cfg, std::vector <std::string> &files, git_repository *repo)
{
	assert(cfg.maxParallelJobs != 0);
	enum { OK = 0, LucyCheck = 1, LucyFail = 2, Internal = 4 }; //TODO

	auto repoIndex = git::execute("index", &git_repository_index, repo);
	int errCode = 0;
	unsigned int runningJobs = 0;
	std::vector <Job> jobs{cfg.maxParallelJobs};

	while (runningJobs != 0 || !files.empty()) {
		while (!files.empty() && runningJobs < cfg.maxParallelJobs) {
			size_t i = 0;
			while (jobs[i].state() != Job::State::Init)
				++i;
			if (!jobs[i].start(cfg, std::move(files.back()), repo, repoIndex.get()))
				return Internal;

			files.pop_back();
			++runningJobs;
		}

		fd_set readFds, writeFds;
		int maxFd = 0;

		FD_ZERO(&readFds);
		FD_ZERO(&writeFds);

		for (const auto &j : jobs) {
			if (j.state() == Job::State::Working) {
				FD_SET(j.readFd(), &readFds);
				maxFd = std::max(maxFd, j.readFd());

				if (j.shouldWrite()) {
					FD_SET(j.writeFd(), &writeFds);
					maxFd = std::max(maxFd, j.writeFd());
				}
			}
		}

		if (select(maxFd + 1, &readFds, &writeFds, nullptr, nullptr) == -1) {
			perror("select");
			return Internal;
		}

		for (auto &j : jobs) {
			if (j.state() != Job::State::Working)
				continue;

			if (FD_ISSET(j.readFd(), &readFds)) {
				j.read();

				if (j.state() == Job::State::Finished) {
					printf("[Checking file: %s]\n", j.filename().c_str());
					if (!j.output().empty())
						printf("%s\n", j.output().c_str());

					const int status = j.exitStatus();
					if (WIFEXITED(status)) {
						const int exitStatus = WEXITSTATUS(status);
						if (exitStatus != 0) {
							errCode |= LucyCheck;
							printf("Job exited with code %d\n", exitStatus);
						}
					}

					if (WIFSIGNALED(status)) {
						errCode |= LucyFail;
						printf("Job killed with signal %d\n", WTERMSIG(status));
					}

					j.reset();
					--runningJobs;
					continue;
				}
			}

			if (j.shouldWrite() && FD_ISSET(j.writeFd(), &writeFds))
				j.write();
		}
	}

	return errCode;
}

int main(int argc, char **argv)
{
	git_libgit2_init();
	atexit([]{git_libgit2_shutdown();});

	auto repo = git::execute("open repo", &git_repository_open, ".");
	auto head = git::getTree(repo, "HEAD^{tree}");

	git_diff_options diffOpts = GIT_DIFF_OPTIONS_INIT;
	auto diff = git::execute("diff", &git_diff_tree_to_index, repo.get(), head.get(), nullptr, &diffOpts);
	auto files = collectFilenames(diff.get());
	auto index = git::execute("index", &git_repository_index, repo.get());

	Config cfg;
	if (!cfg.parse(argc, argv))
		return 1;

	return processFiles(cfg, files, repo.get());
}
