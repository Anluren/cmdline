/**
 * Demonstration of variadic template OptionGroup
 */

#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Variadic Template OptionGroup Demo\n";
    std::cout << "===================================\n\n";
    
    // Create option groups directly with variadic templates
    // No need for makeOptions() wrapper!
    
    std::cout << "Creating option groups with variadic templates:\n\n";
    
    // Group 1: Network options (2 options)
    constexpr auto networkOpts = makeOptionGroup(
        "network", "Network configuration",
        StringOption{"host", "Server hostname"},
        IntOption{"port", "Port number"}
    );
    
    std::cout << "Network group '" << networkOpts.name << "' has " 
              << networkOpts.size() << " options:\n";
    // Use visitor pattern to iterate over options
    std::apply([](const auto&... opts) {
        ((std::cout << "  - " << opts.name << ": " << opts.description << "\n"), ...);
    }, networkOpts.options);
    
    // Group 2: Authentication (3 options)
    constexpr auto authOpts = makeOptionGroup(
        "auth", "Authentication options",
        StringOption{"username", "Username for login"},
        StringOption{"password", "Password for login"},
        IntOption{"timeout", "Auth timeout in seconds"}
    );
    
    std::cout << "\nAuth group '" << authOpts.name << "' has " 
              << authOpts.size() << " options:\n";
    std::apply([](const auto&... opts) {
        ((std::cout << "  - " << opts.name << ": " << opts.description << "\n"), ...);
    }, authOpts.options);
    
    // Group 3: Retry policy (2 options)
    constexpr auto retryOpts = makeOptionGroup(
        "retry", "Retry configuration",
        IntOption{"max-retries", "Maximum retry attempts"},
        IntOption{"delay", "Delay between retries (ms)"}
    );
    
    std::cout << "\nRetry group '" << retryOpts.name << "' has " 
              << retryOpts.size() << " options:\n";
    std::apply([](const auto&... opts) {
        ((std::cout << "  - " << opts.name << ": " << opts.description << "\n"), ...);
    }, retryOpts.options);
    
    // Compose groups together - now just create a new group with all options
    std::cout << "\n\nComposing groups:\n";
    
    // Create command using all options directly
    constexpr auto connectSpec = CommandSpec(
        "connect",
        "Connect to server with auth and retry",
        makeOptions(
            StringOption{"host", "Server hostname"},
            IntOption{"port", "Port number"},
            StringOption{"username", "Username for login"},
            StringOption{"password", "Password for login"},
            IntOption{"timeout", "Auth timeout in seconds"},
            IntOption{"max-retries", "Maximum retry attempts"},
            IntOption{"delay", "Delay between retries (ms)"}
        )
    );
    
    std::cout << "Full connect spec has " << connectSpec.numOptions() << " options\n";
    
    auto connectCmd = makeCommand(connectSpec, [](const auto& args) {
        std::cout << "\n[CONNECT] Executing with:\n";
        if (auto host = args.getString("host")) {
            std::cout << "  Host: " << *host << "\n";
        }
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        if (auto user = args.getString("username")) {
            std::cout << "  Username: " << *user << "\n";
        }
        if (auto timeout = args.getInt("timeout")) {
            std::cout << "  Auth timeout: " << *timeout << "s\n";
        }
        if (auto retries = args.getInt("max-retries")) {
            std::cout << "  Max retries: " << *retries << "\n";
        }
        if (auto delay = args.getInt("delay")) {
            std::cout << "  Retry delay: " << *delay << "ms\n";
        }
        return true;
    });
    
    std::cout << "\n\nTesting command execution:\n";
    connectCmd->execute({
        "--host", "api.example.com",
        "--port", "443",
        "--username", "admin",
        "--timeout", "30",
        "--max-retries", "3",
        "--delay", "1000"
    });
    
    // Compile-time size checks
    static_assert(networkOpts.size() == 2);
    static_assert(authOpts.size() == 3);
    static_assert(retryOpts.size() == 2);
    
    std::cout << "\n✓ All compile-time assertions passed!\n";
    
    return 0;
}
