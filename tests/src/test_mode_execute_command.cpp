#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "CLI executeCommand API Test\n";
    std::cout << "===========================\n\n";

    // Create commands for different modes
    constexpr auto serverSpec = CommandSpec(
        "server",
        "Server operations",
        makeOptions(
            IntOption{"port", "Server port", true, 1024, 65535},
            StringOption{"host", "Server hostname", false},
            IntOption{"workers", "Worker threads", false, 1, 64}
        )
    );
    
    constexpr auto dbSpec = CommandSpec(
        "database",
        "Database operations",
        makeOptions(
            StringOption{"name", "Database name", true},
            StringOption{"user", "Database user", false},
            IntOption{"timeout", "Connection timeout", false, 1, 300}
        )
    );
    
    auto serverCmd = makeCommand(serverSpec, [](const auto& args) {
        std::cout << "[Server Mode] Executing server command\n";
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        if (auto host = args.getString("host")) {
            std::cout << "  Host: " << *host << "\n";
        }
        if (auto workers = args.getInt("workers")) {
            std::cout << "  Workers: " << *workers << "\n";
        }
        return true;
    });
    
    auto dbCmd = makeCommand(dbSpec, [](const auto& args) {
        std::cout << "[Database Mode] Executing database command\n";
        if (auto name = args.getString("name")) {
            std::cout << "  Database: " << *name << "\n";
        }
        if (auto user = args.getString("user")) {
            std::cout << "  User: " << *user << "\n";
        }
        if (auto timeout = args.getInt("timeout")) {
            std::cout << "  Timeout: " << *timeout << "s\n";
        }
        return true;
    });
    
    // Create CLI
    CLI cli;
    cli.addMode("server", serverCmd);
    cli.addMode("database", dbCmd);
    
    // Test 1: Execute command in server mode
    std::cout << "Test 1: Execute command in server mode\n";
    std::cout << "---------------------------------------\n";
    cli.setMode("server");
    std::cout << "Current mode: " << cli.getCurrentMode() << "\n\n";
    
    std::cout << "Executing: server port 8080 host localhost workers 4\n";
    cli.executeCommand("server port 8080 host localhost workers 4");
    std::cout << "\n";
    
    // Test 2: Execute command in database mode
    std::cout << "Test 2: Execute command in database mode\n";
    std::cout << "-----------------------------------------\n";
    cli.setMode("database");
    std::cout << "Current mode: " << cli.getCurrentMode() << "\n\n";
    
    std::cout << "Executing: database name mydb user admin timeout 30\n";
    cli.executeCommand("database name mydb user admin timeout 30");
    std::cout << "\n";
    
    // Test 3: Switch modes using executeCommand
    std::cout << "Test 3: Switch modes using executeCommand\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Current mode: " << cli.getCurrentMode() << "\n";
    std::cout << "Executing: mode server\n";
    cli.executeCommand("mode server");
    std::cout << "Current mode: " << cli.getCurrentMode() << "\n\n";
    
    // Test 4: Execute in switched mode
    std::cout << "Test 4: Execute in switched mode\n";
    std::cout << "---------------------------------\n";
    std::cout << "Executing: server port 9000\n";
    cli.executeCommand("server port 9000");
    std::cout << "\n";
    
    // Test 5: Interactive-style command sequence
    std::cout << "Test 5: Simulate interactive command sequence\n";
    std::cout << "----------------------------------------------\n";
    std::cout << "Commands:\n";
    std::cout << "  1. mode database\n";
    std::cout << "  2. database name testdb user root\n";
    std::cout << "  3. mode server\n";
    std::cout << "  4. server port 3000 workers 8\n";
    std::cout << "  5. mode\n";
    std::cout << "\n";
    
    cli.executeCommand("mode database");
    cli.executeCommand("database name testdb user root");
    cli.executeCommand("mode server");
    cli.executeCommand("server port 3000 workers 8");
    cli.executeCommand("mode");
    std::cout << "\n";
    
    // Test 6: Empty and invalid commands
    std::cout << "Test 6: Empty and invalid commands\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Executing: (empty string)\n";
    cli.executeCommand("");
    std::cout << "\n";
    
    std::cout << "Executing: mode invalid_mode\n";
    cli.executeCommand("mode invalid_mode");
    std::cout << "\n";
    
    std::cout << "All tests completed!\n";
    return 0;
}
