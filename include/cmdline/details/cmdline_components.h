/**
 * \file cmdline_components.h
 * \brief Command, SubcommandDispatcher, and CLI implementations for the compile-time command line library.
 *
 * This file contains the main component classes for building command-line interfaces:
 * - Command: Typed command with options and handler
 * - SubcommandDispatcher: Manages multiple subcommands under a parent
 * - CLI: Main interface for interactive applications with mode management
 *
 * \author cmdline_ct library
 * \see cmdline_types.h for core type definitions
 */

#ifndef CMDLINE_COMPONENTS_H
#define CMDLINE_COMPONENTS_H

#include "cmdline_types.h"

namespace cmdline_ct {

/**
 * \brief Type trait to detect if a handler accepts output streams.
 * \ingroup internal_api
 *
 * Used to support both legacy `bool(ParsedArgs)` and new
 * `bool(ParsedArgs, ostream&, ostream&)` handler signatures.
 *
 * \tparam OptGroup The option group type for ParsedArgs
 * \tparam Callable The handler callable type to check
 *
 * \par Example:
 * \code
 * // Check if handler accepts streams
 * if constexpr (HandlerAcceptsStreams<MyOptGroup, MyHandler>::value) {
 *     handler(args, out, err);
 * } else {
 *     handler(args);
 * }
 * \endcode
 */
template<typename OptGroup, typename Callable, typename = void>
struct HandlerAcceptsStreams : std::false_type {};

/// \cond INTERNAL
template<typename OptGroup, typename Callable>
struct HandlerAcceptsStreams<OptGroup, Callable,
    std::void_t<decltype(std::declval<Callable>()(
        std::declval<const ParsedArgs<OptGroup>&>(),
        std::declval<std::ostream&>(),
        std::declval<std::ostream&>()
    ))>> : std::true_type {};
/// \endcond

/**
 * \brief Lightweight template wrapper for command handlers.
 * \ingroup internal_api
 *
 * Avoids std::function overhead by storing the callable directly.
 * Supports both legacy and stream-aware handler signatures through
 * compile-time dispatch.
 *
 * \tparam OptGroup The option group type
 * \tparam Callable The handler callable type
 *
 * \see makeCommandHandler() for convenient construction
 */
template<typename OptGroup, typename Callable>
struct CommandHandler {
    Callable callable;  ///< The stored callable handler

    /**
     * \brief Construct a CommandHandler from a callable.
     * \param c The callable to wrap
     */
    constexpr CommandHandler(Callable c) : callable(std::move(c)) {}

    /**
     * \brief Invoke the handler with output streams.
     *
     * Automatically dispatches to the appropriate handler signature
     * based on whether the callable accepts streams.
     *
     * \param args The parsed command-line arguments
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if the handler executed successfully, false otherwise
     */
    bool operator()(const ParsedArgs<OptGroup>& args,
                    std::ostream& out, std::ostream& err) const {
        if constexpr (HandlerAcceptsStreams<OptGroup, Callable>::value) {
            return callable(args, out, err);
        } else {
            (void)out; (void)err;  // Unused for legacy handlers
            return callable(args);
        }
    }

    /**
     * \brief Legacy call operator for backward compatibility.
     * \param args The parsed command-line arguments
     * \return true if the handler executed successfully, false otherwise
     */
    constexpr bool operator()(const ParsedArgs<OptGroup>& args) const {
        return callable(args);
    }
};

/**
 * \brief Create a CommandHandler with automatic type deduction.
 * \ingroup internal_api
 *
 * \tparam OptGroup The option group type
 * \tparam Callable The handler callable type (deduced)
 * \param c The callable to wrap
 * \return A CommandHandler wrapping the callable
 *
 * \par Example:
 * \code
 * auto handler = makeCommandHandler<MyOptGroup>([](const auto& args) {
 *     return true;
 * });
 * \endcode
 */
template<typename OptGroup, typename Callable>
constexpr auto makeCommandHandler(Callable&& c) {
    return CommandHandler<OptGroup, std::decay_t<Callable>>{std::forward<Callable>(c)};
}

/**
 * \brief A command with typed options and a handler function.
 * \ingroup public_api
 *
 * Command combines a CommandSpec (defining name, description, and options)
 * with a handler function that processes the parsed arguments.
 *
 * \tparam OptGroup The OptionGroup type defining available options
 * \tparam HandlerType The handler callable type
 *
 * \par Handler Signatures:
 * Handlers can use one of two signatures:
 * - Legacy: `bool(const ParsedArgs<OptGroup>&)`
 * - Stream-aware: `bool(const ParsedArgs<OptGroup>&, std::ostream& out, std::ostream& err)`
 *
 * \par Example:
 * \code
 * constexpr auto spec = CommandSpec("greet", "Greet someone",
 *     makeOptions(StringOption{"name", "Name to greet"}));
 *
 * auto cmd = makeCommand(spec, [](const auto& args, std::ostream& out, std::ostream&) {
 *     if (auto name = args.getString("name")) {
 *         out << "Hello, " << *name << "!\n";
 *     }
 *     return true;
 * });
 *
 * cmd->execute({"--name", "World"});
 * \endcode
 *
 * \see makeCommand() for convenient construction
 * \see CommandSpec for command specification
 */
template<typename OptGroup, typename HandlerType>
class Command {
    // Static assertion to ensure HandlerType is callable with at least the legacy signature
    // Handlers can use either: bool(ParsedArgs) or bool(ParsedArgs, ostream&, ostream&)
    static_assert(std::is_invocable_r_v<bool, HandlerType, const ParsedArgs<OptGroup>&> ||
                  std::is_invocable_r_v<bool, HandlerType, const ParsedArgs<OptGroup>&, std::ostream&, std::ostream&>,
                  "HandlerType must be callable with signature: bool(const ParsedArgs<OptGroup>&) or bool(const ParsedArgs<OptGroup>&, std::ostream&, std::ostream&)");

public:
    /**
     * \brief Construct a Command from a specification and handler.
     * \param spec The command specification
     * \param handler The handler function to invoke on execution
     */
    constexpr Command(const CommandSpec<OptGroup>& spec, HandlerType handler)
        : m_spec(spec), m_handler(handler) {}

    /**
     * \brief Get the command specification.
     * \return Reference to the CommandSpec
     */
    const CommandSpec<OptGroup>& getSpec() const { return m_spec; }

    /**
     * \brief Get the command name.
     * \return The command name as a string_view
     */
    constexpr std::string_view getName() const { return m_spec.name; }

    /**
     * \brief Get the command description.
     * \return The command description as a string_view
     */
    constexpr std::string_view getDescription() const { return m_spec.description; }

    /**
     * \brief Set the output context for this command.
     * \param ctx The OutputContext containing output and error streams
     */
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }

    /**
     * \brief Set the output context using separate streams.
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     */
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }

    /**
     * \brief Get the current output context.
     * \return Reference to the OutputContext
     */
    const OutputContext& getOutputContext() const { return m_context; }

    /**
     * \brief Display a hierarchical view of the command and its options.
     *
     * Uses the stored OutputContext for output.
     *
     * \param indent Indentation prefix for each line
     * \param showOptions Whether to display option details
     */
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        showHierarchy(m_context.output(), indent, showOptions);
    }

    /**
     * \brief Display a hierarchical view of the command and its options.
     * \param out Output stream to write to
     * \param indent Indentation prefix for each line
     * \param showOptions Whether to display option details
     */
    void showHierarchy(std::ostream& out, const std::string& indent = "", bool showOptions = true) const {
        out << indent << m_spec.name << ": " << m_spec.description << "\n";

        if (showOptions) {
            auto opts = m_spec.getAllOptions();
            if (!opts.empty()) {
                out << indent << "  Options:\n";
                for (const auto& opt : opts) {
                    out << indent << "    --" << opt.name << ": " << opt.description;

                    // Show type information
                    if (opt.is_array) {
                        out << " [array]";
                    } else if (opt.is_int) {
                        out << " [int]";
                    } else {
                        out << " [string]";
                    }

                    // Show range if applicable
                    if (opt.min_value || opt.max_value) {
                        out << " (";
                        if (opt.min_value) out << "min=" << *opt.min_value;
                        if (opt.min_value && opt.max_value) out << ", ";
                        if (opt.max_value) out << "max=" << *opt.max_value;
                        out << ")";
                    }

                    // Show required flag
                    if (opt.required) {
                        out << " [required]";
                    }

                    out << "\n";
                }
            }
        }
    }

    /**
     * \brief Check if a string is a recognized option.
     *
     * Accepts both `--name` format and bare option names.
     *
     * \param arg The argument string to check
     * \return true if arg is a known option, false otherwise
     */
    bool isOption(const std::string& arg) const {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
            return m_spec.hasOption(arg.substr(2));
        }
        return m_spec.hasOption(arg);
    }

    /**
     * \brief Parse and execute the command.
     *
     * Parses the arguments and invokes the handler if parsing succeeds.
     * Uses the stored OutputContext.
     *
     * \param args Vector of argument strings
     * \return true if parsing and execution succeeded, false otherwise
     */
    bool execute(const std::vector<std::string>& args) const {
        return execute(args, m_context.output(), m_context.error());
    }

    /**
     * \brief Parse and execute the command with explicit streams.
     * \param args Vector of argument strings
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if parsing and execution succeeded, false otherwise
     */
    bool execute(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) const {
        ParsedArgs<OptGroup> parsed = parse(args, err);
        if (!parsed.parseSuccess) {
            return false;  // Don't execute if parsing failed
        }
        return invoke(parsed, out, err);
    }

    /**
     * \brief Parse and execute with argc/argv style arguments.
     *
     * Uses the stored OutputContext.
     *
     * \param argc Argument count
     * \param argv Argument vector
     * \return true if parsing and execution succeeded, false otherwise
     */
    bool execute(int argc, char* argv[]) const {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    /**
     * \brief Parse and execute with argc/argv and explicit streams.
     * \param argc Argument count
     * \param argv Argument vector
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if parsing and execution succeeded, false otherwise
     */
    bool execute(int argc, char* argv[], std::ostream& out, std::ostream& err) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }

    /**
     * \brief Parse arguments into a ParsedArgs structure.
     *
     * Uses the stored OutputContext for error output.
     *
     * \param args Vector of argument strings
     * \return ParsedArgs containing the parsed options and positional arguments
     */
    ParsedArgs<OptGroup> parse(const std::vector<std::string>& args) const {
        return parse(args, m_context.error());
    }

    /**
     * \brief Parse arguments with explicit error stream.
     * \param args Vector of argument strings
     * \param err Output stream for error messages
     * \return ParsedArgs containing the parsed options and positional arguments
     */
    ParsedArgs<OptGroup> parse(const std::vector<std::string>& args, std::ostream& err) const {
        ParsedArgs<OptGroup> parsed;
        parsed.optionGroup = &m_spec.optionGroup; // Set the option group pointer for runtime name lookups

        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];

            // Check if it's an option (starts with -- or matches option name directly)
            std::string_view optName;
            bool isOpt = false;
            bool lookingForOption = false;

            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
                // Option with '--' prefix
                optName = std::string_view(arg).substr(2);
                lookingForOption = true;
                isOpt = m_spec.hasOption(optName);
            } else {
                // Try matching without prefix
                if (m_spec.hasOption(arg)) {
                    optName = arg;
                    isOpt = true;
                }
            }

            if (isOpt) {
                // Found a valid option - parse it and store in the tuple
                parseOptionIntoTuple(parsed, optName, args, i);
            } else if (lookingForOption) {
                // Invalid option (has -- prefix but not recognized)
                err << "Error: Unknown option '--" << optName << "'\n";
                parsed.parseSuccess = false;
            } else {
                // Positional argument
                parsed.positional.push_back(arg);
            }
        }

        return parsed;
    }

    /**
     * \brief Parse argc/argv style arguments.
     *
     * Uses the stored OutputContext for error output.
     *
     * \param argc Argument count
     * \param argv Argument vector
     * \return ParsedArgs containing the parsed options and positional arguments
     */
    ParsedArgs<OptGroup> parse(int argc, const char* argv[]) const {
        return parse(argc, argv, m_context.error());
    }

    /**
     * \brief Parse argc/argv style arguments with explicit error stream.
     * \param argc Argument count
     * \param argv Argument vector
     * \param err Output stream for error messages
     * \return ParsedArgs containing the parsed options and positional arguments
     */
    ParsedArgs<OptGroup> parse(int argc, const char* argv[], std::ostream& err) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return parse(args, err);
    }

private:
    /// \cond INTERNAL
    /**
     * \brief Parse an option and store it in the correct tuple position.
     * \param parsed The ParsedArgs structure to populate
     * \param optName The option name to parse
     * \param args All arguments
     * \param i Current argument index (may be modified)
     */
    void parseOptionIntoTuple(ParsedArgs<OptGroup>& parsed, std::string_view optName,
                             const std::vector<std::string>& args, size_t& i) const {
        // Use index_sequence to iterate through all options
        parseOptionIntoTupleImpl(parsed, optName, args, i, 
                                std::make_index_sequence<OptGroup::num_options>{});
    }
    
    template<size_t... Is>
    void parseOptionIntoTupleImpl(ParsedArgs<OptGroup>& parsed, std::string_view optName,
                                 const std::vector<std::string>& args, size_t& i,
                                 std::index_sequence<Is...>) const {
        // Try each option index until we find the matching one
        (void)((tryParseOption<Is>(parsed, optName, args, i)) || ...);
    }
    
    template<size_t I>
    bool tryParseOption(ParsedArgs<OptGroup>& parsed, std::string_view optName,
                       const std::vector<std::string>& args, size_t& i) const {
        const auto& opt = std::get<I>(m_spec.optionGroup.options);
        if (opt.name != optName) {
            return false; // Not this option, keep looking
        }
        
        // Found the matching option - parse it based on its type
        using OptType = std::decay_t<decltype(opt)>;
        auto& typedValue = std::get<I>(parsed.options);
        
        if constexpr (std::is_same_v<OptType, IntOption>) {
            // Single integer
            if (i + 1 < args.size()) {
                std::string optValue = args[i + 1];
                ++i;
                auto parsedValue = parseInt(optValue);
                if (parsedValue && opt.isValid(*parsedValue)) {
                    typedValue.set(*parsedValue);
                }
            }
        } else if constexpr (std::is_same_v<OptType, StringOption>) {
            // Single string
            if (i + 1 < args.size()) {
                typedValue.set(args[i + 1]);
                ++i;
            }
        } else if constexpr (std::is_same_v<OptType, IntArrayOption>) {
            // Array of integers
            std::vector<int64_t> arr;
            while (i + 1 < args.size() && !isOption(args[i + 1])) {
                ++i;
                if (auto intVal = parseInt(args[i])) {
                    if (opt.isValid(*intVal)) {
                        arr.push_back(*intVal);
                    }
                }
            }
            typedValue.set(std::move(arr));
        } else if constexpr (std::is_same_v<OptType, StringArrayOption>) {
            // Array of strings
            std::vector<std::string> arr;
            while (i + 1 < args.size() && !isOption(args[i + 1])) {
                ++i;
                arr.push_back(args[i]);
            }
            typedValue.set(std::move(arr));
        }
        
        return true; // Found and processed
    }
    /// \endcond

public:

    /**
     * \brief Invoke the handler with parsed arguments.
     *
     * Uses the stored OutputContext.
     *
     * \param parsed The pre-parsed arguments
     * \return true if the handler executed successfully, false otherwise
     */
    bool invoke(const ParsedArgs<OptGroup>& parsed) const {
        return invoke(parsed, m_context.output(), m_context.error());
    }

    /**
     * \brief Invoke the handler with explicit streams.
     * \param parsed The pre-parsed arguments
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if the handler executed successfully, false otherwise
     */
    bool invoke(const ParsedArgs<OptGroup>& parsed, std::ostream& out, std::ostream& err) const {
        if constexpr (HandlerAcceptsStreams<OptGroup, HandlerType>::value) {
            return m_handler(parsed, out, err);
        } else {
            (void)out; (void)err;  // Unused for legacy handlers
            return m_handler(parsed);
        }
    }

private:
    const CommandSpec<OptGroup>& m_spec;  ///< Command specification
    HandlerType m_handler;                 ///< Handler function
    OutputContext m_context;               ///< Output context for streams
};

/**
 * \brief Create a Command from a specification and handler.
 * \ingroup public_api
 *
 * Factory function that deduces HandlerType automatically from the
 * lambda/function passed.
 *
 * \tparam OptGroup The option group type (deduced from spec)
 * \tparam HandlerType The handler callable type (deduced)
 * \param spec The command specification
 * \param handler The handler function
 * \return shared_ptr to the created Command
 *
 * \par Example:
 * \code
 * constexpr auto spec = CommandSpec("test", "Test command",
 *     makeOptions(IntOption{"count", "Number of iterations"}));
 *
 * auto cmd = makeCommand(spec, [](const auto& args) {
 *     // Handle command
 *     return true;
 * });
 * \endcode
 */
template<typename OptGroup, typename HandlerType>
auto makeCommand(const CommandSpec<OptGroup>& spec, HandlerType handler) {
    return std::make_shared<Command<OptGroup, HandlerType>>(spec, handler);
}

/**
 * \brief Create a Command with a pre-configured OutputContext.
 * \ingroup public_api
 *
 * \tparam OptGroup The option group type (deduced from spec)
 * \tparam HandlerType The handler callable type (deduced)
 * \param spec The command specification
 * \param handler The handler function
 * \param ctx The OutputContext for output redirection
 * \return shared_ptr to the created Command
 *
 * \par Example:
 * \code
 * std::stringstream out, err;
 * OutputContext ctx(out, err);
 *
 * auto cmd = makeCommand(spec, handler, ctx);
 * cmd->execute({"--value", "42"});  // Output goes to stringstreams
 * \endcode
 */
template<typename OptGroup, typename HandlerType>
auto makeCommand(const CommandSpec<OptGroup>& spec, HandlerType handler, const OutputContext& ctx) {
    auto cmd = std::make_shared<Command<OptGroup, HandlerType>>(spec, handler);
    cmd->setOutputContext(ctx);
    return cmd;
}

/**
 * \brief Create a named option group from typed options.
 * \ingroup public_api
 *
 * \tparam Args Option types (deduced)
 * \param name The option group name
 * \param description The option group description
 * \param args The option specifications
 * \return An OptionGroup containing the options
 *
 * \par Example:
 * \code
 * constexpr auto opts = makeOptionGroup("connection", "Connection options",
 *     StringOption{"host", "Server hostname"},
 *     IntOption{"port", "Server port", 1, 65535});
 * \endcode
 */
template<typename... Args>
constexpr auto makeOptionGroup(std::string_view name, std::string_view description, Args... args) {
    return OptionGroup<Args...>{name, description, args...};
}

/**
 * \brief Create an anonymous option group from typed options.
 * \ingroup public_api
 *
 * Convenience function for backward compatibility when option group
 * name and description are not needed.
 *
 * \tparam Args Option types (deduced)
 * \param args The option specifications
 * \return An OptionGroup containing the options
 *
 * \par Example:
 * \code
 * constexpr auto opts = makeOptions(
 *     StringOption{"name", "User name"},
 *     IntOption{"age", "User age", 0, 150});
 * \endcode
 */
template<typename... Args>
constexpr auto makeOptions(Args... args) {
    return OptionGroup<Args...>{"", "", args...};
}

/**
 * \brief Manages multiple subcommands under a parent command.
 * \ingroup public_api
 *
 * SubcommandDispatcher provides a way to organize related commands
 * under a single parent, similar to how `git` has `git add`, `git commit`, etc.
 *
 * Features:
 * - Partial command matching with ambiguity detection
 * - Help query syntax (`?` and `prefix?`)
 * - Built-in help command support
 *
 * \par Example:
 * \code
 * auto dispatcher = makeDispatcher("git", "Git version control");
 *
 * auto addCmd = makeCommand(addSpec, addHandler);
 * auto commitCmd = makeCommand(commitSpec, commitHandler);
 *
 * dispatcher->addSubcommand(addCmd);
 * dispatcher->addSubcommand(commitCmd);
 *
 * // Execute: "add file.cpp" or "commit --message 'fix bug'"
 * dispatcher->execute({"add", "file.cpp"});
 * \endcode
 *
 * \see makeDispatcher() for convenient construction
 */
class SubcommandDispatcher {
public:
    /// \brief Map of subcommand name to command pointer
    using SubcommandMap = std::map<std::string, std::shared_ptr<void>>;
    /// \brief Handler function signature for subcommands
    using SubcommandHandler = std::function<bool(const std::vector<std::string>&, std::ostream&, std::ostream&)>;

    /**
     * \brief Construct a SubcommandDispatcher.
     * \param name The dispatcher name (parent command name)
     * \param description Description of the dispatcher
     */
    SubcommandDispatcher(std::string_view name, std::string_view description = "")
        : m_name(name), m_description(description) {}

    /**
     * \brief Set the output context for this dispatcher.
     * \param ctx The OutputContext containing output and error streams
     */
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }

    /**
     * \brief Set the output context using separate streams.
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     */
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }

    /**
     * \brief Get the current output context.
     * \return Reference to the OutputContext
     */
    const OutputContext& getOutputContext() const { return m_context; }

    /**
     * \brief Add a subcommand to this dispatcher.
     *
     * \tparam OptGroup The command's option group type
     * \tparam HandlerType The command's handler type
     * \param cmd The command to add as a subcommand
     *
     * \par Example:
     * \code
     * auto cmd = makeCommand(spec, handler);
     * dispatcher->addSubcommand(cmd);
     * \endcode
     */
    template<typename OptGroup, typename HandlerType>
    void addSubcommand(std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        std::string cmdName(cmd->getName());
        m_subcommands[cmdName] = cmd;
        m_handlers[cmdName] = [cmd](const std::vector<std::string>& args,
                                     std::ostream& out, std::ostream& err) {
            return cmd->execute(args, out, err);
        };
    }

private:
    /// \cond INTERNAL
    /**
     * \brief Find a command by name with partial matching support.
     * \param name The command name (or prefix) to find
     * \param err Error stream for ambiguity messages
     * \return Iterator to the found command, or end() if not found
     */
    std::map<std::string, SubcommandHandler>::const_iterator findCommand(const std::string& name, std::ostream& err) const {
        // Try exact match first
        auto it = m_handlers.find(name);
        if (it != m_handlers.end()) return it;

        // Try partial match
        std::vector<decltype(m_handlers.begin())> matches;
        for (auto iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
            if (iter->first.compare(0, name.length(), name) == 0) {
                matches.push_back(iter);
            }
        }

        if (matches.size() == 1) return matches[0];

        if (matches.size() > 1) {
            err << "Ambiguous command '" << name << "'. Did you mean:\n";
            for (const auto& match : matches) {
                err << "  " << match->first << "\n";
            }
        }
        return m_handlers.end();
    }
    /// \endcond

public:
    /**
     * \brief Execute with subcommand dispatch.
     *
     * Uses the stored OutputContext. The first argument should be the
     * subcommand name, followed by arguments for that subcommand.
     *
     * \param args Arguments where args[0] is the subcommand name
     * \return true if the subcommand executed successfully, false otherwise
     */
    bool execute(const std::vector<std::string>& args) {
        return execute(args, m_context.output(), m_context.error());
    }

    /**
     * \brief Execute with explicit output streams.
     *
     * Supports:
     * - `?` to show all subcommands
     * - `prefix?` to show matching subcommands
     * - `help` or `--help` or `-h` for help
     * - Partial command matching
     *
     * \param args Arguments where args[0] is the subcommand name
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if the subcommand executed successfully, false otherwise
     */
    bool execute(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
        if (args.empty()) {
            showHelp(out);
            return false;
        }

        std::string subcmdName = args[0];

        // Handle ? query syntax: "?" shows all, "prefix?" shows matching commands
        if (!subcmdName.empty() && subcmdName.back() == '?') {
            if (subcmdName.length() == 1) {
                // Just "?" - show all subcommands
                showHelp(out);
            } else {
                // "prefix?" - show matching subcommands
                std::string prefix = subcmdName.substr(0, subcmdName.length() - 1);
                showMatchingCommands(prefix, out);
            }
            return true;
        }

        // Special commands
        if (subcmdName == "help" || subcmdName == "--help" || subcmdName == "-h") {
            if (args.size() > 1) {
                return showSubcommandHelp(args[1], out, err);
            }
            showHelp(out);
            return true;
        }

        // Find and execute subcommand (with partial matching support)
        auto it = findCommand(subcmdName, err);
        if (it != m_handlers.end()) {
            // Pass remaining args to subcommand
            std::vector<std::string> subcmdArgs(args.begin() + 1, args.end());
            return it->second(subcmdArgs, out, err);
        }

        err << "Unknown subcommand: " << subcmdName << "\n";
        err << "Run '" << m_name << " help' for usage.\n";
        return false;
    }

    /**
     * \brief Execute with argc/argv style arguments.
     *
     * Uses the stored OutputContext.
     *
     * \param argc Argument count
     * \param argv Argument vector (argv[0] should be subcommand name)
     * \return true if the subcommand executed successfully, false otherwise
     */
    bool execute(int argc, char* argv[]) {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    /**
     * \brief Execute with argc/argv and explicit streams.
     * \param argc Argument count
     * \param argv Argument vector
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return true if the subcommand executed successfully, false otherwise
     */
    bool execute(int argc, char* argv[], std::ostream& out, std::ostream& err) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }

    /**
     * \brief Show subcommands matching a prefix.
     *
     * Uses the stored OutputContext.
     *
     * \param prefix The prefix to match against subcommand names
     */
    void showMatchingCommands(const std::string& prefix) const {
        showMatchingCommands(prefix, m_context.output());
    }

    /**
     * \brief Show subcommands matching a prefix with explicit stream.
     * \param prefix The prefix to match against subcommand names
     * \param out Output stream to write to
     */
    void showMatchingCommands(const std::string& prefix, std::ostream& out) const {
        std::vector<std::string> matches;
        for (const auto& [name, _] : m_handlers) {
            if (name.compare(0, prefix.length(), prefix) == 0) {
                matches.push_back(name);
            }
        }

        if (matches.empty()) {
            out << "No subcommands matching '" << prefix << "'\n";
        } else {
            out << "Subcommands matching '" << prefix << "':\n";
            for (const auto& name : matches) {
                out << "  " << name << "\n";
            }
        }
    }

    /**
     * \brief Show help for all subcommands.
     *
     * Uses the stored OutputContext.
     */
    void showHelp() const {
        showHelp(m_context.output());
    }

    /**
     * \brief Show help with explicit stream.
     * \param out Output stream to write help text to
     */
    void showHelp(std::ostream& out) const {
        out << m_name << ": " << m_description << "\n\n";
        out << "Available subcommands:\n";
        for (const auto& [name, _] : m_handlers) {
            out << "  " << name << "\n";
        }
        out << "\nUse '" << m_name << " help <subcommand>' for more information.\n";
    }

    /**
     * \brief Show help for a specific subcommand.
     *
     * Uses the stored OutputContext.
     *
     * \param subcmdName The subcommand to show help for
     * \return true if subcommand exists, false otherwise
     */
    bool showSubcommandHelp(const std::string& subcmdName) const {
        return showSubcommandHelp(subcmdName, m_context.output(), m_context.error());
    }

    /**
     * \brief Show help for a specific subcommand with explicit streams.
     * \param subcmdName The subcommand to show help for
     * \param out Output stream for help text
     * \param err Output stream for error messages
     * \return true if subcommand exists, false otherwise
     */
    bool showSubcommandHelp(const std::string& subcmdName, std::ostream& out, std::ostream& err) const {
        auto it = m_handlers.find(subcmdName);
        if (it == m_handlers.end()) {
            err << "Unknown subcommand: " << subcmdName << "\n";
            return false;
        }

        out << "Subcommand: " << subcmdName << "\n";
        // Note: Could enhance this to show subcommand options
        return true;
    }

    /**
     * \brief Get the dispatcher name.
     * \return The name as a string_view
     */
    std::string_view getName() const { return m_name; }

    /**
     * \brief Get the dispatcher description.
     * \return The description as a string_view
     */
    std::string_view getDescription() const { return m_description; }

    /**
     * \brief Get the map of all subcommands.
     * \return Reference to the SubcommandMap
     */
    const SubcommandMap& getSubcommands() const { return m_subcommands; }

    /**
     * \brief Show hierarchical view of all subcommands.
     *
     * Uses the stored OutputContext.
     *
     * \param indent Indentation prefix for each line
     * \param showOptions Whether to show option details (reserved for future use)
     */
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        showHierarchy(m_context.output(), indent, showOptions);
    }

    /**
     * \brief Show hierarchical view with explicit stream.
     * \param out Output stream to write to
     * \param indent Indentation prefix for each line
     * \param showOptions Whether to show option details (reserved for future use)
     */
    void showHierarchy(std::ostream& out, const std::string& indent = "", bool showOptions = true) const {
        out << indent << m_name << ": " << m_description << "\n";
        out << indent << "  Subcommands:\n";

        for (const auto& [name, cmdPtr] : m_subcommands) {
            out << indent << "    " << name << "\n";
        }

        out << "\n";
        out << indent << "  Use '" << m_name << " help <subcommand>' for details on each subcommand.\n";
    }

private:
    std::string m_name;                                    ///< Dispatcher name
    std::string m_description;                             ///< Dispatcher description
    SubcommandMap m_subcommands;                           ///< Map of subcommand pointers
    std::map<std::string, SubcommandHandler> m_handlers;   ///< Map of subcommand handlers
    OutputContext m_context;                               ///< Output context for streams
};

/**
 * \brief Create a SubcommandDispatcher.
 * \ingroup public_api
 *
 * \param name The dispatcher name (parent command name)
 * \param description Description of the dispatcher
 * \return shared_ptr to the created SubcommandDispatcher
 *
 * \par Example:
 * \code
 * auto dispatcher = makeDispatcher("git", "Git version control");
 * dispatcher->addSubcommand(addCmd);
 * dispatcher->addSubcommand(commitCmd);
 * \endcode
 */
inline auto makeDispatcher(std::string_view name, std::string_view description = "") {
    return std::make_shared<SubcommandDispatcher>(name, description);
}

/**
 * \brief Create a SubcommandDispatcher with a pre-configured OutputContext.
 * \ingroup public_api
 *
 * \param name The dispatcher name (parent command name)
 * \param description Description of the dispatcher
 * \param ctx The OutputContext for output redirection
 * \return shared_ptr to the created SubcommandDispatcher
 */
inline auto makeDispatcher(std::string_view name, std::string_view description, const OutputContext& ctx) {
    auto disp = std::make_shared<SubcommandDispatcher>(name, description);
    disp->setOutputContext(ctx);
    return disp;
}

/**
 * \brief Main interface for interactive command-line applications.
 * \ingroup public_api
 *
 * CLI manages modes, command dispatch, and mode transitions. Each mode
 * can have its own handler that processes commands and optionally
 * transitions to a different mode.
 *
 * \note Formerly known as ModeManager. The alias ModeManager is still available
 *       for backward compatibility.
 *
 * \par Features:
 * - Mode-based command routing
 * - Partial mode matching with ambiguity detection
 * - Help query syntax (`mode ?` and `mode prefix?`)
 * - Built-in `exit`/`quit` commands
 * - Support for both legacy and stream-aware handlers
 *
 * \par Example:
 * \code
 * auto cli = makeCLI();
 *
 * // Add a mode with stream-aware handler
 * cli->addMode("main", [](const std::vector<std::string>& args,
 *                         std::ostream& out, std::ostream& err) -> std::string {
 *     if (args[0] == "hello") {
 *         out << "Hello, World!\n";
 *     }
 *     return "";  // Stay in current mode
 * });
 *
 * // Add a SubcommandDispatcher as a mode
 * cli->addMode("git", gitDispatcher);
 *
 * // Execute commands
 * cli->execute({"hello"});           // In default mode
 * cli->execute({"mode", "git"});     // Switch to git mode
 * cli->execute({"status"});          // Runs git status
 * \endcode
 *
 * \see makeCLI() for convenient construction
 */
class CLI {
public:
    /**
     * \brief Stream-aware mode handler type.
     *
     * Returns the next mode name (empty string to stay in current mode,
     * "exit" to signal exit).
     */
    using ModeHandler = std::function<std::string(const std::vector<std::string>&, std::ostream&, std::ostream&)>;

    /**
     * \brief Legacy mode handler type without streams.
     */
    using LegacyModeHandler = std::function<std::string(const std::vector<std::string>&)>;

    /**
     * \brief Construct a CLI with "default" as the initial mode.
     */
    CLI() : m_currentMode("default") {}

    /**
     * \brief Set the output context for this CLI.
     * \param ctx The OutputContext containing output and error streams
     */
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }

    /**
     * \brief Set the output context using separate streams.
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     */
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }

    /**
     * \brief Get the current output context.
     * \return Reference to the OutputContext
     */
    const OutputContext& getOutputContext() const { return m_context; }

    /**
     * \brief Register a mode with a stream-aware handler.
     *
     * The handler receives arguments and output streams, and returns
     * the name of the next mode (or empty string to stay).
     *
     * \param modeName The mode name
     * \param handler The mode handler function
     */
    void addMode(std::string_view modeName, ModeHandler handler) {
        m_modes[std::string(modeName)] = handler;
    }

    /**
     * \brief Register a mode with a legacy handler.
     *
     * The handler is wrapped to ignore streams.
     *
     * \param modeName The mode name
     * \param handler The legacy mode handler function
     */
    void addMode(std::string_view modeName, LegacyModeHandler handler) {
        m_modes[std::string(modeName)] = [handler](const std::vector<std::string>& args,
                                                    std::ostream&, std::ostream&) -> std::string {
            return handler(args);
        };
    }

    /**
     * \brief Register a SubcommandDispatcher as a mode.
     *
     * Commands in this mode will be dispatched through the dispatcher.
     *
     * \param modeName The mode name
     * \param dispatcher The SubcommandDispatcher to use
     */
    void addMode(std::string_view modeName, std::shared_ptr<SubcommandDispatcher> dispatcher) {
        m_modes[std::string(modeName)] = [dispatcher](const std::vector<std::string>& args,
                                                       std::ostream& out,
                                                       std::ostream& err) -> std::string {
            dispatcher->execute(args, out, err);
            return ""; // Stay in current mode
        };
    }

    /**
     * \brief Register a Command as a mode.
     *
     * Commands in this mode will be executed by the command.
     *
     * \tparam OptGroup The command's option group type
     * \tparam HandlerType The command's handler type
     * \param modeName The mode name
     * \param cmd The Command to use
     */
    template<typename OptGroup, typename HandlerType>
    void addMode(std::string_view modeName, std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        m_modes[std::string(modeName)] = [cmd](const std::vector<std::string>& args,
                                                std::ostream& out,
                                                std::ostream& err) -> std::string {
            cmd->execute(args, out, err);
            return ""; // Stay in current mode
        };
    }

private:
    /// \cond INTERNAL
    /**
     * \brief Find a mode by name with partial matching support.
     * \param name The mode name (or prefix) to find
     * \param err Error stream for ambiguity messages
     * \return Iterator to the found mode, or end() if not found
     */
    std::map<std::string, ModeHandler>::const_iterator findMode(const std::string& name, std::ostream& err) const {
        auto it = m_modes.find(name);
        if (it != m_modes.end()) return it;

        std::vector<decltype(m_modes.begin())> matches;
        for (auto iter = m_modes.begin(); iter != m_modes.end(); ++iter) {
            if (iter->first.compare(0, name.length(), name) == 0) {
                matches.push_back(iter);
            }
        }

        if (matches.size() == 1) return matches[0];

        if (matches.size() > 1) {
            err << "Ambiguous mode '" << name << "'. Did you mean:\n";
            for (const auto& match : matches) {
                err << "  " << match->first << "\n";
            }
        }
        return m_modes.end();
    }
    /// \endcond

public:
    /**
     * \brief Execute a command in the current mode.
     *
     * Uses the stored OutputContext. Handles built-in commands:
     * - `mode` - Show or switch modes
     * - `mode ?` - Show all modes
     * - `mode prefix?` - Show matching modes
     * - `exit`/`quit` - Signal exit
     *
     * \param args Command arguments
     * \return Next mode name, "exit" to signal exit, or empty to stay
     */
    std::string execute(const std::vector<std::string>& args) {
        return execute(args, m_context.output(), m_context.error());
    }

    /**
     * \brief Execute a command with explicit output streams.
     * \param args Command arguments
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return Next mode name, "exit" to signal exit, or empty to stay
     */
    std::string execute(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
        if (args.empty()) {
            return "";
        }

        std::string cmd = args[0];

        // Handle ? query syntax for modes: "mode ?" or "mode prefix?"
        if (cmd == "mode" && args.size() > 1 && !args[1].empty() && args[1].back() == '?') {
            std::string query = args[1];
            if (query.length() == 1) {
                // "mode ?" - show all modes
                out << "Available modes:\n";
                for (const auto& [name, _] : m_modes) {
                    out << "  " << name << "\n";
                }
            } else {
                // "mode prefix?" - show matching modes
                std::string prefix = query.substr(0, query.length() - 1);
                std::vector<std::string> matches;
                for (const auto& [name, _] : m_modes) {
                    if (name.compare(0, prefix.length(), prefix) == 0) {
                        matches.push_back(name);
                    }
                }
                if (matches.empty()) {
                    out << "No modes matching '" << prefix << "'\n";
                } else {
                    out << "Modes matching '" << prefix << "':\n";
                    for (const auto& name : matches) {
                        out << "  " << name << "\n";
                    }
                }
            }
            return "";
        }

        // Check for mode transition commands
        if (cmd == "exit" || cmd == "quit") {
            return "exit";
        }

        if (cmd == "mode") {
            if (args.size() > 1) {
                std::string newMode = args[1];
                auto it = findMode(newMode, err);
                if (it != m_modes.end()) {
                    m_currentMode = it->first;
                    out << "Switched to mode: " << it->first << "\n";
                    return it->first;
                } else if (m_modes.find(newMode) == m_modes.end() &&
                          std::count_if(m_modes.begin(), m_modes.end(),
                              [&](const auto& p) { return p.first.compare(0, newMode.length(), newMode) == 0; }) == 0) {
                    err << "Unknown mode: " << newMode << "\n";
                    return "";
                } else {
                    // Ambiguous match already reported by findMode
                    return "";
                }
            } else {
                out << "Current mode: " << m_currentMode << "\n";
                out << "Available modes:\n";
                for (const auto& [name, _] : m_modes) {
                    out << "  " << name << "\n";
                }
                return "";
            }
        }

        // Execute in current mode
        auto it = m_modes.find(m_currentMode);
        if (it != m_modes.end()) {
            std::string nextMode = it->second(args, out, err);
            if (!nextMode.empty() && nextMode != "exit") {
                m_currentMode = nextMode;
            }
            return nextMode;
        }

        err << "No handler for mode: " << m_currentMode << "\n";
        return "";
    }

    /**
     * \brief Execute with argc/argv style arguments.
     *
     * Uses the stored OutputContext.
     *
     * \param argc Argument count
     * \param argv Argument vector
     * \return Next mode name, "exit" to signal exit, or empty to stay
     */
    std::string execute(int argc, char* argv[]) {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    /**
     * \brief Execute with argc/argv and explicit streams.
     * \param argc Argument count
     * \param argv Argument vector
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return Next mode name, "exit" to signal exit, or empty to stay
     */
    std::string execute(int argc, char* argv[], std::ostream& out, std::ostream& err) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }

    /**
     * \brief Execute a command string.
     *
     * Tokenizes the string by whitespace and executes.
     * Uses the stored OutputContext.
     *
     * \param commandLine The command string to parse and execute
     * \return Next mode name, "exit" to signal exit, or empty to stay
     *
     * \par Example:
     * \code
     * cli->executeCommand("mode git");
     * cli->executeCommand("status --verbose");
     * \endcode
     */
    std::string executeCommand(std::string_view commandLine) {
        return executeCommand(commandLine, m_context.output(), m_context.error());
    }

    /**
     * \brief Execute a command string with explicit streams.
     * \param commandLine The command string to parse and execute
     * \param out Output stream for normal output
     * \param err Output stream for error messages
     * \return Next mode name, "exit" to signal exit, or empty to stay
     */
    std::string executeCommand(std::string_view commandLine, std::ostream& out, std::ostream& err) {
        if (commandLine.empty()) {
            return "";
        }

        // Simple tokenization (split by whitespace)
        std::vector<std::string> args;
        std::istringstream iss{std::string(commandLine)};
        std::string token;
        while (iss >> token) {
            args.push_back(token);
        }

        return execute(args, out, err);
    }

    /**
     * \brief Get the current mode name.
     * \return The current mode name as a string_view
     */
    std::string_view getCurrentMode() const { return m_currentMode; }

    /**
     * \brief Set the current mode directly.
     *
     * Does not invoke any handler, just changes the current mode.
     *
     * \param modeName The mode to switch to
     * \return true if the mode exists and was set, false otherwise
     */
    bool setMode(std::string_view modeName) {
        auto it = m_modes.find(std::string(modeName));
        if (it != m_modes.end()) {
            m_currentMode = modeName;
            return true;
        }
        return false;
    }

    /**
     * \brief Check if a mode exists.
     * \param modeName The mode name to check
     * \return true if the mode exists, false otherwise
     */
    bool hasMode(std::string_view modeName) const {
        return m_modes.find(std::string(modeName)) != m_modes.end();
    }

    /**
     * \brief Get all registered mode names.
     * \return Vector of mode names
     */
    std::vector<std::string> getModes() const {
        std::vector<std::string> result;
        for (const auto& [name, _] : m_modes) {
            result.push_back(name);
        }
        return result;
    }

    /**
     * \brief Show hierarchical view of all modes.
     *
     * Uses the stored OutputContext.
     *
     * \param showOptions Reserved for future use
     */
    void showHierarchy(bool showOptions = true) const {
        showHierarchy(m_context.output(), showOptions);
    }

    /**
     * \brief Show hierarchical view with explicit stream.
     * \param out Output stream to write to
     * \param showOptions Reserved for future use
     */
    void showHierarchy(std::ostream& out, bool showOptions = true) const {
        out << "Mode Manager Hierarchy\n";
        out << "======================\n\n";
        out << "Current mode: " << m_currentMode << "\n\n";
        out << "Available modes:\n";
        for (const auto& [name, _] : m_modes) {
            out << "  - " << name;
            if (name == m_currentMode) {
                out << " (current)";
            }
            out << "\n";
        }
        out << "\nUse 'mode <name>' to switch modes\n";
        out << "Use 'exit' or 'quit' to exit\n";
    }

private:
    std::string m_currentMode;                     ///< Current active mode name
    std::map<std::string, ModeHandler> m_modes;    ///< Map of mode names to handlers
    OutputContext m_context;                       ///< Output context for streams
};

/**
 * \brief Create a CLI instance.
 * \ingroup public_api
 *
 * \return shared_ptr to the created CLI
 *
 * \par Example:
 * \code
 * auto cli = makeCLI();
 * cli->addMode("main", mainHandler);
 * cli->execute({"command", "arg"});
 * \endcode
 */
inline auto makeCLI() {
    return std::make_shared<CLI>();
}

/**
 * \brief Create a CLI with a pre-configured OutputContext.
 * \ingroup public_api
 *
 * \param ctx The OutputContext for output redirection
 * \return shared_ptr to the created CLI
 *
 * \par Example:
 * \code
 * std::stringstream out, err;
 * OutputContext ctx(out, err);
 * auto cli = makeCLI(ctx);
 * \endcode
 */
inline auto makeCLI(const OutputContext& ctx) {
    auto cli = std::make_shared<CLI>();
    cli->setOutputContext(ctx);
    return cli;
}

/**
 * \brief Legacy alias for CLI.
 * \ingroup public_api
 * \deprecated Use CLI instead.
 */
using ModeManager = CLI;

/**
 * \brief Legacy factory function for CLI.
 * \ingroup public_api
 * \deprecated Use makeCLI() instead.
 * \return shared_ptr to the created CLI
 */
inline auto makeModeManager() { return makeCLI(); }

/**
 * \brief Legacy factory function for CLI with OutputContext.
 * \ingroup public_api
 * \deprecated Use makeCLI(ctx) instead.
 * \param ctx The OutputContext for output redirection
 * \return shared_ptr to the created CLI
 */
inline auto makeModeManager(const OutputContext& ctx) { return makeCLI(ctx); }

} // namespace cmdline_ct

#endif // CMDLINE_COMPONENTS_H
