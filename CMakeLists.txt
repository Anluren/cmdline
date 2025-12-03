cmake_minimum_required(VERSION 3.10)
project(cmdline VERSION 1.0.0 LANGUAGES CXX)

# Generate compilation database for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library target
add_library(cmdline SHARED
    src/cmdline.cpp
)

target_include_directories(cmdline 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable shared_from_this for Mode class
target_compile_options(cmdline PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Example executable
add_executable(example
    examples/example.cpp
)

target_link_libraries(example PRIVATE
    cmdline
)

# Compile-time example (header-only, no library dependency)
add_executable(example_constexpr
    examples/example_constexpr.cpp
)

target_include_directories(example_constexpr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Test executables for compile-time features
add_executable(constexpr_test
    tests/constexpr_test.cpp
)

target_include_directories(constexpr_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_lambda_types
    tests/test_lambda_types.cpp
)

target_include_directories(test_lambda_types PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_parse_invoke
    tests/test_parse_invoke.cpp
)

target_include_directories(test_parse_invoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_typed_options
    tests/test_typed_options.cpp
)

target_include_directories(test_typed_options PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_variadic_groups
    tests/test_variadic_groups.cpp
)

target_include_directories(test_variadic_groups PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_int_range
    tests/test_int_range.cpp
)

target_include_directories(test_int_range PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_array_range
    tests/test_array_range.cpp
)

target_include_directories(test_array_range PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_option_matching
    tests/test_option_matching.cpp
)

target_include_directories(test_option_matching PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_subcommands
    tests/test_subcommands.cpp
)

target_include_directories(test_subcommands PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_mode_manager
    tests/test_mode_manager.cpp
)

target_include_directories(test_mode_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_default_values
    tests/test_default_values.cpp
)

target_include_directories(test_default_values PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_option_value
    tests/test_option_value.cpp
)

target_include_directories(test_option_value PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_hierarchy
    tests/test_hierarchy.cpp
)

target_include_directories(test_hierarchy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_hierarchy_multi
    tests/test_hierarchy_multi.cpp
)

target_include_directories(test_hierarchy_multi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_hierarchy_hide_options
    tests/test_hierarchy_hide_options.cpp
)

target_include_directories(test_hierarchy_hide_options PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(test_mode_execute_command
    tests/test_mode_execute_command.cpp
)

target_include_directories(test_mode_execute_command PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Install targets
install(TARGETS cmdline
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES src/cmdline.h
    DESTINATION include
)

install(DIRECTORY include/cmdline
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Print build info
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "No external dependencies - pure C++17 implementation")

