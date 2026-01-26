# Example executable using the header-only library
add_executable(example example.cpp)
target_link_libraries(example PRIVATE cmdline_hdr_only)
