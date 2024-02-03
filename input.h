/**
 * @file input.h
 * @author re-nanashi
 * @brief Header file containing functions that handle input
 */

#ifndef INPUT_H
#define INPUT_H

/**
 * @brief Prompt that returns input
 *
 * @param prompt User string input
 */
char *editor_prompt(char *prompt);

/**
 * @brief Handle cursor movement keys
 *
 * @param key Movement key; arrow keys
 */
void editor_move_cursor(int key);

/* @brief Read keyboard input */
int editor_read_key();

/* @brief Handle keyboard input */
void editor_process_keypress();

#endif /* INPUT_H */
