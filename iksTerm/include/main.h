#ifndef MAIN_H
#define MAIN_H

#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define COLOR_FG "#ffffff"
#define COLOR_BG "#000000"
#define COLOR_CURSOR "#777777"
#define DEFAULT_FONT "fixed"
#define TERM_NAME "iksTerm"
#define DEFAULT_WIDTH 120
#define DEFAULT_HEIGHT 60
#define READ_BUFFER_SIZE 4096
#define TAB_SIZE 4
#define HISTORY_SIZE 10

// Basic ASCII printable range (space to tilde)
#define IS_PRINTABLE_ASCII(c) ((c) >= 0x20 && (c) <= 0x7E)

// Unicode printable ranges (simplified)
#define IS_PRINTABLE_UNICODE(c) ((c) >= 0xA0 && (c) <= 0x10FFFF)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#endif