/**
 * @file input.h
 * @author re-nanashi
 * @brief Header file containing functions that handle input
 */

#ifndef INPUT_H
#define INPUT_H

/* @brief Internal macros */
#define CTRL_KEY(k) ((k)&0x1f) // ctrl + key input

/* @brief Not so ordinary keys */
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

/**
 * @brief Prompt that returns input
 *
 * @param prompt User string input
 */
char *get_user_input_prompt(char *prompt);

/**
 * @brief Handle cursor movement keys
 *
 * @param key Movement key; arrow keys
 */
void input_move_cursor(int key);

/* @brief Read keyboard input */
int input_read_key();

/* @brief Handle keyboard input */
void input_process_keypress();

#endif /* INPUT_H */
