#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Multi-level Hierarchical View Test\n";
    std::cout << "===================================\n\n";

    // Test 1: Simple Command hierarchy
    std::cout << "Test 1: Single command with options\n";
    std::cout << "------------------------------------\n";
    
    constexpr auto serverSpec = CommandSpec(
        "server",
        "Start the server",
        makeOptions(
            IntOption{"port", "Server port", true, 1024, 65535},
            StringOption{"host", "Server hostname", true},
            IntOption{"workers", "Worker threads", false, 1, 64}
        )
    );
    
    auto serverCmd = makeCommand(serverSpec, [](const auto& args) {
        return true;
    });
    
    serverCmd->showHierarchy();
    std::cout << "\n";

    // Test 2: SubcommandDispatcher with multiple commands
    std::cout << "Test 2: SubcommandDispatcher with multiple commands\n";
    std::cout << "----------------------------------------------------\n";
    
    constexpr auto startSpec = CommandSpec(
        "start",
        "Start the server",
        makeOptions(
            IntOption{"port", "Server port", true, 1024, 65535},
            StringOption{"host", "Server hostname", true},
            IntOption{"workers", "Worker threads", false, 1, 64}
        )
    );
    
    constexpr auto stopSpec = CommandSpec(
        "stop",
        "Stop the server",
        makeOptions(
            IntOption{"force", "Force stop"},
            IntOption{"timeout", "Timeout in seconds", false, 0, 300}
        )
    );
    
    constexpr auto statusSpec = CommandSpec(
        "status",
        "Show server status",
        makeOptions(
            IntOption{"verbose", "Verbosity level", false, 0, 3}
        )
    );
    
    auto startCmd = makeCommand(startSpec, [](const auto& args) { return true; });
    auto stopCmd = makeCommand(stopSpec, [](const auto& args) { return true; });
    auto statusCmd = makeCommand(statusSpec, [](const auto& args) { return true; });
    
    auto dispatcher = makeDispatcher("server-ctl", "Server control tool");
    dispatcher->addSubcommand(startCmd);
    dispatcher->addSubcommand(stopCmd);
    dispatcher->addSubcommand(statusCmd);

    dispatcher->showHierarchy();
    std::cout << "\n";

    // Test 3: Show individual commands under dispatcher
    std::cout << "Test 3: Individual commands in dispatcher\n";
    std::cout << "------------------------------------------\n";
    startCmd->showHierarchy("  ");
    std::cout << "\n";
    stopCmd->showHierarchy("  ");
    std::cout << "\n";
    statusCmd->showHierarchy("  ");
    std::cout << "\n";

    // Test 4: ModeManager with commands
    std::cout << "Test 4: ModeManager with different modes\n";
    std::cout << "-----------------------------------------\n";
    
    constexpr auto devSpec = CommandSpec(
        "dev-server",
        "Development server",
        makeOptions(
            IntOption{"port", "Dev port", false, 3000, 9999},
            IntOption{"hot-reload", "Enable hot reload"}
        )
    );
    
    constexpr auto prodSpec = CommandSpec(
        "prod-server",
        "Production server",
        makeOptions(
            IntOption{"port", "Prod port", true, 80, 443},
            StringOption{"ssl-cert", "SSL certificate path", true}
        )
    );
    
    auto devCmd = makeCommand(devSpec, [](const auto& args) { return true; });
    auto prodCmd = makeCommand(prodSpec, [](const auto& args) { return true; });
    
    ModeManager manager;
    manager.addMode("dev", devCmd);
    manager.addMode("prod", prodCmd);
    
    manager.setMode("dev");
    manager.showHierarchy();
    std::cout << "\n";

    // Test 5: Nested structure visualization
    std::cout << "Test 5: Complete nested application structure\n";
    std::cout << "----------------------------------------------\n";
    
    constexpr auto dbCreateSpec = CommandSpec(
        "create",
        "Create a new database",
        makeOptions(
            StringOption{"name", "Database name", true},
            StringOption{"charset", "Character set"},
            IntOption{"size", "Initial size in MB", false, 1, 10000}
        )
    );
    
    constexpr auto dbDeleteSpec = CommandSpec(
        "delete",
        "Delete a database",
        makeOptions(
            StringOption{"name", "Database name", true},
            IntOption{"force", "Force deletion without confirmation"}
        )
    );
    
    constexpr auto dbBackupSpec = CommandSpec(
        "backup",
        "Backup a database",
        makeOptions(
            StringOption{"name", "Database name", true},
            StringOption{"output", "Backup file path", true},
            IntOption{"compress", "Compress backup"}
        )
    );
    
    auto dbCreateCmd = makeCommand(dbCreateSpec, [](const auto& args) { return true; });
    auto dbDeleteCmd = makeCommand(dbDeleteSpec, [](const auto& args) { return true; });
    auto dbBackupCmd = makeCommand(dbBackupSpec, [](const auto& args) { return true; });
    
    auto dbDispatcher = makeDispatcher("db", "Database operations");
    dbDispatcher->addSubcommand(dbCreateCmd);
    dbDispatcher->addSubcommand(dbDeleteCmd);
    dbDispatcher->addSubcommand(dbBackupCmd);
    
    std::cout << "Application: MyDBTool\n";
    std::cout << "└─ ";
    dbDispatcher->showHierarchy();
    std::cout << "   Commands:\n";
    std::cout << "   ├─ ";
    dbCreateCmd->showHierarchy("   │  ");
    std::cout << "\n";
    std::cout << "   ├─ ";
    dbDeleteCmd->showHierarchy("   │  ");
    std::cout << "\n";
    std::cout << "   └─ ";
    dbBackupCmd->showHierarchy("      ");
    std::cout << "\n";

    // Test 6: Hide options for overview
    std::cout << "Test 6: Overview with hidden options (showOptions=false)\n";
    std::cout << "---------------------------------------------------------\n";
    
    std::cout << "Application: MyDBTool\n";
    std::cout << "└─ ";
    dbDispatcher->showHierarchy("", false);
    std::cout << "   Commands:\n";
    std::cout << "   ├─ ";
    dbCreateCmd->showHierarchy("   │  ", false);
    std::cout << "\n";
    std::cout << "   ├─ ";
    dbDeleteCmd->showHierarchy("   │  ", false);
    std::cout << "\n";
    std::cout << "   └─ ";
    dbBackupCmd->showHierarchy("      ", false);
    std::cout << "\n";

    std::cout << "All multi-level hierarchy tests completed!\n";
    return 0;
}
