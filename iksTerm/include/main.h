#ifndef MAIN_H
#define MAIN_H

/**
 * @brief Defines the default shell path.
 */
#define SHELL_PATH "/bin/sh"
/**
 * @brief Defines the default shell name.
 */
#define SHELL_NAME "sh"
/**
 * @brief Defines the default color of foreground.
 */
#define COLOR_FG "#ffffff"
/**
 * @brief Defines the default color of background.
 */
#define COLOR_BG "#000000"
/**
 * @brief Defines the default color of cursor.
 */
#define COLOR_CURSOR "#777777"
/**
 * @brief Defines the default font.
 */
#define DEFAULT_FONT "fixed"
/**
 * @brief Defines the default terminal name.
 */
#define TERM_NAME "iksTerm"
/**
 * @brief Defines the default width.
 */
#define DEFAULT_WIDTH 120
/**
 * @brief Defines the default height.
 */
#define DEFAULT_HEIGHT 60
/**
 * @brief Defines the default size of buffer.
 */
#define READ_BUFFER_SIZE 4096
/**
 * @brief Defines the default tab size.
 */
#define TAB_SIZE 4
/**
 * @brief Defines the default history size.
 */
#define HISTORY_SIZE 10

// Basic ASCII printable range (space to tilde)
#define IS_PRINTABLE_ASCII(c) ((c) >= 0x20 && (c) <= 0x7E)

// Unicode printable ranges (simplified)
#define IS_PRINTABLE_UNICODE(c) ((c) >= 0xA0 && (c) <= 0x10FFFF)

/**
 * @brief Defines the MIN MAX macros.
 */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#endif