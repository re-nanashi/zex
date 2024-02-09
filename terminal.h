/**
 * @file terminal.h
 * @author re-nanashi
 * @brief Header file containing declarations for terminal operations
 */

#ifndef TERMINAL_H
#define TERMINAL_H

/* @brief Disable raw mode/non-canonical mode */
void term_disable_raw_mode();

/* @brief Enable raw mode/non-canonical mode */
void term_enable_raw_mode();

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param rows Pointer to row property of config
 * @param cols Pointer to cols property of config
 */
int term_get_cursor_pos(int *numrows, int *numcols);

/**
 * @brief Get the current window size/dimensions of the terminal
 *
 * @param rows Pointer to rows property of config
 * @param cols Pointer to cols property of config
 */
int term_get_window_sz(int *numrows, int *numcols);

#endif /* TERMINAL_H */
