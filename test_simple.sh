#!/bin/bash

cd /home/dzheng/workspace/cmdline/build

echo "=== Test 1: Decimal option ==="
printf "show test --count 42\nquit\n" | timeout 2 ./example

echo ""
echo "=== Test 2: Hexadecimal option ==="
printf "show test --count 0x2A\nquit\n" | timeout 2 ./example

echo ""
echo "=== Test 3: Binary option ==="
printf "show test --count 0b101010\nquit\n" | timeout 2 ./example

echo ""
echo "=== Test 4: Connect with port options ==="
printf "network\nconnect server --port 0x1F90 --retry 5\nquit\n" | timeout 2 ./example

echo ""
echo "=== Test 5: Config set with hex timeout ==="
printf "config set timeout 0x1000\nquit\n" | timeout 2 ./example
