# Help Query Feature (? Syntax)

## Overview

The cmdline library supports an intuitive help query syntax using the `?` character. Users can append `?` to commands or prefixes to quickly discover available subcommands or modes without needing to type `help`.

## Syntax

### Show All Items

Use a single `?` to show all available items:

```bash
# In SubcommandDispatcher - show all subcommands
?

# In ModeManager - show all modes
mode ?
```

### Show Matching Items

Use `prefix?` to show all items that start with the given prefix:

```bash
# Show all subcommands starting with "sta"
sta?

# Show all modes starting with "dev"
mode dev?
```

## Examples

### SubcommandDispatcher

Given a dispatcher with commands: `start`, `stop`, `status`, `restart`

```bash
> ?
Available subcommands:
  restart
  start
  status
  stop

> s?
Subcommands matching 's':
  start
  status
  stop

> sta?
Subcommands matching 'sta':
  start
  status

> r?
Subcommands matching 'r':
  restart

> xyz?
No subcommands matching 'xyz'
```

### ModeManager

Given modes: `development`, `production`, `testing`

```bash
> mode ?
Available modes:
  development
  production
  testing

> mode dev?
Modes matching 'dev':
  development

> mode p?
Modes matching 'p':
  production

> mode xyz?
No modes matching 'xyz'
```

## Implementation Details

### SubcommandDispatcher

The `execute()` method checks if the command ends with `?`:

```cpp
std::string subcmdName = args[0];

// Handle ? query syntax
if (!subcmdName.empty() && subcmdName.back() == '?') {
    if (subcmdName.length() == 1) {
        // Just "?" - show all subcommands
        showHelp();
    } else {
        // "prefix?" - show matching subcommands
        std::string prefix = subcmdName.substr(0, subcmdName.length() - 1);
        showMatchingCommands(prefix);
    }
    return true;
}
```

The `showMatchingCommands()` method filters by prefix:

```cpp
void showMatchingCommands(const std::string& prefix) const {
    std::vector<std::string> matches;
    for (const auto& [name, _] : m_handlers) {
        if (name.compare(0, prefix.length(), prefix) == 0) {
            matches.push_back(name);
        }
    }
    
    if (matches.empty()) {
        std::cout << "No subcommands matching '" << prefix << "'\n";
    } else {
        std::cout << "Subcommands matching '" << prefix << "':\n";
        for (const auto& name : matches) {
            std::cout << "  " << name << "\n";
        }
    }
}
```

### ModeManager

Similar logic for mode queries:

```cpp
// Handle ? query syntax for modes
if (cmd == "mode" && args.size() > 1 && !args[1].empty() && args[1].back() == '?') {
    std::string query = args[1];
    if (query.length() == 1) {
        // "mode ?" - show all modes
        std::cout << "Available modes:\n";
        for (const auto& [name, _] : m_modes) {
            std::cout << "  " << name << "\n";
        }
    } else {
        // "mode prefix?" - show matching modes
        std::string prefix = query.substr(0, query.length() - 1);
        // ... filter and display matches
    }
    return "";
}
```

## Use Cases

### Quick Discovery

Users can quickly explore what's available without reading documentation:

```bash
# What can I do?
> ?

# What commands start with 's'?
> s?

# What modes are available?
> mode ?
```

### Command Completion Aid

When users partially remember a command:

```bash
# "I know there's a 'status' command but can't remember the exact name"
> sta?
Subcommands matching 'sta':
  start
  status

# "Ah yes, 'status' is what I need"
> status
```

### Learning Tool

New users can explore the interface incrementally:

```bash
# Start broad
> ?
Available subcommands:
  config
  network
  server
  system

# Narrow down
> net?
Subcommands matching 'net':
  network

# Discover submodes
> mode ?
Available modes:
  development
  production
  testing
```

## Benefits

1. **Intuitive**: The `?` character is universally associated with help/questions
2. **Fast**: Quicker than typing `help` or `--help`
3. **Context-Aware**: Shows only relevant matches based on prefix
4. **Discoverable**: Users naturally try `?` when exploring
5. **Non-Disruptive**: Doesn't execute anything, just provides information

## Relationship to Other Features

### vs. `help` Command

- `?` is faster to type
- `?` can filter by prefix (`sta?` vs `help | grep sta`)
- `help` may show more detailed information (descriptions, usage)
- Both serve similar discovery purposes

### vs. Partial Matching

- **Help Query (`?`)**: Shows what's available, doesn't execute anything
- **Partial Matching**: Executes commands with abbreviated names

Example:
```bash
> sta?         # Shows: start, status (help query - no execution)
> sta          # Error: ambiguous (partial matching - would execute if unique)
> star         # Executes: start (partial matching - unique match)
```

### vs. Auto-Completion (Tab)

- **Help Query**: Displays list of matches in terminal
- **Tab Completion**: Completes the command interactively
- Both help users discover commands

Example:
```bash
> sta?         # Displays: start, status
> sta<Tab>     # Shows completion options interactively
```

## Testing

The feature is tested in `tests/test_help_query.cpp`, which covers:

- Querying all subcommands with `?`
- Querying with various prefixes
- Querying with no matches
- Mode queries with `mode ?` and `mode prefix?`

Run the test:

```bash
cd build/tests
./test_help_query
```

## Design Decisions

### Why `?` at the End?

Placing `?` at the end of the prefix (rather than beginning or standalone) makes it:

1. **Readable**: `sta?` reads naturally as "what starts with 'sta'?"
2. **Unambiguous**: Won't conflict with command names starting with `?`
3. **Consistent**: Matches query syntax in some shells and tools
4. **Intuitive**: Question mark naturally comes after what you're asking about

### Why Not Search in Descriptions?

The current implementation only matches command/mode names, not descriptions:

**Pros of name-only matching:**
- Faster (no need to scan descriptions)
- More predictable (users know what will match)
- Cleaner output (fewer false positives)

**Future enhancement:** Could add description search with a different syntax (e.g., `??search term`)

### Prefix vs. Substring Matching

Currently uses prefix matching (commands that *start with* the query):

```bash
> sta?    # Matches: start, status
          # Doesn't match: restart (doesn't start with 'sta')
```

**Rationale:**
- Consistent with partial command matching behavior
- More predictable for users
- Faster to implement and execute

## Future Enhancements

1. **Verbose Mode**: `??` could show detailed help with descriptions
2. **Regex Support**: `sta.*?` for pattern matching
3. **Description Search**: `?? server` to search in descriptions
4. **Fuzzy Matching**: `strt?` suggests `start` even with typo
5. **History Search**: `!?pattern` to search command history

## Integration Example

```cpp
// Create dispatcher
auto dispatcher = makeDispatcher("myapp", "My application");

// Add commands
dispatcher->addSubcommand(startCmd);
dispatcher->addSubcommand(stopCmd);
dispatcher->addSubcommand(statusCmd);

// Users can now query:
dispatcher->execute({"?"});        // Show all
dispatcher->execute({"sta?"});     // Show commands starting with 'sta'
dispatcher->execute({"start"});    // Execute start command
```

No additional configuration needed - the feature works automatically!
