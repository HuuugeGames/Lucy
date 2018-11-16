#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <git2.h>
#include <memory>
#include <string_view>
#include <vector>

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

int main()
{
	git_libgit2_init();
	atexit([]{git_libgit2_shutdown();});

	auto repo = execute("open repo", &git_repository_open, ".");
	auto head = getTree(repo, "HEAD");

	git_diff_options diffOpts = GIT_DIFF_OPTIONS_INIT;
	auto diff = execute("diff", &git_diff_tree_to_index, repo.get(), head.get(), nullptr, &diffOpts);
	auto files = collectFilenames(diff.get());

	return 0;
}
