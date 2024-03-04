/**
 * @file state.h
 * @author re-nanashi
 * @brief Header file containing declarations for state management
 */

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

/* @brief Internal macros */
#define ZEX_QUIT_TIMES 2

typedef char oparg_T;
/// Arguments for Normal mode commands.
typedef struct {
    oparg_T *oap; ///< Operator arguments
    int cmdchar; ///< command character
    int nchar; ///< next command character (optional)
    int opcount; ///< count before an operator
    int count0; ///< count before command, default 0
    int count1; ///< count before command, default 1
    int arg; ///< extra argument from nv_cmds[]
    char *searchbuf; ///< return: pointer to search pattern or NULL
} cmdarg_T;

typedef bool (*state_callback)(cmdarg_T *, int);

// First instance of this function being called should pass nv_mode()
void state_enter(state_callback s, cmdarg_T *arg);

// State callback function declaration
bool nv_mode(cmdarg_T *arg, int k);
bool insert_mode(cmdarg_T *arg, int k);
bool command_line_mode(cmdarg_T *arg, int k);
bool operator_pending_mode(cmdarg_T *arg, int k);
bool count_pending_mode(cmdarg_T *arg, int k);

#endif /* STATE_H */
