#ifndef BUFFER_H
#define BUFFER_H

#include "stddef.h"

#include "config.h"

#define GAP_SIZE 128

void rbuf_init(editor_row_T *row);

void rbuf_destroy(editor_row_T *row);

void rbuf_insert(editor_row_T *row, int c);

void rbuf_insertstr(editor_row_T *row, const char *s);

void rbuf_backward(editor_row_T *row);

void rbuf_forward(editor_row_T *row);

void rbuf_move(editor_row_T *row, ptrdiff_t amt);

void rbuf_delete(editor_row_T *row);

void rbuf_backspace(editor_row_T *row);

#endif
