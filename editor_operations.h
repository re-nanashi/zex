/**
 * @file operations.h
 * @author re-nanashi
 * @brief Header file containing functions that will handle editor operations
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
int op_row_convert_cx_to_rx(erow_t *row, int cx);

/**
 * @brief Update row and change '\t' to spaces
 *
 * Convert all '\t' ch to spaces; use defined macro to
 *
 * @param row Row to update
 */
void op_update_row(erow_t *row);

/**
 * @brief Insert row to editor
 *
 * Insert row by reallocating new size of rows
 * NOTE: Very slow as it will copy previous data and offset it
 *
 * @param at Current y pos of the cursor
 * @param s String to insert
 * @param len Length of string to insert
 */
void op_insert_row(int at, char *s, size_t len);

/**
 * @brief Free a memory allocated for row
 *
 * @param row Pointer to row to free
 */
void op_free_row(erow_t *row);

/**
 * @brief Delete a row
 *
 * @param at Cursor y position; used as index to insert row
 */
void op_delete_row(int at);

/**
 * @brief Insert a character to the line/row
 *
 * @param row  Pointer to row to insert to
 * @param at Cursor x pos; position of the character to be inserted
 * @param c Char to insert
 */
void op_row_insert_ch(erow_t *row, int at, int c);

/**
 * @brief Append string
 *
 * @param row  Pointer to row to insert to
 * @param s String to append
 * @param len Length of string to append
 */
void op_row_append_str(erow_t *row, char *s, size_t len);

/**
 * @brief Delete a character from line/row
 *
 * @param row  Pointer to row to delete from
 * @param at Cursor x pos; position of the character to dlete
 */
void op_row_del_ch(erow_t *row, int at);

/* editor operations */
/**
 * @brief Insert a new char to the editor
 *
 * This inserts a new ch to row. This can also create a new row if cursor is at
 * the end of all lines
 *
 * @param c Character to insert
 */
void op_editor_insert_ch(int c);

/**
 * @brief Insert a new line to the editor
 *
 * Insert a new line to the editor; mainly used to inserts a new line and
 * offsets succeeding rows
 *
 * @param c Character to insert
 */
void op_editor_insert_nline();

/* @brief Deletes a character from string and deletes row if no character left
 * in the row */
// TODO: We can't get past the last ch in a row. Implement modes
void op_editor_del_ch();

#endif /* OPERATIONS_H */
