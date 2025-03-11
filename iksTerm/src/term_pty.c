#include <pty.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <X11/Xutil.h>

#include "main.h"
#include "term.h"
#include "term_pty.h"

/*!
 * \brief Creates PTY pair and forking shell
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
    pty->pid = fork();
    if (pty->pid == 0) {
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

        // Set termios mode to configure reading bytes from pty slave:
        struct termios tios;
        if (tcgetattr(pty->fd_slave, &tios) == -1) {
            perror("tcgetattr");
            return false;
        }
        tios.c_lflag |= ICANON | ECHO;
        tios.c_oflag |= OPOST | ONLCR;// ONLCR: convert \n в \r\n when reading
        tios.c_iflag |= ICRNL;        // ICRNL: convert \r в \n when writing

        tios.c_cc[VERASE] =
            0x08;// Set that driver clean from buffer backspace symbol like VERASE, to have `ls` except of `ls \b s`

// Disabling the output of control characters in the form of carriage notation.
// ECHOCTL (sometimes called ECHOE or ECHOECTL) is responsible for displaying ^M instead of the CR character.
#ifdef ECHOCTL
        tios.c_lflag &= (uint) ~ECHOCTL;
#endif
        if (tcsetattr(pty->fd_slave, TCSANOW, &tios) == -1) {
            perror("tcsetattr");
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
bool term_resize(term_t *term, pty_t *pty, XEvent *event) {
    int new_width = (int) event->xconfigure.width;
    int new_height = (int) event->xconfigure.height;
    if (new_width == term->width && new_height == term->height)
        return false;
    int new_buffer_width = new_width / term->font_width;
    int new_buffer_height = new_height / term->font_height;
    term_move_buffer(term, new_buffer_width, new_buffer_height);
    term->width = new_width;
    term->height = new_height;
    if (!pty_resize(term, pty))
        return false;
    XClearWindow(term->display, term->window);
    return true;
}

/*!
 * \brief Send size of terminal to driver
 */
bool pty_resize(term_t *term, pty_t *pty) {
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

/*!
 * \brief Writes new key data to PTY from terminal
 */
void term_pty_write(pty_t *pty, XKeyEvent *ev) {
    char buf[32] = {};
    KeySym ksym = 0;
    size_t num = (size_t) XLookupString(ev, buf, sizeof(buf), &ksym, 0);

    //printf("Write:\n");
    //for (int i = 0; i < num; i++) {
    //    printf("%d ", buf[i]);
    //}
    //printf("N: %d\n", num);

    if (num > 0) {
        if (write(pty->fd_master, buf, num) == -1) {
            perror("write");
            return;
        }
    }
}

/*!
 * \brief Read from PTY new data and draw it on the screen 
 */
bool term_pty_read(term_t *term, pty_t *pty) {
    char buf_read[READ_BUFFER_SIZE];
    ssize_t n = read(pty->fd_master, buf_read, sizeof(buf_read));

    // EOF indicates that the slave has closed.
    if (n <= 0) {
        return false;
    }
    if (n > 0) {
        term_output(term, buf_read, n);
    }
    char buf[1] = {};
    XSetForeground(term->display, term->graphics_context, term->color_fg);
    for (int y = 0; y < term->buffer_height; y++) {
        for (int x = 0; x < term->buffer_width; x++) {
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
    return true;
}

/*!
 * \brief Destroys terminal
 */
bool term_destroy(term_t *term, pty_t *pty) {
    // Cleanup resources
    XFreeGC(term->display, term->graphics_context);
    XFreeFont(term->display, term->font);
    XUnmapWindow(term->display, term->window);
    XDestroyWindow(term->display, term->window);
    XCloseDisplay(term->display);
    free(term->buffer);
    return true;
}

/*!
 * \brief Catch error with destroying
 */
void term_catch_error(term_t *term, pty_t *pty) {
    term_destroy(term, pty);
    exit(1);
}

/*!
 * \brief Main loop of terminal & pty life
 */
bool run(term_t *term, pty_t *pty) {
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
                if (XFilterEvent(&event, None))
                    continue;
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
                    case ConfigureNotify:
                        term_resize(term, pty, &event);
                        break;
                    // Redraw the terminal content
                    case Expose:
                        term_draw(term);
                        break;
                    // Pass new key to shell
                    case KeyPress:
                        term_pty_write(pty, &event.xkey);
                        break;
                    default:
                        break;
                }
            }
        }
        // PTY Master activity
        if (FD_ISSET(pty->fd_master, &readable)) {
            if (!term_pty_read(term, pty))
                running = false;
            term_draw(term);
        }
    }
    return true;
}