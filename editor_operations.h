/**
 * @file editor_operations.h
 * @author re-nanashi
 * @brief Header file containing functions that will handle editor operations
 */

#ifndef EDITOR_OPERATIONS_H
#define EDITOR_OPERATIONS_H

#include "config.h"

#define ZEX_TAB_STOP 4

/* row operations */
int row_convert_cx_to_rx(erow_t *row, int cx);

void update_row(erow_t *row);

void insert_row(int at, char *s, size_t len);

void free_row(erow_t *row);

void delete_row(int at);

void row_insert_ch(erow_t *row, int at, int c);

void row_append_str(erow_t *row, char *s, size_t len);

void row_del_ch(erow_t *row, int at);

/* editor operations */
void editor_insert_ch(int c);

void editor_insert_nline();

// TODO: We can't get past the last ch in a row. Implement modes
void editor_del_ch();

#endif /* EDITOR_OPERATIONS_H */
