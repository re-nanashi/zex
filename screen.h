/**
 * @file screen.h
 * @author re-nanashi
 * @brief Function definitions responsible for managing terminal display output
 */

#ifndef SCREEN_H
#define SCREEN_H

/* @brief Internal macros */
#define ZEX_VERSION "0.0.1"

/* @brief Append buffer */
typedef struct append_buf {
    /* char string */
    char *b; /* length of the string */
    int len;
} append_buf_T;

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
void write_to_abuf(append_buf_T *ab, const char *str, int len);

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param ab Pointer to append buffer
 */
void free_abuf(append_buf_T *ab);

/* @brief Handle x and y scroll */
void screen_scroll_handler();

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param ab Pointer to append buffer
 * @param format String format
 * @param ... arguments
 */
void screen_draw_welcome_mes(struct append_buf *ab, const char *format, ...);

/**
 * @brief Draw all rows/line to append buffer
 *
 * @param ab Pointer to append buffer
 */
void screen_draw_rows(struct append_buf *ab);

/**
 * @brief Draw status text to append buffer
 *
 * @param ab Pointer to append buffer
 */
void screen_draw_status_bar(struct append_buf *ab);

/**
 * @brief Draw command line to append buffer
 *
 * @param ab Pointer to append buffer
 */
void screen_draw_cmd_line(struct append_buf *ab);

/**
 * @brief Set status message to be displayed in the command line
 *
 * @param format String format
 * @param ... arguments
 */
void statusbar_set_message(const char *format, ...);

/**
 * @brief Refresh terminal screen to draw editor
 *
 * This function draws the editor's display output to the terminal.
 */
void screen_refresh();

/**
 * @brief Refresh screen wrapper to be called on separate thread
 *
 * Will be passed to pthread_create to handle terminal dimension changes
 */
void *thread_screen_refresh();

#endif /* SCREEN_H */
