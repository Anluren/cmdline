/**
 * Demonstration of subcommand support
 */

#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Subcommand Support Demo\n";
    std::cout << "========================\n\n";
    
    // Define subcommand specs
    
    // git add subcommand
    constexpr auto addSpec = CommandSpec(
        "add",
        "Add files to staging area",
        makeOptions(
            StringArrayOption{"files", "Files to add"},
            IntOption{"verbose", "Verbosity level"}
        )
    );
    
    auto addCmd = makeCommand(addSpec, [](const auto& args) {
        std::cout << "[git add] Adding files to staging area\n";
        if (auto files = args.getStringArray("files")) {
            for (const auto& file : *files) {
                std::cout << "  + " << file << "\n";
            }
        }
        if (auto verbose = args.getInt("verbose")) {
            std::cout << "  Verbosity: " << *verbose << "\n";
        }
        return true;
    });
    
    // git commit subcommand
    constexpr auto commitSpec = CommandSpec(
        "commit",
        "Commit changes",
        makeOptions(
            StringOption{"message", "Commit message", true},
            IntOption{"amend", "Amend previous commit"}
        )
    );
    
    auto commitCmd = makeCommand(commitSpec, [](const auto& args) {
        std::cout << "[git commit] Committing changes\n";
        if (auto msg = args.getString("message")) {
            std::cout << "  Message: \"" << *msg << "\"\n";
        }
        if (auto amend = args.getInt("amend")) {
            std::cout << "  Amend: " << (*amend ? "yes" : "no") << "\n";
        }
        return true;
    });
    
    // git push subcommand
    constexpr auto pushSpec = CommandSpec(
        "push",
        "Push changes to remote",
        makeOptions(
            StringOption{"remote", "Remote name"},
            StringOption{"branch", "Branch name"},
            IntOption{"force", "Force push"}
        )
    );
    
    auto pushCmd = makeCommand(pushSpec, [](const auto& args) {
        std::cout << "[git push] Pushing to remote\n";
        if (auto remote = args.getString("remote")) {
            std::cout << "  Remote: " << *remote << "\n";
        }
        if (auto branch = args.getString("branch")) {
            std::cout << "  Branch: " << *branch << "\n";
        }
        if (auto force = args.getInt("force")) {
            std::cout << "  Force: " << (*force ? "yes" : "no") << "\n";
        }
        return true;
    });
    
    // git status subcommand (no options)
    constexpr auto statusSpec = CommandSpec(
        "status",
        "Show working tree status",
        makeOptions()
    );
    
    auto statusCmd = makeCommand(statusSpec, [](const auto& args) {
        std::cout << "[git status] Showing status\n";
        std::cout << "  On branch main\n";
        std::cout << "  Your branch is up to date with 'origin/main'\n";
        std::cout << "  nothing to commit, working tree clean\n";
        return true;
    });
    
    // Create dispatcher and register subcommands
    auto git = makeDispatcher("git", "Git version control system");
    git->addSubcommand(addCmd);
    git->addSubcommand(commitCmd);
    git->addSubcommand(pushCmd);
    git->addSubcommand(statusCmd);
    
    // Test 1: Show help
    std::cout << "Test 1: Show main help\n";
    std::cout << "Command: git help\n";
    std::cout << "---\n";
    git->execute({"help"});
    
    std::cout << "\n\n";
    
    // Test 2: Execute 'add' subcommand
    std::cout << "Test 2: Execute 'add' subcommand\n";
    std::cout << "Command: git add files main.cpp test.cpp verbose 1\n";
    std::cout << "---\n";
    git->execute({"add", "files", "main.cpp", "test.cpp", "verbose", "1"});
    
    std::cout << "\n\n";
    
    // Test 3: Execute 'commit' subcommand with '--' prefix
    std::cout << "Test 3: Execute 'commit' subcommand\n";
    std::cout << "Command: git commit --message \"Initial commit\"\n";
    std::cout << "---\n";
    git->execute({"commit", "--message", "Initial commit"});
    
    std::cout << "\n\n";
    
    // Test 4: Execute 'push' subcommand with mixed prefix
    std::cout << "Test 4: Execute 'push' subcommand\n";
    std::cout << "Command: git push remote origin --branch main\n";
    std::cout << "---\n";
    git->execute({"push", "remote", "origin", "--branch", "main"});
    
    std::cout << "\n\n";
    
    // Test 5: Execute 'status' subcommand (no args)
    std::cout << "Test 5: Execute 'status' subcommand\n";
    std::cout << "Command: git status\n";
    std::cout << "---\n";
    git->execute({"status"});
    
    std::cout << "\n\n";
    
    // Test 6: Unknown subcommand
    std::cout << "Test 6: Unknown subcommand\n";
    std::cout << "Command: git pull\n";
    std::cout << "---\n";
    git->execute({"pull"});
    
    std::cout << "\n\n";
    
    // Test 7: Help for specific subcommand
    std::cout << "Test 7: Help for specific subcommand\n";
    std::cout << "Command: git help commit\n";
    std::cout << "---\n";
    git->execute({"help", "commit"});
    
    std::cout << "\n\n";
    
    // Create another dispatcher for a different tool (docker example)
    std::cout << "Test 8: Different tool with subcommands (docker)\n";
    std::cout << "================================================\n\n";
    
    constexpr auto runSpec = CommandSpec(
        "run",
        "Run a container",
        makeOptions(
            StringOption{"image", "Image name", true},
            StringOption{"name", "Container name"},
            IntOption{"detach", "Run in background"}
        )
    );
    
    auto runCmd = makeCommand(runSpec, [](const auto& args) {
        std::cout << "[docker run] Starting container\n";
        if (auto image = args.getString("image")) {
            std::cout << "  Image: " << *image << "\n";
        }
        if (auto name = args.getString("name")) {
            std::cout << "  Name: " << *name << "\n";
        }
        if (auto detach = args.getInt("detach")) {
            std::cout << "  Detached: " << (*detach ? "yes" : "no") << "\n";
        }
        return true;
    });
    
    constexpr auto psSpec = CommandSpec(
        "ps",
        "List containers",
        makeOptions(
            IntOption{"all", "Show all containers"}
        )
    );
    
    auto psCmd = makeCommand(psSpec, [](const auto& args) {
        std::cout << "[docker ps] Listing containers\n";
        if (auto all = args.getInt("all")) {
            std::cout << "  Show all: " << (*all ? "yes" : "no") << "\n";
        }
        std::cout << "  CONTAINER ID   IMAGE     COMMAND   STATUS\n";
        std::cout << "  abc123def456   nginx     \"nginx\"   Up 2 hours\n";
        return true;
    });
    
    auto docker = makeDispatcher("docker", "Container management tool");
    docker->addSubcommand(runCmd);
    docker->addSubcommand(psCmd);
    
    std::cout << "Command: docker run image nginx name web-server detach 1\n";
    std::cout << "---\n";
    docker->execute({"run", "image", "nginx", "name", "web-server", "detach", "1"});
    
    std::cout << "\n\n";
    
    std::cout << "Command: docker ps --all 1\n";
    std::cout << "---\n";
    docker->execute({"ps", "--all", "1"});
    
    return 0;
}
