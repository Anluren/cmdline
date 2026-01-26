/**
 * Compile-time Command Line Library - Components
 * Command, SubcommandDispatcher, and ModeManager implementations
 */

#ifndef CMDLINE_COMPONENTS_H
#define CMDLINE_COMPONENTS_H

#include "cmdline_types.h"

namespace cmdline_ct {

/**
 * CommandHandler - Lightweight template wrapper for command handlers
 * Avoids std::function overhead by storing the callable directly
 */
template<typename OptGroup, typename Callable>
struct CommandHandler {
    Callable callable;
    
    constexpr CommandHandler(Callable c) : callable(std::move(c)) {}
    
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
    // Static assertion to ensure HandlerType is callable with correct signature
    static_assert(std::is_invocable_r_v<bool, HandlerType, const ParsedArgs<OptGroup>&>,
                  "HandlerType must be callable with signature: bool(const ParsedArgs<OptGroup>&)");
    
public:
    constexpr Command(const CommandSpec<OptGroup>& spec, HandlerType handler)
        : m_spec(spec), m_handler(handler) {}
    
    const CommandSpec<OptGroup>& getSpec() const { return m_spec; }
    constexpr std::string_view getName() const { return m_spec.name; }
    constexpr std::string_view getDescription() const { return m_spec.description; }
    
    // Show hierarchical view of command with all options
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        std::cout << indent << m_spec.name << ": " << m_spec.description << "\n";
        
        if (showOptions) {
            auto opts = m_spec.getAllOptions();
            if (!opts.empty()) {
                std::cout << indent << "  Options:\n";
                for (const auto& opt : opts) {
                    std::cout << indent << "    --" << opt.name << ": " << opt.description;
                    
                    // Show type information
                    if (opt.is_array) {
                        std::cout << " [array]";
                    } else if (opt.is_int) {
                        std::cout << " [int]";
                    } else {
                        std::cout << " [string]";
                    }
                    
                    // Show range if applicable
                    if (opt.min_value || opt.max_value) {
                        std::cout << " (";
                        if (opt.min_value) std::cout << "min=" << *opt.min_value;
                        if (opt.min_value && opt.max_value) std::cout << ", ";
                        if (opt.max_value) std::cout << "max=" << *opt.max_value;
                        std::cout << ")";
                    }
                    
                    // Show required flag
                    if (opt.required) {
                        std::cout << " [required]";
                    }
                    
                    std::cout << "\n";
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
    
    // Execute: parse arguments then invoke handler
    bool execute(const std::vector<std::string>& args) const {
        ParsedArgs<OptGroup> parsed = parse(args);
        if (!parsed.parseSuccess) {
            return false;  // Don't execute if parsing failed
        }
        return invoke(parsed);
    }
    
    // Execute with argc/argv style arguments (common in main function)
    bool execute(int argc, char* argv[]) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args);
    }
    
    // Parse arguments into ParsedArgs structure with type awareness
    ParsedArgs<OptGroup> parse(const std::vector<std::string>& args) const {
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
                std::cerr << "Error: Unknown option '--" << optName << "'\n";
                parsed.parseSuccess = false;
            } else {
                // Positional argument
                parsed.positional.push_back(arg);
            }
        }
        
        return parsed;
    }
    
    // Parse with argc/argv style arguments
    ParsedArgs<OptGroup> parse(int argc, const char* argv[]) const {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return parse(args);
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
    
    // Invoke handler with parsed arguments
    bool invoke(const ParsedArgs<OptGroup>& parsed) const {
        return m_handler(parsed);
    }
    
private:
    const CommandSpec<OptGroup>& m_spec;
    HandlerType m_handler;
};

// Helper function to create commands from constexpr specs
// Deduces HandlerType automatically from the lambda/function passed
template<typename OptGroup, typename HandlerType>
auto makeCommand(const CommandSpec<OptGroup>& spec, HandlerType handler) {
    return std::make_shared<Command<OptGroup, HandlerType>>(spec, handler);
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
    using SubcommandHandler = std::function<bool(const std::vector<std::string>&)>;
    
    SubcommandDispatcher(std::string_view name, std::string_view description = "")
        : m_name(name), m_description(description) {}
    
    // Add a subcommand
    template<typename OptGroup, typename HandlerType>
    void addSubcommand(std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        std::string cmdName(cmd->getName());
        m_subcommands[cmdName] = cmd;
        m_handlers[cmdName] = [cmd](const std::vector<std::string>& args) {
            return cmd->execute(args);
        };
    }
    
private:
    // Helper: Find command by exact or partial match
    auto findCommand(const std::string& name) const {
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
            std::cerr << "Ambiguous command '" << name << "'. Did you mean:\n";
            for (const auto& match : matches) {
                std::cerr << "  " << match->first << "\n";
            }
        }
        return m_handlers.end();
    }
    
public:
    // Execute with subcommand dispatch
    bool execute(const std::vector<std::string>& args) {
        if (args.empty()) {
            showHelp();
            return false;
        }
        
        std::string subcmdName = args[0];
        
        // Handle ? query syntax: "?" shows all, "prefix?" shows matching commands
        if (!subcmdName.empty() && subcmdName.back() == '?') {
            if (subcmdName.length() == 1) {
                // Just "?" - show all subcommands
                showHelp();
            } else {
                // "prefix?" - show matching subcommands
                std::string prefix = subcmdName.substr(0, subcmdName.length() - 1);
                showMatchingCommands(prefix);
            }
            return true;
        }
        
        // Special commands
        if (subcmdName == "help" || subcmdName == "--help" || subcmdName == "-h") {
            if (args.size() > 1) {
                return showSubcommandHelp(args[1]);
            }
            showHelp();
            return true;
        }
        
        // Find and execute subcommand (with partial matching support)
        auto it = findCommand(subcmdName);
        if (it != m_handlers.end()) {
            // Pass remaining args to subcommand
            std::vector<std::string> subcmdArgs(args.begin() + 1, args.end());
            return it->second(subcmdArgs);
        }
        
        std::cerr << "Unknown subcommand: " << subcmdName << "\n";
        std::cerr << "Run '" << m_name << " help' for usage.\n";
        return false;
    }
    
    // Execute with argc/argv style arguments
    bool execute(int argc, char* argv[]) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args);
    }
    
    // Show subcommands matching a prefix
    void showMatchingCommands(const std::string& prefix) const {
        std::vector<std::string> matches;
        for (const auto& [name, _] : m_handlers) {
            if (name.compare(0, prefix.length(), prefix) == 0) {
                matches.push_back(name);
            }
        }
        
        if (matches.empty()) {
            std::cout << "No subcommands matching '" << prefix << "'\n";
        } else {
            std::cout << "Subcommands matching '" << prefix << "':\n";
            for (const auto& name : matches) {
                std::cout << "  " << name << "\n";
            }
        }
    }
    
    // Show help for all subcommands
    void showHelp() const {
        std::cout << m_name << ": " << m_description << "\n\n";
        std::cout << "Available subcommands:\n";
        for (const auto& [name, _] : m_handlers) {
            std::cout << "  " << name << "\n";
        }
        std::cout << "\nUse '" << m_name << " help <subcommand>' for more information.\n";
    }
    
    // Show help for specific subcommand
    bool showSubcommandHelp(const std::string& subcmdName) const {
        auto it = m_handlers.find(subcmdName);
        if (it == m_handlers.end()) {
            std::cerr << "Unknown subcommand: " << subcmdName << "\n";
            return false;
        }
        
        std::cout << "Subcommand: " << subcmdName << "\n";
        // Note: Could enhance this to show subcommand options
        return true;
    }
    
    std::string_view getName() const { return m_name; }
    std::string_view getDescription() const { return m_description; }
    
    const SubcommandMap& getSubcommands() const { return m_subcommands; }
    
    // Show hierarchical view of all subcommands with their options
    void showHierarchy(const std::string& indent = "", bool showOptions = true) const {
        std::cout << indent << m_name << ": " << m_description << "\n";
        std::cout << indent << "  Subcommands:\n";
        
        for (const auto& [name, cmdPtr] : m_subcommands) {
            std::cout << indent << "    " << name << "\n";
        }
        
        std::cout << "\n";
        std::cout << indent << "  Use '" << m_name << " help <subcommand>' for details on each subcommand.\n";
    }
    
private:
    std::string m_name;
    std::string m_description;
    SubcommandMap m_subcommands;
    std::map<std::string, SubcommandHandler> m_handlers;
};

// Helper to create subcommand dispatcher
inline auto makeDispatcher(std::string_view name, std::string_view description = "") {
    return std::make_shared<SubcommandDispatcher>(name, description);
}

/**
 * Mode manager for interactive command mode transitions
 * Allows switching between different command contexts (modes)
 */
class ModeManager {
public:
    using ModeHandler = std::function<std::string(const std::vector<std::string>&)>;
    
    ModeManager() : m_currentMode("default") {}
    
    // Register a mode with its command handler
    // Handler returns the next mode name (or empty string to stay in current mode)
    void addMode(std::string_view modeName, ModeHandler handler) {
        m_modes[std::string(modeName)] = handler;
    }
    
    // Register a subcommand dispatcher as a mode
    void addMode(std::string_view modeName, std::shared_ptr<SubcommandDispatcher> dispatcher) {
        m_modes[std::string(modeName)] = [dispatcher](const std::vector<std::string>& args) -> std::string {
            dispatcher->execute(args);
            return ""; // Stay in current mode
        };
    }
    
    // Register a command as a mode
    template<typename OptGroup, typename HandlerType>
    void addMode(std::string_view modeName, std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        m_modes[std::string(modeName)] = [cmd](const std::vector<std::string>& args) -> std::string {
            cmd->execute(args);
            return ""; // Stay in current mode
        };
    }
    
private:
    // Helper: Find mode by exact or partial match
    auto findMode(const std::string& name) const {
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
            std::cerr << "Ambiguous mode '" << name << "'. Did you mean:\n";
            for (const auto& match : matches) {
                std::cerr << "  " << match->first << "\n";
            }
        }
        return m_modes.end();
    }
    
public:
    // Execute command in current mode
    // Returns the next mode to transition to (or empty to stay)
    std::string execute(const std::vector<std::string>& args) {
        if (args.empty()) {
            return "";
        }
        
        std::string cmd = args[0];
        
        // Handle ? query syntax for modes: "mode ?" or "mode prefix?"
        if (cmd == "mode" && args.size() > 1 && !args[1].empty() && args[1].back() == '?') {
            std::string query = args[1];
            if (query.length() == 1) {
                // "mode ?" - show all modes
                std::cout << "Available modes:\n";
                for (const auto& [name, _] : m_modes) {
                    std::cout << "  " << name << "\n";
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
                    std::cout << "No modes matching '" << prefix << "'\n";
                } else {
                    std::cout << "Modes matching '" << prefix << "':\n";
                    for (const auto& name : matches) {
                        std::cout << "  " << name << "\n";
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
                auto it = findMode(newMode);
                if (it != m_modes.end()) {
                    m_currentMode = it->first;
                    std::cout << "Switched to mode: " << it->first << "\n";
                    return it->first;
                } else if (m_modes.find(newMode) == m_modes.end() && 
                          std::count_if(m_modes.begin(), m_modes.end(),
                              [&](const auto& p) { return p.first.compare(0, newMode.length(), newMode) == 0; }) == 0) {
                    std::cerr << "Unknown mode: " << newMode << "\n";
                    return "";
                } else {
                    // Ambiguous match already reported by findMode
                    return "";
                }
            } else {
                std::cout << "Current mode: " << m_currentMode << "\n";
                std::cout << "Available modes:\n";
                for (const auto& [name, _] : m_modes) {
                    std::cout << "  " << name << "\n";
                }
                return "";
            }
        }
        
        // Execute in current mode
        auto it = m_modes.find(m_currentMode);
        if (it != m_modes.end()) {
            std::string nextMode = it->second(args);
            if (!nextMode.empty() && nextMode != "exit") {
                m_currentMode = nextMode;
            }
            return nextMode;
        }
        
        std::cerr << "No handler for mode: " << m_currentMode << "\n";
        return "";
    }
    
    // Execute with argc/argv style arguments
    std::string execute(int argc, char* argv[]) {
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return execute(args);
    }
    
    // Execute a command string in the current mode
    // Parses the command string and executes it as if it were entered interactively
    // Example: executeCommand("start --port 8080 --host localhost")
    std::string executeCommand(std::string_view commandLine) {
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
        
        return execute(args);
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
    
    // Show hierarchical view of all modes
    void showHierarchy(bool showOptions = true) const {
        std::cout << "Mode Manager Hierarchy\n";
        std::cout << "======================\n\n";
        std::cout << "Current mode: " << m_currentMode << "\n\n";
        std::cout << "Available modes:\n";
        for (const auto& [name, _] : m_modes) {
            std::cout << "  - " << name;
            if (name == m_currentMode) {
                std::cout << " (current)";
            }
            std::cout << "\n";
        }
        std::cout << "\nUse 'mode <name>' to switch modes\n";
        std::cout << "Use 'exit' or 'quit' to exit\n";
    }
    
private:
    std::string m_currentMode;
    std::map<std::string, ModeHandler> m_modes;
};

// Helper to create mode manager
inline auto makeModeManager() {
    return std::make_shared<ModeManager>();
}

} // namespace cmdline_ct

#endif // CMDLINE_COMPONENTS_H
