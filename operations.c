/**
 * @file operations.c
 * @author re-nanashi
 * @brief Editor and row operations
 */

#include "operations.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

/* Row operations */
int
op_row_convert_cx_to_rx(editor_row_T *row, int cx)
{
    int rx = 0;

    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (ZEX_TAB_STOP - 1) - (rx % ZEX_TAB_STOP);
        rx++;
    }

    return rx;
}

void
op_update_row(editor_row_T *row)
{
    // Get the number of '\t' within the line
    int tabs = 0;
    size_t j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs * (ZEX_TAB_STOP - 1) + 1);

    size_t idx = 0;
    for (j = 0; j < row->size; j++) {
        // Skip rendering the gap
        if (j >= row->front && j < row->front + row->gap) continue;

        // Render tab as spaces until tabstop
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % ZEX_TAB_STOP != 0) {
                row->render[idx++] = ' ';
            }
        }
        else {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void
op_insert_row(colnr_T at, char *s)
{
    // Check if there is row/line
    if (at < 0 || at > econfig.line_count) return;
    econfig.rows =
        realloc(econfig.rows, sizeof(editor_row_T) * (econfig.line_count + 1));

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
    op_update_row(&econfig.rows[at]);

    econfig.line_count++;
    econfig.dirty++;
}

void
op_free_row(editor_row_T *row)
{
    rbuf_destroy(row);
}

void
op_delete_row(linenr_T at)
{
    if (at < 0 || at >= econfig.line_count) return;

    op_free_row(&econfig.rows[at]);
    memmove(&econfig.rows[at], &econfig.rows[at + 1],
            sizeof(editor_row_T) * (econfig.line_count - at - 1));

    econfig.line_count--;
    econfig.dirty++;
}

void
op_row_insert_ch(editor_row_T *row, colnr_T at, int c)
{
    // Check if within row size
    size_t rstrlen = row->size - row->gap;
    if (at < 0 || at > rstrlen) at = rstrlen;

    // Move the front of the gap buffer to at
    rbuf_move(row, at - row->front);
    // Insert character to the front of the buffer
    rbuf_insert(row, c);
    // Update char string to render string
    op_update_row(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
op_row_append_str(editor_row_T *row, char *s)
{
    rbuf_insertstr(row, s);
    // Update char string to render string
    op_update_row(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
op_row_del_ch(editor_row_T *row, colnr_T at)
{
    size_t rstrlen = row->size - row->gap;
    if (at < 0 || at >= rstrlen) return;

    // Moves the gap infront of  row[at] then deletes the ch after the gap
    rbuf_move(row, at - row->front);
    rbuf_delete(row);

    // Update the row to be renderable
    op_update_row(row);
    econfig.dirty++;
}

/* Editor operations */
// TODO: We can't get pass line_count as we are still developing Normal Mode
void
op_editor_insert_ch(int c)
{
    if (econfig.cy == econfig.line_count) {
        op_insert_row(econfig.cy, "");
    }

    op_row_insert_ch(&econfig.rows[econfig.cy], econfig.cx, c);
    econfig.cx++;
}

void
op_editor_insert_nline()
{

    // Insert a new row
    if (econfig.cx == 0) op_insert_row(econfig.cy, "");
    // Insert trailing chars to new row
    else {
        editor_row_T *row = &econfig.rows[econfig.cy]; // cursor current row
        rbuf_move(row, econfig.cx - row->front); // move gap infront of cursor
        // Get the size of the trailing characters
        size_t tail_sz = row->size - row->front - row->gap;
        // Insert trailing characters to new row
        op_insert_row(econfig.cy + 1, &row->chars[row->front + row->gap]);

        // Update the sizes
        row = &econfig.rows[econfig.cy];
        row->gap += tail_sz;

        op_update_row(row);
    }

    econfig.cy++;
    econfig.cx = 0;
}

void
op_editor_del_ch()
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
        rbuf_insertstr(prev_row, row->render);

        // Delete row
        op_delete_row(econfig.cy);
        econfig.cy--;
    }
    else {
        op_row_del_ch(row, econfig.cx - 1);
        econfig.cx--;
    }
}
