# Typed Option Classes Design

The option system now uses separate classes for each option type instead of an enum, providing better type safety and compile-time capabilities.

## Option Type Classes

### Base Class
```cpp
struct OptionSpecBase {
    std::string_view name;
    std::string_view description;
    bool required;
};
```

### Concrete Option Types

1. **IntOption** - Single integer value
   ```cpp
   IntOption{"port", "Port number", false}
   ```

2. **StringOption** - Single string value
   ```cpp
   StringOption{"host", "Server hostname", true}
   ```

3. **IntArrayOption** - Array of integers
   ```cpp
   IntArrayOption{"ports", "List of ports"}
   ```

4. **StringArrayOption** - Array of strings
   ```cpp
   StringArrayOption{"servers", "Server names"}
   ```

## Type Traits

Each option class has:
- `value_type` - The C++ type of the option value
- `is_array` - Compile-time boolean flag

Type traits for compile-time checks:
```cpp
is_int_option<T>     // true for IntOption, IntArrayOption
is_string_option<T>  // true for StringOption, StringArrayOption
is_array_option<T>   // true for IntArrayOption, StringArrayOption
```

## Type Erasure

Options are type-erased for storage in arrays:
```cpp
struct AnyOption {
    std::string_view name;
    std::string_view description;
    bool required;
    bool is_int;      // integer vs string
    bool is_array;    // single vs array
};
```

This allows heterogeneous option collections while preserving type information.

## Usage Examples

### Single Command
```cpp
constexpr auto spec = CommandSpec<2>(
    "connect",
    "Connect to server",
    makeOptions(
        StringOption{"host", "Server hostname"},
        IntOption{"port", "Port number"}
    )
);
```

### Option Groups (Composition)
```cpp
// Define reusable groups
constexpr auto networkGroup = makeOptionGroup(
    "network", "Network options",
    StringOption{"host", "Hostname"},
    IntOption{"port", "Port"}
);

constexpr auto retryGroup = makeOptionGroup(
    "retry", "Retry options",
    IntOption{"retry", "Retries"},
    IntOption{"timeout", "Timeout"}
);

// Compose groups into command
constexpr auto spec = CommandSpec<4>(
    "advanced-connect",
    "Connect with retries",
    mergeGroups(networkGroup, retryGroup)
);
```

### Merging Options
```cpp
// Merge group with additional options
mergeWithGroup(
    makeOptions(IntOption{"verbose", "Verbosity"}),
    networkGroup
)

// Merge two groups
mergeGroups(networkGroup, retryGroup)

// Merge two option arrays
mergeOptions(opts1, opts2)
```

## Benefits Over Enum Approach

1. **Type Safety** - Each option type is a distinct class
2. **Extensibility** - Easy to add new option types
3. **Compile-time Properties** - Each class can have compile-time metadata
4. **Better Documentation** - Type names are self-documenting
5. **Template Specialization** - Can specialize templates for specific option types

## Parsing Behavior

- **IntOption**: Parse single integer (hex/bin/dec)
- **StringOption**: Parse single string
- **IntArrayOption**: Collect integers until next `--option`
- **StringArrayOption**: Collect strings until next `--option`

All parsing is type-aware based on the option class used in the spec.
