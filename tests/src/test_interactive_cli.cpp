/**
 * Interactive CLI Demo Test
 *
 * Demonstrates interactive usage of the cmdline library with:
 * - Prompt showing current mode and '>'
 * - Tab completion for commands/subcommands
 * - Command execution with output capture
 */

#include "cmdline/cmdline_hdr_only.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace cmdline_ct;

/**
 * Simple autocomplete helper that finds matching completions
 */
class AutoComplete {
public:
    void addCompletions(const std::string& prefix, const std::vector<std::string>& completions) {
        m_completions[prefix] = completions;
    }

    void addGlobalCompletions(const std::vector<std::string>& completions) {
        m_globalCompletions = completions;
    }

    std::vector<std::string> complete(const std::string& mode, const std::string& partial) const {
        std::vector<std::string> results;

        // First check mode-specific completions
        auto it = m_completions.find(mode);
        if (it != m_completions.end()) {
            for (const auto& comp : it->second) {
                if (comp.compare(0, partial.length(), partial) == 0) {
                    results.push_back(comp);
                }
            }
        }

        // Also check global completions
        for (const auto& comp : m_globalCompletions) {
            if (comp.compare(0, partial.length(), partial) == 0) {
                // Avoid duplicates
                if (std::find(results.begin(), results.end(), comp) == results.end()) {
                    results.push_back(comp);
                }
            }
        }

        return results;
    }

private:
    std::map<std::string, std::vector<std::string>> m_completions;
    std::vector<std::string> m_globalCompletions;
};

/**
 * Interactive CLI session that simulates terminal interaction
 */
class InteractiveCLI {
public:
    InteractiveCLI(std::shared_ptr<CLI> cli)
        : m_cli(cli), m_running(true) {
        setupAutoComplete();
    }

    void setupAutoComplete() {
        // Global commands available in all modes
        m_autocomplete.addGlobalCompletions({"mode", "exit", "quit", "help"});

        // Mode-specific completions
        m_autocomplete.addCompletions("default", {"git", "docker", "config"});
        m_autocomplete.addCompletions("git", {"add", "commit", "status", "log", "push", "pull"});
        m_autocomplete.addCompletions("docker", {"run", "ps", "images", "build", "stop"});
        m_autocomplete.addCompletions("config", {"get", "set", "list"});
    }

    std::string getPrompt() const {
        return std::string(m_cli->getCurrentMode()) + "> ";
    }

    // Simulate tab completion - returns completions for partial input
    std::vector<std::string> tabComplete(const std::string& input) const {
        std::string mode(m_cli->getCurrentMode());
        return m_autocomplete.complete(mode, input);
    }

    // Process tab completion request
    std::string processTab(const std::string& partial) {
        auto completions = tabComplete(partial);
        std::ostringstream oss;
        if (completions.empty()) {
            oss << "(no completions)";
        } else if (completions.size() == 1) {
            oss << "-> " << completions[0];
        } else {
            oss << "Completions: ";
            for (size_t i = 0; i < completions.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << completions[i];
            }
        }
        return oss.str();
    }

    // Process command as vector of arguments (proper parsing)
    std::string processCommand(const std::vector<std::string>& args) {
        std::ostringstream out, err;
        std::string result = m_cli->execute(args, out, err);

        if (result == "exit") {
            m_running = false;
        }

        std::ostringstream combined;
        if (!err.str().empty()) {
            combined << "[ERROR] " << err.str();
        }
        if (!out.str().empty()) {
            combined << out.str();
        }
        return combined.str();
    }

    bool isRunning() const { return m_running; }

private:
    std::shared_ptr<CLI> m_cli;
    AutoComplete m_autocomplete;
    bool m_running;
};

// === Command Specifications (static to ensure lifetime) ===

// Git commands
static constexpr auto addOpts = makeOptionGroup("add", "Stage files",
    StringArrayOption{"files", "Files to stage"}
);
static constexpr auto addSpec = CommandSpec<decltype(addOpts)>{"add", "Stage files", addOpts};

static constexpr auto commitOpts = makeOptionGroup("commit", "Commit changes",
    StringOption{"message", "Commit message"},
    IntOption{"verbose", "Verbosity level", 0}
);
static constexpr auto commitSpec = CommandSpec<decltype(commitOpts)>{"commit", "Commit", commitOpts};

static constexpr auto statusOpts = makeOptions();
static constexpr auto statusSpec = CommandSpec<decltype(statusOpts)>{"status", "Show status", statusOpts};

// Docker commands
static constexpr auto runOpts = makeOptionGroup("run", "Run container",
    StringOption{"image", "Container image"},
    StringOption{"name", "Container name"},
    IntArrayOption{"ports", "Port mappings"}
);
static constexpr auto runSpec = CommandSpec<decltype(runOpts)>{"run", "Run container", runOpts};

static constexpr auto psOpts = makeOptions();
static constexpr auto psSpec = CommandSpec<decltype(psOpts)>{"ps", "List containers", psOpts};

// Config commands
static constexpr auto getOpts = makeOptionGroup("get", "Get config value",
    StringOption{"key", "Config key"}
);
static constexpr auto getSpec = CommandSpec<decltype(getOpts)>{"get", "Get value", getOpts};

static constexpr auto setOpts = makeOptionGroup("set", "Set config value",
    StringOption{"key", "Config key"},
    StringOption{"value", "Config value"}
);
static constexpr auto setSpec = CommandSpec<decltype(setOpts)>{"set", "Set value", setOpts};

// Setup the CLI with sample commands
std::shared_ptr<CLI> createCLI() {
    auto cli = makeCLI();

    // === Git Mode ===
    auto gitDispatcher = makeDispatcher("git", "Git version control");

    auto addCmd = makeCommand(addSpec, [](const auto& args, std::ostream& out, std::ostream&) {
        out << "[git add] Staging files:\n";
        if (auto files = args.getStringArray("files")) {
            for (const auto& f : *files) {
                out << "  + " << f << "\n";
            }
        } else {
            out << "  (no files specified)\n";
        }
        return true;
    });

    auto commitCmd = makeCommand(commitSpec, [](const auto& args, std::ostream& out, std::ostream&) {
        out << "[git commit]\n";
        if (auto msg = args.getString("message")) {
            out << "  Message: \"" << *msg << "\"\n";
        }
        if (auto v = args.getInt("verbose")) {
            out << "  Verbose: " << *v << "\n";
        }
        return true;
    });

    auto statusCmd = makeCommand(statusSpec, [](const auto&, std::ostream& out, std::ostream&) {
        out << "[git status]\n";
        out << "  On branch main\n";
        out << "  nothing to commit, working tree clean\n";
        return true;
    });

    gitDispatcher->addSubcommand(addCmd);
    gitDispatcher->addSubcommand(commitCmd);
    gitDispatcher->addSubcommand(statusCmd);

    // === Docker Mode ===
    auto dockerDispatcher = makeDispatcher("docker", "Container management");

    auto runCmd = makeCommand(runSpec, [](const auto& args, std::ostream& out, std::ostream&) {
        out << "[docker run]\n";
        if (auto img = args.getString("image")) {
            out << "  Image: " << *img << "\n";
        }
        if (auto name = args.getString("name")) {
            out << "  Name: " << *name << "\n";
        }
        if (auto ports = args.getIntArray("ports")) {
            out << "  Ports: ";
            for (size_t i = 0; i < ports->size(); ++i) {
                if (i > 0) out << ", ";
                out << (*ports)[i];
            }
            out << "\n";
        }
        return true;
    });

    auto psCmd = makeCommand(psSpec, [](const auto&, std::ostream& out, std::ostream&) {
        out << "[docker ps]\n";
        out << "CONTAINER ID   IMAGE     STATUS\n";
        out << "abc123         nginx     Up 2 hours\n";
        out << "def456         redis     Up 1 hour\n";
        return true;
    });

    dockerDispatcher->addSubcommand(runCmd);
    dockerDispatcher->addSubcommand(psCmd);

    // === Config Mode ===
    auto configDispatcher = makeDispatcher("config", "Configuration management");

    auto getCmd = makeCommand(getSpec, [](const auto& args, std::ostream& out, std::ostream&) {
        out << "[config get]\n";
        if (auto key = args.getString("key")) {
            out << "  " << *key << " = <value>\n";
        }
        return true;
    });

    auto setCmd = makeCommand(setSpec, [](const auto& args, std::ostream& out, std::ostream&) {
        out << "[config set]\n";
        auto key = args.getString("key");
        auto val = args.getString("value");
        if (key && val) {
            out << "  " << *key << " = " << *val << "\n";
        }
        return true;
    });

    configDispatcher->addSubcommand(getCmd);
    configDispatcher->addSubcommand(setCmd);

    // === Default Mode (command router) ===
    cli->addMode("default", [](const std::vector<std::string>& args,
                               std::ostream& out, std::ostream& err) -> std::string {
        if (args.empty()) return "";

        const std::string& cmd = args[0];
        if (cmd == "git") {
            out << "Entering git mode...\n";
            return "git";
        } else if (cmd == "docker") {
            out << "Entering docker mode...\n";
            return "docker";
        } else if (cmd == "config") {
            out << "Entering config mode...\n";
            return "config";
        } else if (cmd == "help") {
            out << "Available commands: git, docker, config\n";
            out << "Use 'mode <name>' to switch modes\n";
            out << "Use 'exit' or 'quit' to exit\n";
            return "";
        } else {
            err << "Unknown command: " << cmd << "\n";
            err << "Type 'help' for available commands\n";
            return "";
        }
    });

    cli->addMode("git", gitDispatcher);
    cli->addMode("docker", dockerDispatcher);
    cli->addMode("config", configDispatcher);

    return cli;
}

// Tokenize input line into arguments (handles quoted strings)
std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = 0;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
            } else {
                current += c;
            }
        } else if (c == '"' || c == '\'') {
            inQuotes = true;
            quoteChar = c;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

int main(int argc, char* argv[]) {
    std::cout << "Interactive CLI Demo\n";
    std::cout << "====================\n\n";
    std::cout << "Commands:\n";
    std::cout << "  help              - Show available commands\n";
    std::cout << "  git/docker/config - Enter respective mode\n";
    std::cout << "  mode <name>       - Switch to a specific mode\n";
    std::cout << "  <prefix>?         - Tab completion (e.g., 'gi?' or 'ad?')\n";
    std::cout << "  exit/quit         - Exit the CLI\n";
    std::cout << "\n";

    auto cliCore = createCLI();
    InteractiveCLI session(cliCore);

    // Check for --test flag to run automated test instead
    bool testMode = (argc > 1 && std::string(argv[1]) == "--test");

    if (testMode) {
        // Automated test mode with predefined inputs
        std::vector<std::string> testInputs = {
            "help",
            "gi?",
            "do?",
            "co?",
            "git",
            "ad?",
            "com?",
            "st?",
            "status",
            "add files main.cpp test.cpp utils.h",
            "commit message \"Initial commit\" verbose 1",
            "mode docker",
            "r?",
            "p?",
            "ps",
            "run image nginx name webserver ports 80 443",
            "mode config",
            "get key database.host",
            "set key app.debug value true",
            "mode default",
            "mo?",
            "ex?",
            "exit"
        };

        std::cout << "Running in test mode...\n\n";

        for (const auto& input : testInputs) {
            std::cout << session.getPrompt() << input << "\n";

            // Check for tab completion query (ends with ?)
            if (!input.empty() && input.back() == '?' && input.size() > 1) {
                std::string partial = input.substr(0, input.size() - 1);
                std::string completions = session.processTab(partial);
                std::cout << "  " << completions << "\n\n";
            } else {
                auto tokens = tokenize(input);
                std::string output = session.processCommand(tokens);
                if (!output.empty()) {
                    std::istringstream iss(output);
                    std::string line;
                    while (std::getline(iss, line)) {
                        std::cout << "  " << line << "\n";
                    }
                }
                std::cout << "\n";

                if (!session.isRunning()) {
                    std::cout << "Session ended.\n";
                    break;
                }
            }
        }

        std::cout << "\nInteractive CLI Demo completed!\n";
        return 0;
    }

    // Interactive mode - read from stdin
    std::string line;
    while (session.isRunning()) {
        std::cout << session.getPrompt() << std::flush;

        if (!std::getline(std::cin, line)) {
            // EOF reached
            std::cout << "\n";
            break;
        }

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Check for tab completion query (ends with ?)
        // This simulates tab: type "gi?" to see completions for "gi"
        if (line.back() == '?' && line.size() > 1) {
            std::string partial = line.substr(0, line.size() - 1);
            std::string completions = session.processTab(partial);
            std::cout << completions << "\n";
            continue;
        }

        // Regular command - tokenize and execute
        auto tokens = tokenize(line);
        if (tokens.empty()) {
            continue;
        }

        std::string output = session.processCommand(tokens);
        if (!output.empty()) {
            std::cout << output;
            // Add newline if output doesn't end with one
            if (output.back() != '\n') {
                std::cout << "\n";
            }
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}
