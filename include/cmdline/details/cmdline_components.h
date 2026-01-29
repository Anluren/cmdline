/**
 * Compile-time Command Line Library - Components
 * Command, SubcommandDispatcher, and ModeManager implementations
 */

#ifndef CMDLINE_COMPONENTS_H
#define CMDLINE_COMPONENTS_H

#include "cmdline_types.h"

namespace cmdline_ct {

/**
 * Type trait to detect if handler accepts output streams
 * Used to support both legacy bool(ParsedArgs) and new bool(ParsedArgs, ostream&, ostream&) signatures
 */
template<typename OptGroup, typename Callable, typename = void>
struct HandlerAcceptsStreams : std::false_type {};

template<typename OptGroup, typename Callable>
struct HandlerAcceptsStreams<OptGroup, Callable,
    std::void_t<decltype(std::declval<Callable>()(
        std::declval<const ParsedArgs<OptGroup>&>(),
        std::declval<std::ostream&>(),
        std::declval<std::ostream&>()
    ))>> : std::true_type {};

/**
 * CommandHandler - Lightweight template wrapper for command handlers
 * Avoids std::function overhead by storing the callable directly
 * Supports both legacy and stream-aware handler signatures
 */
template<typename OptGroup, typename Callable>
struct CommandHandler {
    Callable callable;

    constexpr CommandHandler(Callable c) : callable(std::move(c)) {}

    // New call operator with streams - dispatches based on handler signature
    bool operator()(const ParsedArgs<OptGroup>& args,
                    std::ostream& out, std::ostream& err) const {
        if constexpr (HandlerAcceptsStreams<OptGroup, Callable>::value) {
            return callable(args, out, err);
        } else {
            (void)out; (void)err;  // Unused for legacy handlers
            return callable(args);
        }
    }

    // Legacy call operator for backward compatibility
    constexpr bool operator()(const ParsedArgs<OptGroup>& args) const {
        return callable(args);
    }
};

// Helper to create CommandHandler with automatic type deduction
template<typename OptGroup, typename Callable>
constexpr auto makeCommandHandler(Callable&& c) {
    return CommandHandler<OptGroup, std::decay_t<Callable>>{std::forward<Callable>(c)};
}

template<typename OptGroup, typename HandlerType>
class Command {
    // Static assertion to ensure HandlerType is callable with at least the legacy signature
    // Handlers can use either: bool(ParsedArgs) or bool(ParsedArgs, ostream&, ostream&)
    static_assert(std::is_invocable_r_v<bool, HandlerType, const ParsedArgs<OptGroup>&> ||
                  std::is_invocable_r_v<bool, HandlerType, const ParsedArgs<OptGroup>&, std::ostream&, std::ostream&>,
                  "HandlerType must be callable with signature: bool(const ParsedArgs<OptGroup>&) or bool(const ParsedArgs<OptGroup>&, std::ostream&, std::ostream&)");

public:
    constexpr Command(const CommandSpec<OptGroup>& spec, HandlerType handler)
        : m_spec(spec), m_handler(handler) {}

    const CommandSpec<OptGroup>& getSpec() const { return m_spec; }
    constexpr std::string_view getName() const { return m_spec.name; }
    constexpr std::string_view getDescription() const { return m_spec.description; }

    // Output context management
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }
    const OutputContext& getOutputContext() const { return m_context; }
    
    // Show hierarchical view of command with all options (uses stored context)
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        showHierarchy(m_context.output(), indent, showOptions);
    }

    // Show hierarchical view with explicit output stream
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
    
    // Helper to check if a string is an option (either '--name' or a known option name)
    bool isOption(const std::string& arg) const {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
            return m_spec.hasOption(arg.substr(2));
        }
        return m_spec.hasOption(arg);
    }
    
    // Execute: parse arguments then invoke handler (uses stored context)
    bool execute(const std::vector<std::string>& args) const {
        return execute(args, m_context.output(), m_context.error());
    }

    // Execute with explicit output streams
    bool execute(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) const {
        ParsedArgs<OptGroup> parsed = parse(args, err);
        if (!parsed.parseSuccess) {
            return false;  // Don't execute if parsing failed
        }
        return invoke(parsed, out, err);
    }

    // Execute with argc/argv style arguments (uses stored context)
    bool execute(int argc, char* argv[]) const {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    // Execute with argc/argv and explicit streams
    bool execute(int argc, char* argv[], std::ostream& out, std::ostream& err) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }
    
    // Parse arguments into ParsedArgs structure (uses stored context for errors)
    ParsedArgs<OptGroup> parse(const std::vector<std::string>& args) const {
        return parse(args, m_context.error());
    }

    // Parse arguments with explicit error stream
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

    // Parse with argc/argv style arguments (uses stored context)
    ParsedArgs<OptGroup> parse(int argc, const char* argv[]) const {
        return parse(argc, argv, m_context.error());
    }

    // Parse with argc/argv and explicit error stream
    ParsedArgs<OptGroup> parse(int argc, const char* argv[], std::ostream& err) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return parse(args, err);
    }

private:
    // Helper to parse an option and store it in the correct tuple position
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

public:

    // Invoke handler with parsed arguments (uses stored context)
    bool invoke(const ParsedArgs<OptGroup>& parsed) const {
        return invoke(parsed, m_context.output(), m_context.error());
    }

    // Invoke handler with explicit streams
    bool invoke(const ParsedArgs<OptGroup>& parsed, std::ostream& out, std::ostream& err) const {
        if constexpr (HandlerAcceptsStreams<OptGroup, HandlerType>::value) {
            return m_handler(parsed, out, err);
        } else {
            (void)out; (void)err;  // Unused for legacy handlers
            return m_handler(parsed);
        }
    }

private:
    const CommandSpec<OptGroup>& m_spec;
    HandlerType m_handler;
    OutputContext m_context;
};

// Helper function to create commands from constexpr specs
// Deduces HandlerType automatically from the lambda/function passed
template<typename OptGroup, typename HandlerType>
auto makeCommand(const CommandSpec<OptGroup>& spec, HandlerType handler) {
    return std::make_shared<Command<OptGroup, HandlerType>>(spec, handler);
}

// Helper to create command with output context
template<typename OptGroup, typename HandlerType>
auto makeCommand(const CommandSpec<OptGroup>& spec, HandlerType handler, const OutputContext& ctx) {
    auto cmd = std::make_shared<Command<OptGroup, HandlerType>>(spec, handler);
    cmd->setOutputContext(ctx);
    return cmd;
}

// Helper to create option group from typed options
template<typename... Args>
constexpr auto makeOptionGroup(std::string_view name, std::string_view description, Args... args) {
    return OptionGroup<Args...>{name, description, args...};
}

// Convenience helper to create anonymous option group (for backward compatibility)
template<typename... Args>
constexpr auto makeOptions(Args... args) {
    return OptionGroup<Args...>{"", "", args...};
}

/**
 * Subcommand dispatcher
 * Manages multiple subcommands under a parent command
 */
class SubcommandDispatcher {
public:
    using SubcommandMap = std::map<std::string, std::shared_ptr<void>>;
    using SubcommandHandler = std::function<bool(const std::vector<std::string>&, std::ostream&, std::ostream&)>;

    SubcommandDispatcher(std::string_view name, std::string_view description = "")
        : m_name(name), m_description(description) {}

    // Output context management
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }
    const OutputContext& getOutputContext() const { return m_context; }

    // Add a subcommand
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
    // Helper: Find command with explicit error stream
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
    
public:
    // Execute with subcommand dispatch (uses stored context)
    bool execute(const std::vector<std::string>& args) {
        return execute(args, m_context.output(), m_context.error());
    }

    // Execute with explicit output streams
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

    // Execute with argc/argv style arguments (uses stored context)
    bool execute(int argc, char* argv[]) {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    // Execute with argc/argv and explicit streams
    bool execute(int argc, char* argv[], std::ostream& out, std::ostream& err) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }
    
    // Show subcommands matching a prefix (uses stored context)
    void showMatchingCommands(const std::string& prefix) const {
        showMatchingCommands(prefix, m_context.output());
    }

    // Show subcommands matching a prefix with explicit stream
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

    // Show help for all subcommands (uses stored context)
    void showHelp() const {
        showHelp(m_context.output());
    }

    // Show help with explicit stream
    void showHelp(std::ostream& out) const {
        out << m_name << ": " << m_description << "\n\n";
        out << "Available subcommands:\n";
        for (const auto& [name, _] : m_handlers) {
            out << "  " << name << "\n";
        }
        out << "\nUse '" << m_name << " help <subcommand>' for more information.\n";
    }

    // Show help for specific subcommand (uses stored context)
    bool showSubcommandHelp(const std::string& subcmdName) const {
        return showSubcommandHelp(subcmdName, m_context.output(), m_context.error());
    }

    // Show help for specific subcommand with explicit streams
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

    std::string_view getName() const { return m_name; }
    std::string_view getDescription() const { return m_description; }

    const SubcommandMap& getSubcommands() const { return m_subcommands; }

    // Show hierarchical view of all subcommands (uses stored context)
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        showHierarchy(m_context.output(), indent, showOptions);
    }

    // Show hierarchical view with explicit stream
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
    std::string m_name;
    std::string m_description;
    SubcommandMap m_subcommands;
    std::map<std::string, SubcommandHandler> m_handlers;
    OutputContext m_context;
};

// Helper to create subcommand dispatcher
inline auto makeDispatcher(std::string_view name, std::string_view description = "") {
    return std::make_shared<SubcommandDispatcher>(name, description);
}

// Helper to create subcommand dispatcher with output context
inline auto makeDispatcher(std::string_view name, std::string_view description, const OutputContext& ctx) {
    auto disp = std::make_shared<SubcommandDispatcher>(name, description);
    disp->setOutputContext(ctx);
    return disp;
}

/**
 * Mode manager for interactive command mode transitions
 * Allows switching between different command contexts (modes)
 */
class ModeManager {
public:
    // Handler accepts streams: returns next mode name (or empty to stay)
    using ModeHandler = std::function<std::string(const std::vector<std::string>&, std::ostream&, std::ostream&)>;
    // Legacy handler type without streams
    using LegacyModeHandler = std::function<std::string(const std::vector<std::string>&)>;

    ModeManager() : m_currentMode("default") {}

    // Output context management
    void setOutputContext(const OutputContext& ctx) { m_context = ctx; }
    void setOutputContext(std::ostream& out, std::ostream& err) {
        m_context = OutputContext(out, err);
    }
    const OutputContext& getOutputContext() const { return m_context; }

    // Register a mode with stream-aware handler
    void addMode(std::string_view modeName, ModeHandler handler) {
        m_modes[std::string(modeName)] = handler;
    }

    // Register a mode with legacy handler (wraps to ignore streams)
    void addMode(std::string_view modeName, LegacyModeHandler handler) {
        m_modes[std::string(modeName)] = [handler](const std::vector<std::string>& args,
                                                    std::ostream&, std::ostream&) -> std::string {
            return handler(args);
        };
    }

    // Register a subcommand dispatcher as a mode
    void addMode(std::string_view modeName, std::shared_ptr<SubcommandDispatcher> dispatcher) {
        m_modes[std::string(modeName)] = [dispatcher](const std::vector<std::string>& args,
                                                       std::ostream& out,
                                                       std::ostream& err) -> std::string {
            dispatcher->execute(args, out, err);
            return ""; // Stay in current mode
        };
    }

    // Register a command as a mode
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
    // Helper: Find mode with explicit error stream
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
    
public:
    // Execute command in current mode (uses stored context)
    std::string execute(const std::vector<std::string>& args) {
        return execute(args, m_context.output(), m_context.error());
    }

    // Execute with explicit output streams
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

    // Execute with argc/argv style arguments (uses stored context)
    std::string execute(int argc, char* argv[]) {
        return execute(argc, argv, m_context.output(), m_context.error());
    }

    // Execute with argc/argv and explicit streams
    std::string execute(int argc, char* argv[], std::ostream& out, std::ostream& err) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args, out, err);
    }

    // Execute a command string (uses stored context)
    std::string executeCommand(std::string_view commandLine) {
        return executeCommand(commandLine, m_context.output(), m_context.error());
    }

    // Execute a command string with explicit streams
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
    
    // Get current mode name
    std::string_view getCurrentMode() const { return m_currentMode; }
    
    // Set current mode
    bool setMode(std::string_view modeName) {
        auto it = m_modes.find(std::string(modeName));
        if (it != m_modes.end()) {
            m_currentMode = modeName;
            return true;
        }
        return false;
    }
    
    // Check if mode exists
    bool hasMode(std::string_view modeName) const {
        return m_modes.find(std::string(modeName)) != m_modes.end();
    }
    
    // Get all mode names
    std::vector<std::string> getModes() const {
        std::vector<std::string> result;
        for (const auto& [name, _] : m_modes) {
            result.push_back(name);
        }
        return result;
    }
    
    // Show hierarchical view of all modes (uses stored context)
    void showHierarchy(bool showOptions = true) const {
        showHierarchy(m_context.output(), showOptions);
    }

    // Show hierarchical view with explicit stream
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
    std::string m_currentMode;
    std::map<std::string, ModeHandler> m_modes;
    OutputContext m_context;
};

// Helper to create mode manager
inline auto makeModeManager() {
    return std::make_shared<ModeManager>();
}

// Helper to create mode manager with output context
inline auto makeModeManager(const OutputContext& ctx) {
    auto mgr = std::make_shared<ModeManager>();
    mgr->setOutputContext(ctx);
    return mgr;
}

} // namespace cmdline_ct

#endif // CMDLINE_COMPONENTS_H
