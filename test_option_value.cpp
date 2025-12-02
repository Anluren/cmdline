#include <iostream>
#include <cassert>
#include <vector>
#include "cmdline_constexpr.h"

using namespace cmdline_ct;

int main() {
    std::cout << "TypedOptionValue Template Test\n";
    std::cout << "===============================\n\n";

    // Test 1: Integer TypedOptionValue
    std::cout << "Test 1: TypedOptionValue<int64_t>\n";
    TypedOptionValue<int64_t> port;
    std::cout << "  Initial state - is_set: " << port.is_set << "\n";
    assert(!port.is_set);
    assert(!port);  // operator bool
    
    port.set(8080);
    std::cout << "  After set(8080) - is_set: " << port.is_set << ", value: " << port.get() << "\n";
    assert(port.is_set);
    assert(port);
    assert(port.get() == 8080);
    assert(*port == 8080);  // operator*
    std::cout << "  ✓ Integer value set and retrieved correctly\n\n";

    // Test 2: String TypedOptionValue
    std::cout << "Test 2: TypedOptionValue<std::string>\n";
    TypedOptionValue<std::string> host;
    assert(!host.is_set);
    
    host.set("example.com");
    std::cout << "  Value: " << host.get() << "\n";
    std::cout << "  Length via ->: " << host->length() << "\n";  // operator->
    assert(host.get() == "example.com");
    assert(host->length() == 11);
    std::cout << "  ✓ String value set and accessed correctly\n\n";

    // Test 3: Vector TypedOptionValue
    std::cout << "Test 3: TypedOptionValue<std::vector<int64_t>>\n";
    TypedOptionValue<std::vector<int64_t>> ports;
    assert(!ports.is_set);
    
    std::vector<int64_t> port_list = {80, 443, 8080};
    ports.set(port_list);
    std::cout << "  Ports: [";
    for (size_t i = 0; i < ports->size(); ++i) {
        std::cout << (*ports)[i];
        if (i < ports->size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    assert(ports->size() == 3);
    assert((*ports)[0] == 80);
    std::cout << "  ✓ Vector value set and accessed correctly\n\n";

    // Test 4: Move semantics
    std::cout << "Test 4: Move semantics\n";
    TypedOptionValue<std::string> moved;
    std::string temp = "temporary string";
    moved.set(std::move(temp));
    std::cout << "  Moved value: " << moved.get() << "\n";
    assert(moved.get() == "temporary string");
    std::cout << "  ✓ Move semantics work correctly\n\n";

    // Test 5: Reset functionality
    std::cout << "Test 5: Reset functionality\n";
    TypedOptionValue<int64_t> resettable(42);
    std::cout << "  Initial - is_set: " << resettable.is_set << ", value: " << resettable.get() << "\n";
    assert(resettable.is_set);
    assert(resettable.get() == 42);
    
    resettable.reset();
    std::cout << "  After reset - is_set: " << resettable.is_set << ", value: " << resettable.get() << "\n";
    assert(!resettable.is_set);
    assert(resettable.get() == 0);
    std::cout << "  ✓ Reset works correctly\n\n";

    // Test 6: Constructor with value
    std::cout << "Test 6: Constructor with value\n";
    TypedOptionValue<int64_t> initialized(9999);
    std::cout << "  is_set: " << initialized.is_set << ", value: " << initialized.get() << "\n";
    assert(initialized.is_set);
    assert(initialized.get() == 9999);
    std::cout << "  ✓ Constructor with value works correctly\n\n";

    // Test 7: Type traits
    std::cout << "Test 7: Compile-time type verification\n";
    static_assert(std::is_same_v<TypedOptionValue<int64_t>::value_type, int64_t>);
    static_assert(std::is_same_v<TypedOptionValue<std::string>::value_type, std::string>);
    static_assert(std::is_same_v<TypedOptionValue<std::vector<int64_t>>::value_type, std::vector<int64_t>>);
    std::cout << "  ✓ All type traits verified at compile-time\n\n";

    // Test 8: Integration with option specs
    std::cout << "Test 8: Integration with OptionSpecBase\n";
    IntOption portSpec("port", "Port number");
    auto defaultVal = portSpec.createDefaultValue();
    TypedOptionValue<decltype(defaultVal)> portValue;
    portValue.set(defaultVal);
    
    TypedOptionValue<IntOption::value_type> typedPort(8080);
    std::cout << "  Port from IntOption::value_type: " << typedPort.get() << "\n";
    assert(typedPort.get() == 8080);
    std::cout << "  ✓ Works seamlessly with option specifications\n\n";

    std::cout << "All tests passed!\n";
    std::cout << "\nTypedOptionValue<T> provides:\n";
    std::cout << "  - Type-safe value storage with compile-time type information\n";
    std::cout << "  - is_set flag to track whether value was provided\n";
    std::cout << "  - Convenient operators (*, ->, bool) for easy access\n";
    std::cout << "  - Move semantics support for efficiency\n";
    std::cout << "  - Integration with option specifications via value_type\n";

    return 0;
}
