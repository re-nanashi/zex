/**
 * @file buffer.h
 * @author re-nanashi
 * @brief Header file containing declarations for gap buffer
 */

#ifndef BUFFER_H
#define BUFFER_H

#include "stddef.h"

#include "config.h"

#define GAP_SIZE 128

/**
 * @brief Initialize gap buffer to the current row
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_init(editor_row_T *row);

/**
 * @brief Destroy the gap buffer in the current row
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_destroy(editor_row_T *row);

/**
 * @brief Insert character to the front of the buffer of the row
 *
 * @param rows Pointer to current row cursor is at
 * @param c Character to insert
 */
void rbuf_insert(editor_row_T *row, int c);

/**
 * @brief Inserts string to a gap buffer
 *
 * This will insert the string to the gap buffer and increases the size of the
 * buffer if size is insufficient
 *
 * @param rows Pointer to current row cursor is at
 * @param s String to insert
 */
void rbuf_insertstr(editor_row_T *row, const char *s);

/**
 * @brief Moves buffer to the left by one
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_backward(editor_row_T *row);

/**
 * @brief Moves buffer to the right by one
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_forward(editor_row_T *row);

/**
 * @brief Moves buffer by the amt
 *
 * @param rows Pointer to current row cursor is at
 * @param amt Amount of movement either to the left(negative amt) or
 * right(positive amt)
 */
void rbuf_move(editor_row_T *row, ptrdiff_t amt);

/**
 * @brief Deletes the first character after the gap buffer
 *
 * This is done by simply increasing the gap
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_delete(editor_row_T *row);

/**
 * @brief Deletes the first character before the gap buffer
 *
 * This is done by simply increasing moving the gap forward
 *
 * @param rows Pointer to current row cursor is at
 */
void rbuf_backspace(editor_row_T *row);

#endif
