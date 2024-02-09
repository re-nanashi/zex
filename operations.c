/**
 * @file operations.c
 * @author re-nanashi
 * @brief Editor and row operations
 */

#include "operations.h"

#include <stdlib.h>
#include <string.h>

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
    int tabs = 0;
    // Get the number of '\t' within the line
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs * (ZEX_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % ZEX_TAB_STOP != 0)
                row->render[idx++] = ' ';
        }
        else {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void
op_insert_row(int at, char *s, size_t len)
{
    if (at < 0 || at > econfig.line_count) return;

    econfig.rows =
        realloc(econfig.rows, sizeof(editor_row_T) * (econfig.line_count + 1));
    memmove(&econfig.rows[at + 1], &econfig.rows[at],
            sizeof(editor_row_T) * (econfig.line_count - at));

    econfig.rows[at].size = len;
    econfig.rows[at].chars = malloc(len + 1);
    memcpy(econfig.rows[at].chars, s, len);
    econfig.rows[at].chars[len] = '\0';

    econfig.rows[at].rsize = 0;
    econfig.rows[at].render = NULL;
    op_update_row(&econfig.rows[at]);

    econfig.line_count++;
    econfig.dirty++;
}

void
op_free_row(editor_row_T *row)
{
    free(row->render);
    free(row->chars);
}

void
op_delete_row(int at)
{
    if (at < 0 || at >= econfig.line_count) return;

    op_free_row(&econfig.rows[at]);
    memmove(&econfig.rows[at], &econfig.rows[at + 1],
            sizeof(editor_row_T) * (econfig.line_count - at - 1));

    econfig.line_count--;
    econfig.dirty++;
}

void
op_row_insert_ch(editor_row_T *row, int at, int c)
{
    // Check if within row size
    if (at < 0 || at > row->size) at = row->size;

    // Add 2 bytes of memory to accomodate new char and null terminator
    row->chars = realloc(row->chars, row->size + 2);

    // Move new string after at to at + 1
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->chars[at] = c;
    row->size++;

    // Update char string to render string
    op_update_row(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
op_row_append_str(editor_row_T *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len); // append s to the end of the row

    // Update new row size
    row->size += len;
    row->chars[row->size] = '\0';

    // Update char string to render string
    op_update_row(row);
    // Flag dirty; changes have been made
    econfig.dirty++;
}

void
op_row_del_ch(editor_row_T *row, int at)
{
    if (at < 0 || at >= row->size) return;

    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;

    op_update_row(row);
    econfig.dirty++;
}

/* Editor operations */
// TODO: We can't get pass line_count as we are still developing Normal Mode
void
op_editor_insert_ch(int c)
{
    if (econfig.cy == econfig.line_count) {
        op_insert_row(econfig.cy, "", 0);
    }

    op_row_insert_ch(&econfig.rows[econfig.cy], econfig.cx, c);
    econfig.cx++;
}

void
op_editor_insert_nline()
{
    if (econfig.cx == 0)
        op_insert_row(econfig.cy, "", 0);
    else {
        editor_row_T *row = &econfig.rows[econfig.cy];
        op_insert_row(econfig.cy + 1, &row->chars[econfig.cx],
                      row->size - econfig.cx);
        row = &econfig.rows[econfig.cy];
        row->size = econfig.cx;
        row->chars[row->size] = '\0';
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
    if (econfig.cx > 0) {
        op_row_del_ch(&econfig.rows[econfig.cy], econfig.cx);
        econfig.cx--;
    }
    else {
        // Cursor is at the beginning of a line
        econfig.cx = econfig.rows[econfig.cy - 1].size; // put cursor at eol
        // Append string to previous string
        op_row_append_str(&econfig.rows[econfig.cy - 1], row->chars, row->size);
        op_delete_row(econfig.cy);
        econfig.cy--;
    }
}
