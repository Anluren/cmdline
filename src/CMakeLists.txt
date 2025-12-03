# Shared library target for cmdline (non-header-only version)
add_library(cmdline SHARED cmdline.cpp)
add_library(cpp_util::cmdline ALIAS cmdline)

target_include_directories(cmdline 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(cmdline PUBLIC
    cxx_std_17
)

# Enable shared_from_this for Mode class
target_compile_options(cmdline PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Install targets
install(TARGETS cmdline
    EXPORT cmdline-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES cmdline.h
    DESTINATION include
)
