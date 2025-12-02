#include "cmdline_constexpr.h"
#include <iostream>
#include <typeinfo>

using namespace cmdline_ct;

int main() {
    constexpr auto spec1 = CommandSpec("test1", "Test 1", makeOptions());
    constexpr auto spec2 = CommandSpec("test2", "Test 2", makeOptions());
    
    // Each lambda creates a unique type
    auto cmd1 = makeCommand(spec1, [](const auto&) { 
        std::cout << "Handler 1\n"; 
        return true; 
    });
    
    auto cmd2 = makeCommand(spec2, [](const auto&) { 
        std::cout << "Handler 2\n"; 
        return true; 
    });
    
    // These will have different types!
    std::cout << "cmd1 type: " << typeid(decltype(cmd1)).name() << "\n";
    std::cout << "cmd2 type: " << typeid(decltype(cmd2)).name() << "\n";
    
    // Test execution
    std::cout << "\nExecuting cmd1:\n";
    cmd1->execute({});
    
    std::cout << "\nExecuting cmd2:\n";
    cmd2->execute({});
    
    // Each lambda can be inlined by the compiler!
    std::cout << "\nBenefit: Each command has its own type, enabling better optimization\n";
    
    return 0;
}
