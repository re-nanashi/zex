/**
 * @file config.h
 * @author re-nanashi
 * @brief Editor state configurations
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <termios.h>
#include <time.h>

/* @brief A row in the editor */
typedef struct editor_row {
    /* text buffer */
    char *chars;
    /* size of the char buffer */
    size_t size;
    /* size of the content before cursor */
    size_t front;
    /* size of the gap buffer */
    size_t gap;
    /* text buffer to render to the screen */
    char *render;
    /* size of the render buffer */
    size_t rsize;
} editor_row_T;

/* @brief Editor states */
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_VISUAL,
    MODE_REPLACE
} Mode;

/* @brief Editor global config */
typedef struct editor_config {
    /* cursor coordinates */
    int cx, cy;
    /* cursor x coordinate for render */
    int rx;
    /* vertical scroll row offset */
    int row_offset;
    /* horizontal scroll col offset */
    int col_offset;
    /* terminal number of rows */
    int screenrows;
    /* terminal number of cols */
    int screencols;
    /* opened file num of rows */
    size_t line_count;
    /* array of all rows with text */
    editor_row_T *rows;
    /* editor mode */
    Mode mode;
    /* changes counter */
    int dirty;
    /* filename str */
    char *filename;
    /* status message str */
    char statusmsg[80];
    /* status message timeout time */
    time_t statusmsg_time;
    /* original termios config */
    struct termios orig_termios;
} editor_config_T;

/* @brief Global config declaration */
extern editor_config_T econfig;

#endif /* CONFIG_H */
