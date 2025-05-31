set(SUDOKU_TESTING_NAME
    watch_conflicts
    watch_refreshes
    watch_consecutive_accesses
)

foreach(SUDOKU_TESTING ${SUDOKU_TESTING_NAME})
    file(GLOB SUDOKU_TESTING_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/${SUDOKU_TESTING}.cc)
    add_executable(${SUDOKU_TESTING} ${SUDOKU_TESTING_SOURCES})
    target_link_libraries(${SUDOKU_TESTING} PRIVATE
        sudoku_internal_lib sudoku_lib spdlog::spdlog)
endforeach(SUDOKU_TESTING ${SUDOKU_TESTING_NAME})
