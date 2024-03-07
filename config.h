/**
 * @file config.h
 * @author re-nanashi
 * @brief Editor state configurations
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <termios.h>
#include <time.h>

/* @brief Line number type */
typedef size_t linenr_T;

/* @brief Column number type */
typedef size_t colnr_T;

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
    MODE_VISUAL,
    MODE_INSERT,
    MODE_COMMAND,
    /* mode for operator keys: g, R, y, d, c */
    MODE_OP_PENDING,
    MODE_COUNT_PENDING
} Mode;

/* @brief Editor global config */
typedef struct editor_config {
    /* cursor x coordinate */
    colnr_T cx;
    /* cursor y coordinate */
    linenr_T cy;
    /* cursor x coordinate for render */
    colnr_T rx;
    /* horizontal scroll col offset */
    colnr_T col_offset;
    /* vertical scroll row offset */
    linenr_T row_offset;
    /* terminal number of rows */
    int screenrows;
    /* terminal number of cols */
    int screencols;
    /* opened file num of rows */
    linenr_T line_count;
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
