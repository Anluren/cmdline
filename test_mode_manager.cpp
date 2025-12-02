#include "cmdline_constexpr.h"
#include <iostream>
#include <sstream>
#include <cassert>

using namespace cmdline_ct;

void testModeTransitions() {
    std::cout << "Mode Manager Test\n";
    std::cout << "=================\n\n";
    
    // Create mode manager
    auto mgr = makeModeManager();
    
    // Create git dispatcher (from previous test)
    auto gitDispatcher = makeDispatcher("git", "Git version control system");
    
    // Define git subcommands
    constexpr auto addOptions = makeOptionGroup("add",
        "Add files to staging area",
        StringArrayOption{"files", "Files to add"},
        IntOption{"verbose", "Verbosity level", 0}
    );
    constexpr auto addSpec = CommandSpec<decltype(addOptions)>{"add", "Add files", addOptions};
    
    auto addCmd = makeCommand(addSpec, [](const auto& args) {
        std::cout << "[git add] Adding files to staging area\n";
        if (auto files = args.getStringArray("files")) {
            for (const auto& f : *files) {
                std::cout << "  + " << f << "\n";
            }
        }
        if (auto v = args.getInt("verbose")) {
            std::cout << "  Verbosity: " << *v << "\n";
        }
        return true;
    });
    
    constexpr auto commitOptions = makeOptionGroup("commit",
        "Commit changes",
        StringOption{"message", "Commit message"}
    );
    constexpr auto commitSpec = CommandSpec<decltype(commitOptions)>{"commit", "Commit changes", commitOptions};
    
    auto commitCmd = makeCommand(commitSpec, [](const auto& args) {
        std::cout << "[git commit] Committing changes\n";
        if (auto msg = args.getString("message")) {
            std::cout << "  Message: \"" << *msg << "\"\n";
        }
        return true;
    });
    
    gitDispatcher->addSubcommand(addCmd);
    gitDispatcher->addSubcommand(commitCmd);
    
    // Create docker dispatcher
    auto dockerDispatcher = makeDispatcher("docker", "Docker container management");
    
    constexpr auto runOptions = makeOptionGroup("run",
        "Run a container",
        StringOption{"image", "Container image"},
        StringOption{"name", "Container name"}
    );
    constexpr auto runSpec = CommandSpec<decltype(runOptions)>{"run", "Run container", runOptions};
    
    auto runCmd = makeCommand(runSpec, [](const auto& args) {
        std::cout << "[docker run] Starting container\n";
        if (auto img = args.getString("image")) {
            std::cout << "  Image: " << *img << "\n";
        }
        if (auto name = args.getString("name")) {
            std::cout << "  Name: " << *name << "\n";
        }
        return true;
    });
    
    dockerDispatcher->addSubcommand(runCmd);
    
    // Create a custom mode with mode transition
    mgr->addMode("default", [](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) return "";
        
        if (args[0] == "git") {
            std::cout << "Switching to git mode...\n";
            return "git";
        } else if (args[0] == "docker") {
            std::cout << "Switching to docker mode...\n";
            return "docker";
        } else {
            std::cout << "[default mode] Unknown command: " << args[0] << "\n";
            std::cout << "Available: git, docker\n";
            return "";
        }
    });
    
    // Add git and docker as separate modes
    mgr->addMode("git", gitDispatcher);
    mgr->addMode("docker", dockerDispatcher);
    
    // Test 1: Check initial mode
    std::cout << "Test 1: Initial mode\n";
    std::cout << "Current mode: " << mgr->getCurrentMode() << "\n\n";
    assert(mgr->getCurrentMode() == "default");
    
    // Test 2: Transition from default to git mode
    std::cout << "Test 2: Transition to git mode\n";
    std::cout << "Command: git\n";
    std::cout << "---\n";
    std::vector<std::string> args1 = {"git"};
    std::string nextMode = mgr->execute(args1);
    std::cout << "Current mode: " << mgr->getCurrentMode() << "\n\n";
    assert(mgr->getCurrentMode() == "git");
    
    // Test 3: Execute git add in git mode
    std::cout << "Test 3: Execute 'add' in git mode\n";
    std::cout << "Command: add files main.cpp test.cpp verbose 1\n";
    std::cout << "---\n";
    std::vector<std::string> args2 = {"add", "files", "main.cpp", "test.cpp", "verbose", "1"};
    mgr->execute(args2);
    std::cout << "\n";
    
    // Test 4: Execute git commit in git mode
    std::cout << "Test 4: Execute 'commit' in git mode\n";
    std::cout << "Command: commit --message \"Fix bug\"\n";
    std::cout << "---\n";
    std::vector<std::string> args3 = {"commit", "--message", "Fix bug"};
    mgr->execute(args3);
    std::cout << "\n";
    
    // Test 5: Use mode command to check current mode
    std::cout << "Test 5: Check current mode with 'mode' command\n";
    std::cout << "Command: mode\n";
    std::cout << "---\n";
    std::vector<std::string> args4 = {"mode"};
    mgr->execute(args4);
    std::cout << "\n";
    
    // Test 6: Switch to docker mode using mode command
    std::cout << "Test 6: Switch to docker mode\n";
    std::cout << "Command: mode docker\n";
    std::cout << "---\n";
    std::vector<std::string> args5 = {"mode", "docker"};
    mgr->execute(args5);
    std::cout << "Current mode: " << mgr->getCurrentMode() << "\n\n";
    assert(mgr->getCurrentMode() == "docker");
    
    // Test 7: Execute docker run in docker mode
    std::cout << "Test 7: Execute 'run' in docker mode\n";
    std::cout << "Command: run image nginx name web-server\n";
    std::cout << "---\n";
    std::vector<std::string> args6 = {"run", "image", "nginx", "name", "web-server"};
    mgr->execute(args6);
    std::cout << "\n";
    
    // Test 8: Switch back to default mode
    std::cout << "Test 8: Switch back to default mode\n";
    std::cout << "Command: mode default\n";
    std::cout << "---\n";
    std::vector<std::string> args7 = {"mode", "default"};
    mgr->execute(args7);
    std::cout << "Current mode: " << mgr->getCurrentMode() << "\n\n";
    assert(mgr->getCurrentMode() == "default");
    
    // Test 9: Programmatically set mode
    std::cout << "Test 9: Programmatically set mode to git\n";
    bool success = mgr->setMode("git");
    std::cout << "Set mode result: " << (success ? "success" : "failed") << "\n";
    std::cout << "Current mode: " << mgr->getCurrentMode() << "\n\n";
    assert(success && mgr->getCurrentMode() == "git");
    
    // Test 10: Get all available modes
    std::cout << "Test 10: List all modes\n";
    auto modes = mgr->getModes();
    std::cout << "Available modes:\n";
    for (const auto& mode : modes) {
        std::cout << "  - " << mode << "\n";
    }
    std::cout << "\n";
    
    // Test 11: Interactive-like workflow
    std::cout << "Test 11: Simulated interactive workflow\n";
    std::cout << "---\n";
    mgr->setMode("default");
    
    std::vector<std::vector<std::string>> commands = {
        {"git"},                                    // Switch to git
        {"add", "files", "file1.txt", "file2.txt"}, // git add
        {"commit", "message", "Initial commit"},    // git commit
        {"mode", "docker"},                          // Switch to docker
        {"run", "image", "redis", "name", "cache"}, // docker run
        {"mode", "default"}                          // Back to default
    };
    
    for (const auto& cmd : commands) {
        std::cout << ">> ";
        for (const auto& arg : cmd) {
            std::cout << arg << " ";
        }
        std::cout << "\n";
        mgr->execute(cmd);
        std::cout << "[Mode: " << mgr->getCurrentMode() << "]\n\n";
    }
    
    std::cout << "All tests passed!\n";
}

int main() {
    testModeTransitions();
    return 0;
}
