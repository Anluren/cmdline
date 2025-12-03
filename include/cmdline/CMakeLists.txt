# cmdline header-only library
add_library(cmdline_hdr_only INTERFACE)
add_library(cpp_util::cmdline_hdr_only ALIAS cmdline_hdr_only)

target_include_directories(cmdline_hdr_only INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(cmdline_hdr_only INTERFACE
    cxx_std_17
)

# Install header files
install(FILES cmdline_hdr_only.h
    DESTINATION include/cmdline
)

install(DIRECTORY details
    DESTINATION include/cmdline
    FILES_MATCHING PATTERN "*.h"
)
