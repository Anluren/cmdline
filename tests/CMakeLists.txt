# ==============================================================================
# Test Executables
# ==============================================================================

# Compile-time features tests
add_executable(constexpr_test src/constexpr_test.cpp)
target_link_libraries(constexpr_test PRIVATE cmdline_hdr_only)

# Lambda and type system tests
add_executable(test_lambda_types src/test_lambda_types.cpp)
target_link_libraries(test_lambda_types PRIVATE cmdline_hdr_only)

# Parsing and invocation tests
add_executable(test_parse_invoke src/test_parse_invoke.cpp)
target_link_libraries(test_parse_invoke PRIVATE cmdline_hdr_only)

add_executable(test_parse_fail src/test_parse_fail.cpp)
target_link_libraries(test_parse_fail PRIVATE cmdline_hdr_only)

# Option type tests
add_executable(test_typed_options src/test_typed_options.cpp)
target_link_libraries(test_typed_options PRIVATE cmdline_hdr_only)

add_executable(test_variadic_groups src/test_variadic_groups.cpp)
target_link_libraries(test_variadic_groups PRIVATE cmdline_hdr_only)

add_executable(test_default_values src/test_default_values.cpp)
target_link_libraries(test_default_values PRIVATE cmdline_hdr_only)

add_executable(test_option_value src/test_option_value.cpp)
target_link_libraries(test_option_value PRIVATE cmdline_hdr_only)

add_executable(test_option_matching src/test_option_matching.cpp)
target_link_libraries(test_option_matching PRIVATE cmdline_hdr_only)

# Range validation tests
add_executable(test_int_range src/test_int_range.cpp)
target_link_libraries(test_int_range PRIVATE cmdline_hdr_only)

add_executable(test_array_range src/test_array_range.cpp)
target_link_libraries(test_array_range PRIVATE cmdline_hdr_only)

# Command and subcommand tests
add_executable(test_subcommands src/test_subcommands.cpp)
target_link_libraries(test_subcommands PRIVATE cmdline_hdr_only)

add_executable(test_hierarchy src/test_hierarchy.cpp)
target_link_libraries(test_hierarchy PRIVATE cmdline_hdr_only)

add_executable(test_hierarchy_multi src/test_hierarchy_multi.cpp)
target_link_libraries(test_hierarchy_multi PRIVATE cmdline_hdr_only)

add_executable(test_hierarchy_hide_options src/test_hierarchy_hide_options.cpp)
target_link_libraries(test_hierarchy_hide_options PRIVATE cmdline_hdr_only)

# Mode manager tests
add_executable(test_mode_manager src/test_mode_manager.cpp)
target_link_libraries(test_mode_manager PRIVATE cmdline_hdr_only)

add_executable(test_mode_execute_command src/test_mode_execute_command.cpp)
target_link_libraries(test_mode_execute_command PRIVATE cmdline_hdr_only)

# Feature tests - partial matching and help query
add_executable(test_partial_matching src/test_partial_matching.cpp)
target_link_libraries(test_partial_matching PRIVATE cmdline_hdr_only)

add_executable(test_help_query src/test_help_query.cpp)
target_link_libraries(test_help_query PRIVATE cmdline_hdr_only)

# Comprehensive coverage tests
add_executable(test_comprehensive_coverage src/test_comprehensive_coverage.cpp)
target_link_libraries(test_comprehensive_coverage PRIVATE cmdline_hdr_only)
