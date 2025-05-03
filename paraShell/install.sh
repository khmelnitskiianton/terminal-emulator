#!/usr/bin/bash
cmake -S . -B build
cmake --build build
sudo install -m 755 build/paraShell /usr/bin