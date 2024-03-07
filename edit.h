/**
 * @file edit.h
 * @author re-nanashi
 * @brief Header file containing functions that will handle editing operations
 */

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "config.h"

#define ZEX_TAB_STOP 4

/* row operations */
/**
 * @brief Convert cx to rx to accomodate '\t' ch
 *
 * @param row Row to convert from
 * @param cx Current cursor position
 */
int row_convert_cx_to_rx(editor_row_T *row, int cx);

/**
 * @brief Update row and change '\t' to spaces
 *
 * Convert all '\t' ch to spaces; use defined macro to
 *
 * @param row Row to update
 */
void row_update(editor_row_T *row);

/**
 * @brief Insert new row to editor
 *
 * Insert row by reallocating new size of rows
 * NOTE: Very slow as it will copy previous data and offset it
 *
 * @param at Current y pos of the cursor
 * @param str String to insert
 * @param len Length of string to insert
 */
void row_new(linenr_T at, char *str);

/**
 * @brief Free a memory allocated for row
 *
 * @param row Pointer to row to free
 */
void row_free(editor_row_T *row);

/**
 * @brief Delete a row
 *
 * @param at Cursor y position; used as index to insert row
 */
void row_delete(linenr_T at);

/**
 * @brief Insert a character to the line/row
 *
 * @param row  Pointer to row to insert to
 * @param at Cursor x pos; position of the character to be inserted
 * @param c Char to insert
 */
void row_insert_char(editor_row_T *row, colnr_T at, int c);

/**
 * @brief Append string
 *
 * @param row  Pointer to row to insert to
 * @param str String to append
 * @param len Length of string to append
 */
void row_append_str(editor_row_T *row, char *str);

/**
 * @brief Delete a character from line/row
 *
 * @param row  Pointer to row to delete from
 * @param at Cursor x pos; position of the character to dlete
 */
void row_delete_char(editor_row_T *row, colnr_T at);

/* editor operations */
/**
 * @brief Insert a new char to the editor
 *
 * This inserts a new ch to row. This can also create a new row if cursor is at
 * the end of all lines
 *
 * @param c Character to insert
 */
void editor_insert_char(int c);

/**
 * @brief Insert a new line to the editor
 *
 * Insert a new line to the editor; mainly used to inserts a new line and
 * offsets succeeding rows
 *
 * @param c Character to insert
 */
void editor_insert_nline();

/* @brief Deletes a character from string and deletes row if no character left
 * in the row */
// TODO: We can't get past the last ch in a row. Implement modes
void editor_delete_char();

#endif /* OPERATIONS_H */
