#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include <pty.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>// For wide character handling

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

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

// Basic ASCII printable range (space to tilde)
#define IS_PRINTABLE_ASCII(c) ((c) >= 0x20 && (c) <= 0x7E)

// Unicode printable ranges (simplified)
#define IS_PRINTABLE_UNICODE(c) ((c) >= 0xA0 && (c) <= 0x10FFFF)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

bool just_wrapped = false;

//=============================================================================

/*!
 * \brief Keep fd of master and slave in PTY
 */
typedef struct pty_t {
    // Shell
    char *shell_path;
    char *shell_name;
    // Master and slave file descriptors
    int fd_master;
    int fd_slave;
} pty_t;

/*!
 * \brief Keep all information about X11 terminal
 */
typedef struct term_t {
    Display *display;
    int screen;
    Window root, window;
    int fd;
    GC graphics_context;
    Atom wm_delete;
    // Color
    char *hex_color_fg;
    char *hex_color_bg;
    char *hex_color_cursor;
    unsigned long int color_fg, color_bg, color_cursor;
    // font
    char *font_name;
    XFontStruct *font;
    uint font_width, font_height;
    // buffer
    char *buffer;
    uint buffer_x, buffer_y, buffer_width, buffer_height;
    // Actual terminal size in pixels
    uint width, height;
    //Array of pointers to command strings
    char **history;
} term_t;

//=============================================================================

bool term_destroy(term_t *term);
void term_catch_error(term_t *term);
bool term_init(term_t *term);
bool pty_new(pty_t *pty);
bool term_resize(pty_t *pty, term_t *term);
void term_draw(term_t *term);
void term_key(pty_t *pty, XKeyEvent *ev);
void term_scroll_buffer(term_t *term);
void term_pty_output(term_t *term, char *buf, ssize_t n);
void term_process_slave(term_t *term, pty_t *pty);
bool is_valid_hex_color(const char *str);
bool run(pty_t *pty, term_t *term);
void print_help();

//=============================================================================

bool term_destroy(term_t *term) {
    // Cleanup resources
    if (!XFreeGC(term->display, term->graphics_context))
        return false;
    if (!XFreeFont(term->display, term->font))
        return false;
    if (!XUnmapWindow(term->display, term->window))
        return false;
    if (!XDestroyWindow(term->display, term->window))
        return false;
    if (!XDestroyWindow(term->display, term->root))
        return false;
    if (!XCloseDisplay(term->display))
        return false;
    return true;
}

void term_catch_error(term_t *term) {
    term_destroy(term);
    exit(1);
}

/*!
 * \brief Initialize and setup X11 window for terminal
 */
bool term_init(term_t *term) {
    // Connect to X server
    if (!(term->display = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        return false;
    }
    // Setup window
    term->screen = DefaultScreen(term->display);
    term->root = RootWindow(term->display, term->screen);
    term->fd = ConnectionNumber(term->display);
    // Window attributes
    XSetWindowAttributes winattr = {
        .background_pixmap = ParentRelative,
        .event_mask = KeyPressMask | KeyReleaseMask | ExposureMask,
    };
    // Color for foreground, background, cursor
    Colormap cmap = 0;
    XColor color = {};
    cmap = DefaultColormap(term->display, term->screen);
    // Set hex colors
    if (!term->hex_color_fg)
        term->hex_color_fg = COLOR_FG;
    if (!term->hex_color_bg)
        term->hex_color_bg = COLOR_BG;
    if (!term->hex_color_cursor)
        term->hex_color_cursor = COLOR_CURSOR;
    // Alloc colors
    XAllocNamedColor(term->display, cmap, term->hex_color_fg, &color, &color);// load fg color
    term->color_fg = color.pixel;
    XAllocNamedColor(term->display, cmap, term->hex_color_bg, &color, &color);// load bg color
    term->color_bg = color.pixel;
    XAllocNamedColor(term->display, cmap, term->hex_color_cursor, &color, &color);// load cursor color
    term->color_cursor = color.pixel;
    // Load font
    if (!term->font_name)
        term->font_name = DEFAULT_FONT;
    term->font = XLoadQueryFont(term->display, term->font_name);
    if (!term->font) {
        fprintf(stderr, "Can't load font \"%s\"! Switch to default \"" DEFAULT_FONT "\"\n", term->font_name);
        term->font_name = DEFAULT_FONT;
        term->font = XLoadQueryFont(term->display, term->font_name);
    }
    // Get font characters width and height
    term->font_width = (uint) term->font->max_bounds.width;
    term->font_height = (uint) term->font->ascent + (uint) term->font->descent * 2;// Total vertical space
    // Have max width/height of screen
    uint max_width = (uint) DisplayWidth(term->display, term->screen) / term->font_width;
    uint max_height = (uint) DisplayHeight(term->display, term->screen) / term->font_height;
    // Buffer
    term->buffer_width = (term->buffer_width > 0) ? term->buffer_width : DEFAULT_WIDTH;
    term->buffer_height = (term->buffer_height > 0) ? term->buffer_height : DEFAULT_HEIGHT;
    if (max_width <= term->buffer_width)
        term->buffer_width = max_width;
    if (max_height <= term->buffer_height)
        term->buffer_height = max_height;
    term->buffer_x = 0;
    term->buffer_y = 0;
    term->buffer = calloc((long unsigned int) term->buffer_width * (long unsigned int) term->buffer_height, sizeof(char));
    if (!term->buffer) {
        perror("calloc");
        return false;
    }
    // Init history

    // Get sizes in pixels
    term->width = term->buffer_width * term->font_width;
    term->height = term->buffer_height * term->font_height;
    // Create window
    term->window = XCreateWindow(term->display,
                                 term->root,
                                 0,
                                 0,
                                 term->width,
                                 term->height,
                                 0,
                                 DefaultDepth(term->display, term->screen),
                                 CopyFromParent,
                                 DefaultVisual(term->display, term->screen),
                                 CWBackPixmap | CWEventMask,
                                 &winattr);
    // Enable WM_DELETE_WINDOW protocol
    term->wm_delete = XInternAtom(term->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(term->display, term->window, &term->wm_delete, 1);
    // Map window
    XStoreName(term->display, term->window, TERM_NAME);
    XMapWindow(term->display, term->window);
    term->graphics_context = XCreateGC(term->display, term->window, 0, NULL);

    XFlush(term->display);
    return true;
}

/*!
 * \brief Creates PTY pair and forking shell
 * 
 */
bool pty_new(pty_t *pty) {
    /*  
     * Create pseudo tty master slave pair 
     * Way 1: 
     *        posix_openpt(O_RDWR | O_NOCTTY);  Opens the PTY master device. This is the file descriptor that
     *                                          we're reading from and writing to in our terminal emulator.
     *        grantpt(master);                  Are housekeeping functions that have to
     *                                          be called before we can open the slave FD. Refer to the manpages
     *                                          on what they do.
     *        unlockpt(master);
     *        ptsname(master);                  We also need a file descriptor for our child process. 
     *                                          We get it by asking for the actual path in /dev/pts 
     *                                          which we then open using a regular open().
     *        open(slave, O_RDWR | O_NOCTTY);
     * Way 2: openpty(master, slave, NULL, NULL, NULL);
    */
    if (openpty(&pty->fd_master, &pty->fd_slave, NULL, NULL, NULL) == -1) {
        perror("openpty");
        return false;
    }

    /* 
     * Create new process for shell
     */
    pid_t pid = fork();
    if (pid == 0) {
        close(pty->fd_master);

        /* 
         * Create a new session and make our terminal this process'
         * controlling terminal. The shell will inherit the status of session leader.
         */
        setsid();
        if (ioctl(pty->fd_slave, TIOCSCTTY, NULL) == -1) {
            perror("ioctl(TIOCSCTTY)");
            return false;
        }

        //* Close master fd, and change stdout, stdin, stderr to slave fd!
        dup2(pty->fd_slave, 0);
        dup2(pty->fd_slave, 1);
        dup2(pty->fd_slave, 2);
        if (pty->fd_slave > 2)
            close(pty->fd_slave);

        //* Launch shell
        if ((!pty->shell_path) || (!pty->shell_name)) {
            pty->shell_path = SHELL_PATH;
            pty->shell_name = SHELL_NAME;
        }
        execl(pty->shell_path, pty->shell_name, NULL);
        return false;
    }
    close(pty->fd_slave);
    return true;
}

/*!
 * \brief Change sizes of terminal's window
 */
bool term_resize(pty_t *pty, term_t *term) {
    /* 
     * Create system struct with sizes of our window
     * This is the very same ioctl that normal programs use to query the
     * window size.
     */
    struct winsize ws = {
        .ws_col = (unsigned short int) term->buffer_width,
        .ws_row = (unsigned short int) term->buffer_height,
    };
    if (ioctl(pty->fd_master, TIOCSWINSZ, &ws) == -1) {
        perror("ioctl(TIOCSWINSZ)");
        return false;
    }
    return true;
}

void term_draw(term_t *term) {
    XSetForeground(term->display, term->graphics_context, term->color_bg);
    XFillRectangle(term->display, term->window, term->graphics_context, 0, 0, term->width, term->height);
    char ch = 0;
    char *buf = &ch;
    XSetForeground(term->display, term->graphics_context, term->color_fg);
    for (uint y = 0; y < term->buffer_height; y++) {
        for (uint x = 0; x < term->buffer_width; x++) {
            *buf = term->buffer[y * term->buffer_width + x];
            // Filter non-printables
            if (!IS_PRINTABLE_ASCII(*buf) /*&& !IS_PRINTABLE_UNICODE(buf[0])*/) {
                continue; // Unicode replacement character <unknown>
            }
            XDrawString(term->display,
                        term->window,
                        term->graphics_context,
                        (int) (x * term->font_width),
                        (int) (y * term->font_height) + term->font->ascent + term->font->descent,
                        buf,
                        1);
        }
    }

    XSetForeground(term->display, term->graphics_context, term->color_cursor);
    XFillRectangle(term->display,
                   term->window,
                   term->graphics_context,
                   (int) (term->buffer_x * term->font_width),
                   (int) (term->buffer_y * term->font_height),
                   term->font_width,
                   term->font_height + (uint) term->font->descent);

    XFlush(term->display);
}

void term_scroll_buffer(term_t *term) {
    memmove(term->buffer, &term->buffer[term->buffer_width], term->buffer_width * (term->buffer_height - 1));
    term->buffer_y = term->buffer_height - 1;
    for (uint i = 0; i < term->buffer_width; i++)
        term->buffer[term->buffer_y * term->buffer_width + i] = 0;
}

void term_pty_output(term_t *term, char *buf, ssize_t n) {
    // Carriage Return: Move to start of line
    for (ssize_t i = 0; i < n; i++) {
        switch (buf[i]) {
            // Carriage Return: Move to start of line
            case '\r':
                term->buffer_x = 0;
                break;
            case '\t':
                term->buffer_x += TAB_SIZE;
                break;
            // Newline: Move to next line (scroll if needed)
            case '\n':
                if (!just_wrapped) {
                    term->buffer_y++;
                    just_wrapped = false;
                }
                break;
            // Printable ASCII: Write to buffer and advance cursor
            default:
                if (IS_PRINTABLE_ASCII(buf[i])) {// ASCII printable range
                    term->buffer[term->buffer_y * term->buffer_width + term->buffer_x] = buf[i];
                    term->buffer_x++;
                    if (term->buffer_x >= term->buffer_width) {
                        term->buffer_x = 0;
                        term->buffer_y++;
                        just_wrapped = true;
                    } else
                        just_wrapped = false;
                }
                break;
        }
        // Scrolling buffer
        if (term->buffer_y >= term->buffer_height) {
            term_scroll_buffer(term);
            term->buffer_y = term->buffer_height - 1;
        }
    }
}

void term_key(pty_t *pty, XKeyEvent *ev) {
    char buf[32] = {};
    KeySym ksym = 0;
    size_t num = (size_t) XLookupString(ev, buf, sizeof(buf), &ksym, 0);
    if (num > 0) {
        write(pty->fd_master, buf, num);
    }
}

void term_process_slave(term_t *term, pty_t *pty) {
    char buf_read[READ_BUFFER_SIZE];
    ssize_t n = read(pty->fd_master, buf_read, sizeof(buf_read));
    if (n > 0) {
        term_pty_output(term, buf_read, n);
    }
    char buf[1] = {};
    XSetForeground(term->display, term->graphics_context, term->color_fg);
    for (uint y = 0; y < term->buffer_height; y++) {
        for (uint x = 0; x < term->buffer_width; x++) {
            buf[0] = term->buffer[y * term->buffer_width + x];
            // Filter non-printables
            if (!IS_PRINTABLE_ASCII(buf[0]) /*&& !IS_PRINTABLE_UNICODE(buf[0])*/) {
                continue;// Unicode replacement character <unknown>
            }
            XDrawString(term->display,
                        term->window,
                        term->graphics_context,
                        (int) (x * term->font_width),
                        (int) (y * term->font_height) + term->font->ascent + term->font->descent,
                        buf,
                        1);
        }
    }
}

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

bool run(pty_t *pty, term_t *term) {
    // Store event from X11 terminal
    XEvent event = {};

    // Create fd set
    int fd_max = pty->fd_master > term->fd ? pty->fd_master : term->fd;// count range of fd to read
    fd_set readable = {};
    bool running = true;
    while (running) {
        // Waiting for I/O with `select` syscall and <sys/select.h>
        FD_ZERO(&readable);// Clearing all file descriptors from the set
        // Add the file descriptors for reading to fd set
        FD_SET(pty->fd_master, &readable);
        FD_SET(term->fd, &readable);
        // Waits for I/O across multiple FDs without polling
        if (select(fd_max + 1, &readable, NULL, NULL, NULL) == -1) {
            perror("select");
            return false;
        }
        // Check which fd has activity
        // Terminal activity
        if (FD_ISSET(term->fd, &readable)) {
            while (XPending(term->display)) {
                XNextEvent(term->display, &event);
                switch (event.type) {
                    case ClientMessage:
                        if (event.xclient.message_type == XInternAtom(term->display, "WM_PROTOCOLS", True) &&
                            event.xclient.data.l[0] == (unsigned int) term->wm_delete) {
                            running = false;
                        }
                        break;
                    case DestroyNotify:
                        // Window destroyed externally
                        running = false;
                        break;
                    // Redraw the terminal content
                    case Expose:
                        term_draw(term);
                        break;
                    // Pass new key to shell
                    case KeyPress:
                        term_key(pty, &event.xkey);
                        break;
                    default:
                        break;
                }
            }
        }
        // PTY Master activity
        if (FD_ISSET(pty->fd_master, &readable)) {
            term_process_slave(term, pty);
            term_draw(term);
        }
    }
    return true;
}

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

int main(int argc, char **argv) {
    term_t term = {};
    pty_t pty = {};

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
                    term.buffer_width = (uint) custom_width;
            } break;
            case 'l': {
                int custom_height = atoi(optarg);
                if (custom_height > 0)
                    term.buffer_height = (uint) custom_height;
            } break;
            case 'f': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term.hex_color_fg = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for foreground. Switch to default \"" COLOR_FG "\"\n",
                            custom_hex_color);
            } break;
            case 'b': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term.hex_color_bg = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for background. Switch to default \"" COLOR_BG "\"\n",
                            custom_hex_color);
            } break;
            case 'c': {
                char *custom_hex_color = optarg;
                if (is_valid_hex_color(custom_hex_color))
                    term.hex_color_cursor = custom_hex_color;
                else
                    fprintf(stderr,
                            "Invalid hex color \"%s\" for cursor. Switch to default \"" COLOR_CURSOR "\"\n",
                            custom_hex_color);
            } break;
            case 's':
                pty.shell_path = optarg;
                if (!access(pty.shell_path, F_OK)) {
                    char *find_shell_name = strrchr(pty.shell_path, '/');// Find last '/' in the path
                    if (find_shell_name)
                        pty.shell_name = find_shell_name + 1;
                }
                else 
                    pty.shell_path = NULL;
                break;
            case 'o':
                term.font_name = optarg;
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
    // Init X11 window
    if (!term_init(&term))
        return 1;
    // Create PTY
    if (!pty_new(&pty))
        return 1;
    if (!term_resize(&pty, &term))
        return 1;
    // Process channel between X-Server and shell
    if (!run(&pty, &term))
        return 1;
    // Destroy X11 window
    if (!term_destroy(&term))
        return 1;
    return 0;
}