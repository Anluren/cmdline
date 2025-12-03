#!/bin/bash

cd /home/dzheng/workspace/cmdline/build/tests

echo "=========================================="
echo "Help Query Feature Test Script (? syntax)"
echo "=========================================="
echo ""

echo "Test 1: SubcommandDispatcher - Query all with '?'"
echo "---------------------------------------------------"
./test_help_query 2>&1 | grep -A 6 "Test 1a:"

echo ""
echo "Test 2: Query commands starting with 'sta?'"
echo "--------------------------------------------"
./test_help_query 2>&1 | grep -A 3 "Test 1b:"

echo ""
echo "Test 3: Query commands starting with 's?'"
echo "------------------------------------------"
./test_help_query 2>&1 | grep -A 4 "Test 1c:"

echo ""
echo "Test 4: Query commands starting with 'r?'"
echo "------------------------------------------"
./test_help_query 2>&1 | grep -A 2 "Test 1d:"

echo ""
echo "Test 5: Query with no matches 'xyz?'"
echo "-------------------------------------"
./test_help_query 2>&1 | grep -A 1 "Test 1e:"

echo ""
echo "Test 6: ModeManager - Query all modes 'mode ?'"
echo "-----------------------------------------------"
./test_help_query 2>&1 | grep -A 4 "Test 2a:"

echo ""
echo "Test 7: Query modes starting with 'mode dev?'"
echo "----------------------------------------------"
./test_help_query 2>&1 | grep -A 2 "Test 2b:"

echo ""
echo "Test 8: Query modes starting with 'mode p?'"
echo "--------------------------------------------"
./test_help_query 2>&1 | grep -A 2 "Test 2c:"

echo ""
echo "=========================================="
echo "All help query tests completed!"
echo "=========================================="
