#include <iostream>
#include <cassert>
#include "cmdline/cmdline_hdr_only.h"

using namespace cmdline_ct;

int main() {
    std::cout << "Default Value Creation Test\n";
    std::cout << "============================\n\n";

    // Create option instances
    IntOption portOpt("port", "Port number", false, 1, 65535);
    StringOption hostOpt("host", "Server hostname");
    IntArrayOption portsOpt("ports", "Port list");
    StringArrayOption tagsOpt("tags", "Tag list");

    // Test createDefaultValue() for each option type
    std::cout << "Test 1: IntOption default value\n";
    auto defaultInt = portOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(defaultInt), int64_t>);
    std::cout << "  Default int64_t value: " << defaultInt << "\n";
    assert(defaultInt == 0);
    std::cout << "  ✓ Correct type and value\n\n";

    std::cout << "Test 2: StringOption default value\n";
    auto defaultString = hostOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(defaultString), std::string>);
    std::cout << "  Default string value: \"" << defaultString << "\"\n";
    assert(defaultString.empty());
    std::cout << "  ✓ Correct type and value (empty string)\n\n";

    std::cout << "Test 3: IntArrayOption default value\n";
    auto defaultIntVec = portsOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(defaultIntVec), std::vector<int64_t>>);
    std::cout << "  Default vector<int64_t> size: " << defaultIntVec.size() << "\n";
    assert(defaultIntVec.empty());
    std::cout << "  ✓ Correct type and value (empty vector)\n\n";

    std::cout << "Test 4: StringArrayOption default value\n";
    auto defaultStringVec = tagsOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(defaultStringVec), std::vector<std::string>>);
    std::cout << "  Default vector<string> size: " << defaultStringVec.size() << "\n";
    assert(defaultStringVec.empty());
    std::cout << "  ✓ Correct type and value (empty vector)\n\n";

    std::cout << "Test 5: Verify CRTP base class works with derived types\n";
    // This demonstrates that createDefaultValue() is defined in base
    // but uses the derived class's value_type
    OptionSpecBase<IntOption>& baseRef = portOpt;
    auto valueFromBase = baseRef.createDefaultValue();
    static_assert(std::is_same_v<decltype(valueFromBase), int64_t>);
    std::cout << "  Value from base reference: " << valueFromBase << "\n";
    std::cout << "  ✓ CRTP allows base class to use derived type\n\n";

    std::cout << "All tests passed!\n";
    std::cout << "\nThe createDefaultValue() method successfully creates\n";
    std::cout << "default instances using each derived class's value_type.\n";
    
    return 0;
}
