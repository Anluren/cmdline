cmake_minimum_required(VERSION 3.10)
project(cmdline VERSION 1.0.0 LANGUAGES CXX)

# Generate compilation database for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add subdirectory for header-only library
add_subdirectory(include/cmdline)

# Add subdirectory for shared library
add_subdirectory(src)

# Add subdirectories for examples and tests
add_subdirectory(examples)
add_subdirectory(tests)

# Print build info
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "No external dependencies - pure C++17 implementation")

