/**
 * Demonstration of IntArrayOption with range validation
 */

#include "cmdline_constexpr.h"
#include <iostream>

using namespace cmdline_ct;

int main() {
    std::cout << "IntArrayOption Range Validation Demo\n";
    std::cout << "=====================================\n\n";
    
    // Define array options with range constraints
    constexpr IntArrayOption portsOpt{"ports", "Port numbers (1-65535)", 1, 65535};
    constexpr IntArrayOption scoresOpt{"scores", "Test scores (0-100)", 0, 100};
    constexpr IntArrayOption unboundedOpt{"values", "Unbounded values"};
    
    constexpr auto batchSpec = CommandSpec(
        "batch",
        "Process batch of values with range validation",
        makeOptions(portsOpt, scoresOpt, unboundedOpt)
    );
    
    auto batchCmd = makeCommand(batchSpec, [](const auto& args) {
        if (auto ports = args.getIntArray("ports")) {
            std::cout << "  Ports: [";
            for (size_t i = 0; i < ports->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*ports)[i];
            }
            std::cout << "]\n";
        }
        
        if (auto scores = args.getIntArray("scores")) {
            std::cout << "  Scores: [";
            for (size_t i = 0; i < scores->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*scores)[i];
            }
            std::cout << "]\n";
        }
        
        if (auto values = args.getIntArray("values")) {
            std::cout << "  Unbounded values: [";
            for (size_t i = 0; i < values->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*values)[i];
            }
            std::cout << "]\n";
        }
        
        return true;
    });
    
    // Test 1: All values within range
    std::cout << "Test 1: All port values within valid range\n";
    batchCmd->execute({"--ports", "80", "443", "8080", "3000"});
    std::cout << "\n";
    
    // Test 2: Mixed valid and invalid values
    std::cout << "Test 2: Mixed valid and invalid port values\n";
    std::cout << "Input: 80, 70000 (invalid), 443, 0 (invalid), 8080\n";
    std::cout << "Expected: Only valid values (80, 443, 8080) should be kept\n";
    batchCmd->execute({"--ports", "80", "70000", "443", "0", "8080"});
    std::cout << "\n";
    
    // Test 3: Scores with range validation
    std::cout << "Test 3: Test scores with some out of range\n";
    std::cout << "Input: 95, 87, 110 (invalid), 92, -5 (invalid), 100\n";
    std::cout << "Expected: Only valid scores (95, 87, 92, 100) should be kept\n";
    batchCmd->execute({"--scores", "95", "87", "110", "92", "-5", "100"});
    std::cout << "\n";
    
    // Test 4: Hex values with range validation
    std::cout << "Test 4: Hex port values with validation\n";
    std::cout << "Input: 0x50 (80), 0x1BB (443), 0x1FFFF (131071, invalid)\n";
    std::cout << "Expected: Only valid hex values should be kept\n";
    batchCmd->execute({"--ports", "0x50", "0x1BB", "0x1FFFF"});
    std::cout << "\n";
    
    // Test 5: Binary values with range validation
    std::cout << "Test 5: Binary score values with validation\n";
    std::cout << "Input: 0b1010000 (80), 0b1100100 (100), 0b10000000 (128, invalid)\n";
    batchCmd->execute({"--scores", "0b1010000", "0b1100100", "0b10000000"});
    std::cout << "\n";
    
    // Test 6: Unbounded array (no range limits)
    std::cout << "Test 6: Unbounded array accepts all values\n";
    batchCmd->execute({"--values", "-999999", "0", "999999", "0x7FFFFFFF"});
    std::cout << "\n";
    
    // Test 7: Multiple arrays in one command
    std::cout << "Test 7: Multiple arrays with different ranges\n";
    batchCmd->execute({
        "--ports", "80", "443", "70000",
        "--scores", "95", "110", "87",
        "--values", "-1000", "1000"
    });
    std::cout << "\n";
    
    // Test 8: Empty array (all values filtered out)
    std::cout << "Test 8: All values out of range - should result in empty array\n";
    std::cout << "Input: All ports > 65535\n";
    batchCmd->execute({"--ports", "70000", "80000", "100000"});
    std::cout << "(No ports should be listed above)\n";
    std::cout << "\n";
    
    // Test 9: Boundary values
    std::cout << "Test 9: Boundary values (min and max)\n";
    std::cout << "Ports: 1 (min), 65535 (max)\n";
    batchCmd->execute({"--ports", "1", "65535"});
    std::cout << "\n";
    
    std::cout << "Test 10: Score boundaries\n";
    std::cout << "Scores: 0 (min), 100 (max)\n";
    batchCmd->execute({"--scores", "0", "100"});
    std::cout << "\n";
    
    // Compile-time validation
    std::cout << "Compile-time Validation Tests:\n";
    std::cout << "------------------------------\n";
    
    static_assert(portsOpt.isValid(1), "Min port should be valid");
    static_assert(portsOpt.isValid(65535), "Max port should be valid");
    static_assert(!portsOpt.isValid(0), "Port 0 should be invalid");
    static_assert(!portsOpt.isValid(65536), "Port 65536 should be invalid");
    
    static_assert(scoresOpt.isValid(0), "Min score should be valid");
    static_assert(scoresOpt.isValid(100), "Max score should be valid");
    static_assert(!scoresOpt.isValid(-1), "Negative score should be invalid");
    static_assert(!scoresOpt.isValid(101), "Score > 100 should be invalid");
    
    std::cout << "✓ All compile-time assertions passed!\n";
    
    return 0;
}
