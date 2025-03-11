#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <X11/Xutil.h>

#include <main.h>
#include <term.h>
#include <term_pty.h>
#include <util.h>

/*!
 * \brief Get and process options
 */
void get_options(term_t *term, pty_t *pty, int argc, char **argv) {
    // Scan options
    int c;
    while (true) {
        static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                               {"width", required_argument, 0, 'w'},
                                               {"length", required_argument, 0, 'l'},
                                               {"foreground", required_argument, 0, 'f'},
                                               {"background", required_argument, 0, 'b'},
                                               {"cursor", required_argument, 0, 'c'},
                                               {"shell", required_argument, 0, 's'},
                                               {"font", required_argument, 0, 'o'},
                                               {0, 0, 0, 0}};
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "hw:l:s:o:f:b:c:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf("option %s", long_options[option_index].name);
                if (optarg)
                    printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 'h':
                print_help();
                exit(0);
                break;

            case 'w': {
                int custom_width = atoi(optarg);
                if (custom_width > 0)
                    term->buffer_width = (int) custom_width;
            } break;
            case 'l': {
                int custom_height = atoi(optarg);
                if (custom_height > 0)
                    term->buffer_height = (int) custom_height;
            } break;
            case 'f': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term->hex_color_fg = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for foreground. Switch to default \"" COLOR_FG "\"\n",
                            custom_hex_color);
            } break;
            case 'b': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term->hex_color_bg = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for background. Switch to default \"" COLOR_BG "\"\n",
                            custom_hex_color);
            } break;
            case 'c': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term->hex_color_cursor = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for cursor. Switch to default \"" COLOR_CURSOR "\"\n",
                            custom_hex_color);
            } break;
            case 's':
                pty->shell_path = optarg;
                if (!access(pty->shell_path, F_OK)) {
                    char *find_shell_name = strrchr(pty->shell_path, '/');// Find last '/' in the path
                    if (find_shell_name)
                        pty->shell_name = find_shell_name + 1;
                } else
                    pty->shell_path = NULL;
                break;
            case 'o':
                term->font_name = optarg;
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                fprintf(stderr, "Unknown getopt return!!!\n");
                exit(1);
                break;
        }
    }
}

/*!
 * \brief Prints help to stdout
 */
void print_help() {
    fprintf(stdout,
            "Usage: " TERM_NAME " [OPTION...]\n"
            "iksTerm (XTerminal) simple terminal emulator on X11.\n"
            "\n"
            "General options:\n"
            "   -h, --help                          Show help.\n"
            "\n"
            "Setup terminal size:\n"
            "   -wNUM, --width=NUM                  Set width of terminal window in cells. Default is 80.\n"
            "   -lNUM, --length=NUM                 Set length of terminal window in cells. Default is 40.\n"
            "\n"
            "Setup terminal color gamma:\n"
            "   -fHEX_NUM, --foreground=HEX_NUM     Set color of foreground(text) in hex format \"#000000\". Default is "
            "\"#ffffff\".\n"
            "   -bHEX_NUM, --background=HEX_NUM     Set color of background(back) in hex format \"#000000\". Default is "
            "\"#000000\".\n"
            "   -cHEX_NUM, --cursor=HEX_NUM         Set color of cursor in hex format \"#000000\". Default is \"#777777\".\n"
            "\n"
            "Setup common terminal things:\n"
            "   -sPATH, --shell=PATH                Set path to shell that launched in terminal. Default is \"/bin/sh\".\n"
            "   -oNAME, --font=NAME                 Set font from X11 by name, use `xlsfonts` to list. Default is \"fixed\".\n"
            "\n"
            "Examples:\n"
            "   $ iksTerm --width=100 -s/bin/bash -c\"#aaa000\"         # Set custom width,shell,and cursor's color\n"
            "   $ iksTerm -l56 --foreground=\"#FF00FF\" -o\"helvetica\"      # Set length, fg color and font\n"
            "If your OS comes with manual pages, you can type 'man iksTerm' for more.\n");
}

/*!
 * \brief Checks string for pattern "#000000" hex number
 */
bool is_valid_hex_color(const char *str) {
    const char valid_chars[] = "0123456789ABCDEF";// Valid hexadecimal characters
    const size_t required_length = 7;             // '#' + 6 characters

    // Check length and leading '#'
    if (!str || strlen(str) != required_length || str[0] != '#') {
        return false;
    }

    // Check remaining 6 characters
    for (size_t i = 1; i < required_length; i++) {
        char c = (char) toupper(str[i]);// Case-insensitive check
        if (strchr(valid_chars, c) == NULL) {
            return false;// Character not in valid set
        }
    }

    return true;
}