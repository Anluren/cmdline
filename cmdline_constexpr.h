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
 * Integer option specification
 */
struct IntOption : OptionSpecBase {
    constexpr IntOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req) {}
    
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
 * Integer array option specification
 */
struct IntArrayOption : OptionSpecBase {
    constexpr IntArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase(n, d, req) {}
    
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
    
    constexpr AnyOption() 
        : name(""), description(""), required(false), is_int(false), is_array(false) {}
    
    template<typename OptType>
    constexpr AnyOption(const OptType& opt)
        : name(opt.name), description(opt.description), required(opt.required),
          is_int(is_int_option<OptType>::value), is_array(is_array_option<OptType>::value) {}
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
 */
template<size_t NumOptions = 0>
struct CommandSpec {
    std::string_view name;
    std::string_view description;
    std::array<AnyOption, NumOptions> options;
    
    constexpr CommandSpec(std::string_view n, std::string_view d = "")
        : name(n), description(d), options{} {}
    
    constexpr CommandSpec(std::string_view n, std::string_view d,
                         const std::array<AnyOption, NumOptions>& opts)
        : name(n), description(d), options(opts) {}
    
    // Helper to find option index by name at compile time
    constexpr std::optional<size_t> findOption(std::string_view optName) const {
        for (size_t i = 0; i < NumOptions; ++i) {
            if (options[i].name == optName) {
                return i;
            }
        }
        return std::nullopt;
    }
    
    // Get option spec by name
    constexpr const AnyOption* getOptionSpec(std::string_view optName) const {
        for (size_t i = 0; i < NumOptions; ++i) {
            if (options[i].name == optName) {
                return &options[i];
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
template<size_t NumOptions = 0, typename HandlerType = CommandHandler>
class Command {
public:
    constexpr Command(const CommandSpec<NumOptions>& spec, HandlerType handler)
        : spec_(spec), handler_(handler) {}
    
    const CommandSpec<NumOptions>& getSpec() const { return spec_; }
    constexpr std::string_view getName() const { return spec_.name; }
    constexpr std::string_view getDescription() const { return spec_.description; }
    
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
            
            // Check if it's an option (starts with --)
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
                std::string optName = arg.substr(2);
                
                // Get option spec to determine type
                const AnyOption* optSpec = spec_.getOptionSpec(optName);
                if (!optSpec) {
                    // Unknown option, skip
                    continue;
                }
                
                OptionValue val;
                val.name = optName;
                
                // Parse based on type flags
                if (optSpec->is_int && !optSpec->is_array) {
                    // Single integer
                    if (i + 1 < args.size()) {
                        std::string optValue = args[i + 1];
                        ++i;
                        val.intValue = OptionValue::parseInt(optValue);
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
                    while (i + 1 < args.size() && !(args[i + 1].size() > 2 && 
                           args[i + 1][0] == '-' && args[i + 1][1] == '-')) {
                        ++i;
                        if (auto intVal = OptionValue::parseInt(args[i])) {
                            arr.push_back(*intVal);
                        }
                    }
                    val.intArray = arr;
                } else if (!optSpec->is_int && optSpec->is_array) {
                    // Array of strings - collect until next option or end
                    std::vector<std::string> arr;
                    while (i + 1 < args.size() && !(args[i + 1].size() > 2 && 
                           args[i + 1][0] == '-' && args[i + 1][1] == '-')) {
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
    const CommandSpec<NumOptions>& spec_;
    HandlerType handler_;
};

// Helper function to create commands from constexpr specs
// Deduces HandlerType automatically from the lambda/function passed
template<size_t N, typename HandlerType>
auto makeCommand(const CommandSpec<N>& spec, HandlerType handler) {
    return std::make_shared<Command<N, HandlerType>>(spec, handler);
}

// Constexpr helper to create option arrays from typed options
template<typename... Args>
constexpr auto makeOptions(Args... args) {
    return std::array<AnyOption, sizeof...(Args)>{AnyOption(args)...};
}

// Helper to create option group (no longer needs makeOptions wrapper)
template<typename... Args>
constexpr auto makeOptionGroup(std::string_view name, std::string_view description, Args... args) {
    return OptionGroup<Args...>{name, description, args...};
}

// Helper to merge option groups into command options
template<size_t N1, size_t N2>
constexpr auto mergeOptions(const std::array<AnyOption, N1>& opts1,
                            const std::array<AnyOption, N2>& opts2) {
    std::array<AnyOption, N1 + N2> result{};
    for (size_t i = 0; i < N1; ++i) {
        result[i] = opts1[i];
    }
    for (size_t i = 0; i < N2; ++i) {
        result[N1 + i] = opts2[i];
    }
    return result;
}

// Merge an option group with options array
template<size_t N1, typename... GroupOpts>
constexpr auto mergeWithGroup(const std::array<AnyOption, N1>& opts,
                              const OptionGroup<GroupOpts...>& group) {
    return mergeOptions(opts, group.options);
}

// Merge multiple option groups
template<typename... Opts1, typename... Opts2>
constexpr auto mergeGroups(const OptionGroup<Opts1...>& g1, const OptionGroup<Opts2...>& g2) {
    return mergeOptions(g1.options, g2.options);
}

} // namespace cmdline_ct

#endif // CMDLINE_CONSTEXPR_H
