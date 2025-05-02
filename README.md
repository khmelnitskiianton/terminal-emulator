# terminal-emulator

![License](https://img.shields.io/github/license/khmelnitskiianton/terminal-emulator)

A simple implementation of a terminal emulator for Linux, featuring a GUI terminal (`iksTerm`) and a basic shell (`iksSh`).

Consist of two programs:

- `iksTerm` - X11 terminal that you can launch on any shell or program.
- `paraShell`   - simple shell.

## iksTerm

![terminal](https://github.com/user-attachments/assets/69f99875-2bab-4fc2-8fd5-93fd94ac945f)

Written on C using X11 and PTY, simple terminal that launches shell and can run different command.

[Technical details of realization](iksTerm/README.md)

Folder with sources: `iksTerm/`

### Build

**Dependences:**
- *Compiler*: `gcc` \
`sudo apt install build-essential`
- *Build util*: `make` \
`sudo apt install make`
- *Graphic system*: `X11` \
`sudo apt install xorg`
- *Compilation database*: `bear` \
`sudo apt install bear` \
(optional use with `make compile`)
- *Automated documentation*: `doxygen` \
`sudo apt install doxygen` \
(optional use with `make doxygen`) `Doxygen/html/index.html`
- *Manual*: `man` \
`sudo apt install man` \
(optional use with `make man`) `man iksTerm`

**Installation**:
```bash
git clone https://github.com/khmelnitskiianton/terminal-emulator.git # clone repo
cd iksTerm
make # compile program store in "bin/" dir
make install # install in $HOME/.local/bin
iksTerm -h
```

## paraShell

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

## License

This project is licensed under the WTFPL License. See the [LICENSE](LICENSE) file for details.
