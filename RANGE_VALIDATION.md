# IntOption Range Validation

## Overview

The `IntOption` and `IntArrayOption` classes support optional range validation, allowing you to specify minimum and maximum values that will be enforced both at compile-time and runtime.

## Features

- **Compile-time validation**: Use `static_assert` with `isValid()` to verify expected values at compile time
- **Runtime validation**: Invalid values are automatically filtered out during argument parsing
- **Support for all integer formats**: Decimal, hexadecimal (0x), and binary (0b)
- **Array support**: Range validation works for both single integers and integer arrays
- **Optional ranges**: Ranges are optional - omit them for unrestricted values

## Usage

### Basic Range Specification

```cpp
// Port number: must be between 1 and 65535
constexpr IntOption portOpt{"port", "Port number (1-65535)", 1, 65535};

// Percentage: must be between 0 and 100
constexpr IntOption percentOpt{"percent", "Percentage (0-100)", 0, 100};

// Temperature: must be between -273 and 1000
constexpr IntOption tempOpt{"temperature", "Temperature in Celsius", -273, 1000};

// No range limits
constexpr IntOption unrestrictedOpt{"unlimited", "No range limits"};
```

### Constructor Signatures

```cpp
// Without range
IntOption(std::string_view name, std::string_view description = "", bool required = false)

// With range (not required)
IntOption(std::string_view name, std::string_view description, int64_t min, int64_t max)

// With range and required flag
IntOption(std::string_view name, std::string_view description, bool required, 
          int64_t min, int64_t max)
```

### Array Options with Ranges

```cpp
// Port array: each port must be in range 1-65535
constexpr IntArrayOption portsOpt{"ports", "Port numbers (1-65535)", 1, 65535};

// Scores array: each score must be in range 0-100
constexpr IntArrayOption scoresOpt{"scores", "Test scores (0-100)", 0, 100};

// Unbounded array
constexpr IntArrayOption valuesOpt{"values", "Any integer values"};
```

## Runtime Behavior

### Single Integer Options

When parsing arguments:
- Values within range are accepted
- Values outside range are silently filtered (option remains unset)
- Works with decimal, hex (0x), and binary (0b) formats

```cpp
// Valid: port set to 8080
cmd->execute({"--port", "8080"});

// Invalid: port out of range, option not set
cmd->execute({"--port", "70000"});

// Valid: hex value within range
cmd->execute({"--port", "0x1F90"});  // 8080
```

### Integer Array Options

When parsing array arguments:
- Only values within range are added to the array
- Out-of-range values are skipped
- Result may be empty if all values are invalid

```cpp
// Result: [80, 443, 8080] - valid values only
cmd->execute({"--ports", "80", "70000", "443", "0", "8080"});

// Result: [] - empty array, all values invalid
cmd->execute({"--ports", "70000", "80000", "100000"});
```

## Compile-time Validation

Use `static_assert` for compile-time verification:

```cpp
constexpr IntOption portOpt{"port", "Port number", 1, 65535};

// These assertions are evaluated at compile time
static_assert(portOpt.isValid(8080), "8080 should be valid port");
static_assert(!portOpt.isValid(0), "0 should be invalid port");
static_assert(!portOpt.isValid(70000), "70000 should be invalid port");

static_assert(portOpt.isValid(1), "Min boundary is valid");
static_assert(portOpt.isValid(65535), "Max boundary is valid");
```

## Complete Example

```cpp
#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    // Define options with ranges
    constexpr IntOption portOpt{"port", "Port (1-65535)", 1, 65535};
    constexpr IntOption percentOpt{"percent", "CPU usage (0-100)", 0, 100};
    constexpr IntArrayOption scoresOpt{"scores", "Test scores (0-100)", 0, 100};
    
    constexpr auto spec = CommandSpec<3>(
        "monitor",
        "System monitoring with validated inputs",
        makeOptions(
            AnyOption{portOpt},
            AnyOption{percentOpt},
            AnyOption{scoresOpt}
        )
    );
    
    auto cmd = makeCommand(spec, [](const ParsedArgs& args) {
        if (auto port = args.getInt("port")) {
            std::cout << "Monitoring port: " << *port << "\n";
        }
        if (auto percent = args.getInt("percent")) {
            std::cout << "CPU threshold: " << *percent << "%\n";
        }
        if (auto scores = args.getIntArray("scores")) {
            std::cout << "Valid scores: ";
            for (auto score : *scores) {
                std::cout << score << " ";
            }
            std::cout << "\n";
        }
        return true;
    });
    
    // Valid inputs
    cmd->execute({"--port", "8080", "--percent", "75"});
    
    // Invalid port (out of range) - port option ignored
    cmd->execute({"--port", "70000", "--percent", "50"});
    
    // Array with mixed validity - only valid scores kept
    cmd->execute({"--scores", "95", "110", "87", "-5", "100"});
    
    // Compile-time validation
    static_assert(portOpt.isValid(8080));
    static_assert(!portOpt.isValid(0));
    
    return 0;
}
```

## Benefits

1. **Type Safety**: Ranges are enforced for integer types only
2. **Automatic Validation**: No manual validation code needed
3. **Compile-time Checks**: Catch errors early with static assertions
4. **Clear API**: Range constraints are visible in option definitions
5. **Flexible**: Ranges are optional, use only when needed
6. **Format Agnostic**: Works with decimal, hex, and binary formats

## Design Considerations

### Why Silent Filtering?

Invalid values are silently filtered rather than causing errors because:
- Allows partial success with array inputs
- Simplifies error handling in command handlers
- Handler can check if option is set to detect validation failures
- Keeps compile-time specs simple

### Alternative: Error Reporting

For applications requiring explicit error reporting, you can check if expected options are present:

```cpp
auto cmd = makeCommand(spec, [](const ParsedArgs& args) {
    if (!args.hasOption("port")) {
        std::cerr << "Error: Port not specified or out of range\n";
        return false;
    }
    // ... continue processing
    return true;
});
```

## See Also

- `test_int_range.cpp` - Single integer option range validation examples
- `test_array_range.cpp` - Integer array option range validation examples
- `TYPED_OPTIONS_DESIGN.md` - Overall typed options design
