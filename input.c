#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "input.h"
#include "output.h"
#include "logger.h"
#include "config.h"
#include "editor_operations.h"
#include "file_io.h"

/* @brief Internal macros */
#define ZEX_QUIT_TIMES 2
#define CTRL_KEY(k)    ((k)&0x1f)
enum EditorKeys {
    BACKSPACE = 127,
    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN
};

int
editor_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    // Handle not so ordinary keys
    if (c == '\x1b') {
        char seq[3];

        // Immediately read two more bytes into seq buffer
        // if both reads time out; user just pressed Escape
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
editor_prompt(char *prompt)
{
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editor_set_status_message(prompt, buf);
        editor_refresh_screen();

        int c = editor_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        }
        else if (c == '\x1b') {
            editor_set_status_message("");
            free(buf);
            return (NULL);
        }
        else if (c == '\r') {
            if (buflen != 0) {
                editor_set_status_message("");
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
editor_move_cursor(int key)
{
    // Check if there is text
    erow_t *row =
        (econfig.cy >= econfig.numrows) ? NULL : &econfig.rows[econfig.cy];

    switch (key) {
        case ARROW_LEFT:
            if (econfig.cx != 0) econfig.cx--;
            break;
        case ARROW_RIGHT:
            if (row && econfig.cx < row->size - 1) econfig.cx++;
            break;
        case ARROW_UP:
            if (econfig.cy < econfig.numrows && econfig.cy != 0) econfig.cy--;
            break;
        case ARROW_DOWN:
            if (econfig.cy < econfig.numrows - 1) econfig.cy++;
            break;
    }

    row = (econfig.cy >= econfig.numrows) ? NULL : &econfig.rows[econfig.cy];
    int rowlen = row ? row->size : 0;
    if (econfig.cx >= rowlen) {
        // If row length is zero, put cx at the start of line (cx = 0)
        // Else, put cursor at the end character
        econfig.cx = rowlen == 0 ? rowlen : rowlen - 1;
    }
}

void
editor_move_cursor_by_word()
{
    // Check if there is a text
    erow_t *row =
        (econfig.cy >= econfig.numrows) ? NULL : &econfig.rows[econfig.cy];

    int *cx = &econfig.cx;
    // Skip succeeding alnum ch if 'w' is pressed while ch under cursor is alnum
    if (isalnum(row->render[*cx])) {
        while (isalnum(row->render[*cx]))
            (*cx)++;
    }
    // Skip succeeding punct ch if 'w' is pressed while ch under cursor is punct
    else if (ispunct(row->render[*cx])) {
        while (ispunct(row->render[*cx]))
            (*cx)++;
    }

    // Skip blank characters
    if (isblank(row->render[*cx])) {
        while (isblank(row->render[*cx]))
            (*cx)++;
    }

    // Stop cursor from moving forward
    if (econfig.cy == econfig.numrows - 1 && (*cx) == row->size) {
        (*cx)--;
        return;
    }

    // Move to cursor to the next line if end of line
    if (*cx == row->size || (row->size == 0 && *cx == 0)) {
        row = (econfig.cy == econfig.numrows - 1) ? NULL
                                                  : &econfig.rows[++econfig.cy];
        if (row) {
            (*cx) = 0;
            // Find the first non-blank character
            while (isblank(row->render[*cx]))
                (*cx)++;
        }
    }
}

void
process_key_normal(int c)
{
    switch (c) {
        // Movement keys
        // Basic Movement
        case 'k':
        case 'j':
        case 'h':
        case 'l': {
            int key;
            if (c == 'k') key = ARROW_UP;
            if (c == 'j') key = ARROW_DOWN;
            if (c == 'h') key = ARROW_LEFT;
            if (c == 'l') key = ARROW_RIGHT;
            editor_move_cursor(key);
        } break;
        // Move cursor to the start of the line
        case '0':
            econfig.cx = 0;
            break;
        // Move cursor to the first character of a word
        case 'w':
            editor_move_cursor_by_word();
            break;

        // Temp default
        default:
            write(STDOUT_FILENO, "\x1b[2J",
                  4); // clears the screen; check VT100
            write(STDOUT_FILENO, "\x1b[H",
                  3); // reposition cursor to top
            exit(0);
            break;
    }
}

// TODO: Process keys according to mode
void
editor_process_keypress()
{
    static int quit_times = ZEX_QUIT_TIMES;

    int c = editor_read_key();

    switch (econfig.mode) {
        case NORMAL:
            process_key_normal(c);
            break;
        case VISUAL:
            break;
        default:
            switch (c) {
                case '\r':
                    editor_insert_nline();
                    break;

                case CTRL_KEY('q'):
                    if (econfig.dirty && quit_times > 0) {
                        editor_set_status_message(
                            "Warning: File has unsaved changes. "
                            "Press CTRL_Q %d more time to quit.",
                            quit_times);
                        quit_times--;
                        return;
                    }
                    write(STDOUT_FILENO, "\x1b[2J",
                          4); // clears the screen; check VT100
                    write(STDOUT_FILENO, "\x1b[H",
                          3); // reposition cursor to top
                    exit(0);
                    break;

                case CTRL_KEY('s'):
                    editor_save();
                    break;

                case HOME_KEY:
                    econfig.cx = 0;
                    break;

                case END_KEY:
                    if (econfig.cy < econfig.numrows) {
                        econfig.cx = econfig.rows[econfig.cy].size - 1;
                    }
                    break;

                case BACKSPACE:
                case CTRL_KEY('h'):
                case DEL_KEY:
                    if (c == DEL_KEY) editor_move_cursor(ARROW_RIGHT);
                    editor_del_ch();
                    break;

                case PAGE_UP:
                case PAGE_DOWN: {
                    if (c == PAGE_UP) {
                        econfig.cy = econfig.row_offset;
                    }
                    else if (c == PAGE_DOWN) {
                        econfig.cy =
                            econfig.row_offset + econfig.screenrows - 1;
                        if (econfig.cy > econfig.numrows)
                            econfig.cy = econfig.numrows - 1;
                    }

                    int times = econfig.screenrows;
                    while (times--) {
                        editor_move_cursor(c == PAGE_UP ? ARROW_UP
                                                        : ARROW_DOWN);
                    }
                } break;

                case ARROW_UP:
                case ARROW_DOWN:
                case ARROW_RIGHT:
                case ARROW_LEFT:
                    editor_move_cursor(c);
                    break;

                case CTRL_KEY('l'):
                case '\x1b':
                    break;

                default:
                    editor_insert_ch(c);
            }
    }

    // reset if another key is pressed
    quit_times = ZEX_QUIT_TIMES;
}
