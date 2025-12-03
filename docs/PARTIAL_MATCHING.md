# Partial Command Matching

## Overview

The cmdline library supports partial (abbreviated) command matching, allowing users to type shortened versions of commands as long as they are unambiguous. This feature works for both subcommands in `SubcommandDispatcher` and modes in `ModeManager`.

## How It Works

### Matching Strategy

1. **Exact Match Priority**: If an exact match exists, it is always used regardless of partial matches
2. **Prefix Matching**: If no exact match exists, all commands starting with the input prefix are considered
3. **Unique Match**: If exactly one command matches the prefix, it is executed
4. **Ambiguous Match**: If multiple commands match the prefix, an error is shown with suggestions
5. **No Match**: If no commands match, a standard "unknown command" error is shown

### Examples

#### Unambiguous Partial Matches

Given commands: `start`, `stop`, `status`

```bash
> star        # Matches only 'start' → executes 'start'
> sto         # Matches only 'stop' → executes 'stop'
> stat        # Matches only 'status' → executes 'status'
```

#### Ambiguous Partial Matches

```bash
> sta         # Matches both 'start' and 'status'
Ambiguous command 'sta'. Did you mean:
  start
  status
Unknown subcommand: sta
Run 'help' for usage.

> s           # Matches all three: 'start', 'status', 'stop'
Ambiguous command 's'. Did you mean:
  start
  status
  stop
Unknown subcommand: s
Run 'help' for usage.
```

#### Exact Match Takes Priority

```bash
> st          # If you have commands 'st', 'start', 'stop'
              # → executes 'st' (exact match), not ambiguous
```

## Implementation Details

### SubcommandDispatcher

The `SubcommandDispatcher` class uses `findCommand()` to locate commands:

```cpp
auto findCommand(std::string_view cmd) const {
    // Try exact match first
    auto it = m_handlers.find(std::string(cmd));
    if (it != m_handlers.end()) {
        return it;
    }
    
    // Try partial match
    std::vector<decltype(it)> matches;
    for (auto iter = m_handlers.begin(); iter != m_handlers.end(); ++iter) {
        if (iter->first.starts_with(cmd)) {
            matches.push_back(iter);
        }
    }
    
    // Return unique match or end() if ambiguous/not found
    if (matches.size() == 1) {
        return matches[0];
    }
    
    // Handle ambiguous matches
    if (matches.size() > 1) {
        std::cerr << "Ambiguous command '" << cmd << "'. Did you mean:\n";
        for (const auto& match : matches) {
            std::cerr << "  " << match->first << "\n";
        }
    }
    
    return m_handlers.end();
}
```

### ModeManager

The `ModeManager` class uses a similar `findMode()` method for mode switching:

```cpp
auto findMode(std::string_view modeName) const {
    // Try exact match first
    auto it = m_modes.find(std::string(modeName));
    if (it != m_modes.end()) {
        return it;
    }
    
    // Try partial match (same logic as findCommand)
    // ...
}
```

When switching modes with the `mode` command, users can use abbreviations:

```bash
[main]> mode dev         # Switches to 'development' mode
[dev]> mode prod         # Switches to 'production' mode
```

## Usage in Applications

### SubcommandDispatcher Example

```cpp
auto dispatcher = makeSubcommandDispatcher();

// Add commands
auto startCmd = makeCommand(/* ... */);
auto stopCmd = makeCommand(/* ... */);
auto statusCmd = makeCommand(/* ... */);

dispatcher->addSubcommand(startCmd);
dispatcher->addSubcommand(stopCmd);
dispatcher->addSubcommand(statusCmd);

// Users can now use abbreviated commands:
dispatcher->execute("star");    // Executes 'start'
dispatcher->execute("sto");     // Executes 'stop'
dispatcher->execute("stat");    // Executes 'status'
dispatcher->execute("sta");     // Error: ambiguous (matches 'start' and 'status')
```

### ModeManager Example

```cpp
auto modeManager = makeModeManager();

// Add modes
modeManager->addMode("development", devCmd);
modeManager->addMode("production", prodCmd);
modeManager->addMode("testing", testCmd);

modeManager->setMode("development");

// Users can switch modes with abbreviations:
modeManager->executeCommand("mode prod");  // Switches to 'production'
modeManager->executeCommand("mode dev");   // Switches to 'development'
modeManager->executeCommand("mode test");  // Switches to 'testing'
modeManager->executeCommand("mode te");    // Switches to 'testing' (if unambiguous)
```

## Benefits

1. **Improved User Experience**: Users can type less, especially for frequently used commands
2. **Backward Compatible**: Exact command names still work perfectly
3. **Safe**: Ambiguous abbreviations are caught and reported with helpful suggestions
4. **Intuitive**: Users naturally try abbreviations, and this feature makes it work
5. **No Configuration**: Works automatically for all commands and modes

## Testing

The feature is comprehensively tested in `tests/test_partial_matching.cpp`, which covers:

- Exact matches
- Unique partial matches
- Ambiguous partial matches
- Unknown commands
- Mode switching with partial names

Run the test:

```bash
cd build/tests
./test_partial_matching
```

## Design Considerations

### Why Prefix Matching Only?

The implementation uses prefix matching (commands that *start with* the input) rather than fuzzy matching or substring matching because:

1. **Predictability**: Users can predict what abbreviations will work
2. **Performance**: O(n) linear scan is acceptable for typical command sets
3. **Simplicity**: Clear and easy to understand behavior
4. **Standards**: Matches behavior of common CLI tools (git, docker, etc.)

### Why Exact Match Priority?

If you have both a command `st` and `start`, typing `st` should execute the `st` command, not report ambiguity. This allows:

- Short command names to coexist with longer ones
- Commands to be added without breaking existing abbreviations
- Natural command hierarchies (e.g., `get`, `get-all`, `get-list`)

## Related Features

- **Command Completion**: Auto-completion with Tab key (see main README)
- **Command Suggestions**: "Did you mean?" for typos (existing feature)
- **Subcommands**: Hierarchical command structure (see main README)
- **Mode Switching**: Multi-level mode navigation (see MODE_MANAGER_DESIGN.md)

## Future Enhancements

Potential improvements for future versions:

1. **Configurable Matching**: Allow applications to disable partial matching if desired
2. **Minimum Prefix Length**: Require at least N characters for partial matching
3. **Fuzzy Matching**: Suggest commands even with typos (Levenshtein distance)
4. **Completion Hints**: Show possible completions as user types
5. **Alias Support**: Explicit short aliases for commands (e.g., `ls` for `list`)
