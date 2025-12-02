/**
 * Demonstration of option matching without '--' prefix
 */

#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Option Matching Demo: With and Without '--' Prefix\n";
    std::cout << "===================================================\n\n";
    
    // Define a simple command with various option types
    constexpr IntOption portOpt{"port", "Port number", 1, 65535};
    constexpr StringOption hostOpt{"host", "Hostname"};
    constexpr IntArrayOption portsOpt{"ports", "Multiple ports", 1, 65535};
    constexpr StringArrayOption tagsOpt{"tags", "Tags"};
    
    constexpr auto connectSpec = CommandSpec(
        "connect",
        "Connect to server",
        makeOptions(portOpt, hostOpt, portsOpt, tagsOpt)
    );
    
    auto connectCmd = makeCommand(connectSpec, [](const auto& args) {
        std::cout << "  Command executed:\n";
        
        if (!args.positional.empty()) {
            std::cout << "    Target: " << args.positional[0] << "\n";
        }
        
        if (auto host = args.getString("host")) {
            std::cout << "    Host: " << *host << "\n";
        }
        
        if (auto port = args.getInt("port")) {
            std::cout << "    Port: " << *port << "\n";
        }
        
        if (auto ports = args.getIntArray("ports")) {
            std::cout << "    Ports: [";
            for (size_t i = 0; i < ports->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*ports)[i];
            }
            std::cout << "]\n";
        }
        
        if (auto tags = args.getStringArray("tags")) {
            std::cout << "    Tags: [";
            for (size_t i = 0; i < tags->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*tags)[i];
            }
            std::cout << "]\n";
        }
        
        return true;
    });
    
    // Test 1: Traditional format with '--' prefix
    std::cout << "Test 1: Traditional format with '--' prefix\n";
    std::cout << "Command: connect server.com --host example.com --port 8080\n";
    connectCmd->execute({"server.com", "--host", "example.com", "--port", "8080"});
    
    std::cout << "\n";
    
    // Test 2: Without '--' prefix
    std::cout << "Test 2: Without '--' prefix\n";
    std::cout << "Command: connect server.com host example.com port 8080\n";
    connectCmd->execute({"server.com", "host", "example.com", "port", "8080"});
    
    std::cout << "\n";
    
    // Test 3: Mixed format
    std::cout << "Test 3: Mixed format (some with '--', some without)\n";
    std::cout << "Command: connect server.com --host example.com port 443\n";
    connectCmd->execute({"server.com", "--host", "example.com", "port", "443"});
    
    std::cout << "\n";
    
    // Test 4: Array options with '--' prefix
    std::cout << "Test 4: Array options with '--' prefix\n";
    std::cout << "Command: connect --ports 80 443 8080 --tags production web\n";
    connectCmd->execute({"--ports", "80", "443", "8080", "--tags", "production", "web"});
    
    std::cout << "\n";
    
    // Test 5: Array options without '--' prefix
    std::cout << "Test 5: Array options without '--' prefix\n";
    std::cout << "Command: connect ports 80 443 8080 tags production web\n";
    connectCmd->execute({"ports", "80", "443", "8080", "tags", "production", "web"});
    
    std::cout << "\n";
    
    // Test 6: Mixed with positional and options
    std::cout << "Test 6: Complex example with positional and mixed options\n";
    std::cout << "Command: connect api.example.com --port 0x1F90 tags api production\n";
    connectCmd->execute({"api.example.com", "--port", "0x1F90", "tags", "api", "production"});
    
    std::cout << "\n";
    
    // Test 7: Hex and binary values without prefix
    std::cout << "Test 7: Hex and binary values without '--' prefix\n";
    std::cout << "Command: connect port 0x1F90 ports 0b1010000 0x1BB\n";
    connectCmd->execute({"port", "0x1F90", "ports", "0b1010000", "0x1BB"});
    
    std::cout << "\n";
    
    // Test 8: Demonstrating that unknown options (with or without '--') are ignored
    std::cout << "Test 8: Unknown options are ignored\n";
    std::cout << "Command: connect --host test.com --unknown value port 8080\n";
    connectCmd->execute({"--host", "test.com", "--unknown", "value", "port", "8080"});
    
    std::cout << "\nNote: Options can be specified with or without '--' prefix!\n";
    std::cout << "The parser automatically recognizes registered option names.\n";
    
    return 0;
}
