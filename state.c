#include "state.h"

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "normal.h"
#include "screen.h"
#include "operations.h"
#include "normal.h"

// TODO: Just make sure that state code is working and

// First instance of this function being called should pass nv_mode()
void
state_enter(state_callback s, cmdarg_T *arg)
{
    while (1) {
        // Flush the UI using data from previous state changes
        screen_refresh();
        int key = input_read_key();

        // Execute the state callback
        bool check_result = s(arg, key);
        if (!check_result) {
            break; // terminate this state
        }
    }
}

bool
nv_mode(cmdarg_T *arg, int key)
{
    if (key == ':') {
        sbar_set_status_message(":");
        cmdarg_T cmdarg;
        cmdarg.count0 = 0;
        // Enter command line mode; command line
        state_enter(command_line_mode, &cmdarg);
    }
    else if (key == 'i') {
        sbar_set_status_message("-- INSERT --");
        // Enter insert mode
        state_enter(insert_mode, NULL);
    }
    else if (key == 'g' || key == 'R' || key == 'y' || key == 'd' || key == 'c')
    {
        sbar_set_status_message("%c", key);
        cmdarg_T oparg;
        oparg.cmdchar = key;
        oparg.count1 = 0;
        state_enter(operator_pending_mode, &oparg);
    }
    else if (isdigit(key) && key != '0') {
        sbar_set_status_message("num");
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

// TODO: Need to print mode in cmd bar and make sure that escape key is working
bool
command_line_mode(cmdarg_T *arg, int key)
{

    if (key == '\r' || key == CTRL_KEY('[')) {
        // Execute if key pressed is either <ESC> or <CR>
        return false;
    }
    else {
        arg->count0++;
        sbar_set_status_message("Command_line_mode(:) %d", arg->count0);
    }

    return true;
}

bool
operator_pending_mode(cmdarg_T *arg, int key)
{

    if (key == CTRL_KEY('[')) {
        // Execute if key pressed is either <ESC> or <CR>
        return false;
    }
    else {
        arg->count1++;
        /* mode for operator keys: g, R, y, d, c */
        sbar_set_status_message("operator_pending_mode(%c) %d", arg->cmdchar,
                                arg->count1);
    }

    return true;
}

bool
count_pending_mode(cmdarg_T *arg, int key)
{
    if (key == CTRL_KEY('[')) {
        // Execute if key pressed is either <ESC> or <CR>
        return false;
    }
    else {
        arg->count1++;
        /* mode for operator keys: g, R, y, d, c */
        sbar_set_status_message("operator_pending_mode(%c) %d", arg->cmdchar,
                                arg->count1);
    }

    return true;
}

/*
// TODO: Command mode functions
void execute_command(oparg_T *);
bool cmd_is_valid(const oparg_T *);

// Where do we get the next input?
bool
command_line_mode(cmdarg_T *arg, int key)
{
    size_t bufsize = 128, buflen = 0;
    arg->oap = malloc(bufsize);
    arg->oap[0] = '\0';

    while (1) {
        // Flush ui
        sbar_set_status_message(":");
        screen_refresh();

        // Read keyboard input from user
        int c = input_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) arg->oap[--buflen] = '\0';
        }
        else if (c == '\x1b') {
            // Escape command mode when user presses escape keybind
            sbar_set_status_message("");
            free(arg->oap);
            goto end;
        }
        else if (c == 0x0d) {
            // Execute command when user presses enter
            if (buflen != 0 && cmd_is_valid(arg->oap)) {
                sbar_set_status_message("");
                execute_command(arg->oap); // free oap after use
            }
            goto end;
        }
        else if (!iscntrl(c) && c < 128) {
            // Grow the buffer if size reaches the limit
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                arg->oap = realloc(arg->oap, bufsize);
            }
            arg->oap[buflen++] = c;
            arg->oap[buflen] = '\0';
        }
    }
end:
    return false;
};

bool
operator_pending_mode(cmdarg_T *arg, int k)
{
}
// bool count_pending_mode(cmdarg_T *arg, int k);

*/

bool
insert_mode(cmdarg_T *arg, int key)
{
    sbar_set_status_message("--INSERT--");

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
