#include <sys/select.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <git2.h>
#include <memory>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Config.hpp"
#include "Job.hpp"

namespace meta {
	template <typename T, typename Deleter>
	auto git_unique_ptr_impl(T *obj, Deleter deleter)
	{
		return std::unique_ptr <T, Deleter>{obj, deleter};
	}

	template <typename T> struct first_arg;

	template <typename Result, typename... Args>
	struct first_arg<Result (*)(Args...)> {
		using type = std::tuple_element_t <0, std::tuple <Args...> >;
	};

	template <typename Result, typename... Args>
	struct first_arg<Result (Args...)> {
		using type = std::tuple_element_t <0, std::tuple <Args...> >;
	};

	template <typename Fn>
	using first_arg_t = typename first_arg<Fn>::type;

	template <typename T> auto git_unique_ptr(T *obj);

	#define git_unique_ptr_spec(T) \
	template <> \
	auto git_unique_ptr(git_ ## T *obj) \
	{ \
		return git_unique_ptr_impl(obj, &git_ ## T ## _free); \
	}

	git_unique_ptr_spec(diff);
	git_unique_ptr_spec(object);
	git_unique_ptr_spec(repository);
	git_unique_ptr_spec(tree);

	#undef git_unique_ptr_spec
}

void check_error(int errcode, const char *msgPrefix)
{
	if (errcode == 0)
		return;
	const auto *e = giterr_last();
	fprintf(stderr, "%s: %s\n", msgPrefix, e->message);
	exit(1);
}

template <typename Fn, typename... Args>
auto execute(const char *onErrorMsg, Fn fn, const Args&... args)
{
	using ptr_t = std::remove_pointer_t <meta::first_arg_t <Fn> >;
	ptr_t rv;
	check_error(fn(&rv, args...), onErrorMsg);
	return meta::git_unique_ptr(rv);
}

template <typename T>
auto getTree(const T &repo, const char *treeish)
{
	auto obj = execute("revparse", &git_revparse_single, repo.get(), treeish);
	auto peeled = execute("peel", &git_object_peel, obj.get(), GIT_OBJ_TREE);
	return meta::git_unique_ptr(reinterpret_cast<git_tree *>(peeled.release()));
}

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

int processFiles(const Config &cfg, std::vector <std::string> &files)
{
	assert(cfg.maxParallelJobs != 0);
	enum { OK = 0, LucyCheck = 1, LucyFail = 2, Internal = 4 }; //TODO

	int errCode = 0;
	unsigned int runningJobs = 0;
	std::vector <Job> jobs{cfg.maxParallelJobs};

	while (runningJobs != 0 || !files.empty()) {
		while (!files.empty() && runningJobs < cfg.maxParallelJobs) {
			size_t i = 0;
			while (jobs[i].state() != Job::State::Init)
				++i;
			if (!jobs[i].start(std::move(files.back())))
				return Internal;

			files.pop_back();
			++runningJobs;
		}

		fd_set fds;
		int maxFd = 0;

		FD_ZERO(&fds);
		for (const auto &j : jobs) {
			if (j.state() == Job::State::Working) {
				FD_SET(j.fd(), &fds);
				maxFd = std::max(maxFd, j.fd());
			}
		}

		if (select(maxFd + 1, &fds, nullptr, nullptr, nullptr) == -1) {
			perror("select");
			return Internal;
		}

		for (auto &j : jobs) {
			if (j.state() == Job::State::Working && FD_ISSET(j.fd(), &fds))
				j.process();
			if (j.state() == Job::State::Finished) {
				//TODO job exit code handling
				if (!j.output().empty())
					printf("[%s]\n%s\n", j.filename().c_str(), j.output().c_str());
				j.reset();
				--runningJobs;
			}
		}
	}

	return errCode;
}

int main()
{
	git_libgit2_init();
	atexit([]{git_libgit2_shutdown();});

	auto repo = execute("open repo", &git_repository_open, ".");
	auto head = getTree(repo, "HEAD");

	git_diff_options diffOpts = GIT_DIFF_OPTIONS_INIT;
	auto diff = execute("diff", &git_diff_tree_to_index, repo.get(), head.get(), nullptr, &diffOpts);
	auto files = collectFilenames(diff.get());

	Config cfg;
	//TODO cmdline
	cfg.maxParallelJobs = std::max(1u, std::thread::hardware_concurrency());

	return processFiles(cfg, files);
}
