# Compile-Time Command Definitions

This demonstrates a compile-time approach to command definitions using C++17 constexpr and templates.

## Key Features

### 1. Zero-Allocation Command Specs
Command specifications are defined at compile time and stored in `.rodata` (read-only data section):

```cpp
// These are compile-time constants - no heap allocation!
constexpr auto showSpec = CommandSpec<2>(
    "show",
    "Display information",
    makeOptions(
        OptionSpec{"verbose", "Enable verbose output"},
        OptionSpec{"count", "Number of items (hex/dec/bin)"}
    )
);
```

### 2. Compile-Time Validation
You can validate command structure at compile time using `static_assert`:

```cpp
static_assert(showSpec.options.size() == 2);
static_assert(showSpec.name == "show");
static_assert(connectSpec.findOption("port").has_value());
static_assert(connectSpec.findOption("invalid") == std::nullopt);
```

### 3. Compile-Time Option Lookup
Find options by name at compile time:

```cpp
constexpr auto portIdx = Commands::connectSpec.findOption("port");
// portIdx is evaluated at compile time!
```

## Performance Benefits

### Memory Layout
- **Traditional approach**: Command specs allocated on heap
- **Compile-time approach**: Command specs in `.rodata` section
  - No heap allocations for specs
  - Better cache locality
  - Specs can never be accidentally modified

### Code Size
Since specs are templates, the compiler can:
- Inline more aggressively
- Eliminate dead code for unused options
- Optimize based on known spec sizes

## Trade-offs

### Advantages
✅ Zero heap allocation for command specs
✅ Compile-time validation catches errors early
✅ Type-safe option specifications
✅ Can't accidentally modify command metadata
✅ Better cache locality

### Limitations
❌ Template instantiation can increase binary size
❌ All command specs must be known at compile time
❌ Can't dynamically add/remove commands at runtime
❌ More complex template syntax

## When to Use

**Use compile-time approach when:**
- Command structure is known at compile time
- Performance is critical (embedded systems, real-time)
- Want compile-time validation
- Commands won't change at runtime

**Use traditional approach when:**
- Need to load commands from config files
- Commands are determined at runtime
- Plugin architecture with dynamic commands
- Simpler code is preferred over performance

## Example Output

The demo shows:
1. Command information (name, description, options)
2. Test executions with various integer formats (decimal, hex, binary)
3. Compile-time option lookups

All command specs are validated at compile time!
