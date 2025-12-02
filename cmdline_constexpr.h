/**
 * Compile-time Command Line Library - C++17
 * 
 * Uses templates and constexpr for zero-allocation command definitions
 */

#ifndef CMDLINE_CONSTEXPR_H
#define CMDLINE_CONSTEXPR_H

#include <string_view>
#include <array>
#include <optional>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace cmdline_ct {

// Forward declarations
struct ParsedArgs;
using CommandHandler = std::function<bool(const ParsedArgs&)>;

/**
 * Base option specification
 */
struct OptionSpecBase {
    std::string_view name;
    std::string_view description;
    bool required;
    
    constexpr OptionSpecBase(std::string_view n, std::string_view d = "", bool req = false)
        : name(n), description(d), required(req) {}
};

/**
 * Integer option specification with optional range validation
 */
struct IntOption : OptionSpecBase {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    
    constexpr IntOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}
    
    constexpr IntOption(std::string_view n, std::string_view d, bool req,
                       int64_t min_val, int64_t max_val)
        : OptionSpecBase(n, d, req), min_value(min_val), max_value(max_val) {}
    
    constexpr IntOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase(n, d, false), min_value(min_val), max_value(max_val) {}
    
    // Validate value is within range
    constexpr bool isValid(int64_t value) const {
        if (min_value && value < *min_value) return false;
        if (max_value && value > *max_value) return false;
        return true;
    }
    
    using value_type = int64_t;
    static constexpr bool is_array = false;
};

/**
 * String option specification
 */
struct StringOption : OptionSpecBase {
    constexpr StringOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req) {}
    
    using value_type = std::string;
    static constexpr bool is_array = false;
};

/**
 * Integer array option specification with optional range validation
 */
struct IntArrayOption : OptionSpecBase {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    
    constexpr IntArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}
    
    constexpr IntArrayOption(std::string_view n, std::string_view d, bool req,
                            int64_t min_val, int64_t max_val)
        : OptionSpecBase(n, d, req), min_value(min_val), max_value(max_val) {}
    
    constexpr IntArrayOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase(n, d, false), min_value(min_val), max_value(max_val) {}
    
    // Validate value is within range
    constexpr bool isValid(int64_t value) const {
        if (min_value && value < *min_value) return false;
        if (max_value && value > *max_value) return false;
        return true;
    }
    
    using value_type = std::vector<int64_t>;
    static constexpr bool is_array = true;
};

/**
 * String array option specification
 */
struct StringArrayOption : OptionSpecBase {
    constexpr StringArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req) {}
    
    using value_type = std::vector<std::string>;
    static constexpr bool is_array = true;
};

// Type traits to identify option types
template<typename T>
struct is_int_option : std::false_type {};

template<>
struct is_int_option<IntOption> : std::true_type {};

template<>
struct is_int_option<IntArrayOption> : std::true_type {};

template<typename T>
struct is_string_option : std::false_type {};

template<>
struct is_string_option<StringOption> : std::true_type {};

template<>
struct is_string_option<StringArrayOption> : std::true_type {};

template<typename T>
struct is_array_option : std::false_type {};

template<>
struct is_array_option<IntArrayOption> : std::true_type {};

template<>
struct is_array_option<StringArrayOption> : std::true_type {};

/**
 * Option group for composing related options (variadic template)
 * Stores typed options in a tuple
 */
template<typename... Opts>
struct OptionGroup {
    std::string_view name;
    std::string_view description;
    std::tuple<Opts...> options;
    
    constexpr OptionGroup(std::string_view n, std::string_view d, Opts... opts)
        : name(n), description(d), options(std::make_tuple(opts...)) {}
    
    constexpr size_t size() const { return sizeof...(Opts); }
    
    // Find option by name using fold expression
    template<typename Visitor>
    constexpr void visitOption(std::string_view optName, Visitor&& visitor) const {
        visitOptionImpl(optName, std::forward<Visitor>(visitor), std::index_sequence_for<Opts...>{});
    }
    
private:
    template<typename Visitor, size_t... Is>
    constexpr void visitOptionImpl(std::string_view optName, Visitor&& visitor, std::index_sequence<Is...>) const {
        (void)((std::get<Is>(options).name == optName && (visitor(std::get<Is>(options)), true)) || ...);
    }
};

// Helper to get tuple size from OptionGroup (must be after OptionGroup definition)
template<typename T>
struct option_group_size;

template<typename... Opts>
struct option_group_size<OptionGroup<Opts...>> {
    static constexpr size_t value = sizeof...(Opts);
};

/**
 * Compile-time command specification with option groups
 * Template parameter is an OptionGroup type
 */
template<typename OptGroup>
struct CommandSpec {
    std::string_view name;
    std::string_view description;
    OptGroup optionGroup;
    
    static constexpr size_t NumOptions = option_group_size<OptGroup>::value;
    
    constexpr CommandSpec(std::string_view n, std::string_view d, const OptGroup& opts)
        : name(n), description(d), optionGroup(opts) {}
    
    // Get number of options
    constexpr size_t numOptions() const { return optionGroup.size(); }
    
    // Check if option exists by name
    constexpr bool hasOption(std::string_view optName) const {
        bool found = false;
        optionGroup.visitOption(optName, [&found](const auto&) { found = true; });
        return found;
    }
    
    // Find option index by name (for compile-time checking)
    constexpr auto findOption(std::string_view optName) const {
        return findOptionImpl(optName, std::make_index_sequence<NumOptions>{});
    }
    
    // Get all options as a vector (for runtime iteration)
    auto getAllOptions() const {
        std::vector<OptionInfo> result;
        getAllOptionsImpl(result, std::make_index_sequence<NumOptions>{});
        return result;
    }
    
    // Alias for backward compatibility
    auto options() const {
        return getAllOptions();
    }
    
private:
    // Helper struct to hold option information for runtime iteration
    struct OptionInfo {
        std::string_view name;
        std::string_view description;
        bool required;
        bool is_int;
        bool is_array;
        std::optional<int64_t> min_value;
        std::optional<int64_t> max_value;
    };
    
    template<size_t... Is>
    constexpr std::optional<size_t> findOptionImpl(std::string_view optName, 
                                                    std::index_sequence<Is...>) const {
        std::optional<size_t> result;
        ((std::get<Is>(optionGroup.options).name == optName ? (result = Is, true) : false) || ...);
        return result;
    }
    
    template<size_t... Is>
    void getAllOptionsImpl(std::vector<OptionInfo>& result, std::index_sequence<Is...>) const {
        (addOptionInfo<Is>(result), ...);
    }
    
    template<size_t I>
    void addOptionInfo(std::vector<OptionInfo>& result) const {
        const auto& opt = std::get<I>(optionGroup.options);
        OptionInfo info;
        info.name = opt.name;
        info.description = opt.description;
        info.required = opt.required;
        
        using OptType = std::decay_t<decltype(opt)>;
        info.is_int = std::is_same_v<OptType, IntOption> || std::is_same_v<OptType, IntArrayOption>;
        info.is_array = std::is_same_v<OptType, IntArrayOption> || std::is_same_v<OptType, StringArrayOption>;
        
        if constexpr (std::is_same_v<OptType, IntOption> || std::is_same_v<OptType, IntArrayOption>) {
            info.min_value = opt.min_value;
            info.max_value = opt.max_value;
        } else {
            info.min_value = std::nullopt;
            info.max_value = std::nullopt;
        }
        
        result.push_back(info);
    }
};

/**
 * Parsed option value - holds any option type value
 */
struct OptionValue {
    std::string name;
    
    // Single values
    std::optional<int64_t> intValue;
    std::optional<std::string> stringValue;
    
    // Array values
    std::optional<std::vector<int64_t>> intArray;
    std::optional<std::vector<std::string>> stringArray;
    
    OptionValue() = default;
    
    static std::optional<int64_t> parseInt(const std::string& str) {
        if (str.empty()) return std::nullopt;
        
        try {
            if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
                return std::stoll(str.substr(2), nullptr, 16);
            }
            else if (str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
                return std::stoll(str.substr(2), nullptr, 2);
            }
            else {
                return std::stoll(str, nullptr, 10);
            }
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Type-safe getters
    bool hasInt() const { return intValue.has_value(); }
    bool hasString() const { return stringValue.has_value(); }
    bool hasIntArray() const { return intArray.has_value(); }
    bool hasStringArray() const { return stringArray.has_value(); }
};

/**
 * Parsed arguments with type-safe accessors
 */
struct ParsedArgs {
    std::vector<std::string> positional;
    std::map<std::string, OptionValue> options;
    
    bool hasOption(const std::string& name) const {
        return options.find(name) != options.end();
    }
    
    std::optional<int64_t> getInt(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second.intValue;
        }
        return std::nullopt;
    }
    
    std::optional<std::string> getString(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second.stringValue;
        }
        return std::nullopt;
    }
    
    std::optional<std::vector<int64_t>> getIntArray(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second.intArray;
        }
        return std::nullopt;
    }
    
    std::optional<std::vector<std::string>> getStringArray(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second.stringArray;
        }
        return std::nullopt;
    }
};

/**
 * Runtime command (references compile-time spec)
 * HandlerType is typically a lambda or function pointer
 */
template<typename OptGroup, typename HandlerType = CommandHandler>
class Command {
public:
    constexpr Command(const CommandSpec<OptGroup>& spec, HandlerType handler)
        : spec_(spec), handler_(handler) {}
    
    const CommandSpec<OptGroup>& getSpec() const { return spec_; }
    constexpr std::string_view getName() const { return spec_.name; }
    constexpr std::string_view getDescription() const { return spec_.description; }
    
    // Helper to check if a string is an option (either '--name' or a known option name)
    bool isOption(const std::string& arg) const {
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
            return spec_.hasOption(arg.substr(2));
        }
        return spec_.hasOption(arg);
    }
    
    // Execute: parse arguments then invoke handler
    bool execute(const std::vector<std::string>& args) const {
        ParsedArgs parsed = parse(args);
        return invoke(parsed);
    }
    
    // Parse arguments into ParsedArgs structure with type awareness
    ParsedArgs parse(const std::vector<std::string>& args) const {
        ParsedArgs parsed;
        
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            
            // Check if it's an option (starts with -- or matches option name directly)
            std::string optName;
            bool isOpt = false;
            
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
                // Option with '--' prefix
                optName = arg.substr(2);
                isOpt = spec_.hasOption(optName);
            } else {
                // Try matching without prefix
                if (spec_.hasOption(arg)) {
                    optName = arg;
                    isOpt = true;
                }
            }
            
            if (isOpt) {
                // Found a valid option - use visitor to parse it
                OptionValue val;
                val.name = optName;
                
                spec_.optionGroup.visitOption(optName, [&](const auto& opt) {
                    using OptType = std::decay_t<decltype(opt)>;
                    
                    if constexpr (std::is_same_v<OptType, IntOption>) {
                        // Single integer
                        if (i + 1 < args.size()) {
                            std::string optValue = args[i + 1];
                            ++i;
                            auto parsedValue = OptionValue::parseInt(optValue);
                            if (parsedValue && opt.isValid(*parsedValue)) {
                                val.intValue = parsedValue;
                            }
                        }
                    } else if constexpr (std::is_same_v<OptType, StringOption>) {
                        // Single string
                        if (i + 1 < args.size()) {
                            val.stringValue = args[i + 1];
                            ++i;
                        }
                    } else if constexpr (std::is_same_v<OptType, IntArrayOption>) {
                        // Array of integers
                        std::vector<int64_t> arr;
                        while (i + 1 < args.size() && !isOption(args[i + 1])) {
                            ++i;
                            if (auto intVal = OptionValue::parseInt(args[i])) {
                                if (opt.isValid(*intVal)) {
                                    arr.push_back(*intVal);
                                }
                            }
                        }
                        val.intArray = arr;
                    } else if constexpr (std::is_same_v<OptType, StringArrayOption>) {
                        // Array of strings
                        std::vector<std::string> arr;
                        while (i + 1 < args.size() && !isOption(args[i + 1])) {
                            ++i;
                            arr.push_back(args[i]);
                        }
                        val.stringArray = arr;
                    }
                });
                
                parsed.options[optName] = val;
            } else {
                // Positional argument
                parsed.positional.push_back(arg);
            }
        }
        
        return parsed;
    }
    
    // Invoke handler with parsed arguments
    bool invoke(const ParsedArgs& parsed) const {
        return handler_(parsed);
    }
    
private:
    const CommandSpec<OptGroup>& spec_;
    HandlerType handler_;
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
        : name_(name), description_(description) {}
    
    // Add a subcommand
    template<typename OptGroup, typename HandlerType>
    void addSubcommand(std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        std::string cmdName(cmd->getName());
        subcommands_[cmdName] = cmd;
        handlers_[cmdName] = [cmd](const std::vector<std::string>& args) {
            return cmd->execute(args);
        };
    }
    
    // Execute with subcommand dispatch
    bool execute(const std::vector<std::string>& args) {
        if (args.empty()) {
            showHelp();
            return false;
        }
        
        const std::string& subcmdName = args[0];
        
        // Special commands
        if (subcmdName == "help" || subcmdName == "--help" || subcmdName == "-h") {
            if (args.size() > 1) {
                return showSubcommandHelp(args[1]);
            }
            showHelp();
            return true;
        }
        
        // Find and execute subcommand
        auto it = handlers_.find(subcmdName);
        if (it != handlers_.end()) {
            // Pass remaining args to subcommand
            std::vector<std::string> subcmdArgs(args.begin() + 1, args.end());
            return it->second(subcmdArgs);
        }
        
        std::cerr << "Unknown subcommand: " << subcmdName << "\n";
        std::cerr << "Run '" << name_ << " help' for usage.\n";
        return false;
    }
    
    // Show help for all subcommands
    void showHelp() const {
        std::cout << name_ << ": " << description_ << "\n\n";
        std::cout << "Available subcommands:\n";
        for (const auto& [name, _] : handlers_) {
            std::cout << "  " << name << "\n";
        }
        std::cout << "\nUse '" << name_ << " help <subcommand>' for more information.\n";
    }
    
    // Show help for specific subcommand
    bool showSubcommandHelp(const std::string& subcmdName) const {
        auto it = handlers_.find(subcmdName);
        if (it == handlers_.end()) {
            std::cerr << "Unknown subcommand: " << subcmdName << "\n";
            return false;
        }
        
        std::cout << "Subcommand: " << subcmdName << "\n";
        // Note: Could enhance this to show subcommand options
        return true;
    }
    
    std::string_view getName() const { return name_; }
    std::string_view getDescription() const { return description_; }
    
    const SubcommandMap& getSubcommands() const { return subcommands_; }
    
private:
    std::string name_;
    std::string description_;
    SubcommandMap subcommands_;
    std::map<std::string, SubcommandHandler> handlers_;
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
    
    ModeManager() : currentMode_("default") {}
    
    // Register a mode with its command handler
    // Handler returns the next mode name (or empty string to stay in current mode)
    void addMode(std::string_view modeName, ModeHandler handler) {
        modes_[std::string(modeName)] = handler;
    }
    
    // Register a subcommand dispatcher as a mode
    void addMode(std::string_view modeName, std::shared_ptr<SubcommandDispatcher> dispatcher) {
        modes_[std::string(modeName)] = [dispatcher](const std::vector<std::string>& args) -> std::string {
            dispatcher->execute(args);
            return ""; // Stay in current mode
        };
    }
    
    // Register a command as a mode
    template<typename OptGroup, typename HandlerType>
    void addMode(std::string_view modeName, std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        modes_[std::string(modeName)] = [cmd](const std::vector<std::string>& args) -> std::string {
            cmd->execute(args);
            return ""; // Stay in current mode
        };
    }
    
    // Execute command in current mode
    // Returns the next mode to transition to (or empty to stay)
    std::string execute(const std::vector<std::string>& args) {
        if (args.empty()) {
            return "";
        }
        
        // Check for mode transition commands
        if (args[0] == "exit" || args[0] == "quit") {
            return "exit";
        }
        
        if (args[0] == "mode") {
            if (args.size() > 1) {
                std::string newMode = args[1];
                if (modes_.find(newMode) != modes_.end()) {
                    currentMode_ = newMode;
                    std::cout << "Switched to mode: " << newMode << "\n";
                    return newMode;
                } else {
                    std::cerr << "Unknown mode: " << newMode << "\n";
                    return "";
                }
            } else {
                std::cout << "Current mode: " << currentMode_ << "\n";
                std::cout << "Available modes:\n";
                for (const auto& [name, _] : modes_) {
                    std::cout << "  " << name << "\n";
                }
                return "";
            }
        }
        
        // Execute in current mode
        auto it = modes_.find(currentMode_);
        if (it != modes_.end()) {
            std::string nextMode = it->second(args);
            if (!nextMode.empty() && nextMode != "exit") {
                currentMode_ = nextMode;
            }
            return nextMode;
        }
        
        std::cerr << "No handler for mode: " << currentMode_ << "\n";
        return "";
    }
    
    // Get current mode name
    std::string_view getCurrentMode() const { return currentMode_; }
    
    // Set current mode
    bool setMode(std::string_view modeName) {
        auto it = modes_.find(std::string(modeName));
        if (it != modes_.end()) {
            currentMode_ = modeName;
            return true;
        }
        return false;
    }
    
    // Check if mode exists
    bool hasMode(std::string_view modeName) const {
        return modes_.find(std::string(modeName)) != modes_.end();
    }
    
    // Get all mode names
    std::vector<std::string> getModes() const {
        std::vector<std::string> result;
        for (const auto& [name, _] : modes_) {
            result.push_back(name);
        }
        return result;
    }
    
private:
    std::string currentMode_;
    std::map<std::string, ModeHandler> modes_;
};

// Helper to create mode manager
inline auto makeModeManager() {
    return std::make_shared<ModeManager>();
}

} // namespace cmdline_ct

#endif // CMDLINE_CONSTEXPR_H
