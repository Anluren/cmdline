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

# Install targets
install(TARGETS cmdline
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES cmdline.h
    DESTINATION include
)

# Print build info
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "No external dependencies - pure C++17 implementation")

