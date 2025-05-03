#!/usr/bin/bash
rm -rf build
cmake -S . -B build
cmake --build build
install -m 755 build/paraShell /usr/bin