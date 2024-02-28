#include <stdbool.h>
#include <ctype.h>

#include "config.h"
#include "input.h"
#include "normal.h"
#include "screen.h"
#include "operations.h"

/* @brief Internal macros */
#define ZEX_QUIT_TIMES 2

typedef struct {
    int cmdchar; /// command character
    int nchar; /// next command character
    unsigned int opcount; /// count before an operator
} cmdarg_T;

typedef bool (*state_callback)(cmdarg_T *, int);

// State callback function declaration
bool nv_mode(cmdarg_T *arg, int k);
bool insert_mode(cmdarg_T *arg, int k);
bool command_line_mode(cmdarg_T *arg, int k);
bool operator_pending_mode(cmdarg_T *arg, int k);
bool count_pending_mode(cmdarg_T *arg, int k);

// First instance of this function being called should pass nv_mode()
void
state_enter(state_callback s, cmdarg_T *arg)
{
    int k;
    do {
        k = input_read_key(); // read keyboard input
    } while (s(arg, k));
}

bool
nvmode(cmdarg_T *arg, int key)
{
    if (key == ':')
        state_enter(command_line_mode, NULL);
    else if (key == 'i')
        state_enter(insert_mode, NULL);
    else if (key == 'g' || key == 'R' || key == 'y' || key == 'd' || key == 'c')
    {
        cmdarg_T oparg;
        oparg.cmdchar = key;
        state_enter(operator_pending_mode, &oparg);
    }
    else if (isdigit(key)) {
        cmdarg_T ctarg;
        ctarg.cmdchar = key;
        // count_pending_mode converts the cmdchar (single digit unsigned int)
        // into a character that is appended to the stringified opcount
        // then back to number to be assigned to opcount
        state_enter(count_pending_mode, &ctarg);
    }
    else {
        nv_process_key(key);
    }

    return true;
}

bool
insert_mode(cmdarg_T *arg, int key)
{
    if (key == CTRL_KEY('[')) return false;

    switch (key) {
        case HOME_KEY:
            econfig.cx = 0;
            break;
        case END_KEY:
            if (econfig.cy < econfig.line_count) {
                econfig.cx = econfig.rows[econfig.cy].size - 1;
            }
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            if (key == PAGE_UP) {
                econfig.cy = econfig.row_offset;
            }
            else if (key == PAGE_DOWN) {
                econfig.cy = econfig.row_offset + econfig.screenrows - 1;
                if (econfig.cy > econfig.line_count)
                    econfig.cy = econfig.line_count - 1;
            }

            int times = econfig.screenrows;
            while (times--) {
                input_move_cursor(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        } break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            input_move_cursor(key);
            break;

        // Insert new line
        case '\r':
            op_editor_insert_nline();
            break;

        // Handle delete keys
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (key == DEL_KEY) input_move_cursor(ARROW_RIGHT);
            op_editor_del_ch();
            break;

        default:
            // Insert character to line/row
            op_editor_insert_ch(key);
    }

    return true;
}

bool command_line_mode(cmdarg_T *arg, int k){};
// bool operator_pending_mode(cmdarg_T *arg, int k);
// bool count_pending_mode(cmdarg_T *arg, int k);
