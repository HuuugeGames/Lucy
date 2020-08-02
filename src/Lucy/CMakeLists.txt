bison_target(Parser grammar.yy ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
flex_target(Lexer scanner.ll ${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp)
add_flex_bison_dependency(Lexer Parser)

add_library(${PROJECT_NAME}Lib STATIC
	${BISON_Parser_OUTPUTS}
	${FLEX_Lexer_OUTPUTS}

	AST.cpp
	BasicBlock.cpp
	Config.cpp
	ControlFlowGraph.cpp
	Driver.cpp
	Function.cpp
	IR.cpp
	IROp.cpp
	Issue.cpp
	Logger.cpp
	Preprocessor.cpp
	RValue.cpp
	Scope.cpp
	VarAccess.cpp
)

add_executable(${PROJECT_NAME}
	main.cpp
)

target_include_directories(${PROJECT_NAME}Lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR} .)
target_compile_features(${PROJECT_NAME}Lib PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME}Lib PRIVATE -Wall -fsanitize=address,undefined -ggdb)
target_link_libraries(${PROJECT_NAME}Lib -fsanitize=address,undefined)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fsanitize=address,undefined -ggdb)
target_link_libraries(${PROJECT_NAME} LucyLib -fsanitize=address,undefined)

add_custom_target(test
	${CMAKE_SOURCE_DIR}/test/run-tests.sh $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_SOURCE_DIR}/test
	DEPENDS ${PROJECT_NAME})
