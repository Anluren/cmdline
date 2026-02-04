/**
 * \file cmdline_hdr_only.h
 * \brief Main include file for the compile-time command line library.
 *
 * Include this single header to access all library functionality.
 */

/**
 * \mainpage cmdline_ct - Compile-Time Command Line Library
 *
 * \section intro_sec Introduction
 *
 * cmdline_ct is a C++17 header-only command-line interface library that uses
 * templates and constexpr for zero-allocation command definitions. It provides
 * type-safe option parsing, subcommand dispatch, and interactive CLI mode management.
 *
 * \section features_sec Key Features
 *
 * - **Compile-time validation**: Command specifications are validated at compile time
 * - **Type-safe options**: Strongly-typed option values (int, string, arrays)
 * - **Zero allocations**: Command definitions use constexpr where possible
 * - **Flexible handlers**: Support for both legacy and stream-aware handler signatures
 * - **Subcommand dispatch**: Organize commands hierarchically (like git, docker)
 * - **Interactive CLI**: Mode-based command routing with transitions
 * - **Output redirection**: Configurable output streams for testing
 *
 * \section quickstart_sec Quick Start
 *
 * \code
 * #include <cmdline/cmdline_hdr_only.h>
 * using namespace cmdline_ct;
 *
 * // Define options
 * constexpr auto opts = makeOptions(
 *     StringOption{"name", "User name"},
 *     IntOption{"count", "Repeat count", 1, 10}
 * );
 *
 * // Create command specification
 * constexpr auto spec = CommandSpec("greet", "Greet a user", opts);
 *
 * // Create command with handler
 * auto cmd = makeCommand(spec, [](const auto& args, std::ostream& out, std::ostream&) {
 *     auto name = args.getString("name").value_or("World");
 *     auto count = args.getInt("count").value_or(1);
 *     for (int i = 0; i < count; ++i) {
 *         out << "Hello, " << name << "!\n";
 *     }
 *     return true;
 * });
 *
 * // Execute
 * cmd->execute({"--name", "Alice", "--count", "3"});
 * \endcode
 *
 * \section modules_sec API Modules
 *
 * The library is organized into the following groups:
 * - \ref public_api "Public API" - User-facing classes and functions
 * - \ref internal_api "Internal API" - Implementation details (not for direct use)
 *
 * \section classes_sec Main Classes
 *
 * | Class | Description |
 * |-------|-------------|
 * | CommandSpec | Compile-time command specification |
 * | Command | Command with typed options and handler |
 * | SubcommandDispatcher | Manages subcommands under a parent |
 * | CLI | Interactive mode-based command routing |
 * | ParsedArgs | Type-safe parsed argument container |
 * | OutputContext | Output stream management |
 *
 * \section options_sec Option Types
 *
 * | Type | Value Type | Description |
 * |------|------------|-------------|
 * | IntOption | int64_t | Single integer with optional range validation |
 * | StringOption | std::string | Single string value |
 * | IntArrayOption | std::vector<int64_t> | Multiple integers |
 * | StringArrayOption | std::vector<std::string> | Multiple strings |
 *
 * \section handlers_sec Handler Signatures
 *
 * Commands support two handler signatures:
 * - **Legacy**: `bool(const ParsedArgs<OptGroup>&)`
 * - **Stream-aware**: `bool(const ParsedArgs<OptGroup>&, std::ostream& out, std::ostream& err)`
 *
 * \section license_sec License
 *
 * This library is provided under the terms of its license.
 * See the repository for details.
 */

/**
 * \defgroup public_api Public API
 * \brief User-facing classes, functions, and types.
 *
 * This group contains all the classes and functions that users should
 * interact with directly when building command-line applications.
 *
 * \section public_classes Main Classes
 * - Command - Execute commands with typed options
 * - CommandSpec - Define command specifications
 * - SubcommandDispatcher - Organize subcommands
 * - CLI - Interactive mode management
 * - ParsedArgs - Access parsed arguments
 * - OutputContext - Redirect output streams
 *
 * \section public_options Option Types
 * - IntOption, StringOption - Single value options
 * - IntArrayOption, StringArrayOption - Multi-value options
 *
 * \section public_factories Factory Functions
 * - makeCommand() - Create commands
 * - makeDispatcher() - Create subcommand dispatchers
 * - makeCLI() - Create CLI instances
 * - makeOptions() - Create option groups
 */

/**
 * \defgroup internal_api Internal API
 * \brief Implementation details not intended for direct use.
 *
 * This group contains internal implementation details, helper classes,
 * and type traits used by the library. Users should generally not
 * interact with these directly.
 *
 * \warning The internal API may change without notice between versions.
 */

#ifndef CMDLINE_HDR_ONLY_H
#define CMDLINE_HDR_ONLY_H

// Include all library components
#include "details/cmdline_types.h"
#include "details/cmdline_components.h"

#endif // CMDLINE_HDR_ONLY_H
