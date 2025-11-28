/**
 * Example usage of the command line library
 */

#include "cmdline.h"
#include <iostream>
#include <iomanip>

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
    // Create root mode
    auto root = std::make_shared<Mode>("main", "> ");
    
    // Add commands to root mode
    auto showCmd = std::make_shared<Command>("show", showHandler, 
                                             "Display information");
    showCmd->addOption("verbose", "Enable verbose output");
    showCmd->addOption("count", "Number of items to show (hex/dec/bin)");
    root->addCommand(showCmd);
    
    // Create config command with subcommands
    auto configCmd = std::make_shared<Command>("config", configHandler,
                                               "Configuration management");
    
    auto setCmd = std::make_shared<Command>("set", setHandler, "Set a configuration value");
    setCmd->addOption("timeout", "Timeout in milliseconds (supports 0x for hex, 0b for binary)");
    configCmd->addSubcommand(setCmd);
    
    configCmd->addSubcommand(std::make_shared<Command>("get", getHandler,
                                                        "Get a configuration value"));
    configCmd->addSubcommand(std::make_shared<Command>("list", listHandler,
                                                        "List all configurations"));
    root->addCommand(configCmd);
    
    // Create network mode
    auto networkMode = std::make_shared<Mode>("network", "net> ");
    networkMode->addCommand(std::make_shared<Command>("status", statusHandler,
                                                       "Show network status"));
    
    auto connectCmd = std::make_shared<Command>("connect", connectHandler, "Connect to a network");
    connectCmd->addOption("port", "Port number (hex: 0x1234, dec: 4660, bin: 0b1001001110100)");
    connectCmd->addOption("retry", "Number of retries");
    networkMode->addCommand(connectCmd);
    networkMode->addCommand(std::make_shared<Command>("disconnect", disconnectHandler,
                                                       "Disconnect from network"));
    
    // Add network mode as submode of root
    root->addSubmode(networkMode);
    
    // Create system mode
    auto systemMode = std::make_shared<Mode>("system", "sys> ");
    systemMode->addCommand(std::make_shared<Command>("info", systemInfoHandler,
                                                      "Show system information"));
    systemMode->addCommand(std::make_shared<Command>("reboot", rebootHandler,
                                                      "Reboot the system"));
    
    // Add system mode as submode of root
    root->addSubmode(systemMode);
    
    // Create wifi mode (nested under network)
    auto wifiMode = std::make_shared<Mode>("wifi", "wifi> ");
    wifiMode->addCommand(std::make_shared<Command>("scan", wifiScanHandler,
                                                    "Scan for WiFi networks"));
    wifiMode->addCommand(std::make_shared<Command>("join", wifiJoinHandler,
                                                    "Join a WiFi network"));
    
    // Add wifi as submode of network (multi-level nesting)
    networkMode->addSubmode(wifiMode);
    
    // Create and run CLI
    CommandLineInterface cli(root);
    cli.run();
    
    return 0;
}
