/**
 * Compile-time Command Line Library - Core Types
 * Type definitions, option specifications, and parsed arguments
 */

#ifndef CMDLINE_TYPES_H
#define CMDLINE_TYPES_H


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
#include <charconv>
#include <sstream>

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
    
    int64_t result = 0;
    const char* start = str.data();
    const char* end = str.data() + str.size();
    std::from_chars_result parse_result;
    
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        // Hexadecimal
        parse_result = std::from_chars(start + 2, end, result, 16);
    }
    else if (str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
        // Binary
        parse_result = std::from_chars(start + 2, end, result, 2);
    }
    else {
        // Decimal
        parse_result = std::from_chars(start, end, result, 10);
    }
    
    if (parse_result.ec == std::errc{} && parse_result.ptr == end) {
        return result;
    }
    return std::nullopt;
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

} // namespace cmdline_ct

#endif // CMDLINE_TYPES_H
