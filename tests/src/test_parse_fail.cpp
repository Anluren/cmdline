#include "cmdline/cmdline_hdr_only.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions(
            IntOption{"port", "Port number"},
            IntOption{"verbose", "Verbose mode"}
        )
    );
    
    auto handler = [](const auto& args) {
        std::cout << "Handler executed successfully!\n";
        return true;
    };
    
    auto cmd = makeCommand(spec, handler);
    
    std::cout << "Test 1: Valid option\n";
    bool result1 = cmd->execute({"--port", "8080"});
    std::cout << "Result: " << (result1 ? "success" : "failure") << "\n\n";
    
    std::cout << "Test 2: Invalid option\n";
    bool result2 = cmd->execute({"--invalid", "value"});
    std::cout << "Result: " << (result2 ? "success" : "failure") << "\n\n";
    
    std::cout << "Test 3: Mix of valid and invalid\n";
    bool result3 = cmd->execute({"--port", "8080", "--invalid", "value"});
    std::cout << "Result: " << (result3 ? "success" : "failure") << "\n";
    
    return 0;
}
