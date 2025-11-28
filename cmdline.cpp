/**
 * Command Line Library Implementation - C++17
 * Pure standard library implementation - no external dependencies
 */

#include "cmdline.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <cstring>

namespace cmdline {

// ============================================================================
// Terminal control helpers
// ============================================================================

namespace {
    // Save original terminal settings
    struct termios orig_termios;
    bool termios_saved = false;

    void disableRawMode() {
        if (termios_saved) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        }
    }

    void enableRawMode() {
        if (!termios_saved) {
            tcgetattr(STDIN_FILENO, &orig_termios);
            termios_saved = true;
            atexit(disableRawMode);
        }
        
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    int readKey() {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == '\x1b') {
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) != 1) return c;
                if (read(STDIN_FILENO, &seq[1], 1) != 1) return c;
                
                if (seq[0] == '[') {
                    if (seq[1] >= '0' && seq[1] <= '9') {
                        if (read(STDIN_FILENO, &seq[2], 1) != 1) return c;
                        if (seq[2] == '~') {
                            switch (seq[1]) {
                                case '3': return 127; // Delete
                            }
                        }
                    } else {
                        switch (seq[1]) {
                            case 'A': return 1000; // Up arrow
                            case 'B': return 1001; // Down arrow
                            case 'C': return 1002; // Right arrow
                            case 'D': return 1003; // Left arrow
                            case 'H': return 1004; // Home
                            case 'F': return 1005; // End
                        }
                    }
                }
            }
            return c;
        }
        return -1;
    }
}

// ============================================================================
// Command Implementation
// ============================================================================

Command::Command(const std::string& name, CommandHandler handler, 
                 const std::string& description)
    : name_(name), handler_(handler), description_(description) {
}

void Command::addSubcommand(std::shared_ptr<Command> subcommand) {
    subcommands_[subcommand->getName()] = subcommand;
}

void Command::addOption(const std::string& name, const std::string& description) {
    options_[name] = description;
}

ParsedArgs Command::parseArguments(const std::vector<std::string>& args) const {
    ParsedArgs parsed;
    
    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        
        // Check if it's an option (starts with --)
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') {
            std::string optName = arg.substr(2);
            
            // Check if this is a defined option
            if (options_.find(optName) != options_.end()) {
                OptionValue optVal;
                optVal.name = optName;
                
                // Get the value (next argument)
                if (i + 1 < args.size()) {
                    ++i;
                    optVal.stringValue = args[i];
                    
                    // Try to parse as integer
                    auto intVal = OptionValue::parseInt(args[i]);
                    if (intVal) {
                        optVal.intValue = *intVal;
                        optVal.isInteger = true;
                    }
                    
                    parsed.options[optName] = optVal;
                } else {
                    // Option without value - treat as flag with empty string
                    optVal.stringValue = "";
                    parsed.options[optName] = optVal;
                }
            } else {
                // Unknown option - treat as positional
                parsed.positional.push_back(arg);
            }
        } else {
            // Positional argument
            parsed.positional.push_back(arg);
        }
    }
    
    return parsed;
}

std::vector<std::string> Command::getMatchingCommands(const std::string& prefix) const {
    std::vector<std::string> matches;
    for (const auto& [name, cmd] : subcommands_) {
        if (name.find(prefix) == 0) {
            matches.push_back(name);
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

bool Command::execute(const std::vector<std::string>& args) const {
    ParsedArgs parsed = parseArguments(args);
    return handler_(parsed);
}

// ============================================================================
// Mode Implementation
// ============================================================================

Mode::Mode(const std::string& name, const std::string& prompt)
    : name_(name), prompt_(prompt) {
    addDefaultCommands();
}

void Mode::addDefaultCommands() {
    // Add help command
    auto helpCmd = std::make_shared<Command>("help", 
        [this](const ParsedArgs& args) {
            return this->helpHandler(args);
        }, 
        "Show available commands");
    commands_["help"] = helpCmd;
    
    // Add exit command
    auto exitCmd = std::make_shared<Command>("exit",
        [this](const ParsedArgs& args) {
            return this->exitHandler(args);
        },
        "Exit current mode or application");
    commands_["exit"] = exitCmd;
}

bool Mode::helpHandler(const ParsedArgs& args) {
    std::cout << "\nAvailable commands in '" << name_ << "' mode:\n";
    
    for (const auto& [name, cmd] : commands_) {
        std::cout << "  " << std::left << std::setw(20) << name 
                  << " - " << cmd->getDescription() << "\n";
        
        // Show options if any
        const auto& opts = cmd->getOptions();
        if (!opts.empty()) {
            for (const auto& [optName, optDesc] : opts) {
                std::cout << "    --" << std::left << std::setw(16) << optName
                          << " - " << optDesc << "\n";
            }
        }
        
        // Show subcommands if any
        for (const auto& [subname, subcmd] : cmd->getSubcommands()) {
            std::cout << "    " << std::left << std::setw(18) << subname
                      << " - " << subcmd->getDescription() << "\n";
        }
    }
    
    if (!submodes_.empty()) {
        std::cout << "\nAvailable submodes:\n";
        for (const auto& [name, mode] : submodes_) {
            std::cout << "  " << name << "\n";
        }
    }
    std::cout << "\n";
    return true;
}

bool Mode::exitHandler(const ParsedArgs& args) {
    return false; // Return false to exit
}

void Mode::addCommand(std::shared_ptr<Command> command) {
    commands_[command->getName()] = command;
}

void Mode::addSubmode(std::shared_ptr<Mode> submode) {
    submode->parent_ = shared_from_this();
    submodes_[submode->getName()] = submode;
}

std::vector<std::string> Mode::getMatchingCommands(const std::string& prefix) const {
    std::vector<std::string> matches;
    
    // Match commands
    for (const auto& [name, cmd] : commands_) {
        if (name.find(prefix) == 0) {
            matches.push_back(name);
        }
    }
    
    // Match submodes
    for (const auto& [name, mode] : submodes_) {
        if (name.find(prefix) == 0) {
            matches.push_back(name);
        }
    }
    
    std::sort(matches.begin(), matches.end());
    return matches;
}

std::shared_ptr<Command> Mode::getCommand(const std::string& name) const {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Mode> Mode::getSubmode(const std::string& name) const {
    auto it = submodes_.find(name);
    if (it != submodes_.end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// CommandLineInterface Implementation
// ============================================================================

CommandLineInterface::CommandLineInterface(std::shared_ptr<Mode> rootMode)
    : rootMode_(rootMode), currentMode_(rootMode), historyIndex_(0), running_(false) {
    modeStack_.push_back(rootMode);
}

std::vector<std::string> CommandLineInterface::getCompletions(const std::string& line) const {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return currentMode_->getMatchingCommands("");
    }
    
    // If line ends with space, complete next token
    if (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        if (tokens.size() == 1) {
            // Complete subcommands or options
            auto cmd = currentMode_->getCommand(tokens[0]);
            if (cmd) {
                auto matches = cmd->getMatchingCommands("");
                // Add options
                for (const auto& [optName, optDesc] : cmd->getOptions()) {
                    matches.push_back("--" + optName);
                }
                return matches;
            }
        }
        return {};
    }
    
    // Complete current token
    if (tokens.size() == 1) {
        return currentMode_->getMatchingCommands(tokens[0]);
    } else if (tokens.size() >= 2) {
        // Check if completing an option
        if (tokens.back().size() >= 2 && tokens.back()[0] == '-' && tokens.back()[1] == '-') {
            auto cmd = currentMode_->getCommand(tokens[0]);
            if (cmd) {
                std::string prefix = tokens.back().substr(2);
                std::vector<std::string> matches;
                for (const auto& [optName, optDesc] : cmd->getOptions()) {
                    if (optName.find(prefix) == 0) {
                        matches.push_back("--" + optName);
                    }
                }
                return matches;
            }
        }
        
        // Complete subcommands
        auto cmd = currentMode_->getCommand(tokens[0]);
        if (cmd) {
            return cmd->getMatchingCommands(tokens.back());
        }
    }
    
    return {};
}

void CommandLineInterface::handleTabCompletion(std::string& line, size_t& cursorPos) {
    auto completions = getCompletions(line);
    
    if (completions.empty()) {
        return;
    }
    
    if (completions.size() == 1) {
        // Single match - complete it
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        bool endsWithSpace = !line.empty() && (line.back() == ' ' || line.back() == '\t');
        
        if (endsWithSpace) {
            line += completions[0] + " ";
        } else if (!tokens.empty()) {
            // Replace last token
            size_t lastTokenPos = line.rfind(tokens.back());
            line = line.substr(0, lastTokenPos) + completions[0] + " ";
        } else {
            line = completions[0] + " ";
        }
        
        cursorPos = line.length();
        
        // Redraw line
        std::cout << "\r\x1b[K" << getPrompt() << line << std::flush;
    } else {
        // Multiple matches - show them
        std::cout << "\n";
        for (const auto& completion : completions) {
            std::cout << completion << "  ";
        }
        std::cout << "\n" << getPrompt() << line << std::flush;
    }
}

std::string CommandLineInterface::readLineWithCompletion(const std::string& prompt) {
    enableRawMode();
    
    std::string line;
    size_t cursorPos = 0;
    
    std::cout << prompt << std::flush;
    
    while (true) {
        int c = readKey();
        
        if (c == -1) continue;
        
        if (c == '\r' || c == '\n') {
            // Enter
            std::cout << "\n";
            break;
        } else if (c == 4) {
            // Ctrl+D (EOF)
            if (line.empty()) {
                disableRawMode();
                return "\x04"; // Special marker for EOF
            }
        } else if (c == 3) {
            // Ctrl+C
            std::cout << "^C\n";
            line.clear();
            cursorPos = 0;
            std::cout << prompt << std::flush;
        } else if (c == '\t') {
            // Tab - completion
            handleTabCompletion(line, cursorPos);
        } else if (c == 127 || c == 8) {
            // Backspace
            if (cursorPos > 0) {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                std::cout << "\r\x1b[K" << prompt << line << std::flush;
                if (cursorPos < line.length()) {
                    std::cout << "\r" << prompt;
                    for (size_t i = 0; i < cursorPos; i++) {
                        std::cout << line[i];
                    }
                    std::cout << std::flush;
                }
            }
        } else if (c == 1000) {
            // Up arrow - history
            if (historyIndex_ > 0 && !commandHistory_.empty()) {
                historyIndex_--;
                line = commandHistory_[historyIndex_];
                cursorPos = line.length();
                std::cout << "\r\x1b[K" << prompt << line << std::flush;
            }
        } else if (c == 1001) {
            // Down arrow - history
            if (!commandHistory_.empty() && historyIndex_ < commandHistory_.size() - 1) {
                historyIndex_++;
                line = commandHistory_[historyIndex_];
                cursorPos = line.length();
                std::cout << "\r\x1b[K" << prompt << line << std::flush;
            } else if (historyIndex_ == commandHistory_.size() - 1) {
                historyIndex_ = commandHistory_.size();
                line.clear();
                cursorPos = 0;
                std::cout << "\r\x1b[K" << prompt << std::flush;
            }
        } else if (c == 1002) {
            // Right arrow
            if (cursorPos < line.length()) {
                cursorPos++;
                std::cout << "\x1b[C" << std::flush;
            }
        } else if (c == 1003) {
            // Left arrow
            if (cursorPos > 0) {
                cursorPos--;
                std::cout << "\x1b[D" << std::flush;
            }
        } else if (c == 1004) {
            // Home
            cursorPos = 0;
            std::cout << "\r" << prompt << std::flush;
        } else if (c == 1005) {
            // End
            cursorPos = line.length();
            std::cout << "\r" << prompt << line << std::flush;
        } else if (c >= 32 && c <= 126) {
            // Printable character
            line.insert(cursorPos, 1, c);
            cursorPos++;
            std::cout << "\r\x1b[K" << prompt << line << std::flush;
            if (cursorPos < line.length()) {
                std::cout << "\r" << prompt;
                for (size_t i = 0; i < cursorPos; i++) {
                    std::cout << line[i];
                }
                std::cout << std::flush;
            }
        }
    }
    
    disableRawMode();
    return line;
}

std::string CommandLineInterface::getPrompt() const {
    std::vector<std::string> modePath;
    auto mode = currentMode_;
    
    while (mode) {
        modePath.insert(modePath.begin(), mode->getName());
        mode = mode->getParent();
    }
    
    std::string prompt = "[";
    for (size_t i = 0; i < modePath.size(); ++i) {
        if (i > 0) prompt += "/";
        prompt += modePath[i];
    }
    prompt += "]" + currentMode_->getPrompt();
    
    return prompt;
}

std::vector<std::string> CommandLineInterface::listMatchingCommands(const std::string& prefix) const {
    return currentMode_->getMatchingCommands(prefix);
}

bool CommandLineInterface::parseAndExecute(const std::string& line) {
    // Trim and check if empty
    std::string trimmedLine = line;
    trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t\n\r"));
    trimmedLine.erase(trimmedLine.find_last_not_of(" \t\n\r") + 1);
    
    if (trimmedLine.empty()) {
        return true;
    }
    
    // Tokenize
    std::istringstream iss(trimmedLine);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return true;
    }
    
    std::string cmdName = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    // Check if it's a submode
    auto submode = currentMode_->getSubmode(cmdName);
    if (submode) {
        enterMode(submode);
        return true;
    }
    
    // Check if it's a command
    auto command = currentMode_->getCommand(cmdName);
    if (command) {
        // Check for subcommands (first non-option argument)
        if (!args.empty() && args[0][0] != '-') {
            const auto& subcommands = command->getSubcommands();
            auto subIt = subcommands.find(args[0]);
            if (subIt != subcommands.end()) {
                std::vector<std::string> subArgs(args.begin() + 1, args.end());
                return subIt->second->execute(subArgs);
            }
        }
        return command->execute(args);
    }
    
    // Try to find matching commands
    auto matches = listMatchingCommands(cmdName);
    if (!matches.empty()) {
        std::cout << "Unknown command '" << cmdName << "'. Did you mean one of these?\n";
        for (const auto& match : matches) {
            std::cout << "  " << match << "\n";
        }
    } else {
        std::cout << "Unknown command: '" << cmdName << "'. Type 'help' for available commands.\n";
    }
    
    return true;
}

void CommandLineInterface::enterMode(std::shared_ptr<Mode> mode) {
    modeStack_.push_back(mode);
    currentMode_ = mode;
    runMode();
}

void CommandLineInterface::exitMode() {
    if (modeStack_.size() > 1) {
        modeStack_.pop_back();
        currentMode_ = modeStack_.back();
    }
}

void CommandLineInterface::runMode() {
    while (running_) {
        std::string prompt = getPrompt();
        std::string line = readLineWithCompletion(prompt);
        
        if (line == "\x04") {
            // EOF (Ctrl+D)
            std::cout << "Exiting...\n";
            running_ = false;
            break;
        }
        
        // Add to history if not empty
        if (!line.empty()) {
            commandHistory_.push_back(line);
            historyIndex_ = commandHistory_.size();
        }
        
        if (!parseAndExecute(line)) {
            // Exit command was issued
            if (modeStack_.size() > 1) {
                exitMode();
                return;
            } else {
                // Exiting root mode
                running_ = false;
                return;
            }
        }
    }
}

void CommandLineInterface::run() {
    running_ = true;
    std::cout << "Welcome to " << rootMode_->getName() << "\n";
    std::cout << "Type 'help' for available commands.\n";
    runMode();
    std::cout << "Goodbye!\n";
}

} // namespace cmdline
