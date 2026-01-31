/**
 * Test output stream customization
 * Validates that output can be redirected to any std::ostream
 */

#include "cmdline/cmdline_hdr_only.h"
#include <iostream>
#include <sstream>
#include <cassert>

using namespace cmdline_ct;

void testCommandWithStreamHandler() {
    std::cout << "Test 1: Command with stream-aware handler\n";

    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions(IntOption{"value", "A value"})
    );

    std::stringstream out, err;

    // New-style handler with streams
    auto cmd = makeCommand(spec, [](const auto& args,
                                     std::ostream& out,
                                     std::ostream& err) {
        out << "Handler executed\n";
        if (auto v = args.getInt("value")) {
            out << "Value: " << *v << "\n";
        }
        return true;
    });

    cmd->execute({"--value", "42"}, out, err);

    assert(out.str().find("Handler executed") != std::string::npos);
    assert(out.str().find("Value: 42") != std::string::npos);
    assert(err.str().empty());

    std::cout << "  Output captured: " << out.str();
    std::cout << "  PASSED\n\n";
}

void testParseErrorToErrorStream() {
    std::cout << "Test 2: Parse errors go to error stream\n";

    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions(IntOption{"known", "Known option"})
    );

    std::stringstream out, err;

    auto cmd = makeCommand(spec, [](const auto& args) { return true; });
    bool result = cmd->execute({"--unknown", "value"}, out, err);

    assert(!result);  // Should fail
    assert(err.str().find("Unknown option") != std::string::npos);
    assert(out.str().empty());

    std::cout << "  Error captured: " << err.str();
    std::cout << "  PASSED\n\n";
}

void testLegacyHandlerCompatibility() {
    std::cout << "Test 3: Legacy handler still works\n";

    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions(IntOption{"value", "A value"})
    );

    bool handlerCalled = false;

    // Legacy handler without streams
    auto cmd = makeCommand(spec, [&handlerCalled](const auto& args) {
        handlerCalled = true;
        return true;
    });

    std::stringstream out, err;
    bool result = cmd->execute({"--value", "42"}, out, err);

    assert(result);
    assert(handlerCalled);

    std::cout << "  Legacy handler executed successfully\n";
    std::cout << "  PASSED\n\n";
}

void testDispatcherWithStreams() {
    std::cout << "Test 4: SubcommandDispatcher with custom streams\n";

    std::stringstream out, err;

    auto dispatcher = makeDispatcher("app", "Test app");

    constexpr auto subSpec = CommandSpec(
        "sub",
        "Subcommand",
        makeOptions()
    );

    auto subCmd = makeCommand(subSpec, [](const auto& args,
                                           std::ostream& out,
                                           std::ostream& err) {
        out << "Subcommand executed\n";
        return true;
    });

    dispatcher->addSubcommand(subCmd);
    dispatcher->execute({"sub"}, out, err);

    assert(out.str().find("Subcommand executed") != std::string::npos);

    std::cout << "  Captured: " << out.str();
    std::cout << "  PASSED\n\n";
}

void testHelpToOutputStream() {
    std::cout << "Test 5: Help output to custom stream\n";

    std::stringstream out, err;

    auto dispatcher = makeDispatcher("app", "Test application");
    dispatcher->showHelp(out);

    assert(out.str().find("Test application") != std::string::npos);
    assert(out.str().find("Available subcommands") != std::string::npos);

    std::cout << "  Help captured successfully\n";
    std::cout << "  PASSED\n\n";
}

void testStoredOutputContext() {
    std::cout << "Test 6: Stored output context\n";

    std::stringstream out, err;

    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions()
    );

    auto cmd = makeCommand(spec, [](const auto& args,
                                     std::ostream& out,
                                     std::ostream& err) {
        out << "Context used\n";
        return true;
    });

    // Set context and use default execute()
    cmd->setOutputContext(out, err);
    cmd->execute({});

    assert(out.str().find("Context used") != std::string::npos);

    std::cout << "  Context-based output works\n";
    std::cout << "  PASSED\n\n";
}

void testDispatcherUnknownCommand() {
    std::cout << "Test 7: Dispatcher unknown command error\n";

    std::stringstream out, err;

    auto dispatcher = makeDispatcher("app", "Test app");
    dispatcher->execute({"nonexistent"}, out, err);

    assert(err.str().find("Unknown subcommand") != std::string::npos);

    std::cout << "  Error: " << err.str();
    std::cout << "  PASSED\n\n";
}

void testCommandShowHierarchy() {
    std::cout << "Test 8: Command showHierarchy to custom stream\n";

    std::stringstream out;

    constexpr auto spec = CommandSpec(
        "mycommand",
        "My description",
        makeOptions(
            IntOption{"port", "Port number"},
            StringOption{"host", "Hostname"}
        )
    );

    auto cmd = makeCommand(spec, [](const auto& args) { return true; });
    cmd->showHierarchy(out);

    assert(out.str().find("mycommand") != std::string::npos);
    assert(out.str().find("My description") != std::string::npos);
    assert(out.str().find("--port") != std::string::npos);
    assert(out.str().find("--host") != std::string::npos);

    std::cout << "  Hierarchy output captured\n";
    std::cout << "  PASSED\n\n";
}

void testCLIWithStreams() {
    std::cout << "Test 9: CLI with custom streams\n";

    std::stringstream out, err;

    auto cli = makeCLI();

    // Add a stream-aware mode handler
    cli->addMode("test", [](const std::vector<std::string>& args,
                            std::ostream& out,
                            std::ostream& err) -> std::string {
        out << "Test mode executed\n";
        return "";
    });

    cli->setMode("test");
    cli->execute({"somecommand"}, out, err);

    assert(out.str().find("Test mode executed") != std::string::npos);

    std::cout << "  Mode output: " << out.str();
    std::cout << "  PASSED\n\n";
}

void testFactoryWithContext() {
    std::cout << "Test 10: Factory functions with OutputContext\n";

    std::stringstream out, err;
    OutputContext ctx(out, err);

    constexpr auto spec = CommandSpec(
        "test",
        "Test command",
        makeOptions()
    );

    // Create command with context
    auto cmd = makeCommand(spec, [](const auto& args,
                                     std::ostream& out,
                                     std::ostream& err) {
        out << "Factory context works\n";
        return true;
    }, ctx);

    // Execute using stored context
    cmd->execute({});

    assert(out.str().find("Factory context works") != std::string::npos);

    std::cout << "  Factory-provided context works\n";
    std::cout << "  PASSED\n\n";
}

int main() {
    std::cout << "Output Stream Tests\n";
    std::cout << "===================\n\n";

    testCommandWithStreamHandler();
    testParseErrorToErrorStream();
    testLegacyHandlerCompatibility();
    testDispatcherWithStreams();
    testHelpToOutputStream();
    testStoredOutputContext();
    testDispatcherUnknownCommand();
    testCommandShowHierarchy();
    testCLIWithStreams();
    testFactoryWithContext();

    std::cout << "All tests passed!\n";
    return 0;
}
