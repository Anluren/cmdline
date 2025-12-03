# Mode Manager Design

## Overview

The `ModeManager` class provides a mechanism for managing different command modes in interactive applications. This allows users to transition between different command contexts, similar to how tools like `git`, `docker`, or database CLI tools work.

## Motivation

Interactive command-line tools often need to support different contexts or "modes". For example:
- A database CLI might have modes for different databases
- A network tool might have modes for different connection types
- A multi-tool application might switch between different sub-tools (git, docker, etc.)

The Mode Manager provides a clean abstraction for handling these mode transitions while integrating seamlessly with the existing command/subcommand infrastructure.

## Key Features

1. **Mode Registration**: Register any command, subcommand dispatcher, or custom handler as a mode
2. **Dynamic Transitions**: Switch between modes programmatically or via commands
3. **Mode-aware Execution**: Execute commands in the context of the current mode
4. **Built-in Commands**: 
   - `mode` - Check current mode or list available modes
   - `mode <name>` - Switch to a different mode
   - `exit` / `quit` - Exit the application
5. **Flexible Integration**: Works with `Command`, `SubcommandDispatcher`, or custom handlers

## Basic Usage

### Creating a Mode Manager

```cpp
auto mgr = makeModeManager();
```

### Adding Modes

#### From a SubcommandDispatcher

```cpp
auto gitDispatcher = makeDispatcher("git", "Git commands");
gitDispatcher->addSubcommand(addCmd);
gitDispatcher->addSubcommand(commitCmd);

mgr->addMode("git", gitDispatcher);
```

#### From a Command

```cpp
constexpr auto opts = makeOptionGroup("list", "List items",
    IntOption{"limit", "Max items", 10}
);
constexpr auto spec = CommandSpec<decltype(opts)>{"list", "List", opts};

auto cmd = makeCommand(spec, [](const ParsedArgs& args) {
    // Handler logic
    return true;
});

mgr->addMode("list", cmd);
```

#### Custom Mode Handler

```cpp
mgr->addMode("default", [](const std::vector<std::string>& args) -> std::string {
    if (args[0] == "git") {
        return "git";  // Transition to git mode
    }
    return "";  // Stay in current mode
});
```

### Executing Commands

```cpp
std::vector<std::string> args = {"add", "files", "main.cpp"};
std::string nextMode = mgr->execute(args);

if (nextMode == "exit") {
    // User wants to exit
} else if (!nextMode.empty()) {
    // Mode transition occurred
}
```

### Mode Queries

```cpp
// Get current mode
std::string current = mgr->getCurrentMode();

// Check if mode exists
bool exists = mgr->hasMode("git");

// Get all modes
std::vector<std::string> modes = mgr->getModes();

// Programmatically set mode
bool success = mgr->setMode("git");
```

## Mode Handler Interface

A mode handler is a function that:
- Takes `const std::vector<std::string>& args` (the command arguments)
- Returns `std::string` (the next mode name, or empty to stay in current mode)

```cpp
using ModeHandler = std::function<std::string(const std::vector<std::string>&)>;
```

### Return Values

- Empty string `""`: Stay in current mode
- Mode name: Transition to that mode
- `"exit"`: Signal application exit

## Interactive Application Pattern

```cpp
int main() {
    auto mgr = makeModeManager();
    
    // Set up modes...
    mgr->addMode("default", defaultHandler);
    mgr->addMode("git", gitDispatcher);
    mgr->addMode("docker", dockerDispatcher);
    
    // Interactive loop
    std::string line;
    while (std::getline(std::cin, line)) {
        // Parse line into args (simplified)
        std::vector<std::string> args = splitLine(line);
        
        std::string nextMode = mgr->execute(args);
        
        if (nextMode == "exit") {
            break;
        }
        
        // Show prompt with current mode
        std::cout << mgr->getCurrentMode() << "> ";
    }
    
    return 0;
}
```

## Example: Multi-Tool Application

```cpp
auto mgr = makeModeManager();

// Default mode: route to sub-tools
mgr->addMode("default", [](const std::vector<std::string>& args) -> std::string {
    if (args.empty()) return "";
    
    if (args[0] == "git") return "git";
    if (args[0] == "docker") return "docker";
    
    std::cerr << "Unknown tool: " << args[0] << "\n";
    return "";
});

// Git mode
auto gitDispatcher = makeDispatcher("git", "Git VCS");
// ... add git subcommands
mgr->addMode("git", gitDispatcher);

// Docker mode
auto dockerDispatcher = makeDispatcher("docker", "Container management");
// ... add docker subcommands
mgr->addMode("docker", dockerDispatcher);

// Usage:
// > git              # Switch to git mode
// git> add files main.cpp
// git> commit message "Fix"
// git> mode docker   # Switch to docker mode
// docker> run image nginx
// docker> mode default  # Back to default
// > exit             # Exit application
```

## Advanced: Mode Transition Logic

Mode handlers can implement sophisticated transition logic:

```cpp
mgr->addMode("config", [](const std::vector<std::string>& args) -> std::string {
    if (args[0] == "save") {
        saveConfig();
        std::cout << "Config saved. Returning to default mode.\n";
        return "default";  // Auto-transition after save
    }
    
    if (args[0] == "edit") {
        std::string section = args.size() > 1 ? args[1] : "general";
        return "edit_" + section;  // Transition to section-specific mode
    }
    
    return "";  // Stay in config mode
});
```

## Built-in Commands

### `mode` Command

Check current mode:
```
> mode
Current mode: git
Available modes:
  default
  git
  docker
```

Switch mode:
```
git> mode docker
Switched to mode: docker
docker>
```

### `exit` / `quit` Commands

Exit the application:
```
> exit
```

## Design Considerations

### Mode Isolation

Each mode operates independently. Commands are parsed and executed in the context of the current mode only.

### State Management

The `ModeManager` tracks the current mode name but doesn't manage mode-specific state. Each mode handler should manage its own state if needed.

### Error Handling

- Unknown modes: Error message, stay in current mode
- Invalid commands in mode: Handled by mode handler
- Mode transitions: Validated before execution

### Integration with Existing Features

The Mode Manager works seamlessly with:
- `Command` - Single commands can be modes
- `SubcommandDispatcher` - Dispatchers handle complex mode behavior
- Typed options - All option types work in any mode
- Range validation - Validated within each mode
- Flexible option parsing - Both `--option` and `option` formats work

## Testing

See `test_mode_manager.cpp` for comprehensive examples including:
- Mode registration and transitions
- Subcommand dispatchers as modes
- Interactive workflow simulation
- Programmatic mode management

## Implementation Notes

- Thread safety: Not thread-safe by design (for single-threaded CLI apps)
- Performance: HashMap-based mode lookup (O(1) average)
- Memory: Shared pointers for command/dispatcher storage
- Dependencies: Requires `<functional>`, `<map>`, `<vector>`, `<iostream>`
