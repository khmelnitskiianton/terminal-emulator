# paraShell

A shell parody for Linux. This project is implemented using fork and pipes. Written in C++ (since C++11 standart).
ParaShell has several internal commands:
- change directory (cd)
- exit

and also provides extensible interface for adding new commands. Implemented using C++ inheritance.

### Dependencies
- Compiler: C++11 compatible compiler (GCC ≥ 4.8.1 or Clang ≥ 3.3)

- Build system: CMake ≥ 3.10

- System libraries: Standard POSIX environment (Linux/Unix)

### Build
1. After cloning this repo:
```
cd terminal-emulator/paraShell
```
2. Generate makefile:
```
cmake -S . -B build
```
3. Build:
```
cmake --build build
```
4. Run:
```
build/paraShell
```
*to run from any directory*:
```
chmod +x ./install.sh
./install.sh
```
