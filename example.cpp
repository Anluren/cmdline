/**
 * Example usage of the command line library
 */

#include "cmdline.h"
#include <iostream>
#include <iomanip>
#include <array>

using namespace cmdline;

// Command handlers
bool showHandler(const ParsedArgs& args) {
    std::cout << "Positional args: ";
    for (const auto& arg : args.positional) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    
    if (!args.options.empty()) {
        std::cout << "Options:\n";
        for (const auto& [name, val] : args.options) {
            std::cout << "  --" << name << " = ";
            if (val.isInteger) {
                std::cout << val.intValue << " (0x" << std::hex << val.intValue << std::dec << ")";
            } else {
                std::cout << "\"" << val.stringValue << "\"";
            }
            std::cout << "\n";
        }
    }
    return true;
}

bool configHandler(const ParsedArgs& args) {
    std::cout << "Configuration command\n";
    return true;
}

bool setHandler(const ParsedArgs& args) {
    if (args.positional.size() >= 2) {
        std::cout << "Setting " << args.positional[0] << " = " << args.positional[1] << "\n";
    } else {
        std::cout << "Usage: config set <key> <value>\n";
    }
    
    // Check for timeout option
    if (auto timeout = args.getInt("timeout")) {
        std::cout << "With timeout: " << *timeout << "ms\n";
    }
    
    return true;
}

bool getHandler(const ParsedArgs& args) {
    if (!args.positional.empty()) {
        std::cout << "Getting value for: " << args.positional[0] << "\n";
    } else {
        std::cout << "Usage: config get <key>\n";
    }
    return true;
}

bool listHandler(const ParsedArgs& args) {
    std::cout << "Listing all configurations:\n";
    std::cout << "  timeout = 30\n";
    std::cout << "  retries = 3\n";
    std::cout << "  verbose = true\n";
    return true;
}

bool statusHandler(const ParsedArgs& args) {
    std::cout << "Network Status: Connected\n";
    std::cout << "IP Address: 192.168.1.100\n";
    std::cout << "Gateway: 192.168.1.1\n";
    return true;
}

bool connectHandler(const ParsedArgs& args) {
    if (!args.positional.empty()) {
        std::cout << "Connecting to: " << args.positional[0] << "\n";
        
        // Check for port option
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        
        // Check for retry option  
        if (auto retry = args.getInt("retry")) {
            std::cout << "  Retries: " << *retry << "\n";
        }
    } else {
        std::cout << "Usage: connect <address> [--port <num>] [--retry <num>]\n";
    }
    return true;
}

bool disconnectHandler(const ParsedArgs& args) {
    std::cout << "Disconnecting...\n";
    return true;
}

bool systemInfoHandler(const ParsedArgs& args) {
    std::cout << "System: Linux x86_64\n";
    std::cout << "Kernel: 5.15.0\n";
    std::cout << "Memory: 16GB\n";
    return true;
}

bool rebootHandler(const ParsedArgs& args) {
    std::cout << "Rebooting... (simulated)\n";
    return true;
}

bool wifiScanHandler(const ParsedArgs& args) {
    std::cout << "Scanning for WiFi networks...\n";
    std::cout << "  MyNetwork (Signal: Strong)\n";
    std::cout << "  GuestWiFi (Signal: Medium)\n";
    std::cout << "  CoffeeShop (Signal: Weak)\n";
    return true;
}

bool wifiJoinHandler(const ParsedArgs& args) {
    if (!args.positional.empty()) {
        std::cout << "Joining WiFi network: " << args.positional[0] << "\n";
    } else {
        std::cout << "Usage: join <network-name>\n";
    }
    return true;
}

int main() {
    const std::array showOpts{
        OptionSpec{"verbose", "Enable verbose output"},
        OptionSpec{"count", "Number of items to show (hex/dec/bin)"}
    };

    const std::array setOpts{
        OptionSpec{"timeout", "Timeout in milliseconds (supports 0x for hex, 0b for binary)"}
    };

    const std::array connectOpts{
        OptionSpec{"port", "Port number (hex: 0x1234, dec: 4660, bin: 0b1001001110100)"},
        OptionSpec{"retry", "Number of retries"}
    };

    ModeSpec rootSpec{
        "main",
        "> ",
        {
            CommandSpec{
                "show",
                showHandler,
                "Display information",
                std::vector<OptionSpec>(showOpts.begin(), showOpts.end()),
                {}
            },
            CommandSpec{
                "config",
                configHandler,
                "Configuration management",
                {},
                {
                    CommandSpec{
                        "set",
                        setHandler,
                        "Set a configuration value",
                        std::vector<OptionSpec>(setOpts.begin(), setOpts.end()),
                        {}
                    },
                    CommandSpec{ "get", getHandler, "Get a configuration value", {}, {} },
                    CommandSpec{ "list", listHandler, "List all configurations", {}, {} }
                }
            }
        },
        {
            ModeSpec{
                "network",
                "net> ",
                {
                    CommandSpec{ "status", statusHandler, "Show network status", {}, {} },
                    CommandSpec{
                        "connect",
                        connectHandler,
                        "Connect to a network",
                        std::vector<OptionSpec>(connectOpts.begin(), connectOpts.end()),
                        {}
                    },
                    CommandSpec{ "disconnect", disconnectHandler, "Disconnect from network", {}, {} }
                },
                {
                    ModeSpec{
                        "wifi",
                        "wifi> ",
                        {
                            CommandSpec{ "scan", wifiScanHandler, "Scan for WiFi networks", {}, {} },
                            CommandSpec{ "join", wifiJoinHandler, "Join a WiFi network", {}, {} }
                        },
                        {}
                    }
                }
            },
            ModeSpec{
                "system",
                "sys> ",
                {
                    CommandSpec{ "info", systemInfoHandler, "Show system information", {}, {} },
                    CommandSpec{ "reboot", rebootHandler, "Reboot the system", {}, {} }
                },
                {}
            }
        }
    };

    auto root = buildMode(rootSpec);
    CommandLineInterface cli(root);
    cli.run();
    
    return 0;
}
