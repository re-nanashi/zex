/**
 * @file editor_config.h
 * @author re-nanashi
 * @brief Editor state configurations
 */

#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>

/* @brief A row in the editor */
typedef struct editor_row {
    /* size of the char buffer */
    int size;
    /* size of the render buffer */
    int rsize;
    /* text buffer */
    char *chars;
    /* text buffer to render to the screen */
    char *render;
} erow_t;

/* @brief Editor global config */
typedef struct editor_config {
    /* cursor coordinates */
    int cx, cy;
    /* cursor horizontal coordinates for render */
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
    int numrows;
    /* array of all rows with text */
    erow_t *rows;
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
} econf_t;

#endif /* EDITOR_CONFIG_H */
