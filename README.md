# Command Line Library (C++17)

A C++17 library for building interactive command-line interfaces with multi-level mode support, auto-completion, and intelligent command matching.

## Features

- **Multi-level Mode Support**: Create hierarchical command modes that users can navigate through
- **Help Query Syntax**: Use `?` to discover commands - type `?` for all commands, `prefix?` for matching commands (e.g., `sta?` shows `start` and `status`)
- **Partial Command Matching**: Type abbreviated commands (e.g., `sta` for `start`) - exact matches take priority, ambiguous matches are reported with suggestions
- **Auto-completion**: Full tab completion for commands and subcommands (custom implementation)
- **Command Matching**: Automatically suggests similar commands when users enter partial or incorrect commands
- **Subcommands**: Support for nested command structures (e.g., `config set`, `config get`)
- **Command History**: Navigate previous commands with ↑/↓ arrow keys
- **Modern C++17**: Uses smart pointers, std::function, std::optional, and other C++17 features
- **Zero Dependencies**: Pure standard library implementation - no external libraries needed!
- **Easy Integration**: Simple API for adding commands and modes

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher (optional - Makefile also provided)
- **No external dependencies** - pure standard library implementation!

## Installation

### Build the library

```bash
mkdir build
cd build
cmake ..
make
```

### Run the example

```bash
./example
```

## Quick Start

```cpp
#include "cmdline.h"

using namespace cmdline;

bool myHandler(const std::vector<std::string>& args) {
    std::cout << "Command executed!\n";
    return true; // Return true to continue, false to exit mode
}

int main() {
    // Create a root mode
    auto root = std::make_shared<Mode>("main", "> ");
    
    // Add a command
    auto cmd = std::make_shared<Command>("mycommand", myHandler, "My command");
    root->addCommand(cmd);
    
    // Create and run the CLI
    CommandLineInterface cli(root);
    cli.run();
    
    return 0;
}
```

## Usage Guide

### Creating Commands

Commands are created with a name, handler function, and optional description:

```cpp
bool showHandler(const std::vector<std::string>& args) {
    std::cout << "Args: ";
    for (const auto& arg : args) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    return true;
}

auto cmd = std::make_shared<Command>("show", showHandler, 
                                     "Display information");
```

### Adding Subcommands

Commands can have subcommands for complex operations:

```cpp
auto configCmd = std::make_shared<Command>("config", configHandler,
                                           "Configuration management");

// Add subcommands
configCmd->addSubcommand(std::make_shared<Command>("set", setHandler,
                                                    "Set a value"));
configCmd->addSubcommand(std::make_shared<Command>("get", getHandler,
                                                    "Get a value"));
configCmd->addSubcommand(std::make_shared<Command>("list", listHandler,
                                                    "List all configs"));

root->addCommand(configCmd);
```

Users can then execute:
- `config set timeout 30`
- `config get timeout`
- `config list`

### Creating Modes

Modes represent different contexts with their own command sets:

```cpp
// Create a mode
auto networkMode = std::make_shared<Mode>("network", "net> ");

// Add commands to the mode
networkMode->addCommand(std::make_shared<Command>("status", statusHandler,
                                                   "Show network status"));
networkMode->addCommand(std::make_shared<Command>("connect", connectHandler,
                                                   "Connect to network"));

// Add as submode of root
root->addSubmode(networkMode);
```

Users type `network` to enter the network mode, and `exit` to return.

### Multi-level Mode Hierarchy

Modes can be nested to create deep hierarchies:

```cpp
// Create root mode
auto root = std::make_shared<Mode>("main", "> ");

// Create network mode
auto networkMode = std::make_shared<Mode>("network", "net> ");
root->addSubmode(networkMode);

// Create wifi mode under network
auto wifiMode = std::make_shared<Mode>("wifi", "wifi> ");
networkMode->addSubmode(wifiMode);

// Prompt will show: [main/network/wifi]wifi>
```

### Auto-completion

The library provides automatic tab completion:

- Press **Tab** to complete command names
- Press **Tab** to complete subcommand names
- Works with partial matches

Example:
```
[main]> con<Tab>
config    connect
[main]> config s<Tab>
[main]> config set
```

### Help Query (? Syntax)

Users can quickly discover available commands using the `?` character:

```
[main]> ?                    # Show all commands
[main]> s?                   # Show commands starting with 's': start, status, stop
[main]> sta?                 # Show commands starting with 'sta': start, status
[main]> mode ?               # Show all available modes
[main]> mode dev?            # Show modes starting with 'dev': development
```

Features:
- **Quick discovery**: `?` shows all items, `prefix?` shows matching items
- **Works everywhere**: SubcommandDispatcher for commands, ModeManager for modes
- **No execution**: Just displays information, never executes commands
- **Helpful for learning**: Explore the interface without reading docs

See [HELP_QUERY.md](docs/HELP_QUERY.md) for detailed documentation.

### Partial Command Matching

Users can type abbreviated commands as long as they are unambiguous:

```
[main]> star      # Matches 'start' if it's the only command starting with 'star'
[main]> sta       # Error if both 'start' and 'status' exist - suggests both
```

Features:
- **Exact matches take priority**: If both `st` and `start` exist, `st` executes the `st` command
- **Unambiguous abbreviations work**: `star` → `start`, `stat` → `status`
- **Ambiguous matches are reported**: `sta` matching both `start` and `status` shows helpful error
- **Works for mode switching**: `mode dev` → switches to `development` mode

See [PARTIAL_MATCHING.md](docs/PARTIAL_MATCHING.md) for detailed documentation.

### Command Matching

When users enter an unknown command, the library suggests matches:

```
[main]> sho
Unknown command 'sho'. Did you mean one of these?
  show
```

## API Reference

### Command Class

```cpp
Command(const std::string& name, 
        CommandHandler handler, 
        const std::string& description = "")
```

**Parameters:**
- `name`: Command name
- `handler`: Function with signature `bool(const std::vector<std::string>&)`
- `description`: Help text (optional)

**Methods:**
- `void addSubcommand(std::shared_ptr<Command> subcommand)`: Add a subcommand
- `std::vector<std::string> getMatchingCommands(const std::string& prefix)`: Get matching subcommands
- `bool execute(const std::vector<std::string>& args)`: Execute the command

### Mode Class

```cpp
Mode(const std::string& name, const std::string& prompt = "> ")
```

**Parameters:**
- `name`: Mode name (shown in prompt)
- `prompt`: Prompt suffix (default: `"> "`)

**Methods:**
- `void addCommand(std::shared_ptr<Command> command)`: Add a command
- `void addSubmode(std::shared_ptr<Mode> submode)`: Add a submode
- `std::vector<std::string> getMatchingCommands(const std::string& prefix)`: Get matching items
- `std::shared_ptr<Command> getCommand(const std::string& name)`: Get command by name
- `std::shared_ptr<Mode> getSubmode(const std::string& name)`: Get submode by name

**Default Commands:**
- `help`: Shows available commands and submodes
- `exit`: Exits current mode (or application if in root mode)

### CommandLineInterface Class

```cpp
CommandLineInterface(std::shared_ptr<Mode> rootMode)
```

**Parameters:**
- `rootMode`: The root mode of the CLI

**Methods:**
- `void run()`: Start the interactive CLI
- `bool parseAndExecute(const std::string& line)`: Parse and execute a command
- `std::vector<std::string> listMatchingCommands(const std::string& prefix)`: List matching commands
- `std::string getPrompt()`: Get current prompt string

## Example Session

```
Welcome to main
Type 'help' for available commands.
[main]> help

Available commands in 'main' mode:
  config               - Configuration management
    get                - Get a configuration value
    list               - List all configurations
    set                - Set a configuration value
  exit                 - Exit current mode or application
  help                 - Show available commands
  show                 - Display information

Available submodes:
  network
  system

[main]> config set timeout 60
Setting timeout = 60
[main]> network
[main/network]net> status
Network Status: Connected
IP Address: 192.168.1.100
Gateway: 192.168.1.1
[main/network]net> wifi
[main/network/wifi]wifi> scan
Scanning for WiFi networks...
  MyNetwork (Signal: Strong)
  GuestWiFi (Signal: Medium)
  CoffeeShop (Signal: Weak)
[main/network/wifi]wifi> exit
[main/network]net> exit
[main]> exit
Goodbye!
```

## Building Your Own Application

Create a simple `Makefile`:

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

myapp: myapp.cpp cmdline.cpp
	$(CXX) $(CXXFLAGS) -o myapp myapp.cpp cmdline.cpp

clean:
	rm -f myapp
```

Or use CMake:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp)

set(CMAKE_CXX_STANDARD 17)

add_executable(myapp myapp.cpp cmdline.cpp)
```

## License

MIT License - feel free to use in your projects.

## Contributing

Contributions welcome! Please ensure code follows C++17 standards and includes appropriate documentation.
