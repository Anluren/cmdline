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
#include <variant>

namespace cmdline_ct {

// Forward declarations
template<typename OptGroup>
struct ParsedArgs;

template<typename OptGroup>
using CommandHandler = std::function<bool(const ParsedArgs<OptGroup>&)>;

/**
 * TypedOptionValue template - stores a value with compile-time type information
 */
template<typename T>
struct TypedOptionValue {
    using value_type = T;
    
    T value;
    bool is_set = false;
    
    TypedOptionValue() : value{}, is_set(false) {}
    explicit TypedOptionValue(const T& val) : value(val), is_set(true) {}
    explicit TypedOptionValue(T&& val) : value(std::move(val)), is_set(true) {}
    
    // Check if value was set
    explicit operator bool() const { return is_set; }
    
    // Access the value
    T& get() { return value; }
    const T& get() const { return value; }
    
    // Set the value
    void set(const T& val) {
        value = val;
        is_set = true;
    }
    
    void set(T&& val) {
        value = std::move(val);
        is_set = true;
    }
    
    // Reset to unset state
    void reset() {
        value = T{};
        is_set = false;
    }
    
    // Dereference operators for convenience
    T& operator*() { return value; }
    const T& operator*() const { return value; }
    
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }
};

/**
 * Base option specification using CRTP for compile-time polymorphism
 */
template<typename Derived>
struct OptionSpecBase {
    std::string_view name;
    std::string_view description;
    bool required;
    
    constexpr OptionSpecBase(std::string_view n, std::string_view d = "", bool req = false)
        : name(n), description(d), required(req) {}
    
    // CRTP accessor to derived type
    constexpr const Derived& derived() const {
        return static_cast<const Derived&>(*this);
    }
    
    constexpr Derived& derived() {
        return static_cast<Derived&>(*this);
    }
    
    // Create default instance of value_type (defined in derived)
    auto createDefaultValue() const {
        return typename Derived::value_type{};
    }
};

/**
 * Integer option specification with optional range validation
 */
struct IntOption : OptionSpecBase<IntOption> {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    
    constexpr IntOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<IntOption>(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}
    
    constexpr IntOption(std::string_view n, std::string_view d, bool req,
                       int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntOption>(n, d, req), min_value(min_val), max_value(max_val) {}
    
    constexpr IntOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntOption>(n, d, false), min_value(min_val), max_value(max_val) {}
    
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
struct StringOption : OptionSpecBase<StringOption> {
    constexpr StringOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<StringOption>(n, d, req) {}
    
    using value_type = std::string;
    static constexpr bool is_array = false;
};

/**
 * Integer array option specification with optional range validation
 */
struct IntArrayOption : OptionSpecBase<IntArrayOption> {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    
    constexpr IntArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<IntArrayOption>(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}
    
    constexpr IntArrayOption(std::string_view n, std::string_view d, bool req,
                            int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntArrayOption>(n, d, req), min_value(min_val), max_value(max_val) {}
    
    constexpr IntArrayOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntArrayOption>(n, d, false), min_value(min_val), max_value(max_val) {}
    
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
struct StringArrayOption : OptionSpecBase<StringArrayOption> {
    constexpr StringArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<StringArrayOption>(n, d, req) {}
    
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
    
    static constexpr size_t num_options = sizeof...(Opts);
    
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


/**
 * Compile-time command specification with option groups
 * Template parameter is an OptionGroup type
 */
template<typename OptGroup>
struct CommandSpec {
    std::string_view name;
    std::string_view description;
    OptGroup optionGroup;
    
    // Use the static member from OptionGroup directly
    static constexpr size_t NumOptions = OptGroup::num_options;
    
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
        (void)((std::get<Is>(optionGroup.options).name == optName ? (result = Is, true) : false) || ...);
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
 * Helper function to parse integer from string (supports decimal, hex, binary)
 */
inline std::optional<int64_t> parseInt(const std::string& str) {
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

/**
 * Parsed option value - holds any option type using std::variant
 */
using ParsedOptionValue = std::variant<
    std::monostate,
    int64_t,
    std::string,
    std::vector<int64_t>,
    std::vector<std::string>
>;

/**
 * Parsed arguments with type-safe accessors
 * Template parameter OptGroup provides compile-time option group information
 * Stores typed option values in a tuple matching the OptionGroup structure
 */
template<typename OptGroup>
struct ParsedArgs {
    std::vector<std::string> positional;
    
    // Extract value types from option specs and wrap in TypedOptionValue
    template<typename... Opts>
    struct MakeValueTuple;
    
    template<typename... Opts>
    struct MakeValueTuple<std::tuple<Opts...>> {
        using type = std::tuple<TypedOptionValue<typename Opts::value_type>...>;
    };
    
    using OptionsTuple = typename MakeValueTuple<decltype(OptGroup::options)>::type;
    OptionsTuple options;
    
    // Store a pointer to the option group for runtime name lookups
    const OptGroup* optionGroup = nullptr;
    
    // Helper to find option index by name at compile time
    template<size_t I = 0>
    static constexpr std::optional<size_t> findOptionIndex(const OptGroup& optGroup, std::string_view name) {
        if constexpr (I < std::tuple_size_v<decltype(OptGroup::options)>) {
            if (std::get<I>(optGroup.options).name == name) {
                return I;
            }
            return findOptionIndex<I + 1>(optGroup, name);
        }
        return std::nullopt;
    }
    
    // Get typed option value by index at compile time
    template<size_t I>
    auto& get() {
        return std::get<I>(options);
    }
    
    template<size_t I>
    const auto& get() const {
        return std::get<I>(options);
    }
    
    // Runtime accessors for backward compatibility
    bool hasOption(const std::string& name) const {
        if (!optionGroup) return false;
        bool found = false;
        hasOptionImpl(name, found, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return found;
    }
    
    std::optional<int64_t> getInt(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<int64_t> result;
        getIntImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }
    
    std::optional<std::string> getString(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::string> result;
        getStringImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }
    
    std::optional<std::vector<int64_t>> getIntArray(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::vector<int64_t>> result;
        getIntArrayImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }
    
    std::optional<std::vector<std::string>> getStringArray(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::vector<std::string>> result;
        getStringArrayImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }

private:
    template<size_t... Is>
    void hasOptionImpl(const std::string& name, bool& found, std::index_sequence<Is...>) const {
        (void)((checkOptionName<Is>(name) && (found = std::get<Is>(options).is_set, true)) || ...);
    }
    
    template<size_t I>
    bool checkOptionName(const std::string& name) const {
        return std::get<I>(optionGroup->options).name == name;
    }
    
    template<size_t... Is>
    void getIntImpl(const std::string& name, std::optional<int64_t>& result, std::index_sequence<Is...>) const {
        (void)((tryGetInt<Is>(name, result)) || ...);
    }
    
    template<size_t I>
    bool tryGetInt(const std::string& name, std::optional<int64_t>& result) const {
        if (std::get<I>(optionGroup->options).name != name) return false;
        using ValueType = typename std::tuple_element_t<I, OptionsTuple>::value_type;
        if constexpr (std::is_same_v<ValueType, int64_t>) {
            const auto& opt = std::get<I>(options);
            if (opt.is_set) {
                result = opt.value;
                return true;
            }
        }
        return false;
    }
    
    template<size_t... Is>
    void getStringImpl(const std::string& name, std::optional<std::string>& result, std::index_sequence<Is...>) const {
        (void)((tryGetString<Is>(name, result)) || ...);
    }
    
    template<size_t I>
    bool tryGetString(const std::string& name, std::optional<std::string>& result) const {
        if (std::get<I>(optionGroup->options).name != name) return false;
        using ValueType = typename std::tuple_element_t<I, OptionsTuple>::value_type;
        if constexpr (std::is_same_v<ValueType, std::string>) {
            const auto& opt = std::get<I>(options);
            if (opt.is_set) {
                result = opt.value;
                return true;
            }
        }
        return false;
    }
    
    template<size_t... Is>
    void getIntArrayImpl(const std::string& name, std::optional<std::vector<int64_t>>& result, std::index_sequence<Is...>) const {
        (void)((tryGetIntArray<Is>(name, result)) || ...);
    }
    
    template<size_t I>
    bool tryGetIntArray(const std::string& name, std::optional<std::vector<int64_t>>& result) const {
        if (std::get<I>(optionGroup->options).name != name) return false;
        using ValueType = typename std::tuple_element_t<I, OptionsTuple>::value_type;
        if constexpr (std::is_same_v<ValueType, std::vector<int64_t>>) {
            const auto& opt = std::get<I>(options);
            if (opt.is_set) {
                result = opt.value;
                return true;
            }
        }
        return false;
    }
    
    template<size_t... Is>
    void getStringArrayImpl(const std::string& name, std::optional<std::vector<std::string>>& result, std::index_sequence<Is...>) const {
        (void)((tryGetStringArray<Is>(name, result)) || ...);
    }
    
    template<size_t I>
    bool tryGetStringArray(const std::string& name, std::optional<std::vector<std::string>>& result) const {
        if (std::get<I>(optionGroup->options).name != name) return false;
        using ValueType = typename std::tuple_element_t<I, OptionsTuple>::value_type;
        if constexpr (std::is_same_v<ValueType, std::vector<std::string>>) {
            const auto& opt = std::get<I>(options);
            if (opt.is_set) {
                result = opt.value;
                return true;
            }
        }
        return false;
    }
};

/**
 * Runtime command (references compile-time spec)
 * HandlerType is typically a lambda or function pointer
 */
template<typename OptGroup, typename HandlerType = CommandHandler<OptGroup>>
class Command {
public:
    constexpr Command(const CommandSpec<OptGroup>& spec, HandlerType handler)
        : m_spec(spec), m_handler(handler) {}
    
    const CommandSpec<OptGroup>& getSpec() const { return m_spec; }
    constexpr std::string_view getName() const { return m_spec.name; }
    constexpr std::string_view getDescription() const { return m_spec.description; }
    
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
        return invoke(parsed);
    }
    
    // Parse arguments into ParsedArgs structure with type awareness
    ParsedArgs<OptGroup> parse(const std::vector<std::string>& args) const {
        ParsedArgs<OptGroup> parsed;
        parsed.optionGroup = &m_spec.optionGroup; // Set the option group pointer for runtime name lookups
        
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            
            // Check if it's an option (starts with -- or matches option name directly)
            std::string optName;
            bool isOpt = false;
            
            if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
                // Option with '--' prefix
                optName = arg.substr(2);
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
            } else {
                // Positional argument
                parsed.positional.push_back(arg);
            }
        }
        
        return parsed;
    }
    
private:
    // Helper to parse an option and store it in the correct tuple position
    void parseOptionIntoTuple(ParsedArgs<OptGroup>& parsed, const std::string& optName, 
                             const std::vector<std::string>& args, size_t& i) const {
        // Use index_sequence to iterate through all options
        parseOptionIntoTupleImpl(parsed, optName, args, i, 
                                std::make_index_sequence<OptGroup::num_options>{});
    }
    
    template<size_t... Is>
    void parseOptionIntoTupleImpl(ParsedArgs<OptGroup>& parsed, const std::string& optName,
                                 const std::vector<std::string>& args, size_t& i,
                                 std::index_sequence<Is...>) const {
        // Try each option index until we find the matching one
        (void)((tryParseOption<Is>(parsed, optName, args, i)) || ...);
    }
    
    template<size_t I>
    bool tryParseOption(ParsedArgs<OptGroup>& parsed, const std::string& optName,
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
        auto it = m_handlers.find(subcmdName);;
        if (it != m_handlers.end()) {
            // Pass remaining args to subcommand
            std::vector<std::string> subcmdArgs(args.begin() + 1, args.end());
            return it->second(subcmdArgs);
        }
        
        std::cerr << "Unknown subcommand: " << subcmdName << "\n";
        std::cerr << "Run '" << m_name << " help' for usage.\n";
        return false;
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
        auto it = m_handlers.find(subcmdName);;
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
                if (m_modes.find(newMode) != m_modes.end()) {
                    m_currentMode = newMode;
                    std::cout << "Switched to mode: " << newMode << "\n";
                    return newMode;
                } else {
                    std::cerr << "Unknown mode: " << newMode << "\n";
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
    
private:
    std::string m_currentMode;
    std::map<std::string, ModeHandler> m_modes;
};

// Helper to create mode manager
inline auto makeModeManager() {
    return std::make_shared<ModeManager>();
}

} // namespace cmdline_ct

#endif // CMDLINE_CONSTEXPR_H
