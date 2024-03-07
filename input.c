/**
 * @file input.c
 * @author re-nanashi
 * @brief Functions that handle user keyboard input
 */

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include "input.h"

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "screen.h"
#include "logger.h"
#include "config.h"
#include "operations.h"
#include "file_io.h"
#include "normal.h"

/* @brief Internal macros */
#define ZEX_QUIT_TIMES 2

int
input_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    // Handle not so ordinary keys; handle keys that emits escape codes
    // Refer to VT100
    if (c == '\x1b') {
        char seq[3];

        // Immediately read two more bytes into seq buffer if both reads
        // timeout; user just pressed Escape
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                // if there is no '~' in the sequence
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            }
            else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        }
        else if (seq[0] == '0') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }

        return '\x1b';
    }
    else {
        return c;
    }
}

char *
get_user_input_prompt(char *prompt)
{
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        sbar_set_status_message(prompt, buf);
        screen_refresh();

        int c = input_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        }
        else if (c == '\x1b') {
            sbar_set_status_message("");
            free(buf);
            return (NULL);
        }
        else if (c == '\r') {
            if (buflen != 0) {
                sbar_set_status_message("");
                return buf;
            }
        }
        else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

void
input_move_cursor(int key)
{
    // Check if there is text
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];

    switch (key) {
        case ARROW_LEFT:
            if (row && econfig.cx != 0) econfig.cx--;
            break;
        case ARROW_RIGHT: {
            colnr_T rsize = row->size - row->gap;
            if (row && econfig.cx < rsize - 1) econfig.cx++;
        } break;
        case ARROW_UP:
            if (row && econfig.cy < econfig.line_count && econfig.cy != 0)
                econfig.cy--;
            break;
        case ARROW_DOWN:
            if (row && econfig.cy < econfig.line_count - 1) econfig.cy++;
            break;
    }

    row = (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    colnr_T rowlen = row ? (row->size - row->gap) : 0;
    if (econfig.cx >= rowlen) {
        // If row length is zero, put cx at the start of line (cx = 0)
        // Else, put cursor at the end character
        econfig.cx = rowlen == 0 ? rowlen : rowlen - 1;
    }
}
