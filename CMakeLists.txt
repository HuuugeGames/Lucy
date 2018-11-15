project(Lucy)
cmake_minimum_required(VERSION 3.8)

find_package(BISON)
find_package(FLEX)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(src)
