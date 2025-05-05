# paraShell

A shell parody for Linux. This project is implemented using fork and pipes. Written in C++ (since C++11 standart).
ParaShell has several internal commands:
- change directory (cd)
- exit

and also provides extensible interface for adding new commands. Implemented using C++ inheritance.

**Creation:**

1. Read the user input string and store it using C++ containers.
2. Count the number commands to properly handle pipes.
3. Spawn new processes for each command.
4. Connect the processes using pipes and file descriptors.

## Result

We have program that starts executable files with user-provided arguments. This program support pipes (e.g., cmd1 | cdm2) and some internal commands (e.g., cd, exit).
