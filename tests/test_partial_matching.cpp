/**
 * Test for partial command matching
 */

#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Partial Command Matching Test\n";
    std::cout << "==============================\n\n";
    
    // Create commands
    auto startOpts = makeOptions(
        IntOption{"--port", "Server port", 1024, 65535}
    );
    auto startSpec = CommandSpec{"start", "Start the server", startOpts};
    auto startCmd = makeCommand(startSpec, [](const auto& opts) {
        std::cout << "[Start] Port: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    auto stopOpts = makeOptions(
        IntOption{"--timeout", "Stop timeout", 0, 300}
    );
    auto stopSpec = CommandSpec{"stop", "Stop the server", stopOpts};
    auto stopCmd = makeCommand(stopSpec, [](const auto& opts) {
        std::cout << "[Stop] Timeout: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    auto statusOpts = makeOptions(
        IntOption{"--verbose", "Verbosity level", 0, 3}
    );
    auto statusSpec = CommandSpec{"status", "Show server status", statusOpts};
    auto statusCmd = makeCommand(statusSpec, [](const auto& opts) {
        std::cout << "[Status] Verbose: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    // Create dispatcher
    auto dispatcher = makeDispatcher("server", "Server control");
    dispatcher->addSubcommand(startCmd);
    dispatcher->addSubcommand(stopCmd);
    dispatcher->addSubcommand(statusCmd);
    
    // Test 1: Exact match
    std::cout << "Test 1: Exact match 'start'\n";
    std::cout << "----------------------------\n";
    dispatcher->execute({"start", "--port", "9000"});
    std::cout << "\n";
    
    // Test 2: Partial match - unique prefix
    std::cout << "Test 2: Partial match 'sta' (matches 'start' and 'status')\n";
    std::cout << "-----------------------------------------------------------\n";
    dispatcher->execute({"sta", "--port", "9000"});
    std::cout << "\n";
    
    // Test 3: Partial match - unique prefix
    std::cout << "Test 3: Partial match 'star' (matches only 'start')\n";
    std::cout << "----------------------------------------------------\n";
    dispatcher->execute({"star", "--port", "9001"});
    std::cout << "\n";
    
    // Test 4: Partial match - unique prefix
    std::cout << "Test 4: Partial match 'stat' (matches only 'status')\n";
    std::cout << "-----------------------------------------------------\n";
    dispatcher->execute({"stat", "--verbose", "2"});
    std::cout << "\n";
    
    // Test 5: Partial match - single character
    std::cout << "Test 5: Partial match 'sto' (matches only 'stop')\n";
    std::cout << "--------------------------------------------------\n";
    dispatcher->execute({"sto", "--timeout", "60"});
    std::cout << "\n";
    
    // Test 6: Ambiguous prefix
    std::cout << "Test 6: Ambiguous prefix 's' (matches all three commands)\n";
    std::cout << "----------------------------------------------------------\n";
    dispatcher->execute({"s", "--port", "9000"});
    std::cout << "\n";
    
    // Test 7: Unknown command
    std::cout << "Test 7: Unknown command 'restart'\n";
    std::cout << "----------------------------------\n";
    dispatcher->execute({"restart"});
    std::cout << "\n";
    
    // Test 8: Mode manager partial matching
    std::cout << "Test 8: ModeManager partial matching\n";
    std::cout << "=====================================\n";
    auto modeManager = makeModeManager();
    
    auto devOpts = makeOptions(IntOption{"--debug", "Debug level", 0, 5});
    auto devSpec = CommandSpec{"dev", "Development mode", devOpts};
    auto devCmd = makeCommand(devSpec, [](const auto& opts) {
        std::cout << "[Dev] Debug: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    auto prodOpts = makeOptions(IntOption{"--workers", "Worker count", 1, 64});
    auto prodSpec = CommandSpec{"prod", "Production mode", prodOpts};
    auto prodCmd = makeCommand(prodSpec, [](const auto& opts) {
        std::cout << "[Prod] Workers: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    modeManager->addMode("development", devCmd);
    modeManager->addMode("production", prodCmd);
    modeManager->setMode("development");
    
    std::cout << "\nTest 8a: Exact mode switch 'production'\n";
    modeManager->executeCommand("mode production");
    
    std::cout << "\nTest 8b: Partial mode switch 'dev' (matches 'development')\n";
    modeManager->executeCommand("mode dev");
    
    std::cout << "\nTest 8c: Partial mode switch 'prod' (matches 'production')\n";
    modeManager->executeCommand("mode prod");
    
    std::cout << "\nTest 8d: Ambiguous mode 'pro' (matches 'production' but might be ambiguous with 'prod...')\n";
    modeManager->executeCommand("mode pro");
    
    std::cout << "\nAll partial matching tests completed!\n";
    
    return 0;
}
