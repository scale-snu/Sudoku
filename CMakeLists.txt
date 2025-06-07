cmake_minimum_required(VERSION 3.11)

project(SudokuProject)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# Please modify the below option.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOMPILE_ZEN_4")
# Supported flag:
#   -DCOMPILE_ALDER_LAKE_DDR4
#   -DCOMPILE_ALDER_LAKE_DDR5
#   -DCOMPILE_RAPTOR_LAKE
#   -DCOMPILE_ZEN_4

add_compile_options(-Wall -Wextra -O0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(sudoku)
