#ifndef TERM_PTY_H
#define TERM_PTY_H

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

bool pty_new(pty_t *pty);
bool term_resize(term_t *term, pty_t *pty, XEvent *event);
bool pty_resize(term_t *term, pty_t *pty);
void term_pty_write(pty_t *pty, XKeyEvent *ev);
void term_pty_read(term_t *term, pty_t *pty);
bool run(term_t *term, pty_t *pty);

#endif