bison_target(Parser grammar.yy ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
flex_target(Lexer scanner.ll ${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp)
add_flex_bison_dependency(Lexer Parser)

add_executable(${PROJECT_NAME}
	${BISON_Parser_OUTPUTS}
	${FLEX_Lexer_OUTPUTS}

	AST.cpp
	BasicBlock.cpp
	Config.cpp
	ControlFlowGraph.cpp
	Driver.cpp
	Preprocessor.cpp

	main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} .)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fsanitize=address,undefined -ggdb)
target_link_libraries(${PROJECT_NAME} -fsanitize=address,undefined)
