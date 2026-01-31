#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmdline/cmdline_hdr_only.h>

using namespace cmdline_ct;

int main() {
    std::cout << "Help Query Feature Test (? syntax)\n";
    std::cout << "===================================\n\n";
    
    // Test 1: SubcommandDispatcher with ? query
    std::cout << "Test 1: SubcommandDispatcher help queries\n";
    std::cout << "------------------------------------------\n";
    
    auto dispatcher = makeDispatcher("server", "Server management");
    
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
    
    auto restartOpts = makeOptions(
        IntOption{"--delay", "Restart delay", 0, 60}
    );
    auto restartSpec = CommandSpec{"restart", "Restart the server", restartOpts};
    auto restartCmd = makeCommand(restartSpec, [](const auto& opts) {
        std::cout << "[Restart] Delay: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    dispatcher->addSubcommand(startCmd);
    dispatcher->addSubcommand(stopCmd);
    dispatcher->addSubcommand(statusCmd);
    dispatcher->addSubcommand(restartCmd);
    
    std::cout << "\nTest 1a: Query all subcommands with '?'\n";
    dispatcher->execute({"?"});
    
    std::cout << "\nTest 1b: Query subcommands starting with 'sta?'\n";
    dispatcher->execute({"sta?"});
    
    std::cout << "\nTest 1c: Query subcommands starting with 's?'\n";
    dispatcher->execute({"s?"});
    
    std::cout << "\nTest 1d: Query subcommands starting with 'r?'\n";
    dispatcher->execute({"r?"});
    
    std::cout << "\nTest 1e: Query with no matches 'xyz?'\n";
    dispatcher->execute({"xyz?"});
    
    std::cout << "\n";
    
    // Test 2: CLI with ? query
    std::cout << "Test 2: CLI mode queries\n";
    std::cout << "=================================\n";
    auto cli = makeCLI();
    
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
    
    auto testOpts = makeOptions(IntOption{"--coverage", "Coverage level", 0, 100});
    auto testSpec = CommandSpec{"test", "Testing mode", testOpts};
    auto testCmd = makeCommand(testSpec, [](const auto& opts) {
        std::cout << "[Test] Coverage: " << opts.template get<0>().value << "\n";
        return true;
    });
    
    cli->addMode("development", devCmd);
    cli->addMode("production", prodCmd);
    cli->addMode("testing", testCmd);
    cli->setMode("development");
    
    std::cout << "\nTest 2a: Query all modes with 'mode ?'\n";
    cli->executeCommand("mode ?");
    
    std::cout << "\nTest 2b: Query modes starting with 'mode dev?'\n";
    cli->executeCommand("mode dev?");
    
    std::cout << "\nTest 2c: Query modes starting with 'mode p?'\n";
    cli->executeCommand("mode p?");
    
    std::cout << "\nTest 2d: Query modes starting with 'mode t?'\n";
    cli->executeCommand("mode t?");
    
    std::cout << "\nTest 2e: Query with no matches 'mode xyz?'\n";
    cli->executeCommand("mode xyz?");
    
    std::cout << "\nAll help query tests completed!\n";
    
    return 0;
}
