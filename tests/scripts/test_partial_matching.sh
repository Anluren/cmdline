#!/bin/bash

cd /home/dzheng/workspace/cmdline/build/tests

echo "=========================================="
echo "Partial Command Matching Test Script"
echo "=========================================="
echo ""

echo "Test 1: Exact match 'start'"
echo "----------------------------"
./test_partial_matching 2>&1 | grep -A 1 "Test 1:"

echo ""
echo "Test 2: Ambiguous partial match 'sta'"
echo "--------------------------------------"
./test_partial_matching 2>&1 | grep -A 5 "Test 2:"

echo ""
echo "Test 3: Unique partial match 'star'"
echo "------------------------------------"
./test_partial_matching 2>&1 | grep -A 1 "Test 3:"

echo ""
echo "Test 4: Unique partial match 'stat'"
echo "------------------------------------"
./test_partial_matching 2>&1 | grep -A 1 "Test 4:"

echo ""
echo "Test 5: Unique partial match 'sto'"
echo "-----------------------------------"
./test_partial_matching 2>&1 | grep -A 1 "Test 5:"

echo ""
echo "Test 6: Ambiguous prefix 's'"
echo "----------------------------"
./test_partial_matching 2>&1 | grep -A 6 "Test 6:"

echo ""
echo "Test 7: Unknown command 'restart'"
echo "----------------------------------"
./test_partial_matching 2>&1 | grep -A 2 "Test 7:"

echo ""
echo "Test 8: ModeManager partial matching"
echo "====================================="
./test_partial_matching 2>&1 | grep -A 20 "Test 8:"

echo ""
echo "=========================================="
echo "All partial matching tests completed!"
echo "=========================================="
