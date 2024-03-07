/**
 * @file input.h
 * @author re-nanashi
 * @brief Header file containing functions that handle input
 */

#ifndef INPUT_H
#define INPUT_H

/* @brief Internal macros */
#define CTRL_KEY(k) ((k) & 0x1f) // ctrl + key input

/* @brief Not so ordinary keys that output espace sequences */
enum EditorKeys {
    BACKSPACE = 127,
    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/* @brief Reads keyboard input */
int input_read_key();

/**
 * @brief Prompt that returns input
 *
 * @param prompt String prompt
 */
char *get_user_input_prompt(char *prompt);

/**
 * @brief Handle cursor movement keys
 *
 * @param key Movement key; arrow keys
 */
void input_move_cursor(int key);

/**
 * @brief Handle keyboard input when in insert mode
 *
 * @param key Keyboard char key
 */
void ins_process_key(int key);

#endif /* INPUT_H */
