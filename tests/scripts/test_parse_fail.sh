#!/bin/bash

# Test script for parse failure detection
# Demonstrates that invalid options prevent handler execution

cd "$(dirname "$0")/../../build/tests" || exit 1

echo "======================================"
echo "Parse Failure Detection Test"
echo "======================================"
echo ""

echo "Running test_parse_fail..."
./test_parse_fail

echo ""
echo "======================================"
echo "Expected behavior:"
echo "  Test 1: Handler executes (valid option)"
echo "  Test 2: Handler does NOT execute (invalid option)"
echo "  Test 3: Handler does NOT execute (mixed valid/invalid)"
echo "======================================"
