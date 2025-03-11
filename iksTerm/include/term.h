#ifndef TERM_H
#define TERM_H

/*!
 * @struct term_t
 * @brief Keep all graphical and buffer information about X11 terminal
 */
typedef struct term_t {
    // X11 components
    Display *display;   ///< Display
    int screen;         ///< Screen index
    Window root, window;///< Windows
    int fd;             ///< File descriptor of terminal
    GC graphics_context;///< Graphics context
    Atom wm_delete;     ///< Atom for deleting
    XSizeHints hints;   ///< Hint to custom resizing

    // Color
    char *hex_color_fg;                                ///< Name of fg color
    char *hex_color_bg;                                ///< Name of bg color
    char *hex_color_cursor;                            ///< Name of cursor color
    unsigned long int color_fg, color_bg, color_cursor;///< Number of allocated colors

    // Font
    char *font_name;            ///< Font name
    XFontStruct *font;          ///< Font structure
    int font_width, font_height;///< Font maximum sizes

    // Buffer
    char *buffer;                        ///< Pointer to window buffer
    int buffer_x, buffer_y;              ///< Cursor position (x,y)
    int buffer_prompt_x, buffer_prompt_y;///< Prompt begin position

    int buffer_width, buffer_height;///< Size of window in cols and rows
    int width, height;              ///< Size of window in pixels
} term_t;

bool term_init(term_t *term);
void term_draw(term_t *term);
void term_scroll_buffer(term_t *term);
void term_output(term_t *term, char *buf, ssize_t n);
void term_set_color(term_t *term);
void term_set_font(term_t *term);
bool term_set_buffer(term_t *term);
bool term_move_buffer(term_t *term, int new_buffer_width, int new_buffer_height);
ssize_t term_parse_esc(term_t *term, char *buf);
void handle_cursor_home(term_t *term);
void handle_clear_screen(term_t *term);

#endif