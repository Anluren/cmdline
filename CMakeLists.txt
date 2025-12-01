cmake_minimum_required(VERSION 3.10)
project(cmdline VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library target
add_library(cmdline SHARED
    cmdline.cpp
)

target_include_directories(cmdline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable shared_from_this for Mode class
target_compile_options(cmdline PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Example executable
add_executable(example
    example.cpp
)

target_link_libraries(example PRIVATE
    cmdline
)

# Compile-time example (header-only, no library dependency)
add_executable(example_constexpr
    example_constexpr.cpp
)

target_include_directories(example_constexpr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Test executables for compile-time features
add_executable(constexpr_test
    constexpr_test.cpp
)

target_include_directories(constexpr_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_lambda_types
    test_lambda_types.cpp
)

target_include_directories(test_lambda_types PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_parse_invoke
    test_parse_invoke.cpp
)

target_include_directories(test_parse_invoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_typed_options
    test_typed_options.cpp
)

target_include_directories(test_typed_options PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_variadic_groups
    test_variadic_groups.cpp
)

target_include_directories(test_variadic_groups PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_int_range
    test_int_range.cpp
)

target_include_directories(test_int_range PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_array_range
    test_array_range.cpp
)

target_include_directories(test_array_range PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_option_matching
    test_option_matching.cpp
)

target_include_directories(test_option_matching PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_subcommands
    test_subcommands.cpp
)

target_include_directories(test_subcommands PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install targets
install(TARGETS cmdline
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES cmdline.h
    DESTINATION include
)

install(FILES cmdline_constexpr.h
    DESTINATION include
)

# Print build info
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "No external dependencies - pure C++17 implementation")

