/**
 * Demonstration of separated parse() and invoke() functions
 */

#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Demonstrating Separated Parse and Invoke\n";
    std::cout << "=========================================\n\n";
    
    // Define command spec with typed options
    constexpr IntOption portOpt{"port", "Port number"};
    constexpr IntOption retryOpt{"retry", "Retry count"};
    
    constexpr auto connectSpec = CommandSpec(
        "connect",
        "Connect to a server",
        makeOptions(portOpt, retryOpt)
    );
    
    // Create command with lambda handler
    auto connectCmd = makeCommand(connectSpec, [](const auto& args) {
        std::cout << "  [HANDLER] Connecting to: " << args.positional[0] << "\n";
        if (auto port = args.getInt("port")) {
            std::cout << "  [HANDLER] Port: " << *port << "\n";
        }
        if (auto retry = args.getInt("retry")) {
            std::cout << "  [HANDLER] Retries: " << *retry << "\n";
        }
        return true;
    });
    
    // Test 1: Use execute() - combines parse + invoke
    std::cout << "Test 1: Using execute() - single call\n";
    connectCmd->execute({"192.168.1.1", "--port", "8080", "--retry", "3"});
    
    std::cout << "\n";
    
    // Test 2: Separate parse and invoke
    std::cout << "Test 2: Using parse() then invoke() - separated\n";
    
    std::vector<std::string> args = {"10.0.0.1", "--port", "0x1F90", "--retry", "5"};
    
    // Step 1: Parse arguments
    std::cout << "  [PARSE] Parsing arguments...\n";
    ParsedArgs parsed = connectCmd->parse(args);
    
    // Inspect parsed results before invoking
    std::cout << "  [PARSE] Parsed positional: ";
    for (const auto& p : parsed.positional) {
        std::cout << p << " ";
    }
    std::cout << "\n";
    
    std::cout << "  [PARSE] Parsed options:\n";
    if (parsed.hasOption("port")) {
        if (auto val = parsed.getInt("port")) {
            std::cout << "    --port = " << *val << " (0x" << std::hex << *val << std::dec << ")\n";
        }
    }
    if (parsed.hasOption("timeout")) {
        if (auto val = parsed.getInt("timeout")) {
            std::cout << "    --timeout = " << *val << "\n";
        }
    }
    
    // Step 2: Invoke handler with parsed data
    std::cout << "  [INVOKE] Invoking handler...\n";
    connectCmd->invoke(parsed);
    
    std::cout << "\n";
    
    // Test 3: Parse once, invoke multiple times with modifications
    std::cout << "Test 3: Parse once, modify, invoke multiple times\n";
    
    ParsedArgs parsed2 = connectCmd->parse({"server.com", "--port", "443"});
    
    std::cout << "  First invocation:\n";
    connectCmd->invoke(parsed2);
    
    // Modify parsed arguments
    parsed2.positional[0] = "backup.server.com";
    // Note: With tuple-based storage, we modify by index (port is option 0)
    parsed2.get<0>().value = 8443;
    parsed2.get<0>().is_set = true;
    
    std::cout << "\n  Second invocation (modified):\n";
    connectCmd->invoke(parsed2);
    
    std::cout << "\n";
    
    // Test 4: Pre-validation before invoking
    std::cout << "Test 4: Validate parsed args before invoking\n";
    
    ParsedArgs parsed3 = connectCmd->parse({"example.com"});
    
    // Validate
    if (parsed3.positional.empty()) {
        std::cout << "  [VALIDATION] ERROR: No server specified\n";
    } else if (!parsed3.hasOption("port")) {
        std::cout << "  [VALIDATION] WARNING: No port specified, using default\n";
        // Add default port (port is option 0)
        parsed3.get<0>().value = 80;
        parsed3.get<0>().is_set = true;
    }
    
    std::cout << "  [INVOKE] Invoking with validated/modified args:\n";
    connectCmd->invoke(parsed3);
    
    return 0;
}
