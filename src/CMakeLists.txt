add_subdirectory(Lucy)
add_subdirectory(pre-commit-hook)

if(${LUCY_BUILD_GUI})
	add_subdirectory(AST-Viewer)
endif(${LUCY_BUILD_GUI})
