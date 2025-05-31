set(SUDOKU_INTERNAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/pool.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/conflicts.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/refreshes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/consecutive_accesses.cc
)

set(SUDOKU_INTERNAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/assembly.h
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/conflicts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/refreshes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/consecutive_accesses.h
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intel_core_12th_ddr4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intel_core_12th_ddr5.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intel_core_14th_ddr5.h
    ${CMAKE_CURRENT_SOURCE_DIR}/amd_ryzen_zen4_ddr5.h
)

add_library(sudoku_internal_lib STATIC
    ${SUDOKU_INTERNAL_SOURCES}
    ${SUDOKU_INTERNAL_HEADERS}
)

target_include_directories(sudoku_internal_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sudoku_internal_lib PUBLIC spdlog::spdlog)
