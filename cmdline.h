/**
 * Command Line Library - C++17
 * 
 * A library for building interactive command-line interfaces with:
 * - Multi-level mode support
 * - Auto-completion using readline
 * - Command matching with prefix search
 */

#ifndef CMDLINE_H
#define CMDLINE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <algorithm>

namespace cmdline {

// Forward declarations
class Command;
class Mode;
class CommandLineInterface;

/**
 * Parsed option value with type information
 */
struct OptionValue {
    std::string name;
    int64_t intValue;
    std::string stringValue;
    bool isInteger;
    
    OptionValue() : intValue(0), isInteger(false) {}
    
    // Parse integer from string (supports 0x hex, 0b binary, decimal)
    static std::optional<int64_t> parseInt(const std::string& str) {
        if (str.empty()) return std::nullopt;
        
        try {
            // Hexadecimal: 0x or 0X prefix
            if (str.size() > 2 && (str[0] == '0') && (str[1] == 'x' || str[1] == 'X')) {
                return std::stoll(str.substr(2), nullptr, 16);
            }
            // Binary: 0b or 0B prefix
            else if (str.size() > 2 && (str[0] == '0') && (str[1] == 'b' || str[1] == 'B')) {
                return std::stoll(str.substr(2), nullptr, 2);
            }
            // Decimal
            else {
                return std::stoll(str, nullptr, 10);
            }
        } catch (...) {
            return std::nullopt;
        }
    }
    
    int64_t asInt() const { return intValue; }
    const std::string& asString() const { return stringValue; }
};

/**
 * Parsed command arguments with options
 */
struct ParsedArgs {
    std::vector<std::string> positional;  // Positional arguments
    std::map<std::string, OptionValue> options;  // Named options
    
    bool hasOption(const std::string& name) const {
        return options.find(name) != options.end();
    }
    
    std::optional<int64_t> getInt(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end() && it->second.isInteger) {
            return it->second.intValue;
        }
        return std::nullopt;
    }
    
    std::optional<std::string> getString(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second.stringValue;
        }
        return std::nullopt;
    }
};

/**
 * Command handler function type
 * Takes parsed arguments and returns true to continue, false to exit
 */
using CommandHandler = std::function<bool(const ParsedArgs&)>;

/**
 * Represents a single command with its handler and metadata
 */
class Command {
public:
    Command(const std::string& name, CommandHandler handler, 
            const std::string& description = "");
    
    // Add a subcommand to this command
    void addSubcommand(std::shared_ptr<Command> subcommand);
    
    // Define an option for this command
    void addOption(const std::string& name, const std::string& description = "");
    
    // Get all subcommands that match the given prefix
    std::vector<std::string> getMatchingCommands(const std::string& prefix) const;
    
    // Execute the command with given arguments
    bool execute(const std::vector<std::string>& args) const;
    
    // Parse arguments into ParsedArgs structure
    ParsedArgs parseArguments(const std::vector<std::string>& args) const;
    
    // Getters
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    const std::map<std::string, std::shared_ptr<Command>>& getSubcommands() const { 
        return subcommands_; 
    }
    const std::map<std::string, std::string>& getOptions() const {
        return options_;
    }

private:
    std::string name_;
    CommandHandler handler_;
    std::string description_;
    std::map<std::string, std::shared_ptr<Command>> subcommands_;
    std::map<std::string, std::string> options_;  // option name -> description
};

/**
 * Represents a command mode with its own set of commands
 */
class Mode : public std::enable_shared_from_this<Mode> {
public:
    Mode(const std::string& name, const std::string& prompt = "> ");
    
    // Add a command to this mode
    void addCommand(std::shared_ptr<Command> command);
    
    // Add a submode to this mode
    void addSubmode(std::shared_ptr<Mode> submode);
    
    // Get all commands and modes that match the given prefix
    std::vector<std::string> getMatchingCommands(const std::string& prefix) const;
    
    // Get a command by name
    std::shared_ptr<Command> getCommand(const std::string& name) const;
    
    // Get a submode by name
    std::shared_ptr<Mode> getSubmode(const std::string& name) const;
    
    // Getters
    const std::string& getName() const { return name_; }
    const std::string& getPrompt() const { return prompt_; }
    std::shared_ptr<Mode> getParent() const { return parent_.lock(); }
    void setParent(std::shared_ptr<Mode> parent) { parent_ = parent; }
    
    const std::map<std::string, std::shared_ptr<Command>>& getCommands() const {
        return commands_;
    }
    
    const std::map<std::string, std::shared_ptr<Mode>>& getSubmodes() const {
        return submodes_;
    }

private:
    void addDefaultCommands();
    bool helpHandler(const ParsedArgs& args);
    bool exitHandler(const ParsedArgs& args);
    
    std::string name_;
    std::string prompt_;
    std::weak_ptr<Mode> parent_;
    std::map<std::string, std::shared_ptr<Command>> commands_;
    std::map<std::string, std::shared_ptr<Mode>> submodes_;
};

/**
 * Main CLI class that handles input, parsing, and execution
 */
class CommandLineInterface {
public:
    explicit CommandLineInterface(std::shared_ptr<Mode> rootMode);
    
    // Start the CLI
    void run();
    
    // List all commands matching the given prefix in current mode
    std::vector<std::string> listMatchingCommands(const std::string& prefix) const;
    
    // Get the current prompt string
    std::string getPrompt() const;
    
    // Parse and execute a command line
    bool parseAndExecute(const std::string& line);

private:
    void enterMode(std::shared_ptr<Mode> mode);
    void exitMode();
    void runMode();
    
    // Input handling with completion
    std::string readLineWithCompletion(const std::string& prompt);
    void handleTabCompletion(std::string& line, size_t& cursorPos);
    std::vector<std::string> getCompletions(const std::string& line) const;
    
    std::shared_ptr<Mode> rootMode_;
    std::shared_ptr<Mode> currentMode_;
    std::vector<std::shared_ptr<Mode>> modeStack_;
    std::vector<std::string> commandHistory_;
    size_t historyIndex_;
    bool running_;
};

} // namespace cmdline

#endif // CMDLINE_H
