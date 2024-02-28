/**
 * @file normal.c
 * @author re-nanashi
 * @brief Functions that handle when on Normal Mode
 */

#include "normal.h"

#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>

#include "config.h"
#include "input.h"
#include "screen.h"
#include "operations.h"

// TODO: Work on normal mode behavior
void
cursor_jump_forward_start_word(shift_status_T sstatus)
{
    // Check if there is a text
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    colnr_T *cx = &econfig.cx;

    switch (sstatus) {
        case SHIFT:
            // Go to the next space/blank ch
            while (!isblank(row->render[*cx]) && *cx < row->size)
                (*cx)++;
            // Go to the next non-space/blank ch
            while (isblank(row->render[*cx]) && *cx < row->size) {
                (*cx)++;
            }
            break;

        case SHIFT_NOT_PRESSED:
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
cursor_jump_forward_end_word(shift_status_T sstatus)
{
    // Check if there is a text
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    colnr_T *cx = &econfig.cx;

    switch (sstatus) {
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

        case SHIFT_NOT_PRESSED:
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
replace_char_at_cur(shift_status_T sstatus)
{
    char c;

    switch (sstatus) {
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

        case SHIFT_NOT_PRESSED: {
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
jump_to_char(int c, shift_status_T sstatus)
{
    editor_row_T *row =
        (econfig.cy >= econfig.line_count) ? NULL : &econfig.rows[econfig.cy];
    colnr_T *cx = &econfig.cx, prev_pos = econfig.cx;
    bool goto_char = false;
    if (c == 'f' || c == 'F') goto_char = true;

    // Inform user of key pressed
    sbar_set_status_message("%c", c);
    screen_refresh();

    int k = input_read_key();
    if (k > 0x1f && k < 0x7f) {
        // Offset cursor x pos by 1 to search next instance of char incase
        // the char to find is the same char as the one the under cursor
        if (row)
            sstatus == SHIFT_NOT_PRESSED ? (*cx)++ : (*cx)--;
        else {
            sbar_set_status_message(""); // remove status
            return;
        }

        // Find next instance of char in the string
        switch (sstatus) {
            case SHIFT:
                while (row->render[*cx] != k && *cx != 0)
                    (*cx)--;
                break;
            case SHIFT_NOT_PRESSED:
                while (row->render[*cx] != k && *cx < row->size - 1)
                    (*cx)++;
                break;
        }

        if (row->render[*cx] == k) {
            if (!goto_char) sstatus != SHIFT ? (*cx)-- : (*cx)++;
        }
        else {
            (*cx) = prev_pos;
        }

        // Remove status after
        sbar_set_status_message("");
    }
}

void
nv_process_key(int c)
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
            cursor_jump_forward_start_word(SHIFT_NOT_PRESSED);
            break;

        // Jump by words
        case 'W':
            cursor_jump_forward_start_word(SHIFT);
            break;

        // Jump by end of words (including punctuation)
        case 'e':
            cursor_jump_forward_end_word(SHIFT_NOT_PRESSED);
            break;

        // Jump by end of words
        case 'E':
            cursor_jump_forward_end_word(SHIFT);
            break;

        // Replace character under cursor
        case 'r':
            replace_char_at_cur(SHIFT_NOT_PRESSED);
            break;
        case 'R':
            replace_char_at_cur(SHIFT);
            break;

        // Jump to character
        // before char
        case 't':
            jump_to_char(c, SHIFT_NOT_PRESSED);
            break;

        case 'T':
            jump_to_char(c, SHIFT);
            break;

        // at char
        case 'f':
            jump_to_char(c, SHIFT_NOT_PRESSED);
            break;

        case 'F':
            jump_to_char(c, SHIFT);
            break;

        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J",
                  4); // clears the screen; check VT100
            write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top
            exit(0);
            break;

        // Handle delete keys
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) input_move_cursor(ARROW_RIGHT);
            op_editor_del_ch();
            break;

        // Temp default
        default:
            // Insert character to line/row
            op_editor_insert_ch(c);
            break;
    }
}
