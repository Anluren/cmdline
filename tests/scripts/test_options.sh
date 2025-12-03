#!/bin/bash

# Script to test the options feature with various integer formats

echo "Testing command line options with hex, decimal, and binary integers"
echo "===================================================================="
echo ""

cd /home/dzheng/workspace/cmdline/build

echo "Test 1: show command with verbose option"
echo "Command: show test --verbose 1"
echo "show test --verbose 1" | ./example
echo ""

echo "Test 2: show command with count in decimal"
echo "Command: show items --count 42"
echo "show items --count 42" | ./example
echo ""

echo "Test 3: show command with count in hexadecimal"
echo "Command: show data --count 0x2A"
echo "show data --count 0x2A" | ./example
echo ""

echo "Test 4: show command with count in binary"
echo "Command: show list --count 0b101010"
echo "show list --count 0b101010" | ./example
echo ""

echo "Test 5: connect command with port in decimal"
echo "Command: network, then connect 192.168.1.1 --port 8080 --retry 3"
printf "network\nconnect 192.168.1.1 --port 8080 --retry 3\nexit\n" | ./example
echo ""

echo "Test 6: connect command with port in hexadecimal"
echo "Command: network, then connect server --port 0x1F90 --retry 5"
printf "network\nconnect server --port 0x1F90 --retry 5\nexit\n" | ./example
echo ""

echo "Test 7: set command with timeout in hex"
echo "Command: config set timeout 0x1000"
echo "config set timeout 0x1000" | ./example
echo ""

echo "Test 8: set command with timeout in binary"
echo "Command: config set retries 0b11"
echo "config set retries 0b11" | ./example
echo ""

echo "All tests completed!"
