/**
 * @file output.h
 * @author re-nanashi
 * @brief Functions responsible for managing terminal display output
 */

#ifndef OUTPUT_H
#define OUTPUT_H

/* @brief Internal macros */
#define ZEX_VERSION "0.0.1"
#define IS_ZERO(x)  (x == 0)

/* @brief Append buffer */
typedef struct append_buf {
    /* char string */
    char *b;
    /* length of the string */
    int len;
} a_buf;

/* @brief Default value/initialization for append buffer */
#define ABUF_INIT                                                              \
    {                                                                          \
        NULL, 0                                                                \
    }

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param ab Pointer to append buffer
 * @param s Char string
 * @param len Char string length
 */
void write_to_abuf(a_buf *ab, const char *s, int len);

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param ab Pointer to append buffer
 */
void abuf_free(a_buf *ab);

/* @brief Handle x and y scroll */
void editor_scroll_handler();

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param ab Pointer to append buffer
 * @param format String format
 * @param ... arguments
 */
void editor_draw_welcome_mes(struct append_buf *ab, const char *format, ...);

/**
 * @brief Draw all rows/line to append buffer
 *
 * @param ab Pointer to append buffer
 */
void editor_draw_rows(struct append_buf *ab);

/**
 * @brief Draw status text to append buffer
 *
 * @param ab Pointer to append buffer
 */
void editor_draw_status_bar(struct append_buf *ab);

/**
 * @brief Draw command line to append buffer
 *
 * @param ab Pointer to append buffer
 */
void editor_draw_cmd_line(struct append_buf *ab);

/**
 * @brief Set status message to be displayed in the command line
 *
 * @param format String format
 * @param ... arguments
 */
void editor_set_status_message(const char *format, ...);

/**
 * @brief Refresh terminal screen to draw editor
 *
 * This function draws the editor's display output to the terminal.
 */
void editor_refresh_screen();

/**
 * @brief Refresh screen wrapper to be called on separate thread
 *
 * Will be passed to pthread_create to handle terminal dimension changes
 */
void *thread_refresh_screen();

#endif /* OUTPUT_H */
