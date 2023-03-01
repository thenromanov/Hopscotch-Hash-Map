project(hopscotch-hash-map)

cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)

include(cmake/BuildFlags.cmake)

find_package(Catch REQUIRED)

include_directories(util)

add_subdirectory(src)
add_subdirectory(tests)

function(add_unit_test TARGET)
        add_executable(${TARGET}
        ${ARGN})
        target_link_libraries(${TARGET}
        contrib_catch_main)
        target_include_directories(${TARGET}
        PUBLIC ./src/)
endfunction()

add_unit_test(test_hash_map tests/test.cpp)