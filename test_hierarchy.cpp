/**
 * Test hierarchical view of commands
 */

#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "Hierarchical View Test\n";
    std::cout << "======================\n\n";
    
    // Test 1: Single command with various option types
    std::cout << "Test 1: Command with various option types\n";
    std::cout << "------------------------------------------\n";
    constexpr auto serverOpts = OptionGroup{
        "server",
        "Server configuration",
        IntOption{"port", "Server port", 1024, 65535},
        StringOption{"host", "Server hostname", true},
        IntOption{"workers", "Worker threads", 1, 64},
        StringOption{"config", "Config file path"},
        IntArrayOption{"backup-ports", "Backup server ports", 1024, 65535},
        StringArrayOption{"tags", "Server tags"}
    };
    
    constexpr auto serverSpec = CommandSpec{"server", "Start the server", serverOpts};
    auto serverCmd = makeCommand(serverSpec, [](const auto& args) { return true; });
    
    serverCmd->showHierarchy();
    std::cout << "\n";
    
    // Test 2: Command with minimal options
    std::cout << "Test 2: Simple command\n";
    std::cout << "----------------------\n";
    constexpr auto simpleOpts = OptionGroup{
        "status",
        "Status options",
        IntOption{"verbose", "Verbosity level", 0, 3}
    };
    
    auto statusCmd = makeCommand(
        CommandSpec{"status", "Show status", simpleOpts},
        [](const auto& args) { return true; }
    );
    
    statusCmd->showHierarchy();
    std::cout << "\n";
    
    // Test 3: Command with no range constraints
    std::cout << "Test 3: Options without range constraints\n";
    std::cout << "------------------------------------------\n";
    constexpr auto unboundedOpts = OptionGroup{
        "process",
        "Processing options",
        IntOption{"threads", "Number of threads"},
        StringOption{"input", "Input file", true},
        IntArrayOption{"ids", "Process IDs"},
        StringArrayOption{"files", "File list", true}
    };
    
    auto processCmd = makeCommand(
        CommandSpec{"process", "Process data", unboundedOpts},
        [](const auto& args) { return true; }
    );
    
    processCmd->showHierarchy();
    
    std::cout << "\nAll hierarchy tests completed successfully!\n";
    
    return 0;
}
