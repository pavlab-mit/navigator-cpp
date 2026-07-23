#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Build release if not already built
if [ ! -f "$SCRIPT_DIR/lib/libnavigator_cpp.so" ]; then
    echo "Building release..."
    "$SCRIPT_DIR/build.sh" -r
fi

# Install to /usr/local
echo "Installing to /usr/local..."
cd "$BUILD_DIR"
sudo cmake --install . --prefix /usr/local

# Refresh the loader cache so /usr/local/lib/libnavigator_cpp.so resolves at runtime
sudo ldconfig

echo ""
echo "Installed:"
echo "  /usr/local/lib/libnavigator_cpp.so"
echo "  /usr/local/include/nav_bindings.h"
echo ""
echo "In your CMakeLists.txt:"
echo ""
echo '  find_library(NAVIGATOR_CPP navigator_cpp)'
echo '  find_path(NAVIGATOR_CPP_INCLUDE nav_bindings.h)'
echo '  target_link_libraries(my_app ${NAVIGATOR_CPP})'
echo '  target_include_directories(my_app PRIVATE ${NAVIGATOR_CPP_INCLUDE})'
echo ""
