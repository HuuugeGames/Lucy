#include <git2.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

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

int printer(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload)
{
	if (delta->status != GIT_DELTA_ADDED && delta->status != GIT_DELTA_MODIFIED)
		return 0;
	fwrite(line->content, 1, line->content_len, stdout);
	return 0;
}

int main()
{
	git_libgit2_init();
	atexit([]{git_libgit2_shutdown();});

	auto repo = execute("open repo", &git_repository_open, ".");
	auto head = getTree(repo, "HEAD");

	git_diff_options diffOpts = GIT_DIFF_OPTIONS_INIT;
	auto diff = execute("diff", &git_diff_tree_to_index, repo.get(), head.get(), nullptr, &diffOpts);
	git_diff_print(diff.get(), GIT_DIFF_FORMAT_NAME_ONLY, printer, nullptr);

	return 0;
}
