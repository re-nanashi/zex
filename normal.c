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
#include "edit.h"
#include "state.h"

#define DIFF_CHAR_TYPE(c1, c2)                                                 \
    ((isalnum(c1) && !isalnum(c2)) || (ispunct(c1) && !ispunct(c2)))

enum Directions { FORWARD = 1, BACKWARD = -1 };

editor_row_T *
get_current_row()
{
    return (econfig.cy >= econfig.line_count) ? NULL
                                              : &econfig.rows[econfig.cy];
}

void
jump_to_next_non_blank_char(editor_row_T *row, colnr_T *cx)
{
    while (!isblank(row->render[*cx]) && *cx < row->size)
        (*cx)++;
}

void
jump_to_next_blank_char(editor_row_T *row, colnr_T *cx)
{
    while (isblank(row->render[*cx]) && *cx < row->size)
        (*cx)++;
}

void
skip_successive_alnum_or_punct_chars(editor_row_T *row, colnr_T *cx)
{
    if (isalnum(row->render[*cx])) {
        while (isalnum(row->render[*cx]))
            (*cx)++;
    }
    else if (ispunct(row->render[*cx])) {
        while (ispunct(row->render[*cx]))
            (*cx)++;
    }
}

void
skip_blank_chars(editor_row_T *row, colnr_T *cx)
{
    while (isblank(row->render[*cx]))
        (*cx)++;
}

void
jump_to_end_of_current_word(editor_row_T *row, colnr_T *cx)
{
    if (!isblank(row->render[*cx]) && isblank(row->render[*cx + 1])) {
        (*cx)++;
        skip_blank_chars(row, cx);
        while (!isblank(row->render[*cx])
               && (isalnum(row->render[*cx]) || ispunct(row->render[*cx])))
            (*cx)++;
        (*cx)--;
    }
    else if (isblank(row->render[*cx]) && *cx < row->size) {
        skip_blank_chars(row, cx);
    }
    else if (*cx >= row->size - 1 && *cx > 0) {
        (*cx)++;
    }
    else {
        while (!isblank(row->render[*cx])
               && (isalnum(row->render[*cx]) || ispunct(row->render[*cx])))
            (*cx)++;
        (*cx)--;
    }
}

void
jump_to_next_word(editor_row_T *row, colnr_T *cx)
{
    if ((isalnum(row->render[*cx]) && !isalnum(row->render[*cx + 1]))
        || (ispunct(row->render[*cx]) && !ispunct(row->render[*cx + 1])))
    {
        (*cx)++;
    }

    skip_successive_alnum_or_punct_chars(row, cx);
    skip_blank_chars(row, cx);
    skip_successive_alnum_or_punct_chars(row, cx);
    (*cx)--;
}

void
handle_end_of_line(editor_row_T *row, colnr_T *cx)
{
    if (econfig.cy == econfig.line_count - 1 && (*cx) == row->size) {
        (*cx)--;
    }
}

void
move_to_next_line_if_needed(editor_row_T *row, colnr_T *cx)
{
    if (*cx == row->size || (row->size == 0 && *cx == 0)) {
        row = (econfig.cy == econfig.line_count - 1)
                  ? NULL
                  : &econfig.rows[++econfig.cy];
        if (row) {
            (*cx) = 0;
            skip_blank_chars(row, cx);
        }
    }
}

void
jump_to_end_of_first_word(editor_row_T *row, colnr_T *cx)
{
    if (isalnum(row->render[*cx])) {
        while (isalnum(row->render[*cx]))
            (*cx)++;
        (*cx)--;
    }
    else if (ispunct(row->render[*cx])) {
        while (ispunct(row->render[*cx]))
            (*cx)++;
        (*cx)--;
    }
}

void
fwd_word(bool flag)
{
    editor_row_T *row = get_current_row();
    colnr_T *cx = &econfig.cx;

    if (flag == true) {
        jump_to_next_non_blank_char(row, cx);
        jump_to_next_blank_char(row, cx);
    }
    else {
        skip_successive_alnum_or_punct_chars(row, cx);
        skip_blank_chars(row, cx);
    }

    handle_end_of_line(row, cx);
    move_to_next_line_if_needed(row, cx);
}

void
end_word(bool flag)
{
    editor_row_T *row = get_current_row();
    colnr_T *cx = &econfig.cx;

    if (flag == true) {
        jump_to_end_of_current_word(row, cx);
    }
    else {
        jump_to_next_word(row, cx);
    }

    handle_end_of_line(row, cx);
    move_to_next_line_if_needed(row, cx);
    jump_to_end_of_first_word(row, cx);
}

void
nv_wordcmd(const cmdarg_T *ca)
{
    bool word_end = false;
    bool flag = ca->arg; // flag for shift mod; if true then shift is pressed

    // Jump cursor to the either first char or last char of the next word
    // Set true if cursor will jump to the end of a word
    if (ca->cmdchar == 'e' || ca->cmdchar == 'E') {
        word_end = true;
    }
    // if e/E go to end of word else if w/W go to start of word
    if (word_end) {
        end_word(flag);
    }
    else {
        // w/W
        fwd_word(flag);
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
                statusbar_set_message("-- REPLACE --");
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
            statusbar_set_message("");
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
    statusbar_set_message("%c", c);
    screen_refresh();

    int k = input_read_key();
    if (k > 0x1f && k < 0x7f) {
        // Offset cursor x pos by 1 to search next instance of char incase
        // the char to find is the same char as the one the under cursor
        if (row)
            sstatus == SHIFT_NOT_PRESSED ? (*cx)++ : (*cx)--;
        else {
            statusbar_set_message(""); // remove status
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
        statusbar_set_message("");
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
            nv_wordcmd(SHIFT_NOT_PRESSED);
            break;

        // Jump by words
        case 'W':
            nv_wordcmd(SHIFT);
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
            editor_delete_char();
            break;

        case CTRL_KEY('['):
            break;

        // Temp default
        default:
            // Insert character to line/row
            // op_editor_insert_ch(c);
            break;
    }
}
