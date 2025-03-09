#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "term.h"
#include "term_pty.h"
#include "util.h"

int main(int argc, char **argv) {
    term_t term = {};
    pty_t pty = {};

    // Get command line options
    get_options(&term, &pty, argc, argv);

    // Init X11 window
    if (!term_init(&term))
        return 1;
    // Create PTY
    if (!pty_new(&pty))
        return 1;
    if (!pty_resize(&term, &pty))
        return 1;
    // Process channel between X-Server and shell
    if (!run(&term, &pty))
        return 1;
    // Destroy X11 window
    if (!term_destroy(&term))
        return 1;
    return 0;
}
