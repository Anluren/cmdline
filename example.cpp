/**
 * Example usage of the command line library
 */

#include "cmdline.h"
#include <iostream>
#include <iomanip>

using namespace cmdline;

// Command handlers
bool showHandler(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cout << "Showing: ";
        for (const auto& arg : args) {
            std::cout << arg << " ";
        }
        std::cout << "\n";
    } else {
        std::cout << "Nothing to show\n";
    }
    return true;
}

bool configHandler(const std::vector<std::string>& args) {
    std::cout << "Configuration: ";
    if (!args.empty()) {
        for (const auto& arg : args) {
            std::cout << arg << " ";
        }
    } else {
        std::cout << "No args";
    }
    std::cout << "\n";
    return true;
}

bool setHandler(const std::vector<std::string>& args) {
    if (args.size() >= 2) {
        std::cout << "Setting " << args[0] << " = " << args[1] << "\n";
    } else {
        std::cout << "Usage: config set <key> <value>\n";
    }
    return true;
}

bool getHandler(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cout << "Getting value for: " << args[0] << "\n";
    } else {
        std::cout << "Usage: config get <key>\n";
    }
    return true;
}

bool listHandler(const std::vector<std::string>& args) {
    std::cout << "Listing all configurations:\n";
    std::cout << "  timeout = 30\n";
    std::cout << "  retries = 3\n";
    std::cout << "  verbose = true\n";
    return true;
}

bool statusHandler(const std::vector<std::string>& args) {
    std::cout << "Network Status: Connected\n";
    std::cout << "IP Address: 192.168.1.100\n";
    std::cout << "Gateway: 192.168.1.1\n";
    return true;
}

bool connectHandler(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cout << "Connecting to: " << args[0] << "\n";
    } else {
        std::cout << "Usage: connect <address>\n";
    }
    return true;
}

bool disconnectHandler(const std::vector<std::string>& args) {
    std::cout << "Disconnecting...\n";
    return true;
}

bool systemInfoHandler(const std::vector<std::string>& args) {
    std::cout << "System: Linux x86_64\n";
    std::cout << "Kernel: 5.15.0\n";
    std::cout << "Memory: 16GB\n";
    return true;
}

bool rebootHandler(const std::vector<std::string>& args) {
    std::cout << "Rebooting... (simulated)\n";
    return true;
}

bool wifiScanHandler(const std::vector<std::string>& args) {
    std::cout << "Scanning for WiFi networks...\n";
    std::cout << "  MyNetwork (Signal: Strong)\n";
    std::cout << "  GuestWiFi (Signal: Medium)\n";
    std::cout << "  CoffeeShop (Signal: Weak)\n";
    return true;
}

bool wifiJoinHandler(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cout << "Joining WiFi network: " << args[0] << "\n";
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
    root->addCommand(showCmd);
    
    // Create config command with subcommands
    auto configCmd = std::make_shared<Command>("config", configHandler,
                                               "Configuration management");
    configCmd->addSubcommand(std::make_shared<Command>("set", setHandler,
                                                        "Set a configuration value"));
    configCmd->addSubcommand(std::make_shared<Command>("get", getHandler,
                                                        "Get a configuration value"));
    configCmd->addSubcommand(std::make_shared<Command>("list", listHandler,
                                                        "List all configurations"));
    root->addCommand(configCmd);
    
    // Create network mode
    auto networkMode = std::make_shared<Mode>("network", "net> ");
    networkMode->addCommand(std::make_shared<Command>("status", statusHandler,
                                                       "Show network status"));
    networkMode->addCommand(std::make_shared<Command>("connect", connectHandler,
                                                       "Connect to a network"));
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
