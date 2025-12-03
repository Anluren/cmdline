# Example executables
add_executable(example example.cpp)
target_link_libraries(example PRIVATE cmdline)
target_include_directories(example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(example_constexpr example_constexpr.cpp)
target_link_libraries(example_constexpr PRIVATE cmdline_hdr_only)
