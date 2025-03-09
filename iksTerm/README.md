# iksTerm

Simple terminal emulator

## Good Articles

- [Basics of terminal emulator](https://www.uninformativ.de/blog/postings/2018-02-24/0/POSTING-en.html)
- [Anatomy of terminal emulator](https://poor.dev/blog/terminal-anatomy/)
- [Theory of terminal emulator](https://medium.com/@artur.araqelyan.0001/developing-terminal-emulator-in-c-9f3d2007e7c1)
- [Linux pty article](https://dev.to/napicella/linux-terminals-tty-pty-and-shell-192e)
- [Linux tty understanding](https://ishuah.com/2021/02/04/understanding-the-linux-tty-subsystem/)
- [Linux TTY demystified](https://www.linusakesson.net/programming/tty/)
- [Linux Kernel](https://aeb.win.tue.nl/linux/lk/lk-10.html)

## Structure of terminal emulation

Terminal-emulation consist of 3 elements:
- Terminal
- Shell
- PTY

The shell is a program that provides an interface to the operating system, allowing the user to interact with its file-system, run processes and often have access to basic scripting capabilities.

The terminal emulator is a graphical application whose role it is to interpret data coming from the shell and display it on screen. 

PTY is a bi-directional asynchronous communication channel between the terminal and shell.

### Terminal

Using X Window System, create window on which we emulate everything using buffer that we change when we reading data from pty(that is shell) and update after user's actions.

**Develop:**

1. Create display, window and e.t.c to setup our terminal for user.
2. Handle events from user and translate it as new enter or changes in window.
3. Handle new information from shell on the other side of PTY.
4. Depending on new enter change buffer and send incoming data to shell.
5. Reflect it using buffer and redrawing window.

### PTY(Pseudo Terminal)

One channel of the pty represents the communication directed from the terminal emulator to the shell (STDIN) and the other channel refers to the communication directed from the shell to the terminal emulator (STDOUT). When a user types text into the terminal, it is sent to the shell through the pty’s STDIN channel, and when the shell would like to display text to the user on their terminal emulator, it is sent to it through the pty’s STDOUT channel.

**Creation:**

1. Create master-slave pair of file destructors with PTY master device that provides by Linux Core using syscalls or standard pty functions. 
2. Create new process for shell
3. Create new session for it, make terminal controlling for this terminal and change stdin,stdout,stderr to slave fd.
 
As the result we have channel between X11 Terminal and Shell

## Result

We have X11 GUI application that consist of graphic window and pty with which I can run shell or other application that can interact with user!

In basic version we have:
- Different options to setup terminal
- Entering text and sending it to shell
- See shell output
- Use popular control symbols
## Future Development

- [x] Resizing
- [ ] Custom font upload(improve)
- [ ] Process control sequences & signals
- [ ] UTF-8 except of ASCII
- [ ] Keep history and get it by arrows
- [x] Handle more control chars (`\b`)