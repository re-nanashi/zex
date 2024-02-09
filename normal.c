#include "normal.h"

#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "screen.h"

void
cursor_jump_forward_start_word(sflag_t sflag)
{
    // Check if there is a text
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    int *cx = &econfig.cx;

    switch (sflag) {
        case SHIFT:
            // Go to the next space/blank ch
            while (!isblank(row->render[*cx]) && *cx < row->size)
                (*cx)++;

            // Go to the next non-space/blank ch
            while (isblank(row->render[*cx]) && *cx < row->size) {
                (*cx)++;
            }

            break;

        case UNSHIFT:
            // Skip succeeding alnum ch if 'w' is pressed while ch under cursor
            // is alnum
            if (isalnum(row->render[*cx])) {
                while (isalnum(row->render[*cx]))
                    (*cx)++;
            }
            // Skip succeeding punct ch if 'w' is pressed while ch under cursor
            // is punct
            else if (ispunct(row->render[*cx])) {
                while (ispunct(row->render[*cx]))
                    (*cx)++;
            }

            // Skip blank characters
            if (isblank(row->render[*cx])) {
                while (isblank(row->render[*cx]))
                    (*cx)++;
            }

            break;
    }

    // Stop cursor from moving forward if last row
    if (econfig.cy == econfig.line_count - 1 && (*cx) == row->size) {
        (*cx)--;
        return;
    }

    // Move to cursor to the next line if end of line
    if (*cx == row->size || (row->size == 0 && *cx == 0)) {
        row = (econfig.cy == econfig.line_count - 1)
                  ? NULL
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
replace_char_at_cur(sflag_t sflag)
{
    char c;

    switch (sflag) {
        // TODO: Can't replace last character and insert new character
        case SHIFT: {
            while (c != CTRL_KEY('[')) {
                // Show mode status
                sbar_set_status_message("-- REPLACE --");
                screen_refresh();

                c = input_read_key();
                if (c > 0x1f && c < 0x7f) {
                    // Get current row where cursor is at
                    editor_row_T *row = (econfig.cy >= econfig.line_count)
                                            ? NULL
                                            : &econfig.rows[econfig.cy];

                    if (econfig.cx < row->size) row->render[econfig.cx++] = c;
                }
            }
            // Remove mode status
            sbar_set_status_message("");
        } break;

        case UNSHIFT: {
            c = input_read_key();
            if (c > 0x1f && c < 0x7f) {
                // Get current row where cursor is at
                editor_row_T *row = (econfig.cy >= econfig.line_count)
                                        ? NULL
                                        : &econfig.rows[econfig.cy];
                row->render[econfig.cx] = c;
            }
        } break;
    }
}

void
cursor_jump_forward_end_word(sflag_t sflag)
{
    // Check if there is a text
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    int *cx = &econfig.cx;

    switch (sflag) {
        case SHIFT:
            if (!isblank(row->render[*cx]) && isblank(row->render[*cx + 1])) {
                // Go to the blank char
                (*cx)++;

                // Skip blanks chars; Assume: will end up at the first ch of
                // word OR end up at the last ch of the row
                while (isblank(row->render[*cx]) && *cx < row->size)
                    (*cx)++;

                // Go to the end of the word
                while (
                    !isblank(row->render[*cx])
                    && (isalnum(row->render[*cx]) || ispunct(row->render[*cx])))
                    (*cx)++;
                (*cx)--;
            }
            else if (isblank(row->render[*cx]) && *cx < row->size) {
                while (isblank(row->render[*cx]))
                    (*cx)++; // will probably end up either the start of the
                             // word or at '/n'
            }
            // If we are at the end of the line
            else if (*cx >= row->size - 1) {
                // We are not in a blank line
                if (*cx > 0) (*cx)++;
            }
            else {
                // Go to the end of the word
                while (
                    !isblank(row->render[*cx])
                    && (isalnum(row->render[*cx]) || ispunct(row->render[*cx])))
                    (*cx)++;
                (*cx)--;
            }

            break;

        case UNSHIFT:
            // Check if next ch is different type than the current ch
            if ((isalnum(row->render[*cx]) && !isalnum(row->render[*cx + 1]))
                || (ispunct(row->render[*cx])
                    && !ispunct(row->render[*cx + 1])))
            {
                // Go to the next word
                (*cx)++;
            }

            // Go to the end of the word
            if (isalnum(row->render[*cx])) {
                while (isalnum(row->render[*cx]))
                    (*cx)++;
                (*cx)--;
            }
            // Go to the end of the word
            else if (ispunct(row->render[*cx])) {
                while (ispunct(row->render[*cx]))
                    (*cx)++;
                (*cx)--;
            }

            // Skip blank characters then go to the end of the next word
            if (isblank(row->render[*cx])) {
                while (isblank(row->render[*cx]))
                    (*cx)++;

                if (isalnum(row->render[*cx])) {
                    while (isalnum(row->render[*cx]))
                        (*cx)++;
                    (*cx)--;
                }

                if (ispunct(row->render[*cx])) {
                    while (ispunct(row->render[*cx]))
                        (*cx)++;
                    (*cx)--;
                }
            }

            break;
    }

    // Stop cursor from moving forward if last row
    if (econfig.cy == econfig.line_count - 1 && (*cx) == row->size) {
        (*cx)--;
        return;
    }

    // Move to cursor to the next line if end of line
    if (*cx == row->size || (row->size == 0 && *cx == 0)) {
        row = (econfig.cy == econfig.line_count - 1)
                  ? NULL
                  : &econfig.rows[++econfig.cy];
        if (row) {
            (*cx) = 0;
            // Find the first non-blank character
            while (isblank(row->render[*cx]))
                (*cx)++;

            // Go to the end of the first non-blank string
            if (isalnum(row->render[*cx])) {
                while (isalnum(row->render[*cx]))
                    (*cx)++;
                (*cx)--;
            }

            if (ispunct(row->render[*cx])) {
                while (ispunct(row->render[*cx]))
                    (*cx)++;
                (*cx)--;
            }
        }
    }
}

void
nvrc_process_key(int c)
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
            input_move_cursor(key);
        } break;

        // Move cursor to the start of the line
        case '0':
            econfig.cx = 0;
            break;

        // Jump by start of words (including punctuation)
        case 'w':
            cursor_jump_forward_start_word(UNSHIFT);
            break;

        // Jump by words
        case 'W':
            cursor_jump_forward_start_word(SHIFT);
            break;

        // Jump by end of words (including punctuation)
        case 'e':
            cursor_jump_forward_end_word(UNSHIFT);
            break;

        // Jump by end of words
        case 'E':
            cursor_jump_forward_end_word(SHIFT);
            break;

        // Replace character under cursor
        case 'r':
            replace_char_at_cur(UNSHIFT);
            break;
        case 'R':
            replace_char_at_cur(SHIFT);
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
