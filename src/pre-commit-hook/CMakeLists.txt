set(GIT_HOOK pre-commit-${PROJECT_NAME})

pkg_check_modules(LIBGIT2 libgit2)

add_executable(${GIT_HOOK}
	Config.cpp
	Job.cpp
	main.cpp
)

target_include_directories(${GIT_HOOK} PRIVATE .)
target_compile_features(${GIT_HOOK} PRIVATE cxx_std_17)
target_compile_options(${GIT_HOOK} PRIVATE -Wall -fsanitize=address,undefined -ggdb)
target_link_libraries(${GIT_HOOK} -fsanitize=address,undefined ${LIBGIT2_LDFLAGS})
