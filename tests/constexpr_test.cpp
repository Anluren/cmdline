#include <array>
#include <string_view>
#include <iostream>

// Define a constexpr array of string_view
constexpr std::array<std::string_view, 4> options = {"help", "exit", "quit", "show"};

// Compile-time index finder
constexpr size_t find_index(std::string_view sv) {
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i] == sv) return i;
    }
    return size_t(-1);  // Not found
}

int main() {
    // These are all evaluated at compile time!
    constexpr size_t help_idx = find_index("help");
    constexpr size_t exit_idx = find_index("exit");
    constexpr size_t quit_idx = find_index("quit");
    constexpr size_t show_idx = find_index("show");
    constexpr size_t not_found = find_index("foo");
    
    // Prove it with static_assert (compile-time only!)
    static_assert(help_idx == 0, "help should be at index 0");
    static_assert(exit_idx == 1, "exit should be at index 1");
    static_assert(quit_idx == 2, "quit should be at index 2");
    static_assert(show_idx == 3, "show should be at index 3");
    static_assert(not_found == size_t(-1), "foo should not be found");
    
    // Display the results
    std::cout << "Compile-time computed indices:\n";
    std::cout << "  help: " << help_idx << "\n";
    std::cout << "  exit: " << exit_idx << "\n";
    std::cout << "  quit: " << quit_idx << "\n";
    std::cout << "  show: " << show_idx << "\n";
    std::cout << "  foo (not found): " << not_found << "\n";
    
    // Can also use at runtime
    std::string_view search = "exit";
    size_t runtime_idx = find_index(search);
    std::cout << "\nRuntime search for '" << search << "': " << runtime_idx << "\n";
    
    return 0;
}
