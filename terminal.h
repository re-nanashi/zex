/**
 * @file terminal.h
 * @author re-nanashi
 * @brief Header file containing function declarations for terminal operations
 */

#ifndef TERMINAL_H
#define TERMINAL_H

/* @brief Disable raw mode/non-canonical mode */
void disable_raw_mode();

/* @brief Enable raw mode/non-canonical mode */
void enable_raw_mode();

/**
 * @brief Get the current position of the cursor on the terminal
 *
 * @param rows Pointer to row property of config
 * @param cols Pointer to cols property of config
 */
int get_cursor_pos(int *rows, int *cols);

/**
 * @brief Get the current window size/dimensions of the terminal
 *
 * @param rows Pointer to rows property of config
 * @param cols Pointer to cols property of config
 */
int get_window_sz(int *rows, int *cols);

#endif /* TERMINAL_H */
