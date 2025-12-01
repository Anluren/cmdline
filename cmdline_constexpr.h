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
 * Type-erased option specification for storage
 */
struct AnyOption {
    std::string_view name;
    std::string_view description;
    bool required;
    bool is_int;
    bool is_array;
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    
    constexpr AnyOption() 
        : name(""), description(""), required(false), is_int(false), is_array(false),
          min_value(std::nullopt), max_value(std::nullopt) {}
    
    template<typename OptType>
    constexpr AnyOption(const OptType& opt)
        : name(opt.name), description(opt.description), required(opt.required),
          is_int(is_int_option<OptType>::value), is_array(is_array_option<OptType>::value),
          min_value(std::nullopt), max_value(std::nullopt) {}
    
    // Specialized constructor for IntOption to capture range
    constexpr AnyOption(const IntOption& opt)
        : name(opt.name), description(opt.description), required(opt.required),
          is_int(true), is_array(false),
          min_value(opt.min_value), max_value(opt.max_value) {}
    
    // Specialized constructor for IntArrayOption to capture range
    constexpr AnyOption(const IntArrayOption& opt)
        : name(opt.name), description(opt.description), required(opt.required),
          is_int(true), is_array(true),
          min_value(opt.min_value), max_value(opt.max_value) {}
    
    // Validate integer value against range
    constexpr bool isValid(int64_t value) const {
        if (min_value && value < *min_value) return false;
        if (max_value && value > *max_value) return false;
        return true;
    }
};

/**
 * Option group for composing related options (variadic template)
 */
template<typename... Options>
struct OptionGroup {
    std::string_view name;
    std::string_view description;
    std::array<AnyOption, sizeof...(Options)> options;
    
    constexpr OptionGroup(std::string_view n, std::string_view d, Options... opts)
        : name(n), description(d), options{AnyOption(opts)...} {}
    
    constexpr size_t size() const { return sizeof...(Options); }
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
    
    constexpr CommandSpec(std::string_view n, std::string_view d, const OptGroup& opts)
        : name(n), description(d), optionGroup(opts) {}
    
    // Get number of options
    constexpr size_t numOptions() const { return optionGroup.size(); }
    
    // Access options array
    constexpr const auto& options() const { return optionGroup.options; }
    
    // Helper to find option index by name at compile time
    constexpr std::optional<size_t> findOption(std::string_view optName) const {
        for (size_t i = 0; i < optionGroup.size(); ++i) {
            if (optionGroup.options[i].name == optName) {
                return i;
            }
        }
        return std::nullopt;
    }
    
    // Get option spec by name
    constexpr const AnyOption* getOptionSpec(std::string_view optName) const {
        for (size_t i = 0; i < optionGroup.size(); ++i) {
            if (optionGroup.options[i].name == optName) {
                return &optionGroup.options[i];
            }
        }
        return nullptr;
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
            return spec_.getOptionSpec(arg.substr(2)) != nullptr;
        }
        return spec_.getOptionSpec(arg) != nullptr;
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
            const AnyOption* optSpec = nullptr;
            
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
                // Option with '--' prefix
                optName = arg.substr(2);
                optSpec = spec_.getOptionSpec(optName);
            } else {
                // Try matching without prefix
                optSpec = spec_.getOptionSpec(arg);
                if (optSpec) {
                    optName = arg;
                }
            }
            
            if (optSpec) {
                // Found a valid option
                
                OptionValue val;
                val.name = optName;
                
                // Parse based on type flags
                if (optSpec->is_int && !optSpec->is_array) {
                    // Single integer
                    if (i + 1 < args.size()) {
                        std::string optValue = args[i + 1];
                        ++i;
                        auto parsedValue = OptionValue::parseInt(optValue);
                        if (parsedValue) {
                            // Validate range if specified
                            if (optSpec->isValid(*parsedValue)) {
                                val.intValue = parsedValue;
                            } else {
                                // Value out of range - could add error reporting here
                                // For now, skip this option
                            }
                        }
                    }
                } else if (!optSpec->is_int && !optSpec->is_array) {
                    // Single string
                    if (i + 1 < args.size()) {
                        val.stringValue = args[i + 1];
                        ++i;
                    }
                } else if (optSpec->is_int && optSpec->is_array) {
                    // Array of integers - collect until next option or end
                    std::vector<int64_t> arr;
                    while (i + 1 < args.size() && !isOption(args[i + 1])) {
                        ++i;
                        if (auto intVal = OptionValue::parseInt(args[i])) {
                            // Validate range if specified
                            if (optSpec->isValid(*intVal)) {
                                arr.push_back(*intVal);
                            }
                            // Skip values that are out of range
                        }
                    }
                    val.intArray = arr;
                } else if (!optSpec->is_int && optSpec->is_array) {
                    // Array of strings - collect until next option or end
                    std::vector<std::string> arr;
                    while (i + 1 < args.size() && !isOption(args[i + 1])) {
                        ++i;
                        arr.push_back(args[i]);
                    }
                    val.stringArray = arr;
                }
                
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
    void addMode(const std::string& modeName, ModeHandler handler) {
        modes_[modeName] = handler;
    }
    
    // Register a subcommand dispatcher as a mode
    void addMode(const std::string& modeName, std::shared_ptr<SubcommandDispatcher> dispatcher) {
        modes_[modeName] = [dispatcher](const std::vector<std::string>& args) -> std::string {
            dispatcher->execute(args);
            return ""; // Stay in current mode
        };
    }
    
    // Register a command as a mode
    template<typename OptGroup, typename HandlerType>
    void addMode(const std::string& modeName, std::shared_ptr<Command<OptGroup, HandlerType>> cmd) {
        modes_[modeName] = [cmd](const std::vector<std::string>& args) -> std::string {
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
    std::string getCurrentMode() const { return currentMode_; }
    
    // Set current mode
    bool setMode(const std::string& modeName) {
        if (modes_.find(modeName) != modes_.end()) {
            currentMode_ = modeName;
            return true;
        }
        return false;
    }
    
    // Check if mode exists
    bool hasMode(const std::string& modeName) const {
        return modes_.find(modeName) != modes_.end();
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
