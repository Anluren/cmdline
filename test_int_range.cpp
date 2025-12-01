/**
 * Demonstration of IntOption with range validation
 */

#include "cmdline_constexpr.h"
#include <iostream>
#include <iomanip>

using namespace cmdline_ct;

int main() {
    std::cout << "IntOption Range Validation Demo\n";
    std::cout << "================================\n\n";
    
    // Define options with different range constraints
    constexpr IntOption portOpt{"port", "Port number (1-65535)", 1, 65535};
    constexpr IntOption percentOpt{"percent", "Percentage (0-100)", 0, 100};
    constexpr IntOption tempOpt{"temperature", "Temperature in Celsius (-273-1000)", -273, 1000};
    constexpr IntOption unrestrictedOpt{"unlimited", "No range limits"};
    
    constexpr auto configSpec = CommandSpec(
        "config",
        "Configure with range-validated options",
        makeOptions(portOpt, percentOpt, tempOpt, unrestrictedOpt)
    );
    
    auto configCmd = makeCommand(configSpec, [](const ParsedArgs& args) {
        std::cout << "Configuration applied:\n";
        
        if (auto port = args.getInt("port")) {
            std::cout << "  Port: " << *port << "\n";
        }
        if (auto percent = args.getInt("percent")) {
            std::cout << "  Percentage: " << *percent << "%\n";
        }
        if (auto temp = args.getInt("temperature")) {
            std::cout << "  Temperature: " << *temp << "°C\n";
        }
        if (auto unlimited = args.getInt("unlimited")) {
            std::cout << "  Unlimited: " << *unlimited << "\n";
        }
        
        return true;
    });
    
    // Test 1: Valid values
    std::cout << "Test 1: All values within range\n";
    configCmd->execute({"--port", "8080", "--percent", "75", "--temperature", "25"});
    std::cout << "\n";
    
    // Test 2: Port out of range (too high)
    std::cout << "Test 2: Port out of range (70000 > 65535)\n";
    std::cout << "Expected: Port option should be ignored\n";
    configCmd->execute({"--port", "70000", "--percent", "50"});
    std::cout << "\n";
    
    // Test 3: Port out of range (too low)
    std::cout << "Test 3: Port out of range (0 < 1)\n";
    std::cout << "Expected: Port option should be ignored\n";
    configCmd->execute({"--port", "0", "--percent", "50"});
    std::cout << "\n";
    
    // Test 4: Percentage out of range
    std::cout << "Test 4: Percentage out of range (150 > 100)\n";
    std::cout << "Expected: Percent option should be ignored\n";
    configCmd->execute({"--port", "443", "--percent", "150"});
    std::cout << "\n";
    
    // Test 5: Temperature at boundary (minimum)
    std::cout << "Test 5: Temperature at minimum boundary (-273)\n";
    configCmd->execute({"--temperature", "-273"});
    std::cout << "\n";
    
    // Test 6: Temperature out of range (below absolute zero)
    std::cout << "Test 6: Temperature out of range (-300 < -273)\n";
    std::cout << "Expected: Temperature option should be ignored\n";
    configCmd->execute({"--temperature", "-300"});
    std::cout << "\n";
    
    // Test 7: Unrestricted option (no range limits)
    std::cout << "Test 7: Unrestricted option accepts any value\n";
    configCmd->execute({"--unlimited", "-999999", "--port", "8080"});
    std::cout << "\n";
    
    // Test 8: Hex values with range validation
    std::cout << "Test 8: Hex value within range (0x1F90 = 8080)\n";
    configCmd->execute({"--port", "0x1F90"});
    std::cout << "\n";
    
    // Test 9: Hex value out of range
    std::cout << "Test 9: Hex value out of range (0x1FFFF = 131071 > 65535)\n";
    std::cout << "Expected: Port option should be ignored\n";
    configCmd->execute({"--port", "0x1FFFF"});
    std::cout << "\n";
    
    // Test 10: Multiple options with mixed validity
    std::cout << "Test 10: Multiple options - some valid, some invalid\n";
    std::cout << "Expected: Only valid options (port, temperature) should be set\n";
    configCmd->execute({"--port", "443", "--percent", "200", "--temperature", "100"});
    std::cout << "\n";
    
    // Compile-time range validation
    std::cout << "Compile-time Validation Tests:\n";
    std::cout << "------------------------------\n";
    
    static_assert(portOpt.isValid(8080), "8080 should be valid port");
    static_assert(!portOpt.isValid(0), "0 should be invalid port");
    static_assert(!portOpt.isValid(70000), "70000 should be invalid port");
    
    static_assert(percentOpt.isValid(0), "0 should be valid percent");
    static_assert(percentOpt.isValid(100), "100 should be valid percent");
    static_assert(!percentOpt.isValid(-1), "-1 should be invalid percent");
    static_assert(!percentOpt.isValid(101), "101 should be invalid percent");
    
    static_assert(tempOpt.isValid(-273), "Absolute zero should be valid");
    static_assert(tempOpt.isValid(1000), "1000°C should be valid");
    static_assert(!tempOpt.isValid(-274), "Below absolute zero should be invalid");
    
    std::cout << "✓ All compile-time assertions passed!\n";
    
    return 0;
}
