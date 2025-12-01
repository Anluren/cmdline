/**
 * Example demonstrating compile-time command definitions
 */

#include "cmdline_constexpr.h"
#include <iostream>
#include <iomanip>

using namespace cmdline_ct;

// Define command specifications at compile time using typed option classes
namespace Commands {
    // Show command with compile-time options
    constexpr auto showSpec = CommandSpec(
        "show",
        "Display information",
        makeOptions(
            IntOption{"verbose", "Enable verbose output"},
            IntOption{"count", "Number of items (hex/dec/bin)"}
        )
    );
    
    // Connect command with compile-time options
    constexpr auto connectSpec = CommandSpec(
        "connect",
        "Connect to a network",
        makeOptions(
            IntOption{"port", "Port number (hex: 0x, bin: 0b, dec)"},
            IntOption{"retry", "Number of retries"}
        )
    );
    
    // Set command with compile-time options
    constexpr auto setSpec = CommandSpec(
        "set",
        "Set a configuration value",
        makeOptions(
            IntOption{"timeout", "Timeout in milliseconds"}
        )
    );
    
    // Simple commands without options
    constexpr auto statusSpec = CommandSpec("status", "Show status", makeOptions());
    constexpr auto listSpec = CommandSpec("list", "List all items", makeOptions());
    
    // Compile-time validation - these are evaluated at compile time!
    static_assert(showSpec.numOptions() == 2);
    static_assert(showSpec.name == "show");
    static_assert(connectSpec.findOption("port").has_value());
    static_assert(connectSpec.findOption("invalid") == std::nullopt);
}

// Handler functions
bool showHandler(const ParsedArgs& args) {
    std::cout << "Show command - Positional args: ";
    for (const auto& arg : args.positional) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    
    if (auto verbose = args.getInt("verbose")) {
        std::cout << "  Verbose: " << (*verbose ? "enabled" : "disabled") << "\n";
    }
    
    if (auto count = args.getInt("count")) {
        std::cout << "  Count: " << *count << " (0x" << std::hex << *count << std::dec << ")\n";
    }
    
    return true;
}

bool connectHandler(const ParsedArgs& args) {
    if (!args.positional.empty()) {
        std::cout << "Connecting to: " << args.positional[0] << "\n";
        
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        
        if (auto retry = args.getInt("retry")) {
            std::cout << "  Retries: " << *retry << "\n";
        }
    }
    return true;
}

bool setHandler(const ParsedArgs& args) {
    if (args.positional.size() >= 2) {
        std::cout << "Setting " << args.positional[0] << " = " << args.positional[1] << "\n";
        
        if (auto timeout = args.getInt("timeout")) {
            std::cout << "  Timeout: " << *timeout << "ms\n";
        }
    }
    return true;
}

bool statusHandler(const ParsedArgs& args) {
    std::cout << "Status: OK\n";
    return true;
}

bool listHandler(const ParsedArgs& args) {
    std::cout << "Items:\n";
    std::cout << "  - Item 1\n";
    std::cout << "  - Item 2\n";
    std::cout << "  - Item 3\n";
    return true;
}

template<typename CmdType>
void printCommandInfo(const char* name, CmdType& cmd) {
    std::cout << "\n" << name << " command:\n";
    std::cout << "  Name: " << cmd.getName() << "\n";
    std::cout << "  Description: " << cmd.getDescription() << "\n";
    
    const auto& spec = cmd.getSpec();
    if (spec.numOptions() > 0) {
        std::cout << "  Options:\n";
        for (const auto& opt : spec.options()) {
            std::cout << "    --" << opt.name << ": " << opt.description << "\n";
        }
    }
}

int main() {
    std::cout << "Compile-Time Command Line Demo\n";
    std::cout << "================================\n";
    
    // Create commands from compile-time specs with inline lambdas
    // Each command gets its own type based on the lambda!
    auto showCmd = makeCommand(Commands::showSpec, [](const ParsedArgs& args) {
        std::cout << "Show command - Positional args: ";
        for (const auto& arg : args.positional) {
            std::cout << arg << " ";
        }
        std::cout << "\n";
        
        if (auto verbose = args.getInt("verbose")) {
            std::cout << "  Verbose: " << (*verbose ? "enabled" : "disabled") << "\n";
        }
        
        if (auto count = args.getInt("count")) {
            std::cout << "  Count: " << *count << " (0x" << std::hex << *count << std::dec << ")\n";
        }
        
        return true;
    });
    
    auto connectCmd = makeCommand(Commands::connectSpec, [](const ParsedArgs& args) {
        if (!args.positional.empty()) {
            std::cout << "Connecting to: " << args.positional[0] << "\n";
            
            if (auto port = args.getInt("port")) {
                std::cout << "  Port: " << *port << "\n";
            }
            
            if (auto retry = args.getInt("retry")) {
                std::cout << "  Retries: " << *retry << "\n";
            }
        }
        return true;
    });
    
    auto setCmd = makeCommand(Commands::setSpec, [](const ParsedArgs& args) {
        if (args.positional.size() >= 2) {
            std::cout << "Setting " << args.positional[0] << " = " << args.positional[1] << "\n";
            
            if (auto timeout = args.getInt("timeout")) {
                std::cout << "  Timeout: " << *timeout << "ms\n";
            }
        }
        return true;
    });
    
    auto statusCmd = makeCommand(Commands::statusSpec, [](const ParsedArgs& args) {
        std::cout << "Status: OK\n";
        return true;
    });
    
    auto listCmd = makeCommand(Commands::listSpec, [](const ParsedArgs& args) {
        std::cout << "Items:\n";
        std::cout << "  - Item 1\n";
        std::cout << "  - Item 2\n";
        std::cout << "  - Item 3\n";
        return true;
    });
    
    // Print command information
    printCommandInfo("Show", *showCmd);
    printCommandInfo("Connect", *connectCmd);
    printCommandInfo("Set", *setCmd);
    
    // Test execution with different argument patterns
    std::cout << "\n\nTest Executions:\n";
    std::cout << "================\n";
    
    std::cout << "\n1. show test --count 42\n";
    showCmd->execute({"test", "--count", "42"});
    
    std::cout << "\n2. show data --count 0x2A --verbose 1\n";
    showCmd->execute({"data", "--count", "0x2A", "--verbose", "1"});
    
    std::cout << "\n3. show items --count 0b101010\n";
    showCmd->execute({"items", "--count", "0b101010"});
    
    std::cout << "\n4. connect 192.168.1.1 --port 0x1F90 --retry 5\n";
    connectCmd->execute({"192.168.1.1", "--port", "0x1F90", "--retry", "5"});
    
    std::cout << "\n5. set timeout 0x1000\n";
    setCmd->execute({"timeout", "0x1000"});
    
    std::cout << "\n6. status\n";
    statusCmd->execute({});
    
    std::cout << "\n7. list\n";
    listCmd->execute({});
    
    // Demonstrate compile-time option lookup
    std::cout << "\n\nCompile-Time Lookups:\n";
    std::cout << "=====================\n";
    
    // These are evaluated at compile time!
    constexpr auto portIdx = Commands::connectSpec.findOption("port");
    constexpr auto retryIdx = Commands::connectSpec.findOption("retry");
    constexpr auto invalidIdx = Commands::connectSpec.findOption("invalid");
    
    std::cout << "Port option index: " << (portIdx ? std::to_string(*portIdx) : "not found") << "\n";
    std::cout << "Retry option index: " << (retryIdx ? std::to_string(*retryIdx) : "not found") << "\n";
    std::cout << "Invalid option index: " << (invalidIdx ? "found" : "not found") << "\n";
    
    return 0;
}
