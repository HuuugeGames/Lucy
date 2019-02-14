#pragma once

#include <git2.h>
#include <memory>

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
	inline auto git_unique_ptr(git_ ## T *obj) \
	{ \
		return git_unique_ptr_impl(obj, &git_ ## T ## _free); \
	}

	git_unique_ptr_spec(blob);
	git_unique_ptr_spec(diff);
	git_unique_ptr_spec(index);
	git_unique_ptr_spec(object);
	git_unique_ptr_spec(repository);
	git_unique_ptr_spec(tree);

	#undef git_unique_ptr_spec
}

namespace git {
	inline void check_error(int errcode, const char *msgPrefix)
	{
		if (errcode == 0)
			return;
		const auto *e = git_error_last();
		fprintf(stderr, "%s: %s\n", msgPrefix, e->message);
		exit(1);
	}

	template <typename Fn, typename... Args>
	inline auto execute(const char *onErrorMsg, Fn fn, const Args&... args)
	{
		using ptr_t = std::remove_pointer_t <meta::first_arg_t <Fn> >;
		ptr_t rv;
		check_error(fn(&rv, args...), onErrorMsg);
		return meta::git_unique_ptr(rv);
	}

	template <typename T>
	inline auto getTree(const T &repo, const char *treeish)
	{
		auto obj = execute("revparse", &git_revparse_single, repo.get(), treeish);
		return execute("tree lookup", &git_tree_lookup, repo.get(), git_object_id(obj.get()));
	}
}
