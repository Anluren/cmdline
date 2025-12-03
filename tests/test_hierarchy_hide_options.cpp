#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Hierarchy with Hidden Options Test\n";
    std::cout << "===================================\n\n";

    // Create commands with options
    constexpr auto serverSpec = CommandSpec(
        "server",
        "Start the server",
        makeOptions(
            IntOption{"port", "Server port", true, 1024, 65535},
            StringOption{"host", "Server hostname", true},
            IntOption{"workers", "Worker threads", false, 1, 64},
            StringOption{"config", "Config file path"}
        )
    );
    
    constexpr auto dbSpec = CommandSpec(
        "database",
        "Database operations",
        makeOptions(
            StringOption{"name", "Database name", true},
            IntOption{"timeout", "Connection timeout", false, 1, 300},
            StringOption{"user", "Database user"},
            StringOption{"password", "Database password"}
        )
    );
    
    auto serverCmd = makeCommand(serverSpec, [](const auto& args) { return true; });
    auto dbCmd = makeCommand(dbSpec, [](const auto& args) { return true; });

    // Test 1: Show command WITH options (default)
    std::cout << "Test 1: Show command WITH options (default behavior)\n";
    std::cout << "-----------------------------------------------------\n";
    serverCmd->showHierarchy();
    std::cout << "\n";

    // Test 2: Show command WITHOUT options
    std::cout << "Test 2: Show command WITHOUT options (showOptions=false)\n";
    std::cout << "---------------------------------------------------------\n";
    serverCmd->showHierarchy("", false);
    std::cout << "\n";

    // Test 3: Multiple commands with options hidden
    std::cout << "Test 3: Multiple commands with options hidden\n";
    std::cout << "----------------------------------------------\n";
    serverCmd->showHierarchy("", false);
    dbCmd->showHierarchy("", false);
    std::cout << "\n";

    // Test 4: SubcommandDispatcher
    std::cout << "Test 4: SubcommandDispatcher (showOptions has no effect currently)\n";
    std::cout << "-------------------------------------------------------------------\n";
    
    constexpr auto startSpec = CommandSpec(
        "start",
        "Start the server",
        makeOptions(
            IntOption{"port", "Server port", true, 1024, 65535}
        )
    );
    
    constexpr auto stopSpec = CommandSpec(
        "stop",
        "Stop the server",
        makeOptions(
            IntOption{"timeout", "Shutdown timeout", false, 0, 300}
        )
    );
    
    auto startCmd = makeCommand(startSpec, [](const auto& args) { return true; });
    auto stopCmd = makeCommand(stopSpec, [](const auto& args) { return true; });
    
    auto dispatcher = makeDispatcher("service", "Service management");
    dispatcher->addSubcommand(startCmd);
    dispatcher->addSubcommand(stopCmd);
    
    std::cout << "WITH options flag (true):\n";
    dispatcher->showHierarchy("", true);
    std::cout << "\n";
    
    std::cout << "WITHOUT options flag (false):\n";
    dispatcher->showHierarchy("", false);
    std::cout << "\n";

    // Test 5: Nested hierarchy with mixed visibility
    std::cout << "Test 5: Nested structure with selective option display\n";
    std::cout << "--------------------------------------------------------\n";
    
    std::cout << "Overview (no options):\n";
    std::cout << "  ";
    dispatcher->showHierarchy("  ", false);
    std::cout << "    Available commands:\n";
    std::cout << "      ";
    startCmd->showHierarchy("      ", false);
    std::cout << "      ";
    stopCmd->showHierarchy("      ", false);
    std::cout << "\n";
    
    std::cout << "Detailed view (with options):\n";
    std::cout << "  ";
    dispatcher->showHierarchy("  ", true);
    std::cout << "    Available commands:\n";
    std::cout << "      ";
    startCmd->showHierarchy("      ", true);
    std::cout << "\n";
    std::cout << "      ";
    stopCmd->showHierarchy("      ", true);
    std::cout << "\n";

    std::cout << "All tests completed!\n";
    return 0;
}
