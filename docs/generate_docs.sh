#!/bin/bash
# Script to generate Doxygen documentation for cmdline_ct library

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: doxygen is not installed."
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen"
    echo "  macOS: brew install doxygen"
    echo "  Fedora: sudo dnf install doxygen"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p output

# Generate documentation
echo "Generating documentation..."
doxygen Doxyfile

echo ""
echo "Documentation generated successfully!"
echo "Open docs/output/html/index.html in your browser to view."
echo ""

# Optionally open in browser
if [[ "$1" == "--open" ]]; then
    if command -v xdg-open &> /dev/null; then
        xdg-open output/html/index.html
    elif command -v open &> /dev/null; then
        open output/html/index.html
    else
        echo "Could not detect browser opener. Please open manually:"
        echo "  $SCRIPT_DIR/output/html/index.html"
    fi
fi
