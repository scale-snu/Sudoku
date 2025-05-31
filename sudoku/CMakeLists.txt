add_subdirectory(internal)
add_subdirectory(testing)

set(SUDOKU_BINARY_NAME
    reverse_functions
    identify_bits
    validate_mapping
    decompose_functions
)

set(SUDOKU_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sudoku.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/sudoku_addressing.cc
)

set(SUDOKU_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/sudoku.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sudoku_addressing.h
)

add_library(sudoku_lib STATIC
    ${SUDOKU_SOURCES}
    ${SUDOKU_HEADERS}
)

target_link_libraries(sudoku_lib PRIVATE sudoku_internal_lib)

target_include_directories(sudoku_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
) 

foreach(SUDOKU_BINARY ${SUDOKU_BINARY_NAME})
    file(GLOB SUDOKU_BINARY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/${SUDOKU_BINARY}.cc)
    add_executable(${SUDOKU_BINARY} ${SUDOKU_BINARY_SOURCES})
    target_link_libraries(${SUDOKU_BINARY} PRIVATE 
        sudoku_internal_lib sudoku_lib spdlog::spdlog)
endforeach(SUDOKU_BINARY ${SUDOKU_BINARY_NAME})
