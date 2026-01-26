#!/usr/bin/env python3
"""
Test runner for cmdline library tests
Feeds input to test programs and validates output
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Tuple, Optional

# Base directory for tests
TESTS_DIR = Path(__file__).parent.parent / "build" / "tests"
if not TESTS_DIR.exists():
    TESTS_DIR = Path(__file__).parent.parent.parent / "build" / "tests"

class TestResult:
    def __init__(self, name: str, passed: bool, message: str = ""):
        self.name = name
        self.passed = passed
        self.message = message
    
    def __str__(self):
        status = "✓ PASS" if self.passed else "✗ FAIL"
        msg = f" - {self.message}" if self.message else ""
        return f"{status}: {self.name}{msg}"


def run_test(executable: str, expected_output: List[str], 
             expected_errors: Optional[List[str]] = None) -> TestResult:
    """Run a test executable and check for expected output"""
    exe_path = TESTS_DIR / executable
    
    if not exe_path.exists():
        return TestResult(executable, False, f"Executable not found: {exe_path}")
    
    try:
        result = subprocess.run(
            [str(exe_path)],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        stdout = result.stdout
        stderr = result.stderr
        
        # Check expected output
        for expected in expected_output:
            if expected not in stdout:
                return TestResult(
                    executable, False, 
                    f"Expected '{expected}' not found in stdout"
                )
        
        # Check expected errors
        if expected_errors:
            for expected_err in expected_errors:
                if expected_err not in stderr:
                    return TestResult(
                        executable, False,
                        f"Expected error '{expected_err}' not found in stderr"
                    )
        
        return TestResult(executable, True)
        
    except subprocess.TimeoutExpired:
        return TestResult(executable, False, "Test timed out")
    except Exception as e:
        return TestResult(executable, False, f"Exception: {str(e)}")


def run_interactive_test(executable: str, commands: List[str],
                         expected_outputs: List[str]) -> TestResult:
    """Run a test with interactive input"""
    exe_path = TESTS_DIR / executable
    
    if not exe_path.exists():
        return TestResult(executable, False, f"Executable not found: {exe_path}")
    
    try:
        # Join commands with newlines
        input_data = '\n'.join(commands) + '\n'
        
        result = subprocess.run(
            [str(exe_path)],
            input=input_data,
            capture_output=True,
            text=True,
            timeout=5
        )
        
        stdout = result.stdout
        
        # Check each expected output
        for expected in expected_outputs:
            if expected not in stdout:
                return TestResult(
                    executable, False,
                    f"Expected '{expected}' not found in output"
                )
        
        return TestResult(executable, True)
        
    except subprocess.TimeoutExpired:
        return TestResult(executable, False, "Test timed out")
    except Exception as e:
        return TestResult(executable, False, f"Exception: {str(e)}")


def main():
    print("=" * 60)
    print("cmdline Library Test Suite")
    print("=" * 60)
    print()
    
    results = []
    
    # Test 1: Parse Failure Detection
    print("Running parse failure tests...")
    results.append(run_test(
        "test_parse_fail",
        expected_output=[
            "Test 1: Valid option",
            "Handler executed successfully!",
            "Result: success",
            "Test 2: Invalid option",
            "Result: failure",
            "Test 3: Mix of valid and invalid",
            "Result: failure"
        ],
        expected_errors=[
            "Error: Unknown option '--invalid'"
        ]
    ))
    
    # Test 2: Int Range Validation
    print("Running int range validation tests...")
    results.append(run_test(
        "test_int_range",
        expected_output=[
            "IntOption Range Validation Demo",
            "Test 1: All values within range",
            "Port: 8080",
            "Percentage: 75%",
            "Temperature: 25°C"
        ]
    ))
    
    # Test 3: Array Range Validation
    print("Running array range validation tests...")
    results.append(run_test(
        "test_array_range",
        expected_output=[
            "IntArrayOption Range Validation Demo",
            "Test 1: All port values within valid range",
            "Ports: [80, 443, 8080, 3000]"
        ]
    ))
    
    # Test 4: Typed Options
    print("Running typed options tests...")
    results.append(run_test(
        "test_typed_options",
        expected_output=[
            "Typed Options and Composition Demo",
            "Test 1: Single value options",
            "Connecting to: example.com",
            "Port: 8080"
        ]
    ))
    
    # Test 5: Parse and Invoke
    print("Running parse and invoke tests...")
    results.append(run_test(
        "test_parse_invoke",
        expected_output=[
            "Demonstrating Separated Parse and Invoke",
            "Test 1: Using execute() - single call",
            "[HANDLER] Connecting to: 192.168.1.1",
            "[HANDLER] Port: 8080"
        ]
    ))
    
    # Test 6: Constexpr Features
    print("Running constexpr tests...")
    results.append(run_test(
        "constexpr_test",
        expected_output=[
            "Compile-time computed indices:",
            "help: 0",
            "exit: 1",
            "Runtime search for 'exit': 1"
        ]
    ))
    
    # Test 7: Default Values
    print("Running default values tests...")
    results.append(run_test(
        "test_default_values",
        expected_output=[
            "Default Value Creation Test",
            "Test 1: IntOption default value",
            "Default int64_t value: 0",
            "✓ Correct type and value"
        ]
    ))
    
    # Test 8: Subcommands
    print("Running subcommands tests...")
    results.append(run_test(
        "test_subcommands",
        expected_output=[
            "Subcommand Support Demo",
            "Test 1: Show main help",
            "git: Git version control system",
            "Available subcommands:"
        ]
    ))

    # Test 9: Comprehensive Coverage
    print("Running comprehensive coverage tests...")
    results.append(run_test(
        "test_comprehensive_coverage",
        expected_output=[
            "Comprehensive Coverage Tests",
            "Test 1: OptionGroup visitOption",
            "Test 2: IntOption range validation edge cases",
            "Test 3: ParsedArgs parseSuccess flag",
            "Test 4: Command argc/argv parsing",
            "Test 5: ModeManager edge cases",
            "Test 6: Ambiguous partial matching",
            "Test 7: SubcommandDispatcher specific help",
            "Test 8: Command showHierarchy",
            "Test 9: SubcommandDispatcher showHierarchy",
            "Test 10: ModeManager showHierarchy",
            "Test 11: Integer parsing edge cases",
            "Test 12: ParsedArgs typed tuple access",
            "Test 13: Unknown mode handling",
            "Test 14: SubcommandDispatcher unknown command",
            "Test 15: Command isOption helper",
            "Test 16: Option parsing without -- prefix",
            "Test 17: makeOptionGroup named variant",
            "Test 18: SubcommandDispatcher empty args and getters",
            "Test 19: SubcommandDispatcher argc/argv execute",
            "Test 20: ModeManager argc/argv execute",
            "Test 21: ModeManager no handler for current mode",
            "Test 22: IntArrayOption range validation constructors",
            "Test 23: ModeManager addMode with SubcommandDispatcher",
            "Test 24: Type mismatch in getter functions",
            "Test 25: StringArrayOption full coverage",
            "Test 26: OptionGroup size() function",
            "Test 27: Non-existent option lookups",
            "All comprehensive tests passed!"
        ]
    ))

    print()
    print("=" * 60)
    print("Test Results")
    print("=" * 60)
    
    for result in results:
        print(result)
    
    print()
    passed = sum(1 for r in results if r.passed)
    total = len(results)
    print(f"Total: {passed}/{total} tests passed")
    
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
