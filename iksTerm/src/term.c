#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>

#include <X11/Xutil.h>
#include <X11/Xlib.h>

#include <main.h>
#include <term.h>
#include <term_pty.h>
#include <util.h>

static bool just_wrapped = false; // flag to avoid double moving next line

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
        .event_mask = FocusChangeMask | KeyPressMask | KeyReleaseMask
		| ExposureMask | VisibilityChangeMask | StructureNotifyMask
		| ButtonMotionMask | ButtonPressMask | ButtonReleaseMask | ClientMessage,
    };
    // Set colors
    term_set_color(term);
    // Load font
    term_set_font(term);
    // Load buffer
    if (!term_set_buffer(term))
        return false;
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
    // Set resizing with hint
    term->hints.flags = PBaseSize | PResizeInc;
    term->hints.base_width =  term->font_width;    // Базовая ширина окна
    term->hints.base_height =  term->font_height;  // Базовая высота окна
    term->hints.width_inc =  term->font_width;     // Шаг изменения ширины (по горизонтали)
    term->hints.height_inc =  term->font_height;   // Шаг изменения высоты (по вертикали)
    XSetWMNormalHints(term->display, term->window, &term->hints);
    // Map window
    XStoreName(term->display, term->window, TERM_NAME);
    XMapWindow(term->display, term->window);
    term->graphics_context = XCreateGC(term->display, term->window, 0, NULL);

    XFlush(term->display);

    return true;
}

bool term_move_buffer(term_t *term, int new_buffer_width, int new_buffer_height) {
    char* new_buffer = calloc((long unsigned int) new_buffer_width * (long unsigned int) new_buffer_height, sizeof(char));
    if (!new_buffer) {
        perror("calloc");
        return false;
    }
    //for (int current_row = 0; current_row < (term->buffer_height - new_buffer_height); current_row++) {
	//	term_scroll_buffer(term);
	//}
    int last_non_empty = 0;
    for (int i = 0; i < term->buffer_height; i++) {
        bool row_has_content = false;
        for (int j = 0; j < term->buffer_width; j++) {
            if (term->buffer[i * term->buffer_width + j] != '\0') {
                row_has_content = true;
                break;
            }
        }
        if (row_has_content) {
            last_non_empty = i;
        }
    }
    int effective_rows = last_non_empty + 1;
    int start_row = 0;
    if (new_buffer_height <= term->buffer_height) 
        start_row = (effective_rows > new_buffer_height) ? (effective_rows - new_buffer_height) : 0;
    
    int rows_to_copy = MIN(new_buffer_height, term->buffer_height - start_row);
    int min_width = MIN(new_buffer_width, term->buffer_width);

    for (int i = 0; i < rows_to_copy; i++) {
        memcpy(new_buffer + i*new_buffer_width, term->buffer + (i + start_row)*term->buffer_width, min_width*sizeof(char));
    }
    free(term->buffer);
    term->buffer = new_buffer;
    term->buffer_width = new_buffer_width;
    term->buffer_height = new_buffer_height;  // Update this only if you're changing the total rows count.
    if (term->buffer_x >= new_buffer_width) { 
        term->buffer_x = 0;
        term->buffer_y = term->buffer_y + 1;
    }
    if (term->buffer_y >= new_buffer_height) { 
        term->buffer_y = new_buffer_height-1;
    }
    return true;
}

/*!
 * \brief Draw buffer on terminal
 */
void term_draw(term_t *term) {
    XSetForeground(term->display, term->graphics_context, term->color_bg);
    XFillRectangle(term->display, term->window, term->graphics_context, 0, 0, term->width, term->height);
    char ch = 0;
    char *buf = &ch;
    XSetForeground(term->display, term->graphics_context, term->color_fg);
    for (int y = 0; y < term->buffer_height; y++) {
        for (int x = 0; x < term->buffer_width; x++) {
            *buf = term->buffer[y * term->buffer_width + x];
            printf("%d ", *buf);
            // Filter non-printables
            if (!IS_PRINTABLE_ASCII(*buf) /*&& !IS_PRINTABLE_UNICODE(buf[0])*/) {
                continue; // Unicode replacement character <unknown>
            }
            XDrawString(term->display,
                        term->window,
                        term->graphics_context,
                         (x * term->font_width),
                         (y * term->font_height) + term->font->ascent + term->font->descent,
                        buf,
                        1);
        }
        printf("\n");
    }

    XSetForeground(term->display, term->graphics_context, term->color_cursor);
    XFillRectangle(term->display,
                   term->window,
                   term->graphics_context,
                   term->buffer_x * term->font_width,
                   term->buffer_y * term->font_height,
                   term->font_width,
                   term->font_height + term->font->descent);

    XFlush(term->display);
}

/*!
 * \brief Scroll terminal for one line
 */
void term_scroll_buffer(term_t *term) {
    memmove(term->buffer, &term->buffer[term->buffer_width], term->buffer_width * (term->buffer_height - 1));
    term->buffer_y = term->buffer_height - 1;
    for (int i = 0; i < term->buffer_width; i++)
        term->buffer[term->buffer_y * term->buffer_width + i] = 0;
}

/*!
 * \brief Process data from PTY and changes buffer
 */
void term_output(term_t *term, char *buf, ssize_t n) {
    // Carriage Return: Move to start of line
    printf("Read:\n");
    for (ssize_t i = 0; i < n; i++) {
        printf("%d ", buf[i]);
        switch (buf[i]) {
            case '\r': /* CR */
                term->buffer_x = 0; 
                break;
            case '\t': { /* HT */
                int j = 0;
                for (; (j < TAB_SIZE) && (term->buffer_x < term->buffer_width); j++) {
                    term->buffer[term->buffer_y * term->buffer_width + term->buffer_x] = ' '; 
                    term->buffer_x++;
                }
                if (j != TAB_SIZE) {
                    term->buffer_x = 0;
                    term->buffer_y++;
                    for (; j < TAB_SIZE; j++) {
                        term->buffer[term->buffer_y * term->buffer_width + term->buffer_x] = ' '; 
                        term->buffer_x++;
                    }
                }
            }
                break;
            case '\b': { /* BR*/
                if ((term->buffer_x == term->buffer_prompt_x) && (term->buffer_y == term->buffer_prompt_y))
                    break;
                term->buffer[term->buffer_y * term->buffer_width + term->buffer_x] = '\0'; 
                term->buffer_x--;
                if (term->buffer_x < 0) {
                    term->buffer_x = term->buffer_width - 1;
                    if (term->buffer_y != 0)   
                        term->buffer_y--;
                }
                term->buffer[term->buffer_y * term->buffer_width + term->buffer_x] = '\0'; 
            }
                break;    
            case '\f':   /* LF */
	        case '\v':   /* VT */
            case '\n': /* LF */
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
    printf("\n");
}

/*!
 * \brief Destroys terminal
 */
bool term_destroy(term_t *term) {
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
 * \brief Destroys terminal and exit program
 */
void term_catch_error(term_t *term) {
    term_destroy(term);
    exit(1);
}

void term_set_color(term_t *term) {
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
}

void term_set_font(term_t *term) {
    if (!term->font_name)
        term->font_name = DEFAULT_FONT;
    term->font = XLoadQueryFont(term->display, term->font_name);
    if (!term->font) {
        fprintf(stderr, "Can't load font \"%s\"! Switch to default \"" DEFAULT_FONT "\"\n", term->font_name);
        term->font_name = DEFAULT_FONT;
        term->font = XLoadQueryFont(term->display, term->font_name);
    }
    // Get font characters width and height
    term->font_width = term->font->max_bounds.width;
    term->font_height = term->font->ascent + term->font->descent * 2;// Total vertical space
}

bool term_set_buffer(term_t *term) {
    // Have max width/height of screen
    int max_width =  DisplayWidth(term->display, term->screen) / term->font_width;
    int max_height =  DisplayHeight(term->display, term->screen) / term->font_height;
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
    return true;
}