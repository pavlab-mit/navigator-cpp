#!/bin/bash

set -e

echo "Removing navigator-cpp from /usr/local..."
sudo rm -f /usr/local/lib/libnavigator_cpp.a
sudo rm -f /usr/local/include/nav_bindings.h
echo "Done."
