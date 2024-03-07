/**
 * @file edit.c
 * @author re-nanashi
 * @brief Editor and row operations
 */

#include "edit.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

/* Row operations */
int
row_convert_cx_to_rx(editor_row_T *row, int cx)
{
    int rx = 0;
    // Loop through the characters in the row and replace the tab character to
    // the number of tab stop spaces declared in the header file
    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (ZEX_TAB_STOP - 1) - (rx % ZEX_TAB_STOP);
        rx++;
    }
    return rx; // return new length of row
}

void
row_update(editor_row_T *row)
{
    // Get the number of '\t' within the line
    int tabs = 0;
    size_t j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    // Initialize render buffer
    free(row->render);
    row->render = malloc(row->size + tabs * (ZEX_TAB_STOP - 1) + 1);

    // Update the row to be renderable; Renderable means the there are no more
    // tab characters and now replaced by spaces configured at the header file.
    // It also means it skips rendering the gap buffer and only renders the
    // appropriate text to the screen
    size_t i = 0;
    for (j = 0; j < row->size; j++) {
        // Skip rendering the gap
        if (j >= row->front && j < row->front + row->gap) continue;
        // Render tab as spaces until tabstop
        if (row->chars[j] == '\t') {
            row->render[i++] = ' ';
            while (i % ZEX_TAB_STOP != 0) {
                row->render[i++] = ' '; // replace tabs with spaces
            }
        }
        else {
            row->render[i++] = row->chars[j];
        }
    }
    row->render[i] = '\0';
    row->rsize = i; // rsize is the real size(number of characters) of the row
}

void
row_new(colnr_T at, char *s)
{
    // Check if there is row/line
    if (at < 0 || at > econfig.line_count) return;
    econfig.rows =
        realloc(econfig.rows, sizeof(editor_row_T) * (econfig.line_count + 1));

    // Move the current row where the cursor is at to the next row
    memmove(&econfig.rows[at + 1], &econfig.rows[at],
            sizeof(editor_row_T) * (econfig.line_count - at));

    // Init gap buffer
    rbuf_init(&econfig.rows[at]);

    // Insert string to buffer
    rbuf_insertstr(&econfig.rows[at], s);
    size_t nlen = econfig.rows[at].size;
    econfig.rows[at].chars[nlen] = '\0';

    // Update row to be renderable to screen
    econfig.rows[at].rsize = 0;
    econfig.rows[at].render = NULL;
    row_update(&econfig.rows[at]);

    // Update editor status
    econfig.line_count++;
    econfig.dirty++;
}

// Basically, a wrapper for rbuf_destroy
void
row_free(editor_row_T *row)
{
    rbuf_destroy(row);
}

void
row_delete(linenr_T at)
{
    if (at < 0 || at >= econfig.line_count) return; // no row to delete

    // Free the current row where the cursor is at then adjust the proceeding
    // rows. Put the next row to the row that was deleted.
    row_free(&econfig.rows[at]);
    memmove(&econfig.rows[at], &econfig.rows[at + 1],
            sizeof(editor_row_T) * (econfig.line_count - at - 1));

    // Update editor status
    econfig.line_count--;
    econfig.dirty++;
}

void
row_insert_char(editor_row_T *row, colnr_T at, int c)
{
    // Check if within row size
    size_t rstrlen = row->size - row->gap;
    if (at < 0 || at > rstrlen) at = rstrlen;

    // Move the front of the gap buffer to at
    ptrdiff_t offset = at - row->front;
    rbuf_move(row, offset);

    // Insert character to the front of the buffer
    rbuf_insert(row, c);
    // Update char string to render string
    row_update(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
row_append_str(editor_row_T *row, char *s)
{
    rbuf_insertstr(row, s);
    // Update char string to render string
    row_update(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
row_delete_char(editor_row_T *row, colnr_T at)
{
    size_t rstrlen = row->size - row->gap;
    if (at < 0 || at >= rstrlen) return;

    // Moves the gap infront of  row[at] then deletes the ch after the gap
    rbuf_move(row, at - row->front);
    rbuf_delete(row);

    // Update the row to be renderable
    row_update(row);
    econfig.dirty++;
}

/* Editor operations */
// TODO: We can't get pass line_count as we are still developing Normal Mode
void
editor_insert_char(int c)
{
    if (econfig.cy == econfig.line_count) {
        row_new(econfig.cy, "");
    }

    row_insert_char(&econfig.rows[econfig.cy], econfig.cx, c);
    econfig.cx++;
}

void
editor_insert_nline()
{

    // Insert a new row
    if (econfig.cx == 0) row_new(econfig.cy, "");
    // Insert trailing chars to new row
    else {
        editor_row_T *row = &econfig.rows[econfig.cy]; // cursor current row
        rbuf_move(row, econfig.cx - row->front); // move gap infront of cursor
        // Get the size of the trailing characters
        size_t tail_sz = row->size - row->front - row->gap;
        // Insert trailing characters to new row
        row_new(econfig.cy + 1, &row->chars[row->front + row->gap]);
        // Update the sizes
        row = &econfig.rows[econfig.cy];
        row->gap += tail_sz;
        // Make row renderable
        row_update(row);
    }

    // Update cursor position
    econfig.cy++;
    econfig.cx = 0;
}

void
editor_delete_char()
{
    if (econfig.cy == econfig.line_count) return;
    if (econfig.cx == 0 && econfig.cy == 0) return; // no char to delete

    editor_row_T *row = &econfig.rows[econfig.cy];
    // Check if cur is at the beginning of a line
    if (econfig.cx == 0) {
        editor_row_T *prev_row = &econfig.rows[econfig.cy - 1];

        // Move gap to the end of prev_row's char
        size_t tail_sz = prev_row->size - prev_row->front
                         - prev_row->gap; // current number of chars after gap
        if (tail_sz) {
            rbuf_move(prev_row,
                      tail_sz); // offset gap's front pointer by tail_sz amt
        }

        // Put cursor at the EOL of prev_row then insert text
        econfig.cx = prev_row->size - prev_row->gap;
        row_append_str(prev_row, row->render);

        // Delete row
        row_delete(econfig.cy);
        econfig.cy--;
    }
    else {
        row_delete_char(row, econfig.cx - 1);
        econfig.cx--;
    }
}
