/**
 * \file cmdline_types.h
 * \brief Core type definitions for the compile-time command line library.
 *
 * This file contains:
 * - OutputContext: Output stream management
 * - TypedOptionValue: Value wrapper with set/unset state
 * - Option types: IntOption, StringOption, IntArrayOption, StringArrayOption
 * - OptionGroup: Container for related options
 * - CommandSpec: Compile-time command specification
 * - ParsedArgs: Type-safe parsed argument container
 *
 * \author cmdline_ct library
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

/** \addtogroup public_api
 * \{
 */

/**
 * \brief Holds output streams for command execution.
 *
 * Allows redirecting output to any std::ostream (e.g., stringstream for testing).
 * By default, output goes to std::cout and errors to std::cerr.
 *
 * \par Example:
 * \code
 * // Redirect to stringstreams for testing
 * std::stringstream out, err;
 * OutputContext ctx(out, err);
 *
 * // Use same stream for both
 * std::stringstream combined;
 * OutputContext ctx2(combined);
 * \endcode
 */
struct OutputContext {
    std::ostream* out = &std::cout;  ///< Output stream pointer
    std::ostream* err = &std::cerr;  ///< Error stream pointer

    /// \brief Default constructor using std::cout and std::cerr
    OutputContext() = default;

    /**
     * \brief Construct with separate output and error streams.
     * \param o Output stream reference
     * \param e Error stream reference
     */
    OutputContext(std::ostream& o, std::ostream& e) : out(&o), err(&e) {}

    /**
     * \brief Construct with single stream for both output and error.
     * \param both Stream to use for both output and error
     */
    explicit OutputContext(std::ostream& both) : out(&both), err(&both) {}

    /**
     * \brief Get the output stream.
     * \return Reference to the output stream
     */
    std::ostream& output() const { return *out; }

    /**
     * \brief Get the error stream.
     * \return Reference to the error stream
     */
    std::ostream& error() const { return *err; }
};

/**
 * \brief Wrapper for option values with set/unset state tracking.
 * \ingroup public_api
 *
 * Provides an optional-like interface for storing parsed option values
 * with compile-time type information.
 *
 * \tparam T The value type to store
 *
 * \par Example:
 * \code
 * TypedOptionValue<int64_t> val;
 * if (!val) {
 *     std::cout << "Not set\n";
 * }
 * val.set(42);
 * if (val) {
 *     std::cout << "Value: " << val.get() << "\n";
 * }
 * \endcode
 */
template<typename T>
struct TypedOptionValue {
    using value_type = T;  ///< The stored value type

    T value;        ///< The stored value
    bool is_set = false;  ///< Whether the value has been set

    /// \brief Default constructor (value is unset)
    TypedOptionValue() : value{}, is_set(false) {}

    /**
     * \brief Construct with a value (marks as set).
     * \param val The value to store
     */
    explicit TypedOptionValue(const T& val) : value(val), is_set(true) {}

    /**
     * \brief Construct with a moved value (marks as set).
     * \param val The value to move in
     */
    explicit TypedOptionValue(T&& val) : value(std::move(val)), is_set(true) {}

    /**
     * \brief Check if value was set.
     * \return true if value was set, false otherwise
     */
    explicit operator bool() const { return is_set; }

    /**
     * \brief Access the value.
     * \return Reference to the value
     */
    T& get() { return value; }

    /**
     * \brief Access the value (const).
     * \return Const reference to the value
     */
    const T& get() const { return value; }

    /**
     * \brief Set the value.
     * \param val The value to set
     */
    void set(const T& val) {
        value = val;
        is_set = true;
    }

    /**
     * \brief Set the value (move).
     * \param val The value to move in
     */
    void set(T&& val) {
        value = std::move(val);
        is_set = true;
    }

    /**
     * \brief Reset to unset state.
     */
    void reset() {
        value = T{};
        is_set = false;
    }

    /// \brief Dereference operator
    T& operator*() { return value; }
    /// \brief Dereference operator (const)
    const T& operator*() const { return value; }

    /// \brief Arrow operator
    T* operator->() { return &value; }
    /// \brief Arrow operator (const)
    const T* operator->() const { return &value; }
};

/** \} */ // end of public_api group

/**
 * \brief Base class for option specifications using CRTP.
 * \ingroup internal_api
 *
 * Provides common fields (name, description, required) and CRTP
 * accessors for compile-time polymorphism.
 *
 * \tparam Derived The derived option type (CRTP pattern)
 */
template<typename Derived>
struct OptionSpecBase {
    std::string_view name;         ///< Option name (used as --name on command line)
    std::string_view description;  ///< Human-readable description
    bool required;                 ///< Whether this option is required

    /**
     * \brief Construct an option specification.
     * \param n Option name
     * \param d Description (default: empty)
     * \param req Whether required (default: false)
     */
    constexpr OptionSpecBase(std::string_view n, std::string_view d = "", bool req = false)
        : name(n), description(d), required(req) {}

    /// \brief CRTP accessor to derived type (const)
    constexpr const Derived& derived() const {
        return static_cast<const Derived&>(*this);
    }

    /// \brief CRTP accessor to derived type
    constexpr Derived& derived() {
        return static_cast<Derived&>(*this);
    }

    /// \brief Create default instance of value_type
    auto createDefaultValue() const {
        return typename Derived::value_type{};
    }
};

/**
 * \brief Integer option specification with optional range validation.
 * \ingroup public_api
 *
 * Stores a single int64_t value. Supports optional min/max range validation.
 *
 * \par Example:
 * \code
 * // Simple integer option
 * IntOption{"count", "Number of iterations"}
 *
 * // With range validation
 * IntOption{"port", "Server port", 1, 65535}
 *
 * // Required with range
 * IntOption{"id", "User ID", true, 1, 1000}
 * \endcode
 */
struct IntOption : OptionSpecBase<IntOption> {
    std::optional<int64_t> min_value;  ///< Minimum allowed value (optional)
    std::optional<int64_t> max_value;  ///< Maximum allowed value (optional)

    /// \brief Construct without range validation
    constexpr IntOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<IntOption>(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}

    /// \brief Construct with range validation and required flag
    constexpr IntOption(std::string_view n, std::string_view d, bool req,
                       int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntOption>(n, d, req), min_value(min_val), max_value(max_val) {}

    /// \brief Construct with range validation (optional by default)
    constexpr IntOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntOption>(n, d, false), min_value(min_val), max_value(max_val) {}

    /**
     * \brief Validate value is within range.
     * \param value The value to validate
     * \return true if value is within range, false otherwise
     */
    constexpr bool isValid(int64_t value) const {
        if (min_value && value < *min_value) return false;
        if (max_value && value > *max_value) return false;
        return true;
    }

    using value_type = int64_t;           ///< The stored value type
    static constexpr bool is_array = false;  ///< Not an array type
};

/**
 * \brief String option specification.
 * \ingroup public_api
 *
 * Stores a single std::string value.
 *
 * \par Example:
 * \code
 * StringOption{"name", "User name"}
 * StringOption{"config", "Config file path", true}  // required
 * \endcode
 */
struct StringOption : OptionSpecBase<StringOption> {
    /// \brief Construct a string option
    constexpr StringOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<StringOption>(n, d, req) {}

    using value_type = std::string;          ///< The stored value type
    static constexpr bool is_array = false;  ///< Not an array type
};

/**
 * \brief Integer array option specification with optional range validation.
 * \ingroup public_api
 *
 * Stores multiple int64_t values. Supports optional min/max range validation
 * for each element.
 *
 * \par Example:
 * \code
 * // Simple integer array
 * IntArrayOption{"ids", "List of IDs"}
 *
 * // With range validation for each element
 * IntArrayOption{"ports", "Port numbers", 1, 65535}
 * \endcode
 */
struct IntArrayOption : OptionSpecBase<IntArrayOption> {
    std::optional<int64_t> min_value;  ///< Minimum allowed value per element (optional)
    std::optional<int64_t> max_value;  ///< Maximum allowed value per element (optional)

    /// \brief Construct without range validation
    constexpr IntArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<IntArrayOption>(n, d, req), min_value(std::nullopt), max_value(std::nullopt) {}

    /// \brief Construct with range validation and required flag
    constexpr IntArrayOption(std::string_view n, std::string_view d, bool req,
                            int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntArrayOption>(n, d, req), min_value(min_val), max_value(max_val) {}

    /// \brief Construct with range validation (optional by default)
    constexpr IntArrayOption(std::string_view n, std::string_view d, int64_t min_val, int64_t max_val)
        : OptionSpecBase<IntArrayOption>(n, d, false), min_value(min_val), max_value(max_val) {}

    /**
     * \brief Validate a single value is within range.
     * \param value The value to validate
     * \return true if value is within range, false otherwise
     */
    constexpr bool isValid(int64_t value) const {
        if (min_value && value < *min_value) return false;
        if (max_value && value > *max_value) return false;
        return true;
    }

    using value_type = std::vector<int64_t>;  ///< The stored value type
    static constexpr bool is_array = true;    ///< Is an array type
};

/**
 * \brief String array option specification.
 * \ingroup public_api
 *
 * Stores multiple std::string values.
 *
 * \par Example:
 * \code
 * StringArrayOption{"files", "Input files"}
 * StringArrayOption{"tags", "Tags to apply", true}  // required
 * \endcode
 */
struct StringArrayOption : OptionSpecBase<StringArrayOption> {
    /// \brief Construct a string array option
    constexpr StringArrayOption(std::string_view n, std::string_view d = "", bool req = false)
        : OptionSpecBase<StringArrayOption>(n, d, req) {}

    using value_type = std::vector<std::string>;  ///< The stored value type
    static constexpr bool is_array = true;        ///< Is an array type
};

/**
 * \brief Type trait to check if a type is an integer option.
 * \ingroup internal_api
 * \tparam T The type to check
 */
template<typename T>
struct is_int_option : std::false_type {};

/// \cond INTERNAL
template<>
struct is_int_option<IntOption> : std::true_type {};

template<>
struct is_int_option<IntArrayOption> : std::true_type {};
/// \endcond

/**
 * \brief Type trait to check if a type is a string option.
 * \ingroup internal_api
 * \tparam T The type to check
 */
template<typename T>
struct is_string_option : std::false_type {};

/// \cond INTERNAL
template<>
struct is_string_option<StringOption> : std::true_type {};

template<>
struct is_string_option<StringArrayOption> : std::true_type {};
/// \endcond

/**
 * \brief Type trait to check if a type is an array option.
 * \ingroup internal_api
 * \tparam T The type to check
 */
template<typename T>
struct is_array_option : std::false_type {};

/// \cond INTERNAL
template<>
struct is_array_option<IntArrayOption> : std::true_type {};

template<>
struct is_array_option<StringArrayOption> : std::true_type {};
/// \endcond

/**
 * \brief Container for composing related options.
 * \ingroup public_api
 *
 * Stores typed options in a tuple with compile-time type information.
 * Created using makeOptions() or makeOptionGroup().
 *
 * \tparam Opts The option types
 *
 * \par Example:
 * \code
 * constexpr auto opts = OptionGroup<IntOption, StringOption>{
 *     "connection", "Connection settings",
 *     IntOption{"port", "Server port"},
 *     StringOption{"host", "Server hostname"}
 * };
 * \endcode
 *
 * \see makeOptions() for creating anonymous groups
 * \see makeOptionGroup() for creating named groups
 */
template<typename... Opts>
struct OptionGroup {
    std::string_view name;            ///< Group name (can be empty)
    std::string_view description;     ///< Group description (can be empty)
    std::tuple<Opts...> options;      ///< Tuple of option specifications

    static constexpr size_t num_options = sizeof...(Opts);  ///< Number of options

    /**
     * \brief Construct an option group.
     * \param n Group name
     * \param d Group description
     * \param opts Option specifications
     */
    constexpr OptionGroup(std::string_view n, std::string_view d, Opts... opts)
        : name(n), description(d), options(std::make_tuple(opts...)) {}

    /**
     * \brief Get the number of options.
     * \return Number of options in the group
     */
    constexpr size_t size() const { return sizeof...(Opts); }

    /**
     * \brief Visit an option by name.
     *
     * Calls the visitor with the option if found.
     *
     * \tparam Visitor Callable type
     * \param optName Option name to find
     * \param visitor Callable to invoke with the option
     */
    template<typename Visitor>
    constexpr void visitOption(std::string_view optName, Visitor&& visitor) const {
        visitOptionImpl(optName, std::forward<Visitor>(visitor), std::index_sequence_for<Opts...>{});
    }

private:
    /// \cond INTERNAL
    template<typename Visitor, size_t... Is>
    constexpr void visitOptionImpl(std::string_view optName, Visitor&& visitor, std::index_sequence<Is...>) const {
        (void)((std::get<Is>(options).name == optName && (visitor(std::get<Is>(options)), true)) || ...);
    }
    /// \endcond
};


/**
 * \brief Compile-time command specification.
 * \ingroup public_api
 *
 * Defines a command with its name, description, and available options.
 * Used to create Command instances.
 *
 * \tparam OptGroup The OptionGroup type defining available options
 *
 * \par Example:
 * \code
 * constexpr auto spec = CommandSpec(
 *     "greet",
 *     "Greet a user",
 *     makeOptions(
 *         StringOption{"name", "User name"},
 *         IntOption{"count", "Times to greet", 1, 10}
 *     )
 * );
 *
 * auto cmd = makeCommand(spec, handler);
 * \endcode
 *
 * \see Command for using specifications with handlers
 */
template<typename OptGroup>
struct CommandSpec {
    std::string_view name;    ///< Command name
    std::string_view description;  ///< Command description
    OptGroup optionGroup;     ///< The option group

    /// \brief Number of options (compile-time constant)
    static constexpr size_t NumOptions = OptGroup::num_options;

    /**
     * \brief Construct a command specification.
     * \param n Command name
     * \param d Command description
     * \param opts Option group
     */
    constexpr CommandSpec(std::string_view n, std::string_view d, const OptGroup& opts)
        : name(n), description(d), optionGroup(opts) {}

    /**
     * \brief Get the number of options.
     * \return Number of options
     */
    constexpr size_t numOptions() const { return optionGroup.size(); }

    /**
     * \brief Check if an option exists by name.
     * \param optName The option name to check
     * \return true if the option exists, false otherwise
     */
    constexpr bool hasOption(std::string_view optName) const {
        bool found = false;
        optionGroup.visitOption(optName, [&found](const auto&) { found = true; });
        return found;
    }

    /**
     * \brief Find option index by name.
     * \param optName The option name to find
     * \return Optional index of the option, or nullopt if not found
     */
    constexpr auto findOption(std::string_view optName) const {
        return findOptionImpl(optName, std::make_index_sequence<NumOptions>{});
    }

    /**
     * \brief Helper struct for runtime option information.
     *
     * Used by getAllOptions() for runtime iteration over options.
     */
    struct OptionInfo {
        std::string_view name;               ///< Option name
        std::string_view description;        ///< Option description
        bool required;                       ///< Whether required
        bool is_int;                         ///< Whether integer type
        bool is_array;                       ///< Whether array type
        std::optional<int64_t> min_value;    ///< Minimum value (if applicable)
        std::optional<int64_t> max_value;    ///< Maximum value (if applicable)
    };

    /**
     * \brief Get all options as a vector.
     *
     * Useful for runtime iteration over options.
     *
     * \return Vector of OptionInfo structs
     */
    auto getAllOptions() const {
        std::vector<OptionInfo> result;
        getAllOptionsImpl(result, std::make_index_sequence<NumOptions>{});
        return result;
    }

    /**
     * \brief Alias for getAllOptions().
     * \deprecated Use getAllOptions() instead
     * \return Vector of OptionInfo structs
     */
    auto options() const {
        return getAllOptions();
    }

private:
    /// \cond INTERNAL
    
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
    /// \endcond
};

/**
 * \brief Parse an integer from a string.
 * \ingroup public_api
 *
 * Supports multiple formats:
 * - Decimal: "123", "-456"
 * - Hexadecimal: "0x1A", "0XFF"
 * - Binary: "0b1010", "0B1111"
 *
 * \param str The string to parse
 * \return The parsed integer, or nullopt if parsing failed
 *
 * \par Example:
 * \code
 * parseInt("42");      // Returns 42
 * parseInt("0xFF");    // Returns 255
 * parseInt("0b1010");  // Returns 10
 * parseInt("invalid"); // Returns nullopt
 * \endcode
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
 * \brief Type alias for any parsed option value.
 * \ingroup public_api
 *
 * Holds any option type using std::variant. Used for type-erased storage.
 */
using ParsedOptionValue = std::variant<
    std::monostate,
    int64_t,
    std::string,
    std::vector<int64_t>,
    std::vector<std::string>
>;

/**
 * \brief Container for parsed command-line arguments.
 * \ingroup public_api
 *
 * Provides type-safe access to parsed options and positional arguments.
 * Options can be accessed by compile-time index or runtime name.
 *
 * \tparam OptGroup The OptionGroup type
 *
 * \par Compile-time access (preferred):
 * \code
 * auto& value = args.get<0>();  // First option
 * if (value) {
 *     std::cout << value.get();
 * }
 * \endcode
 *
 * \par Runtime access by name:
 * \code
 * if (auto val = args.getInt("port")) {
 *     std::cout << "Port: " << *val << "\n";
 * }
 * if (auto name = args.getString("name")) {
 *     std::cout << "Name: " << *name << "\n";
 * }
 * \endcode
 */
template<typename OptGroup>
struct ParsedArgs {
    std::vector<std::string> positional;  ///< Positional arguments (non-option)

    /// \cond INTERNAL
    // Extract value types from option specs and wrap in TypedOptionValue
    template<typename... Opts>
    struct MakeValueTuple;

    template<typename... Opts>
    struct MakeValueTuple<std::tuple<Opts...>> {
        using type = std::tuple<TypedOptionValue<typename Opts::value_type>...>;
    };
    /// \endcond

    /// \brief Tuple type for storing option values
    using OptionsTuple = typename MakeValueTuple<decltype(OptGroup::options)>::type;
    OptionsTuple options;  ///< Tuple of typed option values

    const OptGroup* optionGroup = nullptr;  ///< Pointer to option group for runtime lookups
    bool parseSuccess = true;  ///< Whether parsing succeeded without errors

    /**
     * \brief Find option index by name at compile time.
     * \tparam I Starting index (default 0)
     * \param optGroup The option group
     * \param name Option name to find
     * \return Optional index, or nullopt if not found
     */
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

    /**
     * \brief Get option value by compile-time index.
     * \tparam I Option index
     * \return Reference to the TypedOptionValue
     */
    template<size_t I>
    auto& get() {
        return std::get<I>(options);
    }

    /**
     * \brief Get option value by compile-time index (const).
     * \tparam I Option index
     * \return Const reference to the TypedOptionValue
     */
    template<size_t I>
    const auto& get() const {
        return std::get<I>(options);
    }

    /**
     * \brief Check if an option was set by name.
     * \param name Option name
     * \return true if the option was set, false otherwise
     */
    bool hasOption(const std::string& name) const {
        if (!optionGroup) return false;
        bool found = false;
        hasOptionImpl(name, found, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return found;
    }

    /**
     * \brief Get an integer option value by name.
     * \param name Option name
     * \return The value if set, or nullopt
     */
    std::optional<int64_t> getInt(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<int64_t> result;
        getIntImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }

    /**
     * \brief Get a string option value by name.
     * \param name Option name
     * \return The value if set, or nullopt
     */
    std::optional<std::string> getString(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::string> result;
        getStringImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }

    /**
     * \brief Get an integer array option value by name.
     * \param name Option name
     * \return The values if set, or nullopt
     */
    std::optional<std::vector<int64_t>> getIntArray(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::vector<int64_t>> result;
        getIntArrayImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }

    /**
     * \brief Get a string array option value by name.
     * \param name Option name
     * \return The values if set, or nullopt
     */
    std::optional<std::vector<std::string>> getStringArray(const std::string& name) const {
        if (!optionGroup) return std::nullopt;
        std::optional<std::vector<std::string>> result;
        getStringArrayImpl(name, result, std::make_index_sequence<std::tuple_size_v<OptionsTuple>>{});
        return result;
    }

private:
    /// \cond INTERNAL
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
    /// \endcond
};

} // namespace cmdline_ct

#endif // CMDLINE_TYPES_H
