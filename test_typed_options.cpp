/**
 * Demonstration of typed options and option group composition
 */

#include "cmdline_constexpr.h"
#include <iostream>
#include <iomanip>

using namespace cmdline_ct;

int main() {
    std::cout << "Typed Options and Composition Demo\n";
    std::cout << "===================================\n\n";
    
    // Define reusable option groups using typed option classes
    constexpr auto networkGroup = makeOptionGroup(
        "network", "Network-related options",
        StringOption{"host", "Server hostname", true},
        IntOption{"port", "Port number", false}
    );
    
    constexpr auto retryGroup = makeOptionGroup(
        "retry", "Retry configuration",
        IntOption{"retry", "Number of retries"},
        IntOption{"timeout", "Timeout in milliseconds"}
    );
    
    constexpr auto verboseGroup = makeOptionGroup(
        "output", "Output options",
        IntOption{"verbose", "Verbosity level (0-3)"}
    );
    
    // Create commands by composing option groups
    
    // Test 1: Command with integer and string options
    std::cout << "Test 1: Single value options\n";
    std::cout << "-----------------------------\n";
    
    constexpr auto connectSpec = CommandSpec(
        "connect",
        "Connect to server",
        makeOptions(
            StringOption{"host", "Server hostname", true},
            IntOption{"port", "Port number", false},
            IntOption{"timeout", "Connection timeout"}
        )
    );
    
    auto connectCmd = makeCommand(connectSpec, [](const ParsedArgs& args) {
        if (auto host = args.getString("host")) {
            std::cout << "  Connecting to: " << *host << "\n";
        }
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        if (auto timeout = args.getInt("timeout")) {
            std::cout << "  Timeout: " << *timeout << "ms\n";
        }
        return true;
    });
    
    connectCmd->execute({"--host", "example.com", "--port", "0x1F90", "--timeout", "5000"});
    
    // Test 2: Command with integer arrays
    std::cout << "\n\nTest 2: Integer array options\n";
    std::cout << "------------------------------\n";
    
    constexpr auto configSpec = CommandSpec(
        "config",
        "Configure settings",
        makeOptions(
            IntArrayOption{"ports", "List of ports"},
            IntOption{"threads", "Number of threads"}
        )
    );
    
    auto configCmd = makeCommand(configSpec, [](const ParsedArgs& args) {
        if (auto ports = args.getIntArray("ports")) {
            std::cout << "  Ports: ";
            for (auto p : *ports) {
                std::cout << p << " ";
            }
            std::cout << "\n";
        }
        if (auto threads = args.getInt("threads")) {
            std::cout << "  Threads: " << *threads << "\n";
        }
        return true;
    });
    
    configCmd->execute({"--ports", "80", "443", "0x1F90", "0b10000000000", "--threads", "4"});
    
    // Test 3: Command with string arrays
    std::cout << "\n\nTest 3: String array options\n";
    std::cout << "-----------------------------\n";
    
    constexpr auto deploySpec = CommandSpec(
        "deploy",
        "Deploy to servers",
        makeOptions(
            StringArrayOption{"servers", "List of server names"},
            StringOption{"version", "Version to deploy"}
        )
    );
    
    auto deployCmd = makeCommand(deploySpec, [](const ParsedArgs& args) {
        if (auto servers = args.getStringArray("servers")) {
            std::cout << "  Deploying to servers:\n";
            for (const auto& s : *servers) {
                std::cout << "    - " << s << "\n";
            }
        }
        if (auto version = args.getString("version")) {
            std::cout << "  Version: " << *version << "\n";
        }
        return true;
    });
    
    deployCmd->execute({"--servers", "web1", "web2", "web3", "--version", "v2.0.1"});
    
    // Test 4: Composing multiple option groups
    std::cout << "\n\nTest 4: Multiple option groups composition\n";
    std::cout << "--------------------------------------------\n";
    
    constexpr auto advancedConnectSpec = CommandSpec(
        "advanced-connect",
        "Connect with full options",
        makeOptions(
            StringOption{"host", "Server hostname", true},
            IntOption{"port", "Port number", false},
            IntOption{"retry", "Number of retries"},
            IntOption{"timeout", "Timeout in milliseconds"}
        )
    );
    
    auto advConnectCmd = makeCommand(advancedConnectSpec, [](const ParsedArgs& args) {
        std::cout << "  Network options:\n";
        if (auto host = args.getString("host")) {
            std::cout << "    Host: " << *host << "\n";
        }
        if (auto port = args.getInt("port")) {
            std::cout << "    Port: " << *port << "\n";
        }
        
        std::cout << "  Retry options:\n";
        if (auto retry = args.getInt("retry")) {
            std::cout << "    Retries: " << *retry << "\n";
        }
        if (auto timeout = args.getInt("timeout")) {
            std::cout << "    Timeout: " << *timeout << "ms\n";
        }
        return true;
    });
    
    advConnectCmd->execute({"--host", "api.example.com", "--port", "443", 
                            "--retry", "3", "--timeout", "0x7D0"});
    
    // Test 5: Mixed types
    std::cout << "\n\nTest 5: Mixed positional and typed options\n";
    std::cout << "-------------------------------------------\n";
    
    constexpr auto copySpec = CommandSpec(
        "copy",
        "Copy files",
        makeOptions(
            StringArrayOption{"exclude", "Patterns to exclude"},
            IntOption{"verbose", "Verbosity level"}
        )
    );
    
    auto copyCmd = makeCommand(copySpec, [](const ParsedArgs& args) {
        std::cout << "  Source: " << (args.positional.size() > 0 ? args.positional[0] : "none") << "\n";
        std::cout << "  Dest: " << (args.positional.size() > 1 ? args.positional[1] : "none") << "\n";
        
        if (auto exclude = args.getStringArray("exclude")) {
            std::cout << "  Exclude patterns:\n";
            for (const auto& pattern : *exclude) {
                std::cout << "    - " << pattern << "\n";
            }
        }
        if (auto verbose = args.getInt("verbose")) {
            std::cout << "  Verbose: level " << *verbose << "\n";
        }
        return true;
    });
    
    copyCmd->execute({"/src", "/dst", "--exclude", "*.tmp", "*.log", "*.bak", "--verbose", "2"});
    
    std::cout << "\n\nDemo complete!\n";
    return 0;
}
