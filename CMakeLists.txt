project(Lucy)
cmake_minimum_required(VERSION 3.8)

find_package(BISON)
find_package(FLEX)
find_package(PkgConfig)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(src)

if(${LUCY_BUILD_GUI})
	pkg_check_modules(LibGVC REQUIRED libgvc)
	pkg_check_modules(GLEW REQUIRED glew)
	pkg_check_modules(OpenGL REQUIRED gl)
	pkg_check_modules(SDL2 REQUIRED sdl2)

	add_subdirectory(imgui)
endif(${LUCY_BUILD_GUI})
