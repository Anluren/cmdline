# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build the project
mkdir -p build && cd build && cmake .. && ninja

# Build from repo root (if build/ exists)
cd build && ninja

# Run all tests (from repo root)
python3 tests/scripts/run_tests.py

# Run a single test (from build directory)
./tests/test_parse_fail
./tests/test_int_range
./tests/constexpr_test
./tests/test_comprehensive_coverage
# etc - test executables match test source file names

# Run the example (from build directory)
./examples/example

# Run interactive CLI test
./tests/test_interactive_cli --test
```

## Architecture

This is a C++17 header-only command-line interface library using templates and constexpr for zero-allocation command definitions.

### Library Structure (`include/cmdline/`)
- Entry point: `include/cmdline/cmdline_hdr_only.h`
- Core types: `include/cmdline/details/cmdline_types.h`
- Components: `include/cmdline/details/cmdline_components.h`
- Namespace: `cmdline_ct`
- Link target: `cmdline_hdr_only` (header-only, interface library)

### Main Classes
- `CommandSpec<OptGroup>` - compile-time command specification
- `Command<OptGroup, HandlerType>` - command with typed options and handler
- `SubcommandDispatcher` - manages multiple subcommands under a parent
- `CLI` - main interface for interactive applications (handles modes, transitions, command contexts)
- `ParsedArgs<OptGroup>` - typed tuple storage for parsed arguments
- `OutputContext` - holds output streams (stdout/stderr) for redirection
- `TypedOptionValue<T>` - wrapper for option values with set/unset state

Note: `ModeManager` is a legacy alias for `CLI`.

### Option Type System
Options are strongly typed with CRTP base `OptionSpecBase<Derived>`:
- `IntOption` - single int64_t with optional min/max range validation
- `StringOption` - single string
- `IntArrayOption` - vector of int64_t with optional range validation
- `StringArrayOption` - vector of strings

Options are grouped with `OptionGroup<Opts...>` or `makeOptions(...)` and used to create `CommandSpec<OptGroup>`.

### Handler Signatures
Commands support two handler signatures:
- Legacy: `bool(const ParsedArgs<OptGroup>&)` - simple handlers
- Stream-aware: `bool(const ParsedArgs<OptGroup>&, std::ostream& out, std::ostream& err)` - for output redirection

### Factory Functions
- `makeCommand(spec, handler)` - create command from spec and handler
- `makeCommand(spec, handler, ctx)` - create command with OutputContext
- `makeDispatcher(name, description)` - create subcommand dispatcher
- `makeDispatcher(name, description, ctx)` - create dispatcher with OutputContext
- `makeCLI()` - create CLI instance
- `makeCLI(ctx)` - create CLI with OutputContext
- `makeOptions(...)` - create anonymous option group
- `makeOptionGroup(name, description, ...)` - create named option group

### Key Features
- Compile-time command validation with `static_assert`
- Multi-level mode hierarchy with `CLI`
- Subcommand dispatch with `SubcommandDispatcher`
- Help query syntax (`?` and `prefix?`)
- Partial command matching with ambiguity detection
- Integer parsing supports decimal, hex (0x), and binary (0b) formats
- Configurable output streams via `OutputContext` for testing and redirection
- Support for both legacy and stream-aware handler signatures
- `executeCommand(string)` method for parsing command strings directly

## Test Structure

Tests are individual executables in `tests/src/`. Each test validates specific functionality and prints results to stdout/stderr. The Python test runner (`tests/scripts/run_tests.py`) executes tests and checks for expected output strings.

Key test files:
- `test_comprehensive_coverage.cpp` - edge cases and full API coverage
- `test_full_coverage.cpp` - complete API coverage
- `test_typed_options.cpp` - typed option handling
- `test_mode_manager.cpp` - mode transitions
- `test_subcommands.cpp` - subcommand dispatch
- `test_help_query.cpp` - help query (`?`) syntax
- `test_partial_matching.cpp` - partial command matching
- `test_output_streams.cpp` - output stream redirection and OutputContext
- `test_interactive_cli.cpp` - interactive CLI demo with tab completion
- `test_default_values.cpp` - default value handling
- `test_hierarchy.cpp` / `test_hierarchy_multi.cpp` - hierarchy display
- `test_lambda_types.cpp` - lambda handler type deduction
- `test_variadic_groups.cpp` - variadic option groups
