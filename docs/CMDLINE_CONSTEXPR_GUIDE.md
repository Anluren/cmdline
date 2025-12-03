# cmdline_hdr_only.h - Comprehensive Guide

## Table of Contents
1. [Overview](#overview)
2. [Design Philosophy](#design-philosophy)
3. [Core Components](#core-components)
4. [Option Types](#option-types)
5. [Command Definition](#command-definition)
6. [Subcommands](#subcommands)
7. [Mode Manager](#mode-manager)
8. [Parsing and Execution](#parsing-and-execution)
9. [Examples](#examples)
10. [Best Practices](#best-practices)
11. [Advanced Usage](#advanced-usage)

---

## Overview

`cmdline_hdr_only.h` is a modern C++17 header-only library for building type-safe, compile-time command-line interfaces. It leverages template metaprogramming and constexpr to provide zero-overhead command definitions with full type safety.

### Key Features

- **Header-only**: Single header file, no compilation required
- **C++17 Standard**: Pure C++17, no external dependencies
- **Type-safe**: Compile-time type checking for options with tuple-based storage
- **Zero-overhead**: Constexpr specifications compiled away
- **Flexible Parsing**: Supports both `--option value` and `option value` formats
- **Range Validation**: Optional min/max validation for integer options
- **Subcommands**: Hierarchical command structures (like git, docker)
- **Mode System**: Interactive mode transitions for complex workflows
- **argc/argv Interface**: Native support for main() style arguments
- **No Exceptions**: Uses std::from_chars for exception-free parsing
- **Modern C++**: Uses std::string_view, std::optional, variadic templates, std::tuple

---

## Design Philosophy

### 1. Compile-Time Specifications

Options and commands are defined using `constexpr`, meaning they're fully resolved at compile time:

```cpp
constexpr auto portOpt = IntOption{"port", "Server port", 1024, 65535};
constexpr auto opts = makeOptionGroup("server", "Start server", portOpt);
constexpr auto spec = CommandSpec<decltype(opts)>{"server", "Server command", opts};
```

**Benefits:**
- Zero runtime overhead for specifications
- Compile-time validation of option definitions
- No memory allocation for option metadata

### 2. Type Safety

Each option type carries compile-time type information:

```cpp
IntOption        -> int64_t
StringOption     -> std::string
IntArrayOption   -> std::vector<int64_t>
StringArrayOption -> std::vector<std::string>
```

The parser enforces these types at runtime, and your handlers receive properly typed values.

### 3. Variadic Templates

Option groups use variadic templates to accept any number of options:

```cpp
template<typename... Options>
struct OptionGroup {
    std::string_view name;
    std::string_view description;
    std::tuple<Options...> options;
};
```

This allows flexible composition without runtime overhead.

### 4. Shared Ownership

Commands and dispatchers use `std::shared_ptr` for safe shared ownership across modes and lambdas:

```cpp
auto cmd = makeCommand(spec, handler);  // Returns shared_ptr<Command<...>>
mgr->addMode("git", gitDispatcher);     // Safe sharing
```

---

## Core Components

### TypedOptionValue

Template-based storage for parsed option values with compile-time type information:

```cpp
template<typename T>
struct TypedOptionValue {
    T value;
    bool is_set = false;
    
    void set(T v);
    void reset();
    
    // Convenient accessors
    T& operator*();
    T* operator->();
    explicit operator bool() const;
};
```

**Features:**
- Type-safe value storage with compile-time type
- `is_set` flag to track if value was provided
- Convenient operators for easy access
- Move semantics support

### parseInt Function

**Exception-free integer parsing** using `std::from_chars`:

```cpp
inline std::optional<int64_t> parseInt(const std::string& str);
```

**Features:**
- Supports hex (0x), binary (0b), decimal parsing for integers
- No exceptions - returns `std::nullopt` on parse errors
- Uses `std::from_chars` for efficient, exception-free parsing
- Validates that entire string was consumed

### ParsedArgs

Template-based container for parsed command-line arguments with compile-time type safety:

```cpp
template<typename OptGroup>
struct ParsedArgs {
    std::vector<std::string> positional;
    
    // Tuple of TypedOptionValue matching the OptionGroup
    using OptionsTuple = std::tuple<TypedOptionValue<T1>, TypedOptionValue<T2>, ...>;
    OptionsTuple options;
    
    // Compile-time accessors by index
    template<size_t I>
    auto& get();
    
    // Runtime accessors by name
    bool hasOption(const std::string& name) const;
    std::optional<int64_t> getInt(const std::string& name) const;
    std::optional<std::string> getString(const std::string& name) const;
    std::optional<std::vector<int64_t>> getIntArray(const std::string& name) const;
    std::optional<std::vector<std::string>> getStringArray(const std::string& name) const;
};
```

**Key Changes:**
- Options stored in `std::tuple` instead of `std::map` for compile-time type safety
- Each option's type is known at compile time
- Compile-time access via `get<I>()` for index-based access
- Runtime access via `getInt()`, `getString()`, etc. for name-based access

**Usage:**
```cpp
// Runtime access by name
auto handler = [](const auto& args) {
    if (auto port = args.getInt("port")) {
        std::cout << "Port: " << *port << "\n";
    }
    return true;
};

// Compile-time access by index
auto handler = [](const auto& args) {
    auto& portValue = args.get<0>();  // First option (port)
    if (portValue.is_set) {
        std::cout << "Port: " << portValue.value << "\n";
    }
    return true;
};

// Modifying parsed values
auto parsed = cmd->parse(args);
parsed.get<0>().value = 8080;  // Set port to 8080
parsed.get<0>().is_set = true;
cmd->invoke(parsed);
```

---

## Option Types

### IntOption

Single integer value with optional range validation.

**Constructors:**
```cpp
// Basic
IntOption{"port", "Server port"}

// With required flag
IntOption{"port", "Server port", true}

// With range validation
IntOption{"port", "Server port", 1024, 65535}

// With required + range
IntOption{"port", "Server port", true, 1024, 65535}
```

**Features:**
- Parses decimal, hex (0x), binary (0b) formats
- Optional min/max validation
- Compile-time `isValid()` for constexpr checking
- Runtime validation during parsing (out-of-range values filtered)

**Example:**
```cpp
constexpr auto portOpt = IntOption{"port", "Server port", 1024, 65535};
constexpr auto verboseOpt = IntOption{"verbose", "Verbosity (0-3)", 0, 3};
```

### StringOption

Single string value.

**Constructor:**
```cpp
StringOption{"name", "Description", required=false}
```

**Example:**
```cpp
constexpr auto hostOpt = StringOption{"host", "Hostname", true};
constexpr auto msgOpt = StringOption{"message", "Commit message"};
```

### IntArrayOption

Multiple integer values with optional range validation.

**Constructors:** Same as IntOption.

**Parsing:**
Collects all non-option arguments until the next option or end:

```bash
./app values 10 20 30 40  # Collects [10, 20, 30, 40]
./app values 10 20 --next ...  # Collects [10, 20], stops at --next
```

**Example:**
```cpp
constexpr auto portsOpt = IntArrayOption{"ports", "Multiple ports", 1024, 65535};

// Usage: ./app ports 8080 8081 8082
```

### StringArrayOption

Multiple string values.

**Constructor:**
```cpp
StringArrayOption{"files", "File list", required=false}
```

**Example:**
```cpp
constexpr auto filesOpt = StringArrayOption{"files", "Files to process"};

// Usage: ./app files main.cpp test.cpp utils.cpp
```

---

## Command Definition

### OptionGroup

Groups related options together.

**Creation:**
```cpp
template<typename... Options>
constexpr auto makeOptionGroup(
    std::string_view name,
    std::string_view description,
    Options... options
);
```

**Example:**
```cpp
constexpr auto serverOpts = makeOptionGroup(
    "server",
    "Server options",
    IntOption{"port", "Port number", 1024, 65535},
    StringOption{"host", "Hostname", true},
    IntOption{"workers", "Worker threads", 1, 64}
);
```

**Anonymous Groups:**
```cpp
// For simple commands without grouping
constexpr auto opts = makeOptions(
    IntOption{"verbose", "Verbosity"},
    StringOption{"output", "Output file"}
);
```

### CommandSpec

Template-based command specification.

**Structure:**
```cpp
template<typename OptGroup>
struct CommandSpec {
    std::string_view name;
    std::string_view description;
    OptGroup options;
    
    const AnyOption* getOptionSpec(std::string_view name) const;
    std::vector<AnyOption> getAllOptions() const;
};
```

**Creation:**
```cpp
constexpr auto opts = makeOptionGroup(...);
constexpr auto spec = CommandSpec<decltype(opts)>{
    "mycommand",
    "Command description",
    opts
};
```

**Example:**
```cpp
constexpr auto addOpts = makeOptionGroup(
    "add",
    "Add files to index",
    StringArrayOption{"files", "Files to add", true},
    IntOption{"verbose", "Verbosity level", 0, 2}
);

constexpr auto addSpec = CommandSpec<decltype(addOpts)>{
    "add",
    "Add files to the index",
    addOpts
};
```

### Command

Runtime command object that combines specification with handler.

**Template Parameters:**
- `OptGroup`: The option group type (from CommandSpec)
- `HandlerType`: Function/lambda type (defaults to `CommandHandler`)

**Creation:**
```cpp
template<typename OptGroup, typename HandlerType>
auto makeCommand(
    const CommandSpec<OptGroup>& spec,
    HandlerType handler
) -> std::shared_ptr<Command<OptGroup, HandlerType>>;
```

**Example:**
```cpp
auto addCmd = makeCommand(addSpec, [](const ParsedArgs& args) {
    if (auto files = args.getStringArray("files")) {
        for (const auto& file : *files) {
            std::cout << "Adding: " << file << "\n";
        }
    }
    if (auto v = args.getInt("verbose")) {
        std::cout << "Verbosity: " << *v << "\n";
    }
    return true;
});
```

**Methods:**
```cpp
// Vector<string> interface
bool execute(const std::vector<std::string>& args);  // Parse + invoke
ParsedArgs<OptGroup> parse(const std::vector<std::string>& args);  // Just parse
bool invoke(const ParsedArgs<OptGroup>& parsed);  // Just invoke handler

// argc/argv interface (for main() style arguments)
bool execute(int argc, char* argv[]);  // Parse + invoke from argv
ParsedArgs<OptGroup> parse(int argc, char* argv[]);  // Parse from argv
```

---

## Subcommands

### SubcommandDispatcher

Manages hierarchical command structures (like git, docker).

**Purpose:** Route to different commands based on the first argument (subcommand name).

**Creation:**
```cpp
auto dispatcher = makeDispatcher("git", "Git version control");
```

**Adding Subcommands:**
```cpp
dispatcher->addSubcommand(addCmd);
dispatcher->addSubcommand(commitCmd);
dispatcher->addSubcommand(pushCmd);
```

**Execution:**
```cpp
std::vector<std::string> args = {"add", "files", "main.cpp"};
bool success = dispatcher->execute(args);
// Routes to addCmd with args: ["files", "main.cpp"]
```

**Built-in Help:**
```bash
$ git help
git: Git version control

Available subcommands:
  add
  commit
  push

Use 'git help <subcommand>' for more information.

$ git help add
Subcommand: add
```

**Example:**
```cpp
// Define subcommands
constexpr auto addSpec = CommandSpec<decltype(addOpts)>{"add", "Add files", addOpts};
constexpr auto commitSpec = CommandSpec<decltype(commitOpts)>{"commit", "Commit", commitOpts};

auto addCmd = makeCommand(addSpec, addHandler);
auto commitCmd = makeCommand(commitSpec, commitHandler);

// Create dispatcher
auto git = makeDispatcher("git", "Git VCS");
git->addSubcommand(addCmd);
git->addSubcommand(commitCmd);

// Execute
std::vector<std::string> args = {"commit", "--message", "Fix bug"};
git->execute(args);  // Routes to commitCmd
```

**Features:**
- Automatic routing based on first argument
- Built-in help system (`help`, `--help`, `-h`)
- Passes remaining arguments to subcommand
- Returns false for unknown subcommands

---

## Mode Manager

### ModeManager

Interactive mode system for command context switching.

**Purpose:** Build interactive CLIs that switch between different command contexts (like switching between git mode and docker mode).

**Creation:**
```cpp
auto mgr = makeModeManager();
```

**Adding Modes:**

1. **From SubcommandDispatcher:**
```cpp
auto gitDispatcher = makeDispatcher("git", "Git commands");
gitDispatcher->addSubcommand(addCmd);
gitDispatcher->addSubcommand(commitCmd);

mgr->addMode("git", gitDispatcher);
```

2. **From Command:**
```cpp
auto listCmd = makeCommand(listSpec, listHandler);
mgr->addMode("list", listCmd);
```

3. **Custom Handler:**
```cpp
mgr->addMode("default", [](const std::vector<std::string>& args) -> std::string {
    if (args[0] == "git") return "git";      // Transition to git mode
    if (args[0] == "docker") return "docker"; // Transition to docker mode
    return "";  // Stay in current mode
});
```

**Mode Handler Interface:**
```cpp
using ModeHandler = std::function<std::string(const std::vector<std::string>&)>;
```

**Return Values:**
- `""` (empty): Stay in current mode
- `"modename"`: Transition to specified mode
- `"exit"`: Signal application exit

**Execution:**
```cpp
std::vector<std::string> args = {"add", "files", "main.cpp"};
std::string nextMode = mgr->execute(args);

if (nextMode == "exit") {
    // Exit application
} else if (!nextMode.empty()) {
    // Mode transition occurred
}
```

**Built-in Commands:**

1. **mode** - Show current mode and list available modes:
```bash
git> mode
Current mode: git
Available modes:
  default
  git
  docker
```

2. **mode <name>** - Switch to specified mode:
```bash
git> mode docker
Switched to mode: docker
docker>
```

3. **exit** / **quit** - Exit application:
```bash
> exit
```

**API Methods:**
```cpp
std::string_view getCurrentMode() const;
bool setMode(std::string_view modeName);  // Programmatic mode switch
bool hasMode(std::string_view modeName) const;
std::vector<std::string> getModes() const;
```

**Interactive Application Pattern:**
```cpp
int main() {
    auto mgr = makeModeManager();
    
    // Setup modes
    mgr->addMode("default", defaultHandler);
    mgr->addMode("git", gitDispatcher);
    mgr->addMode("docker", dockerDispatcher);
    
    // Interactive loop
    std::string line;
    while (std::getline(std::cin, line)) {
        auto args = splitLine(line);  // Parse line into args
        
        std::string nextMode = mgr->execute(args);
        if (nextMode == "exit") break;
        
        // Show prompt with current mode
        std::cout << mgr->getCurrentMode() << "> ";
    }
    
    return 0;
}
```

---

## Parsing and Execution

### Option Format Flexibility

The library supports both formats interchangeably:

```bash
# With -- prefix
./app --port 8080 --host localhost

# Without -- prefix
./app port 8080 host localhost

# Mixed
./app --port 8080 host localhost
```

### Parsing Algorithm

1. **Identify options**: Check if argument matches option name (with or without `--`)
2. **Consume values**: 
   - Single values: Next argument
   - Arrays: Collect until next option or end
3. **Range validation**: For integer types, filter out-of-range values
4. **Store**: Place in `ParsedArgs::options` map
5. **Positional**: Arguments that don't match options go to `ParsedArgs::positional`

### argc/argv Interface

Native support for `main(int argc, char* argv[])` style arguments:

```cpp
int main(int argc, char* argv[]) {
    auto cmd = makeCommand(spec, handler);
    
    // Skip program name (argv[0]) and pass remaining arguments
    return cmd->execute(argc - 1, argv + 1) ? 0 : 1;
}
```

**Available for:**
- `Command::execute(int argc, char* argv[])`
- `Command::parse(int argc, char* argv[])`
- `SubcommandDispatcher::execute(int argc, char* argv[])`
- `ModeManager::execute(int argc, char* argv[])`

**Benefits:**
- No manual conversion to `vector<string>` needed
- Natural interface for command-line tools
- Internally converts to vector with proper lifetime management

### Integer Parsing

Exception-free integer parsing using `std::from_chars`:

```cpp
"42"      -> 42
"0x2A"    -> 42  (hex)
"0b101010" -> 42  (binary)
"invalid" -> std::nullopt  (no exception thrown)
```

**Implementation:**
- Uses `std::from_chars` for parsing
- Returns `std::nullopt` on parse errors (no exceptions)
- Validates that entire string was consumed

### Array Parsing

Arrays consume arguments until the next option or end:

```bash
./app files a.cpp b.cpp c.cpp       # All three files
./app files a.cpp b.cpp --next val  # Only a.cpp and b.cpp
```

### Range Validation

For `IntOption` and `IntArrayOption` with min/max:

**Compile-time:**
```cpp
constexpr auto opt = IntOption{"port", "Port", 1024, 65535};
static_assert(opt.isValid(8080), "8080 is valid");
static_assert(!opt.isValid(80), "80 is invalid");
```

**Runtime:**
```cpp
// During parsing, out-of-range values are silently filtered
// For single values: Option not set if invalid
// For arrays: Invalid values skipped
```

---

## Examples

### Example 1: Simple Server Command with argc/argv

```cpp
#include "cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main(int argc, char* argv[]) {
    // Define options
    constexpr auto opts = makeOptionGroup(
        "server",
        "Server options",
        IntOption{"port", "Port number", 1024, 65535},
        StringOption{"host", "Hostname"},
        IntOption{"workers", "Worker threads", 1, 64}
    );
    
    // Define command
    constexpr auto spec = CommandSpec<decltype(opts)>{
        "server",
        "Start the server",
        opts
    };
    
    // Create command with handler
    auto cmd = makeCommand(spec, [](const auto& args) {
        int64_t port = args.getInt("port").value_or(8080);
        std::string host = args.getString("host").value_or("localhost");
        int64_t workers = args.getInt("workers").value_or(4);
        
        std::cout << "Starting server...\n";
        std::cout << "  Host: " << host << "\n";
        std::cout << "  Port: " << port << "\n";
        std::cout << "  Workers: " << workers << "\n";
        
        return true;
    });
    
    // Parse and execute using argc/argv interface
    // Skip program name (argv[0])
    return cmd->execute(argc - 1, argv + 1) ? 0 : 1;
}
```

**Usage:**
```bash
./server --port 8080 --host 0.0.0.0 --workers 8
./server port 8080 host 0.0.0.0 workers 8  # Also valid
./server --port 0x1F90 --host localhost    # Hex values supported
```

### Example 2: Git-like Subcommands

```cpp
#include "cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main(int argc, char* argv[]) {
    // Define 'add' subcommand
    constexpr auto addOpts = makeOptionGroup(
        "add",
        "Add files",
        StringArrayOption{"files", "Files to add", true},
        IntOption{"verbose", "Verbosity", 0, 2}
    );
    constexpr auto addSpec = CommandSpec<decltype(addOpts)>{"add", "Add files", addOpts};
    
    auto addCmd = makeCommand(addSpec, [](const auto& args) {
        std::cout << "Adding files:\n";
        if (auto files = args.getStringArray("files")) {
            for (const auto& f : *files) {
                std::cout << "  + " << f << "\n";
            }
        }
        return true;
    });
    
    // Define 'commit' subcommand
    constexpr auto commitOpts = makeOptionGroup(
        "commit",
        "Commit changes",
        StringOption{"message", "Commit message", true}
    );
    constexpr auto commitSpec = CommandSpec<decltype(commitOpts)>{"commit", "Commit", commitOpts};
    
    auto commitCmd = makeCommand(commitSpec, [](const auto& args) {
        if (auto msg = args.getString("message")) {
            std::cout << "Committing: " << *msg << "\n";
        }
        return true;
    });
    
    // Create dispatcher
    auto git = makeDispatcher("git", "Git version control");
    git->addSubcommand(addCmd);
    git->addSubcommand(commitCmd);
    
    // Execute
    std::vector<std::string> args(argv + 1, argv + argc);
    return git->execute(args) ? 0 : 1;
}
```

**Usage:**
```bash
./git add files main.cpp test.cpp verbose 1
./git commit --message "Initial commit"
./git help
./git help add
```

### Example 3: Interactive Multi-Mode Application

```cpp
#include "cmdline_hdr_only.h"
#include <iostream>
#include <sstream>

using namespace cmdline_ct;

std::vector<std::string> splitLine(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int main() {
    auto mgr = makeModeManager();
    
    // Create git dispatcher
    auto gitDispatcher = makeDispatcher("git", "Git commands");
    // ... add git subcommands ...
    
    // Create docker dispatcher
    auto dockerDispatcher = makeDispatcher("docker", "Docker commands");
    // ... add docker subcommands ...
    
    // Default mode: route to tools
    mgr->addMode("default", [](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) return "";
        if (args[0] == "git") return "git";
        if (args[0] == "docker") return "docker";
        std::cerr << "Unknown tool: " << args[0] << "\n";
        return "";
    });
    
    mgr->addMode("git", gitDispatcher);
    mgr->addMode("docker", dockerDispatcher);
    
    // Interactive loop
    std::cout << mgr->getCurrentMode() << "> ";
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << mgr->getCurrentMode() << "> ";
            continue;
        }
        
        auto args = splitLine(line);
        std::string nextMode = mgr->execute(args);
        
        if (nextMode == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        std::cout << mgr->getCurrentMode() << "> ";
    }
    
    return 0;
}
```

**Interactive Session:**
```
default> git
Switching to git mode...
git> add files main.cpp test.cpp
Adding files:
  + main.cpp
  + test.cpp
git> commit message "Fix bug"
Committing: Fix bug
git> mode docker
Switched to mode: docker
docker> run image nginx
Starting container: nginx
docker> mode default
Switched to mode: default
default> exit
Goodbye!
```

### Example 4: Range Validation

```cpp
constexpr auto opts = makeOptionGroup(
    "validate",
    "Validation example",
    IntOption{"port", "Valid port", 1024, 65535},
    IntOption{"level", "Level 0-100", 0, 100},
    IntArrayOption{"scores", "Valid scores", 0, 100}
);

constexpr auto spec = CommandSpec<decltype(opts)>{"validate", "Test", opts};

auto cmd = makeCommand(spec, [](const ParsedArgs& args) {
    // Only valid values will be present
    if (auto port = args.getInt("port")) {
        std::cout << "Port: " << *port << "\n";  // Always in [1024, 65535]
    }
    
    if (auto scores = args.getIntArray("scores")) {
        std::cout << "Valid scores:\n";
        for (auto s : *scores) {
            std::cout << "  " << s << "\n";  // All in [0, 100]
        }
    }
    
    return true;
});
```

**Usage:**
```bash
# Valid
./app port 8080 scores 75 80 95 100
# Port: 8080
# Valid scores:
#   75
#   80
#   95
#   100

# Invalid values filtered
./app port 80 scores 75 150 -10 95
# (port not set, 80 < 1024)
# Valid scores:
#   75
#   95
# (150 and -10 filtered out)
```

---

## Best Practices

### 1. Use constexpr for Specifications

Always define options and specs as `constexpr`:

```cpp
// Good
constexpr auto opts = makeOptionGroup(...);
constexpr auto spec = CommandSpec<decltype(opts)>{...};

// Bad (unnecessary runtime overhead)
auto opts = makeOptionGroup(...);
auto spec = CommandSpec<decltype(opts)>{...};
```

### 2. Leverage Type Deduction

Use `decltype` for option group types:

```cpp
constexpr auto opts = makeOptionGroup(...);
constexpr auto spec = CommandSpec<decltype(opts)>{...};  // Good

// Instead of manually specifying complex template types
```

### 3. Handler Error Handling

Return `false` from handlers to indicate failure:

```cpp
auto handler = [](const ParsedArgs& args) {
    auto port = args.getInt("port");
    if (!port) {
        std::cerr << "Error: --port is required\n";
        return false;  // Indicate failure
    }
    // ... success path
    return true;
};
```

### 4. Use Range Validation

Prefer compile-time range validation over manual checks:

```cpp
// Good
constexpr auto portOpt = IntOption{"port", "Port", 1024, 65535};

// Less good (manual validation)
constexpr auto portOpt = IntOption{"port", "Port"};
auto handler = [](const ParsedArgs& args) {
    if (auto p = args.getInt("port")) {
        if (*p < 1024 || *p > 65535) {
            std::cerr << "Invalid port\n";
            return false;
        }
    }
    // ...
};
```

### 5. Organize Complex Applications

For large apps, organize by feature:

```cpp
// server.h
namespace server {
    constexpr auto options = makeOptionGroup(...);
    constexpr auto spec = CommandSpec<decltype(options)>{...};
    auto createCommand();
}

// client.h
namespace client {
    constexpr auto options = makeOptionGroup(...);
    constexpr auto spec = CommandSpec<decltype(options)>{...};
    auto createCommand();
}

// main.cpp
auto dispatcher = makeDispatcher("myapp", "My application");
dispatcher->addSubcommand(server::createCommand());
dispatcher->addSubcommand(client::createCommand());
```

### 6. Test with Both Formats

Test option parsing with both `--` and without:

```cpp
// Both should work
args = {"--port", "8080", "--host", "localhost"};
cmd->execute(args);

args = {"port", "8080", "host", "localhost"};
cmd->execute(args);
```

### 7. Provide Helpful Descriptions

Write clear, concise descriptions:

```cpp
// Good
IntOption{"port", "Server listening port (1024-65535)", 1024, 65535}

// Less helpful
IntOption{"port", "port", 1024, 65535}
```

### 8. Use Shared Pointers Correctly

Commands and dispatchers are automatically wrapped in `shared_ptr`:

```cpp
auto cmd = makeCommand(...);  // Returns shared_ptr<Command<...>>
auto disp = makeDispatcher(...);  // Returns shared_ptr<SubcommandDispatcher>

// Safe to share
mgr->addMode("mode1", cmd);
mgr->addMode("mode2", cmd);  // Same command in multiple modes
```

---

## Advanced Usage

### Custom Value Parsing

Extend `OptionValue::parseInt` for custom formats:

```cpp
// The library already supports:
// - Decimal: "42"
// - Hex: "0x2A"
// - Binary: "0b101010"

// For custom needs, parse in your handler:
auto handler = [](const ParsedArgs& args) {
    if (auto str = args.getString("custom")) {
        // Custom parsing logic
        auto value = parseCustomFormat(*str);
    }
    return true;
};
```

### Dynamic Option Lists

For options determined at runtime, use the map directly:

```cpp
auto handler = [](const ParsedArgs& args) {
    for (const auto& [name, value] : args.options) {
        std::cout << "Option: " << name << "\n";
        // Access value fields based on type
    }
    return true;
};
```

### Nested Subcommands

Chain dispatchers for deep hierarchies:

```cpp
// git remote add <name> <url>
auto remoteDispatcher = makeDispatcher("remote", "Remote management");
remoteDispatcher->addSubcommand(addRemoteCmd);
remoteDispatcher->addSubcommand(removeRemoteCmd);

auto gitDispatcher = makeDispatcher("git", "Git VCS");
gitDispatcher->addSubcommand(commitCmd);
gitDispatcher->addSubcommand(pushCmd);

// Wrap remote dispatcher as a command
// (Note: Current API doesn't directly support this, would need extension)
```

### Mode Transition Logic

Implement complex state machines with modes:

```cpp
mgr->addMode("init", [](const std::vector<std::string>& args) -> std::string {
    // Do initialization
    if (initSuccess()) {
        return "ready";  // Transition to ready mode
    }
    return "error";  // Transition to error mode
});

mgr->addMode("ready", [](const std::vector<std::string>& args) -> std::string {
    // Process commands
    if (args[0] == "start") {
        return "running";
    }
    return "";
});

mgr->addMode("running", [](const std::vector<std::string>& args) -> std::string {
    if (args[0] == "stop") {
        return "ready";
    }
    // Continue running
    return "";
});
```

### Compile-Time Validation

Use `static_assert` with constexpr functions:

```cpp
constexpr auto port = IntOption{"port", "Port", 1024, 65535};

static_assert(port.isValid(8080), "8080 should be valid");
static_assert(!port.isValid(80), "80 should be invalid");

// Ensure min < max at compile time
static_assert(port.min_value < port.max_value, "Invalid range");
```

### Thread Safety

The library is not thread-safe by design (for CLI apps). For multi-threaded usage:

```cpp
// Option 1: One ModeManager per thread
thread_local auto mgr = makeModeManager();

// Option 2: External synchronization
std::mutex mtx;
auto mgr = makeModeManager();

void threadFunc() {
    std::lock_guard<std::mutex> lock(mtx);
    mgr->execute(args);
}
```

---

## Performance Characteristics

### Compile-Time

- **Option specifications**: Zero runtime cost (constexpr)
- **CommandSpec**: Zero runtime cost (constexpr)
- **Template instantiation**: One instantiation per unique option group type

### Runtime

- **Command creation**: One allocation per command (`shared_ptr`)
- **Parsing**: O(n) where n = number of arguments
- **Option lookup**: O(m) where m = number of options (linear search in tuple)
- **Mode dispatch**: O(1) map lookup
- **Handler invocation**: Direct function call (no virtual dispatch)

### Memory

- **Per command**: ~100 bytes (shared_ptr + lambda capture)
- **Per mode**: ~50 bytes (map entry)
- **Parsed args**: Depends on argument count and array sizes

---

## Limitations

1. **Option lookup is linear**: Searching tuple of options is O(n)
   - Negligible for typical command-line tools (<20 options)
   - Compile-time access via `get<I>()` is O(1)
   
2. **No automatic help generation**: Must implement custom help
   - Built-in help for subcommands, but not for individual options
   
3. **No short options**: Only long option names supported
   - No `-p` for `--port` (could be added)
   
4. **No option aliases**: Each option has one name
   - Can't have both `--message` and `--msg`
   
5. **No dependency between options**: Can't express "if A then B required"
   - Must validate in handler
   
6. **Array parsing stops at next option**: Can't mix array values with options
   - `files a.cpp --verbose b.cpp` won't work as expected

7. **Tuple storage requires compile-time types**: Option groups must be known at compile time
   - Can't dynamically add/remove options at runtime
   - Use runtime name-based accessors for dynamic cases

---

## Future Enhancements

Possible additions (not currently implemented):

1. **Short options**: `-p 8080` in addition to `--port 8080`
2. **Option aliases**: `--message` and `-m`
3. **Auto-generated help**: Detailed help from option specifications
4. **Option dependencies**: Express relationships between options
5. **Validators**: Custom validation functions beyond min/max
6. **Environment variable support**: Fall back to env vars
7. **Configuration file support**: Load defaults from config files
8. **Completion generation**: Bash/zsh completion scripts

---

## Conclusion

`cmdline_hdr_only.h` provides a modern, type-safe approach to command-line parsing in C++17. Its key strengths are:

- **Zero-overhead abstractions**: Compile-time specifications
- **Type safety**: Compile-time option types with runtime enforcement
- **Flexibility**: Support for simple commands, subcommands, and interactive modes
- **Ease of use**: Minimal boilerplate, intuitive API

The library is ideal for:
- Modern C++ command-line tools
- Interactive CLI applications
- Tools with hierarchical commands (like git, docker)
- Applications requiring type-safe argument parsing

For comprehensive examples, see:
- `test_typed_options.cpp` - Basic option types
- `test_int_range.cpp`, `test_array_range.cpp` - Range validation
- `test_option_matching.cpp` - Option format flexibility
- `test_subcommands.cpp` - Hierarchical commands
- `test_mode_manager.cpp` - Interactive mode system
