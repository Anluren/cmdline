#include "cmdline_constexpr.h"
#include <iostream>
#include <typeinfo>

using namespace cmdline_ct;

int main() {
    constexpr auto spec1 = CommandSpec<0>("test1", "Test 1");
    constexpr auto spec2 = CommandSpec<0>("test2", "Test 2");
    
    // Each lambda creates a unique type
    auto cmd1 = makeCommand(spec1, [](const ParsedArgs&) { 
        std::cout << "Handler 1\n"; 
        return true; 
    });
    
    auto cmd2 = makeCommand(spec2, [](const ParsedArgs&) { 
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
