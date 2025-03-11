#ifndef TERM_PTY_H
#define TERM_PTY_H

/*!
 * @struct pty_t
 * @brief Keep fd of master and slave in PTY
 */
typedef struct pty_t {
    // Shell
    char *shell_path;///< The shell path.
    char *shell_name;///< The shell name.
    // Master and slave file descriptors
    int fd_master;///< The master file descriptor.
    int fd_slave; ///< The slave file descriptor.
    pid_t pid;    ///< The PID of shell process.
} pty_t;

bool pty_new(pty_t *pty);
bool term_resize(term_t *term, pty_t *pty, XEvent *event);
bool pty_resize(term_t *term, pty_t *pty);
void term_pty_write(pty_t *pty, XKeyEvent *ev);
bool term_pty_read(term_t *term, pty_t *pty);
bool run(term_t *term, pty_t *pty);

bool term_destroy(term_t *term, pty_t *pty);
void term_catch_error(term_t *term, pty_t *pty);

#endif