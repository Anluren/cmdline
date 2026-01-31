/**
 * Full code coverage tests for cmdline_hdr_only library
 * Covers edge cases and code paths not hit by other tests
 */

#include <iostream>
#include <cassert>
#include <sstream>
#include <type_traits>
#include "cmdline/cmdline_hdr_only.h"

using namespace cmdline_ct;

// Capture stdout for testing
class CaptureOutput {
public:
    CaptureOutput() : old_buf(std::cout.rdbuf(buffer.rdbuf())) {}
    ~CaptureOutput() { std::cout.rdbuf(old_buf); }
    std::string get() const { return buffer.str(); }
private:
    std::stringstream buffer;
    std::streambuf* old_buf;
};

// Capture stderr for testing
class CaptureError {
public:
    CaptureError() : old_buf(std::cerr.rdbuf(buffer.rdbuf())) {}
    ~CaptureError() { std::cerr.rdbuf(old_buf); }
    std::string get() const { return buffer.str(); }
private:
    std::stringstream buffer;
    std::streambuf* old_buf;
};

// ============================================================================
// Test 1: Type traits coverage
// ============================================================================
void testTypeTraits() {
    std::cout << "Test 1: Type traits coverage\n";

    // is_int_option
    static_assert(is_int_option<IntOption>::value, "IntOption should be int option");
    static_assert(is_int_option<IntArrayOption>::value, "IntArrayOption should be int option");
    static_assert(!is_int_option<StringOption>::value, "StringOption should not be int option");
    static_assert(!is_int_option<StringArrayOption>::value, "StringArrayOption should not be int option");
    std::cout << "  ✓ is_int_option trait works correctly\n";

    // is_string_option
    static_assert(is_string_option<StringOption>::value, "StringOption should be string option");
    static_assert(is_string_option<StringArrayOption>::value, "StringArrayOption should be string option");
    static_assert(!is_string_option<IntOption>::value, "IntOption should not be string option");
    static_assert(!is_string_option<IntArrayOption>::value, "IntArrayOption should not be string option");
    std::cout << "  ✓ is_string_option trait works correctly\n";

    // is_array_option
    static_assert(is_array_option<IntArrayOption>::value, "IntArrayOption should be array option");
    static_assert(is_array_option<StringArrayOption>::value, "StringArrayOption should be array option");
    static_assert(!is_array_option<IntOption>::value, "IntOption should not be array option");
    static_assert(!is_array_option<StringOption>::value, "StringOption should not be array option");
    std::cout << "  ✓ is_array_option trait works correctly\n";

    // is_array static member
    static_assert(!IntOption::is_array, "IntOption::is_array should be false");
    static_assert(!StringOption::is_array, "StringOption::is_array should be false");
    static_assert(IntArrayOption::is_array, "IntArrayOption::is_array should be true");
    static_assert(StringArrayOption::is_array, "StringArrayOption::is_array should be true");
    std::cout << "  ✓ is_array static member works correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 2: CommandSpec findOption and numOptions
// ============================================================================
void testCommandSpecMethods() {
    std::cout << "Test 2: CommandSpec findOption and numOptions\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"},
        IntArrayOption{"ids", "ID list"}
    );
    constexpr auto spec = CommandSpec{"test", "Test command", opts};

    // Test numOptions
    static_assert(spec.numOptions() == 3, "Should have 3 options");
    assert(spec.numOptions() == 3);
    std::cout << "  ✓ numOptions() returns correct count\n";

    // Test findOption for existing options
    auto portIdx = spec.findOption("port");
    assert(portIdx.has_value() && *portIdx == 0);
    std::cout << "  ✓ findOption('port') returns index 0\n";

    auto hostIdx = spec.findOption("host");
    assert(hostIdx.has_value() && *hostIdx == 1);
    std::cout << "  ✓ findOption('host') returns index 1\n";

    auto idsIdx = spec.findOption("ids");
    assert(idsIdx.has_value() && *idsIdx == 2);
    std::cout << "  ✓ findOption('ids') returns index 2\n";

    // Test findOption for non-existent option
    auto nonExistent = spec.findOption("nonexistent");
    assert(!nonExistent.has_value());
    std::cout << "  ✓ findOption('nonexistent') returns nullopt\n";

    // Test NumOptions static constexpr
    static_assert(decltype(spec)::NumOptions == 3, "NumOptions should be 3");
    std::cout << "  ✓ NumOptions static constexpr works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 3: OptionGroup num_options static constexpr
// ============================================================================
void testOptionGroupNumOptions() {
    std::cout << "Test 3: OptionGroup num_options static constexpr\n";

    constexpr auto opts1 = makeOptions(IntOption{"a", "A"});
    static_assert(decltype(opts1)::num_options == 1, "Should have 1 option");

    constexpr auto opts2 = makeOptions(
        IntOption{"a", "A"},
        StringOption{"b", "B"}
    );
    static_assert(decltype(opts2)::num_options == 2, "Should have 2 options");

    constexpr auto opts4 = makeOptions(
        IntOption{"a", "A"},
        StringOption{"b", "B"},
        IntArrayOption{"c", "C"},
        StringArrayOption{"d", "D"}
    );
    static_assert(decltype(opts4)::num_options == 4, "Should have 4 options");
    std::cout << "  ✓ OptionGroup::num_options works at compile-time\n";

    std::cout << "\n";
}

// ============================================================================
// Test 4: ParsedArgs with null optionGroup
// ============================================================================
void testParsedArgsNullOptionGroup() {
    std::cout << "Test 4: ParsedArgs with null optionGroup\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port"},
        StringOption{"host", "Host"},
        IntArrayOption{"ids", "IDs"},
        StringArrayOption{"files", "Files"}
    );

    // Create ParsedArgs without setting optionGroup
    ParsedArgs<decltype(opts)> parsed;
    // optionGroup is nullptr by default

    // All name-based getters should return nullopt
    assert(!parsed.hasOption("port"));
    assert(!parsed.getInt("port").has_value());
    assert(!parsed.getString("host").has_value());
    assert(!parsed.getIntArray("ids").has_value());
    assert(!parsed.getStringArray("files").has_value());
    std::cout << "  ✓ All getters return nullopt when optionGroup is null\n";

    std::cout << "\n";
}

// ============================================================================
// Test 5: ParsedArgs const get<I>()
// ============================================================================
void testParsedArgsConstGet() {
    std::cout << "Test 5: ParsedArgs const get<I>()\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port"},
        StringOption{"host", "Host"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto& args) {
        // This lambda receives const ParsedArgs&, so we test const get<I>()
        const auto& portVal = args.template get<0>();
        const auto& hostVal = args.template get<1>();

        assert(portVal.is_set && portVal.value == 8080);
        assert(hostVal.is_set && hostVal.value == "localhost");
        return true;
    });

    bool result = cmd->execute({"--port", "8080", "--host", "localhost"});
    assert(result);
    std::cout << "  ✓ const get<I>() works correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 6: Partial range validation (only min or only max)
// ============================================================================
void testPartialRangeValidation() {
    std::cout << "Test 6: Partial range validation\n";

    // Test IntOption with range - boundary cases
    constexpr IntOption rangeOpt("val", "Value", 10, 100);

    // At boundaries
    assert(rangeOpt.isValid(10));   // Exactly at min
    assert(rangeOpt.isValid(100));  // Exactly at max
    assert(!rangeOpt.isValid(9));   // Just below min
    assert(!rangeOpt.isValid(101)); // Just above max
    std::cout << "  ✓ IntOption boundary validation works\n";

    // IntArrayOption with range
    constexpr IntArrayOption arrOpt("vals", "Values", 0, 50);
    assert(arrOpt.isValid(0));
    assert(arrOpt.isValid(50));
    assert(!arrOpt.isValid(-1));
    assert(!arrOpt.isValid(51));
    std::cout << "  ✓ IntArrayOption boundary validation works\n";

    // Test with only min (max is nullopt) - actually both must be set in constructor
    // So let's test the no-range case thoroughly
    constexpr IntOption noRange("free", "Free value");
    assert(!noRange.min_value.has_value());
    assert(!noRange.max_value.has_value());
    assert(noRange.isValid(INT64_MIN));
    assert(noRange.isValid(INT64_MAX));
    assert(noRange.isValid(0));
    std::cout << "  ✓ No-range option accepts any value\n";

    std::cout << "\n";
}

// ============================================================================
// Test 7: Mode transitions returning new mode name
// ============================================================================
void testModeTransitions() {
    std::cout << "Test 7: Mode transitions returning new mode name\n";

    auto cli = makeCLI();

    // Default mode that transitions to other modes based on command
    cli->addMode("default", [](const std::vector<std::string>& args) -> std::string {
        if (!args.empty()) {
            if (args[0] == "go-alpha") return "alpha";
            if (args[0] == "go-beta") return "beta";
        }
        return "";  // Stay in current mode
    });

    cli->addMode("alpha", [](const std::vector<std::string>& args) -> std::string {
        if (!args.empty() && args[0] == "back") return "default";
        std::cout << "[alpha mode]\n";
        return "";
    });

    cli->addMode("beta", [](const std::vector<std::string>& args) -> std::string {
        if (!args.empty() && args[0] == "back") return "default";
        std::cout << "[beta mode]\n";
        return "";
    });

    // Test transitions
    assert(cli->getCurrentMode() == "default");

    std::string result = cli->execute({"go-alpha"});
    assert(result == "alpha");
    assert(cli->getCurrentMode() == "alpha");
    std::cout << "  ✓ Transition to alpha mode works\n";

    {
        CaptureOutput capture;
        cli->execute({"test"});
        assert(capture.get().find("[alpha mode]") != std::string::npos);
    }
    std::cout << "  ✓ Commands execute in alpha mode\n";

    result = cli->execute({"back"});
    assert(result == "default");
    assert(cli->getCurrentMode() == "default");
    std::cout << "  ✓ Transition back to default mode works\n";

    result = cli->execute({"go-beta"});
    assert(result == "beta");
    assert(cli->getCurrentMode() == "beta");
    std::cout << "  ✓ Transition to beta mode works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 8: CLI addMode with Command (not dispatcher)
// ============================================================================
void testCLIWithCommand() {
    std::cout << "Test 8: CLI addMode with Command\n";

    auto cli = makeCLI();

    // Create a command
    auto opts = makeOptions(
        IntOption{"count", "Count value"},
        StringOption{"name", "Name"}
    );
    auto spec = CommandSpec{"cmd", "Test command", opts};

    bool handlerCalled = false;
    int64_t capturedCount = -1;
    std::string capturedName;

    auto cmd = makeCommand(spec, [&](const auto& args) {
        handlerCalled = true;
        if (auto c = args.getInt("count")) capturedCount = *c;
        if (auto n = args.getString("name")) capturedName = *n;
        std::cout << "[Command executed]\n";
        return true;
    });

    // Add command as a mode
    cli->addMode("cmdmode", cmd);
    cli->setMode("cmdmode");

    // Execute in command mode
    {
        CaptureOutput capture;
        cli->execute({"--count", "42", "--name", "test"});
        assert(capture.get().find("[Command executed]") != std::string::npos);
    }
    assert(handlerCalled);
    assert(capturedCount == 42);
    assert(capturedName == "test");
    std::cout << "  ✓ CLI with Command works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 9: SubcommandDispatcher showMatchingCommands direct call
// ============================================================================
void testShowMatchingCommands() {
    std::cout << "Test 9: SubcommandDispatcher showMatchingCommands\n";

    auto dispatcher = makeDispatcher("app", "Application");

    auto opts = makeOptions(IntOption{"v", "value"});
    auto spec1 = CommandSpec{"start", "Start service", opts};
    auto spec2 = CommandSpec{"stop", "Stop service", opts};
    auto spec3 = CommandSpec{"status", "Show status", opts};
    auto spec4 = CommandSpec{"restart", "Restart service", opts};

    dispatcher->addSubcommand(makeCommand(spec1, [](const auto&) { return true; }));
    dispatcher->addSubcommand(makeCommand(spec2, [](const auto&) { return true; }));
    dispatcher->addSubcommand(makeCommand(spec3, [](const auto&) { return true; }));
    dispatcher->addSubcommand(makeCommand(spec4, [](const auto&) { return true; }));

    // Test showMatchingCommands with prefix that matches multiple
    {
        CaptureOutput capture;
        dispatcher->showMatchingCommands("st");
        std::string output = capture.get();
        assert(output.find("start") != std::string::npos);
        assert(output.find("stop") != std::string::npos);
        assert(output.find("status") != std::string::npos);
        assert(output.find("restart") == std::string::npos);  // Doesn't start with "st"
    }
    std::cout << "  ✓ showMatchingCommands with 'st' prefix works\n";

    // Test showMatchingCommands with prefix that matches one
    {
        CaptureOutput capture;
        dispatcher->showMatchingCommands("re");
        std::string output = capture.get();
        assert(output.find("restart") != std::string::npos);
    }
    std::cout << "  ✓ showMatchingCommands with 're' prefix works\n";

    // Test showMatchingCommands with no matches
    {
        CaptureOutput capture;
        dispatcher->showMatchingCommands("xyz");
        std::string output = capture.get();
        assert(output.find("No subcommands matching") != std::string::npos);
    }
    std::cout << "  ✓ showMatchingCommands with no matches works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 10: OptionSpecBase derived() methods
// ============================================================================
void testOptionSpecBaseDerived() {
    std::cout << "Test 10: OptionSpecBase derived() methods\n";

    // Test IntOption derived()
    IntOption intOpt("port", "Port number", 1024, 65535);
    const IntOption& constIntOpt = intOpt;

    // Non-const derived()
    auto& derived1 = intOpt.derived();
    assert(derived1.name == "port");
    assert(derived1.min_value == 1024);
    std::cout << "  ✓ Non-const derived() works for IntOption\n";

    // Const derived()
    const auto& derived2 = constIntOpt.derived();
    assert(derived2.name == "port");
    assert(derived2.max_value == 65535);
    std::cout << "  ✓ Const derived() works for IntOption\n";

    // Test StringOption derived()
    StringOption strOpt("host", "Hostname", true);
    assert(strOpt.derived().required == true);
    std::cout << "  ✓ derived() works for StringOption\n";

    // Test createDefaultValue()
    auto intDefault = intOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(intDefault), int64_t>, "Should create int64_t");
    std::cout << "  ✓ createDefaultValue() creates correct type\n";

    auto strDefault = strOpt.createDefaultValue();
    static_assert(std::is_same_v<decltype(strDefault), std::string>, "Should create std::string");
    std::cout << "  ✓ createDefaultValue() works for StringOption\n";

    std::cout << "\n";
}

// ============================================================================
// Test 11: Command::parse with const char* argv[]
// ============================================================================
void testCommandParseConstArgv() {
    std::cout << "Test 11: Command::parse with const char* argv[]\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port"},
        StringOption{"host", "Host"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    // Use const char* array
    const char* argv[] = {"--port", "9000", "--host", "example.com"};
    auto parsed = cmd->parse(4, argv);

    assert(parsed.parseSuccess);
    assert(parsed.getInt("port").value_or(0) == 9000);
    assert(parsed.getString("host").value_or("") == "example.com");
    std::cout << "  ✓ parse() with const char* argv[] works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 12: makeCommandHandler helper
// ============================================================================
void testMakeCommandHandler() {
    std::cout << "Test 12: makeCommandHandler helper\n";

    constexpr auto opts = makeOptions(IntOption{"val", "Value"});

    // Create handler using makeCommandHandler
    bool called = false;
    auto handler = makeCommandHandler<decltype(opts)>([&called](const ParsedArgs<decltype(opts)>& args) {
        called = true;
        return args.template get<0>().is_set;
    });

    // Create parsed args manually
    ParsedArgs<decltype(opts)> parsed;
    parsed.optionGroup = &opts;
    parsed.template get<0>().set(42);

    // Invoke handler
    bool result = handler(parsed);
    assert(called);
    assert(result);
    std::cout << "  ✓ makeCommandHandler creates callable handler\n";

    std::cout << "\n";
}

// ============================================================================
// Test 13: Integer parsing edge cases (more coverage)
// ============================================================================
void testIntegerParsingMore() {
    std::cout << "Test 13: Integer parsing additional edge cases\n";

    // Large positive number
    assert(parseInt("9223372036854775807") == INT64_MAX);
    std::cout << "  ✓ parseInt handles INT64_MAX\n";

    // Large negative number
    assert(parseInt("-9223372036854775808") == INT64_MIN);
    std::cout << "  ✓ parseInt handles INT64_MIN\n";

    // Hex uppercase
    assert(parseInt("0XFF") == 255);
    std::cout << "  ✓ parseInt handles 0X prefix\n";

    // Binary uppercase
    assert(parseInt("0B1111") == 15);
    std::cout << "  ✓ parseInt handles 0B prefix\n";

    // Partial number (should fail)
    assert(!parseInt("42abc").has_value());
    std::cout << "  ✓ parseInt rejects partial numbers\n";

    // Whitespace (should fail - no trimming)
    assert(!parseInt(" 42").has_value());
    assert(!parseInt("42 ").has_value());
    std::cout << "  ✓ parseInt rejects whitespace\n";

    std::cout << "\n";
}

// ============================================================================
// Test 14: Positional arguments
// ============================================================================
void testPositionalArguments() {
    std::cout << "Test 14: Positional arguments\n";

    constexpr auto opts = makeOptions(
        IntOption{"verbose", "Verbosity"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    std::vector<std::string> capturedPositional;
    auto cmd = makeCommand(spec, [&capturedPositional](const auto& args) {
        capturedPositional = args.positional;
        return true;
    });

    cmd->execute({"file1.txt", "--verbose", "2", "file2.txt", "file3.txt"});

    assert(capturedPositional.size() == 3);
    assert(capturedPositional[0] == "file1.txt");
    assert(capturedPositional[1] == "file2.txt");
    assert(capturedPositional[2] == "file3.txt");
    std::cout << "  ✓ Positional arguments captured correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 15: All option constructor variants
// ============================================================================
void testAllOptionConstructors() {
    std::cout << "Test 15: All option constructor variants\n";

    // IntOption: (name, desc)
    constexpr IntOption int1("a", "A");
    assert(!int1.required && !int1.min_value && !int1.max_value);

    // IntOption: (name, desc, required)
    constexpr IntOption int2("b", "B", true);
    assert(int2.required && !int2.min_value && !int2.max_value);

    // IntOption: (name, desc, min, max)
    constexpr IntOption int3("c", "C", 0, 100);
    assert(!int3.required && int3.min_value == 0 && int3.max_value == 100);

    // IntOption: (name, desc, required, min, max)
    constexpr IntOption int4("d", "D", true, 10, 20);
    assert(int4.required && int4.min_value == 10 && int4.max_value == 20);
    std::cout << "  ✓ All IntOption constructors work\n";

    // StringOption: (name, desc)
    constexpr StringOption str1("e", "E");
    assert(!str1.required);

    // StringOption: (name, desc, required)
    constexpr StringOption str2("f", "F", true);
    assert(str2.required);
    std::cout << "  ✓ All StringOption constructors work\n";

    // IntArrayOption: (name, desc)
    constexpr IntArrayOption arr1("g", "G");
    assert(!arr1.required && !arr1.min_value && !arr1.max_value);

    // IntArrayOption: (name, desc, required)
    constexpr IntArrayOption arr2("h", "H", true);
    assert(arr2.required && !arr2.min_value && !arr2.max_value);

    // IntArrayOption: (name, desc, min, max)
    constexpr IntArrayOption arr3("i", "I", 0, 50);
    assert(!arr3.required && arr3.min_value == 0 && arr3.max_value == 50);

    // IntArrayOption: (name, desc, required, min, max)
    constexpr IntArrayOption arr4("j", "J", true, 5, 15);
    assert(arr4.required && arr4.min_value == 5 && arr4.max_value == 15);
    std::cout << "  ✓ All IntArrayOption constructors work\n";

    // StringArrayOption: (name, desc)
    constexpr StringArrayOption sarr1("k", "K");
    assert(!sarr1.required);

    // StringArrayOption: (name, desc, required)
    constexpr StringArrayOption sarr2("l", "L", true);
    assert(sarr2.required);
    std::cout << "  ✓ All StringArrayOption constructors work\n";

    std::cout << "\n";
}

// ============================================================================
// Test 16: Command getSpec, getName, getDescription
// ============================================================================
void testCommandGetters() {
    std::cout << "Test 16: Command getters\n";

    constexpr auto opts = makeOptions(IntOption{"val", "Value"});
    constexpr auto spec = CommandSpec{"mycommand", "My command description", opts};

    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    assert(cmd->getName() == "mycommand");
    assert(cmd->getDescription() == "My command description");

    const auto& specRef = cmd->getSpec();
    assert(specRef.name == "mycommand");
    assert(specRef.numOptions() == 1);
    std::cout << "  ✓ Command getters work correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 17: CLI getModes
// ============================================================================
void testCLIGetModes() {
    std::cout << "Test 17: CLI getModes\n";

    auto cli = makeCLI();
    cli->addMode("alpha", [](const auto&) -> std::string { return ""; });
    cli->addMode("beta", [](const auto&) -> std::string { return ""; });
    cli->addMode("gamma", [](const auto&) -> std::string { return ""; });

    auto modes = cli->getModes();
    assert(modes.size() == 3);

    // Check all modes are present (order may vary due to map)
    bool hasAlpha = false, hasBeta = false, hasGamma = false;
    for (const auto& m : modes) {
        if (m == "alpha") hasAlpha = true;
        if (m == "beta") hasBeta = true;
        if (m == "gamma") hasGamma = true;
    }
    assert(hasAlpha && hasBeta && hasGamma);
    std::cout << "  ✓ getModes() returns all registered modes\n";

    std::cout << "\n";
}

// ============================================================================
// Test 18: SubcommandDispatcher help flags (-h, --help)
// ============================================================================
void testSubcommandHelpFlags() {
    std::cout << "Test 18: SubcommandDispatcher help flags\n";

    auto dispatcher = makeDispatcher("app", "Test application");
    auto opts = makeOptions(IntOption{"v", "value"});
    auto spec = CommandSpec{"run", "Run command", opts};
    dispatcher->addSubcommand(makeCommand(spec, [](const auto&) { return true; }));

    // Test -h flag
    {
        CaptureOutput capture;
        bool result = dispatcher->execute({"-h"});
        assert(result);
        assert(capture.get().find("app") != std::string::npos);
    }
    std::cout << "  ✓ -h flag shows help\n";

    // Test --help flag
    {
        CaptureOutput capture;
        bool result = dispatcher->execute({"--help"});
        assert(result);
        assert(capture.get().find("app") != std::string::npos);
    }
    std::cout << "  ✓ --help flag shows help\n";

    std::cout << "\n";
}

// ============================================================================
// Test 19: Command invoke directly
// ============================================================================
void testCommandInvoke() {
    std::cout << "Test 19: Command invoke directly\n";

    constexpr auto opts = makeOptions(
        IntOption{"val", "Value"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    int64_t captured = -1;
    auto cmd = makeCommand(spec, [&captured](const auto& args) {
        if (auto v = args.getInt("val")) captured = *v;
        return true;
    });

    // Parse separately
    auto parsed = cmd->parse({"--val", "999"});
    assert(parsed.parseSuccess);
    assert(captured == -1);  // Not invoked yet

    // Invoke separately
    bool result = cmd->invoke(parsed);
    assert(result);
    assert(captured == 999);
    std::cout << "  ✓ parse() + invoke() works separately\n";

    std::cout << "\n";
}

// ============================================================================
// Test 20: Range filtering in arrays
// ============================================================================
void testArrayRangeFiltering() {
    std::cout << "Test 20: Range filtering in arrays\n";

    constexpr auto opts = makeOptions(
        IntArrayOption{"vals", "Values", 10, 50}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    std::vector<int64_t> captured;
    auto cmd = makeCommand(spec, [&captured](const auto& args) {
        if (auto v = args.getIntArray("vals")) captured = *v;
        return true;
    });

    // Mix of valid and invalid values
    cmd->execute({"--vals", "5", "15", "25", "55", "30", "100", "10", "50"});

    // Only values in [10, 50] should be kept
    assert(captured.size() == 5);
    assert(captured[0] == 15);
    assert(captured[1] == 25);
    assert(captured[2] == 30);
    assert(captured[3] == 10);
    assert(captured[4] == 50);
    std::cout << "  ✓ Out-of-range values filtered from arrays\n";

    std::cout << "\n";
}

// ============================================================================
// Test 21: CLI executeCommand
// ============================================================================
void testCLIExecuteCommand() {
    std::cout << "Test 21: CLI executeCommand\n";

    auto cli = makeCLI();

    bool handlerCalled = false;
    std::string lastCmd;
    cli->addMode("default", [&](const std::vector<std::string>& args) -> std::string {
        handlerCalled = true;
        if (!args.empty()) lastCmd = args[0];
        return "";
    });

    // Test executeCommand with string
    std::string result = cli->executeCommand("hello world");
    assert(handlerCalled);
    assert(lastCmd == "hello");
    std::cout << "  ✓ executeCommand parses and executes string\n";

    // Test empty command
    handlerCalled = false;
    result = cli->executeCommand("");
    assert(!handlerCalled);  // Empty command should not call handler
    assert(result == "");
    std::cout << "  ✓ executeCommand handles empty string\n";

    std::cout << "\n";
}

// ============================================================================
// Test 22: Command showHierarchy with only max range
// ============================================================================
void testShowHierarchyRangeDisplay() {
    std::cout << "Test 22: Command showHierarchy range display\n";

    constexpr auto opts = makeOptions(
        IntOption{"both", "Both min and max", 0, 100},
        IntOption{"none", "No range"},
        StringOption{"str", "String option"},
        StringArrayOption{"arr", "String array"}
    );
    constexpr auto spec = CommandSpec{"rangetest", "Range test command", opts};

    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    {
        CaptureOutput capture;
        cmd->showHierarchy("", true);
        std::string output = capture.get();

        // Check range display
        assert(output.find("min=0") != std::string::npos);
        assert(output.find("max=100") != std::string::npos);

        // Check type indicators
        assert(output.find("[int]") != std::string::npos);
        assert(output.find("[string]") != std::string::npos);
        assert(output.find("[array]") != std::string::npos);
    }
    std::cout << "  ✓ showHierarchy displays all range and type info\n";

    std::cout << "\n";
}

// ============================================================================
// Test 23: hasOption in ParsedArgs
// ============================================================================
void testParsedArgsHasOption() {
    std::cout << "Test 23: ParsedArgs hasOption\n";

    constexpr auto opts = makeOptions(
        IntOption{"present", "Present option"},
        IntOption{"absent", "Absent option"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto& args) {
        // "present" is set, "absent" is not
        assert(args.hasOption("present"));
        assert(!args.hasOption("absent"));
        assert(!args.hasOption("nonexistent"));
        return true;
    });

    cmd->execute({"--present", "42"});
    std::cout << "  ✓ hasOption works correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 24: getAllOptions returns correct info
// ============================================================================
void testGetAllOptionsInfo() {
    std::cout << "Test 24: getAllOptions returns correct info\n";

    constexpr auto opts = makeOptions(
        IntOption{"intopt", "Integer", true, 0, 100},
        StringOption{"stropt", "String", false},
        IntArrayOption{"intarr", "Int array", 10, 20},
        StringArrayOption{"strarr", "String array", true}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto allOpts = spec.getAllOptions();
    assert(allOpts.size() == 4);

    // Check intopt
    assert(allOpts[0].name == "intopt");
    assert(allOpts[0].is_int == true);
    assert(allOpts[0].is_array == false);
    assert(allOpts[0].required == true);
    assert(allOpts[0].min_value == 0);
    assert(allOpts[0].max_value == 100);
    std::cout << "  ✓ IntOption info correct\n";

    // Check stropt
    assert(allOpts[1].name == "stropt");
    assert(allOpts[1].is_int == false);
    assert(allOpts[1].is_array == false);
    assert(allOpts[1].required == false);
    assert(!allOpts[1].min_value.has_value());
    std::cout << "  ✓ StringOption info correct\n";

    // Check intarr
    assert(allOpts[2].name == "intarr");
    assert(allOpts[2].is_int == true);
    assert(allOpts[2].is_array == true);
    assert(allOpts[2].min_value == 10);
    assert(allOpts[2].max_value == 20);
    std::cout << "  ✓ IntArrayOption info correct\n";

    // Check strarr
    assert(allOpts[3].name == "strarr");
    assert(allOpts[3].is_int == false);
    assert(allOpts[3].is_array == true);
    assert(allOpts[3].required == true);
    std::cout << "  ✓ StringArrayOption info correct\n";

    // Test options() alias
    auto optionsAlias = spec.options();
    assert(optionsAlias.size() == allOpts.size());
    std::cout << "  ✓ options() alias works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 25: Unique partial match success
// ============================================================================
void testUniquePartialMatch() {
    std::cout << "Test 25: Unique partial match success\n";

    auto dispatcher = makeDispatcher("app", "Application");

    auto opts = makeOptions(IntOption{"v", "value"});
    auto spec1 = CommandSpec{"start", "Start", opts};
    auto spec2 = CommandSpec{"stop", "Stop", opts};
    auto spec3 = CommandSpec{"restart", "Restart", opts};

    bool startCalled = false, restartCalled = false;
    dispatcher->addSubcommand(makeCommand(spec1, [&](const auto&) { startCalled = true; return true; }));
    dispatcher->addSubcommand(makeCommand(spec2, [](const auto&) { return true; }));
    dispatcher->addSubcommand(makeCommand(spec3, [&](const auto&) { restartCalled = true; return true; }));

    // "re" uniquely matches "restart"
    dispatcher->execute({"re"});
    assert(restartCalled);
    std::cout << "  ✓ Unique partial match 're' -> 'restart' works\n";

    // "star" uniquely matches "start"
    dispatcher->execute({"star"});
    assert(startCalled);
    std::cout << "  ✓ Unique partial match 'star' -> 'start' works\n";

    std::cout << "\n";
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "Full Coverage Tests\n";
    std::cout << "===================\n\n";

    testTypeTraits();
    testCommandSpecMethods();
    testOptionGroupNumOptions();
    testParsedArgsNullOptionGroup();
    testParsedArgsConstGet();
    testPartialRangeValidation();
    testModeTransitions();
    testCLIWithCommand();
    testShowMatchingCommands();
    testOptionSpecBaseDerived();
    testCommandParseConstArgv();
    testMakeCommandHandler();
    testIntegerParsingMore();
    testPositionalArguments();
    testAllOptionConstructors();
    testCommandGetters();
    testCLIGetModes();
    testSubcommandHelpFlags();
    testCommandInvoke();
    testArrayRangeFiltering();
    testCLIExecuteCommand();
    testShowHierarchyRangeDisplay();
    testParsedArgsHasOption();
    testGetAllOptionsInfo();
    testUniquePartialMatch();

    std::cout << "===================\n";
    std::cout << "All full coverage tests passed!\n";

    return 0;
}
