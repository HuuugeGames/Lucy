project(Lucy)
cmake_minimum_required(VERSION 3.8)

find_package(BISON)
find_package(FLEX)
find_package(PkgConfig)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(imgui)
add_subdirectory(src)
