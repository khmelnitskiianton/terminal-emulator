#ifndef TERM_H
#define TERM_H

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
    XSizeHints hints;

    // Color
    char *hex_color_fg;
    char *hex_color_bg;
    char *hex_color_cursor;
    unsigned long int color_fg, color_bg, color_cursor;

    // Font
    char *font_name;
    XFontStruct *font;
    uint font_width, font_height;

    // Buffer
    char *buffer;
    uint buffer_x, buffer_y, buffer_width, buffer_height;
    // Actual terminal size in pixels
    uint width, height;
    
    // Array of pointers to command strings
    char **history;
} term_t;

bool term_destroy(term_t *term);
void term_catch_error(term_t *term);
bool term_init(term_t *term);
void term_draw(term_t *term);
void term_scroll_buffer(term_t *term);
void term_output(term_t *term, char *buf, ssize_t n);
void term_set_color(term_t *term);
void term_set_font(term_t *term);
bool term_set_buffer(term_t *term);
bool term_move_buffer(term_t *term, uint new_buffer_width, uint new_buffer_height);

#endif