set(TARGET_EXE AST-Viewer)

add_executable(${TARGET_EXE}
	graphviz.cpp
	main.cpp
)

target_compile_definitions(${TARGET_EXE} PRIVATE $<TARGET_PROPERTY:ImGui,INTERFACE_COMPILE_DEFINITIONS>)
target_compile_features(${TARGET_EXE} PRIVATE cxx_std_17)
target_compile_options(${TARGET_EXE} PRIVATE -Wall -fsanitize=address,undefined -ggdb)

target_include_directories(${TARGET_EXE} PRIVATE
	${PROJECT_SOURCE_DIR}/imgui
	${PROJECT_SOURCE_DIR}/src
	$<TARGET_PROPERTY:ImGui,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:Lucy,INTERFACE_INCLUDE_DIRECTORIES>
	${LibGVC_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_EXE}
	LucyLib
	ImGui
	${LibGVC_LIBRARIES}
	${GLEW_LIBRARIES}
	${OpenGL_LIBRARIES}
	${SDL2_LIBRARIES}
	-lSDL2_image
)
