# terminal-emulator

![License](https://img.shields.io/github/license/khmelnitskiianton/terminal-emulator)

A simple implementation of a terminal emulator for Linux, featuring a GUI terminal (`iksTerm`) and a basic shell (`iksSh`).

Consist of two programs:

- `iksTerm` - X11 terminal that you can launch on any shell or program.
- `iksSh`   - simple shell.

## iksTerm

![terminal](https://github.com/user-attachments/assets/69f99875-2bab-4fc2-8fd5-93fd94ac945f)

Written on C using X11 and PTY, simple terminal that launches shell and can run different command.

[Technical details of realization](iksTerm/README.md)

Folder with sources: `iksTerm/`

### Build

**Dependences:**
- Compiler: `gcc`               (`sudo apt install build-essential`)
- Build util: `make`            (`sudo apt install make`)
- Graphic system: `X11`         (`sudo apt install xorg`)
- Compilation database: `bear`  (`sudo apt install bear`) (optional use with `make compile`)

```bash
git clone git@github.com:khmelnitskiianton/terminal-emulator.git # clone
cd iksTerm    
make # compile program store in "bin/" dir
make install
man iksTerm  # Man page of iksTerm
iksTerm -h
```

## iksSh

`in stage of development`

## License

This project is licensed under the WTFPL License. See the [LICENSE](LICENSE) file for details.
