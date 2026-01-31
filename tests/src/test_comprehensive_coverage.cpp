/**
 * Comprehensive coverage tests for cmdline_hdr_only library
 * Tests edge cases and less-covered functionality
 */

#include <iostream>
#include <cassert>
#include <sstream>
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

// ============================================================================
// Test 1: OptionGroup visitOption functionality
// ============================================================================
void testOptionGroupVisitor() {
    std::cout << "Test 1: OptionGroup visitOption\n";

    constexpr auto opts = makeOptionGroup("test", "Test options",
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"},
        IntArrayOption{"ids", "ID list"}
    );

    // Test visiting existing option
    bool foundPort = false;
    opts.visitOption("port", [&foundPort](const auto& opt) {
        foundPort = true;
        assert(opt.name == "port");
    });
    assert(foundPort);
    std::cout << "  ✓ visitOption finds existing option\n";

    // Test visiting non-existent option (visitor not called)
    bool foundInvalid = false;
    opts.visitOption("invalid", [&foundInvalid](const auto&) {
        foundInvalid = true;
    });
    assert(!foundInvalid);
    std::cout << "  ✓ visitOption correctly skips non-existent option\n";

    std::cout << "\n";
}

// ============================================================================
// Test 2: IntOption range validation edge cases
// ============================================================================
void testIntOptionRangeValidation() {
    std::cout << "Test 2: IntOption range validation edge cases\n";

    // Option with range [0, 100]
    constexpr IntOption percentOpt("percent", "Percentage", 0, 100);

    assert(percentOpt.isValid(0));    // Lower bound
    assert(percentOpt.isValid(100));  // Upper bound
    assert(percentOpt.isValid(50));   // Middle
    assert(!percentOpt.isValid(-1));  // Below lower bound
    assert(!percentOpt.isValid(101)); // Above upper bound
    std::cout << "  ✓ Range boundary validation works correctly\n";

    // Option without range (all values valid)
    constexpr IntOption noRangeOpt("value", "Any value");
    assert(noRangeOpt.isValid(INT64_MIN));
    assert(noRangeOpt.isValid(INT64_MAX));
    assert(noRangeOpt.isValid(0));
    std::cout << "  ✓ No-range option accepts all values\n";

    std::cout << "\n";
}

// ============================================================================
// Test 3: ParsedArgs parseSuccess flag
// ============================================================================
void testParseSuccessFlag() {
    std::cout << "Test 3: ParsedArgs parseSuccess flag\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"}
    );
    constexpr auto spec = CommandSpec{"test", "Test command", opts};

    auto cmd = makeCommand(spec, [](const auto& args) {
        return args.parseSuccess;
    });

    // Test valid options
    bool result1 = cmd->execute({"--port", "8080", "--host", "localhost"});
    assert(result1);
    std::cout << "  ✓ Valid options set parseSuccess = true\n";

    // Test with unknown option (captures stderr)
    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        bool result2 = cmd->execute({"--invalid", "value"});
        std::cerr.rdbuf(oldErr);
        assert(!result2);
        assert(errBuffer.str().find("Unknown option") != std::string::npos);
    }
    std::cout << "  ✓ Unknown option sets parseSuccess = false\n";

    std::cout << "\n";
}

// ============================================================================
// Test 4: Command argc/argv parsing
// ============================================================================
void testArgcArgvParsing() {
    std::cout << "Test 4: Command argc/argv parsing\n";

    constexpr auto opts = makeOptions(
        IntOption{"count", "Count value"}
    );
    constexpr auto spec = CommandSpec{"test", "Test command", opts};

    int capturedCount = -1;
    auto cmd = makeCommand(spec, [&capturedCount](const auto& args) {
        if (auto c = args.getInt("count")) {
            capturedCount = static_cast<int>(*c);
        }
        return true;
    });

    // Test with argc/argv
    const char* argv[] = {"--count", "42"};
    cmd->execute(2, const_cast<char**>(argv));
    assert(capturedCount == 42);
    std::cout << "  ✓ argc/argv parsing works correctly\n";

    // Test parse() with argc/argv
    const char* argv2[] = {"--count", "0xFF"};
    auto parsed = cmd->parse(2, argv2);
    assert(parsed.getInt("count").value_or(0) == 255);
    std::cout << "  ✓ parse() with argc/argv handles hex values\n";

    std::cout << "\n";
}

// ============================================================================
// Test 5: CLI hasMode and edge cases
// ============================================================================
void testCLIEdgeCases() {
    std::cout << "Test 5: CLI edge cases\n";

    auto cli = makeCLI();

    cli->addMode("alpha", [](const auto&) -> std::string { return ""; });
    cli->addMode("beta", [](const auto&) -> std::string { return ""; });
    cli->addMode("gamma", [](const auto&) -> std::string { return ""; });

    // Test hasMode
    assert(cli->hasMode("alpha"));
    assert(cli->hasMode("beta"));
    assert(!cli->hasMode("nonexistent"));
    std::cout << "  ✓ hasMode works correctly\n";

    // Test setMode with invalid mode
    bool success = cli->setMode("nonexistent");
    assert(!success);
    std::cout << "  ✓ setMode returns false for invalid mode\n";

    // Test exit command
    auto result = cli->execute({"exit"});
    assert(result == "exit");
    std::cout << "  ✓ exit command returns 'exit'\n";

    // Test quit command
    result = cli->execute({"quit"});
    assert(result == "exit");
    std::cout << "  ✓ quit command returns 'exit'\n";

    // Test empty args
    result = cli->execute({});
    assert(result == "");
    std::cout << "  ✓ Empty args returns empty string\n";

    std::cout << "\n";
}

// ============================================================================
// Test 6: Ambiguous partial matching
// ============================================================================
void testAmbiguousPartialMatching() {
    std::cout << "Test 6: Ambiguous partial matching\n";

    auto cli = makeCLI();
    cli->addMode("start", [](const auto&) -> std::string { return ""; });
    cli->addMode("stop", [](const auto&) -> std::string { return ""; });
    cli->addMode("status", [](const auto&) -> std::string { return ""; });

    // Capture stderr for ambiguous match warning
    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        cli->execute({"mode", "st"});  // Ambiguous: start, stop, status
        std::cerr.rdbuf(oldErr);
        // Should print "Ambiguous mode"
        assert(errBuffer.str().find("Ambiguous") != std::string::npos ||
               errBuffer.str().find("start") != std::string::npos);
    }
    std::cout << "  ✓ Ambiguous mode switching handled\n";

    // Test SubcommandDispatcher ambiguous matching
    auto dispatcher = makeDispatcher("test", "Test dispatcher");

    auto opts = makeOptions(IntOption{"value", "Value"});
    auto spec1 = CommandSpec{"start", "Start", opts};
    auto spec2 = CommandSpec{"stop", "Stop", opts};
    auto spec3 = CommandSpec{"status", "Status", opts};

    auto cmd1 = makeCommand(spec1, [](const auto&) { return true; });
    auto cmd2 = makeCommand(spec2, [](const auto&) { return true; });
    auto cmd3 = makeCommand(spec3, [](const auto&) { return true; });

    dispatcher->addSubcommand(cmd1);
    dispatcher->addSubcommand(cmd2);
    dispatcher->addSubcommand(cmd3);

    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        dispatcher->execute({"st"});  // Ambiguous
        std::cerr.rdbuf(oldErr);
        assert(errBuffer.str().find("Ambiguous") != std::string::npos);
    }
    std::cout << "  ✓ Ambiguous subcommand handled\n";

    std::cout << "\n";
}

// ============================================================================
// Test 7: SubcommandDispatcher specific help
// ============================================================================
void testSubcommandSpecificHelp() {
    std::cout << "Test 7: SubcommandDispatcher specific help\n";

    auto dispatcher = makeDispatcher("git", "Git commands");

    auto opts = makeOptions(StringOption{"message", "Commit message"});
    auto spec = CommandSpec{"commit", "Commit changes", opts};
    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    dispatcher->addSubcommand(cmd);

    // Test help for specific subcommand
    {
        CaptureOutput capture;
        dispatcher->execute({"help", "commit"});
        assert(capture.get().find("commit") != std::string::npos);
    }
    std::cout << "  ✓ Help for specific subcommand works\n";

    // Test help for non-existent subcommand
    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        bool result = dispatcher->showSubcommandHelp("nonexistent");
        std::cerr.rdbuf(oldErr);
        assert(!result);
    }
    std::cout << "  ✓ Help for non-existent subcommand returns false\n";

    std::cout << "\n";
}

// ============================================================================
// Test 8: Command showHierarchy
// ============================================================================
void testCommandShowHierarchy() {
    std::cout << "Test 8: Command showHierarchy\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port number (1024-65535)", 1024, 65535},
        StringOption{"host", "Hostname"},
        IntArrayOption{"ids", "ID list", true}  // Required
    );
    constexpr auto spec = CommandSpec{"connect", "Connect to server", opts};

    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    {
        CaptureOutput capture;
        cmd->showHierarchy("  ", true);
        std::string output = capture.get();
        assert(output.find("connect") != std::string::npos);
        assert(output.find("--port") != std::string::npos);
        assert(output.find("[int]") != std::string::npos);
        assert(output.find("[array]") != std::string::npos);
        assert(output.find("min=") != std::string::npos);
        assert(output.find("[required]") != std::string::npos);
    }
    std::cout << "  ✓ showHierarchy displays all option details\n";

    // Test with showOptions=false
    {
        CaptureOutput capture;
        cmd->showHierarchy("", false);
        std::string output = capture.get();
        assert(output.find("connect") != std::string::npos);
        assert(output.find("--port") == std::string::npos);
    }
    std::cout << "  ✓ showHierarchy respects showOptions flag\n";

    std::cout << "\n";
}

// ============================================================================
// Test 9: SubcommandDispatcher showHierarchy
// ============================================================================
void testDispatcherShowHierarchy() {
    std::cout << "Test 9: SubcommandDispatcher showHierarchy\n";

    auto dispatcher = makeDispatcher("server", "Server management");

    auto opts1 = makeOptions(IntOption{"port", "Port"});
    auto spec1 = CommandSpec{"start", "Start server", opts1};
    auto cmd1 = makeCommand(spec1, [](const auto&) { return true; });

    auto opts2 = makeOptions(IntOption{"timeout", "Timeout"});
    auto spec2 = CommandSpec{"stop", "Stop server", opts2};
    auto cmd2 = makeCommand(spec2, [](const auto&) { return true; });

    dispatcher->addSubcommand(cmd1);
    dispatcher->addSubcommand(cmd2);

    {
        CaptureOutput capture;
        dispatcher->showHierarchy();
        std::string output = capture.get();
        assert(output.find("server") != std::string::npos);
        assert(output.find("Subcommands") != std::string::npos);
        assert(output.find("start") != std::string::npos);
        assert(output.find("stop") != std::string::npos);
    }
    std::cout << "  ✓ SubcommandDispatcher showHierarchy works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 10: CLI showHierarchy
// ============================================================================
void testCLIShowHierarchy() {
    std::cout << "Test 10: CLI showHierarchy\n";

    auto cli = makeCLI();
    cli->addMode("development", [](const auto&) -> std::string { return ""; });
    cli->addMode("production", [](const auto&) -> std::string { return ""; });
    cli->setMode("development");

    {
        CaptureOutput capture;
        cli->showHierarchy();
        std::string output = capture.get();
        assert(output.find("Mode Manager Hierarchy") != std::string::npos);
        assert(output.find("development") != std::string::npos);
        assert(output.find("production") != std::string::npos);
        assert(output.find("(current)") != std::string::npos);
    }
    std::cout << "  ✓ CLI showHierarchy works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 11: Integer parsing edge cases
// ============================================================================
void testIntegerParsingEdgeCases() {
    std::cout << "Test 11: Integer parsing edge cases\n";

    // Test parseInt directly
    assert(parseInt("42") == 42);
    assert(parseInt("0x2A") == 42);
    assert(parseInt("0X2a") == 42);
    assert(parseInt("0b101010") == 42);
    assert(parseInt("0B101010") == 42);
    assert(parseInt("-1") == -1);
    std::cout << "  ✓ parseInt handles various formats\n";

    // Test invalid integers
    assert(!parseInt("").has_value());
    assert(!parseInt("abc").has_value());
    assert(!parseInt("12.34").has_value());
    assert(!parseInt("0xGGG").has_value());
    std::cout << "  ✓ parseInt returns nullopt for invalid input\n";

    std::cout << "\n";
}

// ============================================================================
// Test 12: ParsedArgs typed tuple access
// ============================================================================
void testParsedArgsTupleAccess() {
    std::cout << "Test 12: ParsedArgs typed tuple access\n";

    constexpr auto opts = makeOptions(
        IntOption{"first", "First option"},
        StringOption{"second", "Second option"},
        IntArrayOption{"third", "Third option"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    int64_t capturedFirst = -1;
    std::string capturedSecond;
    std::vector<int64_t> capturedThird;

    auto cmd = makeCommand(spec, [&](const auto& args) {
        // Access by index using get<I>()
        if (args.template get<0>().is_set) {
            capturedFirst = args.template get<0>().value;
        }
        if (args.template get<1>().is_set) {
            capturedSecond = args.template get<1>().value;
        }
        if (args.template get<2>().is_set) {
            capturedThird = args.template get<2>().value;
        }
        return true;
    });

    cmd->execute({"--first", "100", "--second", "hello", "--third", "1", "2", "3"});

    assert(capturedFirst == 100);
    assert(capturedSecond == "hello");
    assert(capturedThird.size() == 3);
    assert(capturedThird[0] == 1 && capturedThird[1] == 2 && capturedThird[2] == 3);
    std::cout << "  ✓ Typed tuple access works correctly\n";

    std::cout << "\n";
}

// ============================================================================
// Test 13: Unknown mode handling
// ============================================================================
void testUnknownModeHandling() {
    std::cout << "Test 13: Unknown mode handling\n";

    auto cli = makeCLI();
    cli->addMode("valid", [](const auto&) -> std::string { return ""; });

    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        cli->execute({"mode", "nonexistent"});
        std::cerr.rdbuf(oldErr);
        assert(errBuffer.str().find("Unknown mode") != std::string::npos);
    }
    std::cout << "  ✓ Unknown mode error message displayed\n";

    std::cout << "\n";
}

// ============================================================================
// Test 14: SubcommandDispatcher unknown command
// ============================================================================
void testSubcommandUnknownCommand() {
    std::cout << "Test 14: SubcommandDispatcher unknown command\n";

    auto dispatcher = makeDispatcher("test", "Test");

    auto opts = makeOptions(IntOption{"v", "value"});
    auto spec = CommandSpec{"known", "Known command", opts};
    auto cmd = makeCommand(spec, [](const auto&) { return true; });
    dispatcher->addSubcommand(cmd);

    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        bool result = dispatcher->execute({"unknown"});
        std::cerr.rdbuf(oldErr);
        assert(!result);
        assert(errBuffer.str().find("Unknown subcommand") != std::string::npos);
    }
    std::cout << "  ✓ Unknown subcommand error message displayed\n";

    std::cout << "\n";
}

// ============================================================================
// Test 15: Command isOption helper
// ============================================================================
void testCommandIsOption() {
    std::cout << "Test 15: Command isOption helper\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto&) { return true; });

    assert(cmd->isOption("--port"));
    assert(cmd->isOption("port"));
    assert(cmd->isOption("--host"));
    assert(cmd->isOption("host"));
    assert(!cmd->isOption("--unknown"));
    assert(!cmd->isOption("unknown"));
    std::cout << "  ✓ isOption correctly identifies options\n";

    std::cout << "\n";
}

// ============================================================================
// Test 16: Option parsing without -- prefix
// ============================================================================
void testOptionParsingWithoutPrefix() {
    std::cout << "Test 16: Option parsing without -- prefix\n";

    constexpr auto opts = makeOptions(
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    int64_t capturedPort = -1;
    std::string capturedHost;

    auto cmd = makeCommand(spec, [&](const auto& args) {
        if (auto p = args.getInt("port")) capturedPort = *p;
        if (auto h = args.getString("host")) capturedHost = *h;
        return true;
    });

    // Test parsing with option names directly (without -- prefix)
    cmd->execute({"port", "8080", "host", "localhost"});
    assert(capturedPort == 8080);
    assert(capturedHost == "localhost");
    std::cout << "  ✓ Options parsed without -- prefix\n";

    std::cout << "\n";
}

// ============================================================================
// Test 17: makeOptionGroup named variant
// ============================================================================
void testMakeOptionGroupNamed() {
    std::cout << "Test 17: makeOptionGroup named variant\n";

    // Use makeOptionGroup with name and description
    constexpr auto namedOpts = makeOptionGroup("network", "Network options",
        IntOption{"port", "Port number"},
        StringOption{"host", "Hostname"}
    );

    assert(namedOpts.name == "network");
    assert(namedOpts.description == "Network options");
    assert(namedOpts.size() == 2);
    std::cout << "  ✓ makeOptionGroup with name and description works\n";

    // Create command with named option group
    constexpr auto spec = CommandSpec{"connect", "Connect command", namedOpts};
    auto cmd = makeCommand(spec, [](const auto&) { return true; });
    assert(cmd->getName() == "connect");
    std::cout << "  ✓ CommandSpec with named OptionGroup works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 18: SubcommandDispatcher empty args and getters
// ============================================================================
void testSubcommandDispatcherEmptyArgsAndGetters() {
    std::cout << "Test 18: SubcommandDispatcher empty args and getters\n";

    auto dispatcher = makeDispatcher("myapp", "My Application");

    auto opts = makeOptions(IntOption{"value", "Value"});
    auto spec = CommandSpec{"cmd", "Command", opts};
    auto cmd = makeCommand(spec, [](const auto&) { return true; });
    dispatcher->addSubcommand(cmd);

    // Test getters
    assert(dispatcher->getName() == "myapp");
    assert(dispatcher->getDescription() == "My Application");
    assert(dispatcher->getSubcommands().size() == 1);
    std::cout << "  ✓ SubcommandDispatcher getters work\n";

    // Test empty args (shows help)
    {
        CaptureOutput capture;
        bool result = dispatcher->execute({});
        std::string output = capture.get();
        assert(!result);  // Returns false for empty args
        assert(output.find("myapp") != std::string::npos);
    }
    std::cout << "  ✓ SubcommandDispatcher empty args shows help\n";

    std::cout << "\n";
}

// ============================================================================
// Test 19: SubcommandDispatcher argc/argv execute
// ============================================================================
void testSubcommandDispatcherArgcArgv() {
    std::cout << "Test 19: SubcommandDispatcher argc/argv execute\n";

    auto dispatcher = makeDispatcher("app", "Application");

    bool handlerCalled = false;
    auto opts = makeOptions(IntOption{"num", "Number"});
    auto spec = CommandSpec{"run", "Run command", opts};
    auto cmd = makeCommand(spec, [&handlerCalled](const auto&) {
        handlerCalled = true;
        return true;
    });
    dispatcher->addSubcommand(cmd);

    // Test execute with argc/argv
    char* argv[] = {const_cast<char*>("run"), const_cast<char*>("--num"), const_cast<char*>("42")};
    bool result = dispatcher->execute(3, argv);
    assert(result);
    assert(handlerCalled);
    std::cout << "  ✓ SubcommandDispatcher argc/argv execute works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 20: CLI argc/argv execute
// ============================================================================
void testCLIArgcArgv() {
    std::cout << "Test 20: CLI argc/argv execute\n";

    auto cli = makeCLI();

    bool handlerCalled = false;
    cli->addMode("default", [&handlerCalled](const std::vector<std::string>& args) -> std::string {
        if (!args.empty() && args[0] == "test") {
            handlerCalled = true;
        }
        return "";
    });

    // Test execute with argc/argv
    char* argv[] = {const_cast<char*>("test")};
    cli->execute(1, argv);
    assert(handlerCalled);
    std::cout << "  ✓ CLI argc/argv execute works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 21: CLI no handler for current mode
// ============================================================================
void testCLINoHandler() {
    std::cout << "Test 21: CLI no handler for current mode\n";

    auto cli = makeCLI();
    // Set mode to something that doesn't exist (edge case - shouldn't happen normally)
    // We can't directly set an invalid mode, so we test the flow differently

    // Add a mode, then remove it conceptually by creating a new manager
    // Actually, let's test the case where current mode has no handler
    // This is tricky because setMode validates. Let's test another path.

    // The "No handler for mode" error occurs when m_modes.find(m_currentMode) fails
    // This can happen if the mode map is empty or current mode isn't in it
    // Default mode is "default" which won't be found initially

    auto cli2 = makeCLI();
    // Don't add any modes - default mode won't have a handler
    {
        std::stringstream errBuffer;
        auto oldErr = std::cerr.rdbuf(errBuffer.rdbuf());
        cli2->execute({"somecommand"});  // No handler for "default" mode
        std::cerr.rdbuf(oldErr);
        assert(errBuffer.str().find("No handler for mode") != std::string::npos);
    }
    std::cout << "  ✓ CLI no handler error displayed\n";

    std::cout << "\n";
}

// ============================================================================
// Test 22: IntArrayOption range validation constructors
// ============================================================================
void testIntArrayOptionRangeConstructors() {
    std::cout << "Test 22: IntArrayOption range validation constructors\n";

    // Constructor with required flag and range
    constexpr IntArrayOption rangeOpt1("ports", "Port list", true, 1, 65535);
    assert(rangeOpt1.required == true);
    assert(rangeOpt1.min_value == 1);
    assert(rangeOpt1.max_value == 65535);
    assert(rangeOpt1.isValid(80));
    assert(!rangeOpt1.isValid(0));
    assert(!rangeOpt1.isValid(70000));
    std::cout << "  ✓ IntArrayOption with required and range works\n";

    // Constructor with range only (not required)
    constexpr IntArrayOption rangeOpt2("ids", "ID list", 0, 100);
    assert(rangeOpt2.required == false);
    assert(rangeOpt2.min_value == 0);
    assert(rangeOpt2.max_value == 100);
    assert(rangeOpt2.isValid(50));
    assert(!rangeOpt2.isValid(-1));
    assert(!rangeOpt2.isValid(101));
    std::cout << "  ✓ IntArrayOption with range only works\n";

    // Use in actual command to verify runtime behavior
    constexpr auto opts = makeOptions(rangeOpt1);
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    std::vector<int64_t> capturedPorts;
    auto cmd = makeCommand(spec, [&capturedPorts](const auto& args) {
        if (auto p = args.getIntArray("ports")) {
            capturedPorts = *p;
        }
        return true;
    });

    cmd->execute({"--ports", "80", "443", "8080"});
    assert(capturedPorts.size() == 3);
    std::cout << "  ✓ IntArrayOption with range works in command\n";

    std::cout << "\n";
}

// ============================================================================
// Test 23: CLI addMode with SubcommandDispatcher
// ============================================================================
void testCLIWithDispatcher() {
    std::cout << "Test 23: CLI addMode with SubcommandDispatcher\n";

    auto cli = makeCLI();

    // Create a dispatcher and add it as a mode
    auto dispatcher = makeDispatcher("git", "Git commands");

    auto opts = makeOptions(StringOption{"message", "Message"});
    auto spec = CommandSpec{"commit", "Commit", opts};
    auto cmd = makeCommand(spec, [](const auto&) {
        std::cout << "[commit executed]\n";
        return true;
    });
    dispatcher->addSubcommand(cmd);

    cli->addMode("git", dispatcher);
    cli->setMode("git");

    // Execute a command in git mode
    {
        CaptureOutput capture;
        cli->execute({"commit", "--message", "test"});
        assert(capture.get().find("[commit executed]") != std::string::npos);
    }
    std::cout << "  ✓ CLI with SubcommandDispatcher works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 24: Type mismatch in getter functions
// ============================================================================
void testGetterTypeMismatch() {
    std::cout << "Test 24: Type mismatch in getter functions\n";

    // Create options with all types
    constexpr auto opts = makeOptions(
        IntOption{"intval", "Integer value"},
        StringOption{"strval", "String value"},
        IntArrayOption{"intarr", "Integer array"},
        StringArrayOption{"strarr", "String array"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto& args) {
        // Try to get int from string option (type mismatch - returns nullopt)
        auto wrongInt = args.getInt("strval");
        assert(!wrongInt.has_value());

        // Try to get string from int option (type mismatch - returns nullopt)
        auto wrongStr = args.getString("intval");
        assert(!wrongStr.has_value());

        // Try to get int array from string option (type mismatch - returns nullopt)
        auto wrongIntArr = args.getIntArray("strval");
        assert(!wrongIntArr.has_value());

        // Try to get string array from int option (type mismatch - returns nullopt)
        auto wrongStrArr = args.getStringArray("intval");
        assert(!wrongStrArr.has_value());

        // Correct type lookups should work
        auto correctInt = args.getInt("intval");
        auto correctStr = args.getString("strval");
        auto correctIntArr = args.getIntArray("intarr");
        auto correctStrArr = args.getStringArray("strarr");

        assert(correctInt.has_value() && *correctInt == 42);
        assert(correctStr.has_value() && *correctStr == "hello");
        assert(correctIntArr.has_value() && correctIntArr->size() == 2);
        assert(correctStrArr.has_value() && correctStrArr->size() == 2);

        return true;
    });

    bool result = cmd->execute({
        "--intval", "42",
        "--strval", "hello",
        "--intarr", "1", "2",
        "--strarr", "a", "b"
    });
    assert(result);
    std::cout << "  ✓ Type mismatch returns nullopt\n";
    std::cout << "  ✓ Correct type lookups succeed\n";

    std::cout << "\n";
}

// ============================================================================
// Test 25: StringArrayOption full coverage
// ============================================================================
void testStringArrayOptionCoverage() {
    std::cout << "Test 25: StringArrayOption full coverage\n";

    // Test default constructor
    constexpr StringArrayOption opt1("files", "File list");
    assert(opt1.name == "files");
    assert(opt1.required == false);
    std::cout << "  ✓ StringArrayOption default constructor works\n";

    // Test required constructor
    constexpr StringArrayOption opt2("paths", "Path list", true);
    assert(opt2.required == true);
    std::cout << "  ✓ StringArrayOption required constructor works\n";

    // Use in command
    constexpr auto opts = makeOptions(
        StringArrayOption{"tags", "Tag list"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    std::vector<std::string> captured;
    auto cmd = makeCommand(spec, [&captured](const auto& args) {
        if (auto t = args.getStringArray("tags")) {
            captured = *t;
        }
        return true;
    });

    cmd->execute({"--tags", "alpha", "beta", "gamma"});
    assert(captured.size() == 3);
    assert(captured[0] == "alpha" && captured[1] == "beta" && captured[2] == "gamma");
    std::cout << "  ✓ StringArrayOption parsing works\n";

    std::cout << "\n";
}

// ============================================================================
// Test 26: OptionGroup size() function
// ============================================================================
void testOptionGroupSize() {
    std::cout << "Test 26: OptionGroup size() function\n";

    constexpr auto opts1 = makeOptions(
        IntOption{"a", "A"}
    );
    static_assert(opts1.size() == 1);

    constexpr auto opts2 = makeOptions(
        IntOption{"a", "A"},
        StringOption{"b", "B"}
    );
    static_assert(opts2.size() == 2);

    constexpr auto opts3 = makeOptions(
        IntOption{"a", "A"},
        StringOption{"b", "B"},
        IntArrayOption{"c", "C"}
    );
    static_assert(opts3.size() == 3);

    // Runtime check
    assert(opts1.size() == 1);
    assert(opts2.size() == 2);
    assert(opts3.size() == 3);
    std::cout << "  ✓ OptionGroup size() works at compile-time and runtime\n";

    std::cout << "\n";
}

// ============================================================================
// Test 27: Non-existent option lookups
// ============================================================================
void testNonExistentOptionLookup() {
    std::cout << "Test 27: Non-existent option lookups\n";

    constexpr auto opts = makeOptions(
        IntOption{"existing", "Existing option"}
    );
    constexpr auto spec = CommandSpec{"test", "Test", opts};

    auto cmd = makeCommand(spec, [](const auto& args) {
        // Try to get non-existent options
        auto noInt = args.getInt("nonexistent");
        auto noStr = args.getString("nonexistent");
        auto noIntArr = args.getIntArray("nonexistent");
        auto noStrArr = args.getStringArray("nonexistent");

        assert(!noInt.has_value());
        assert(!noStr.has_value());
        assert(!noIntArr.has_value());
        assert(!noStrArr.has_value());

        return true;
    });

    cmd->execute({"--existing", "42"});
    std::cout << "  ✓ Non-existent option lookups return nullopt\n";

    std::cout << "\n";
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "Comprehensive Coverage Tests\n";
    std::cout << "============================\n\n";

    testOptionGroupVisitor();
    testIntOptionRangeValidation();
    testParseSuccessFlag();
    testArgcArgvParsing();
    testCLIEdgeCases();
    testAmbiguousPartialMatching();
    testSubcommandSpecificHelp();
    testCommandShowHierarchy();
    testDispatcherShowHierarchy();
    testCLIShowHierarchy();
    testIntegerParsingEdgeCases();
    testParsedArgsTupleAccess();
    testUnknownModeHandling();
    testSubcommandUnknownCommand();
    testCommandIsOption();
    testOptionParsingWithoutPrefix();
    testMakeOptionGroupNamed();
    testSubcommandDispatcherEmptyArgsAndGetters();
    testSubcommandDispatcherArgcArgv();
    testCLIArgcArgv();
    testCLINoHandler();
    testIntArrayOptionRangeConstructors();
    testCLIWithDispatcher();
    testGetterTypeMismatch();
    testStringArrayOptionCoverage();
    testOptionGroupSize();
    testNonExistentOptionLookup();

    std::cout << "============================\n";
    std::cout << "All comprehensive tests passed!\n";

    return 0;
}
