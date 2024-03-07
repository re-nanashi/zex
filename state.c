#include "state.h"

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "input.h"
#include "normal.h"
#include "screen.h"
#include "edit.h"
#include "normal.h"

// Returns the current mode string "NORMAL", "VISUAL", "INSERT", and "COMMAND"
const char *
get_mode(Mode mode)
{
    switch (mode) {
        case MODE_VISUAL:
            return "VISUAL";
        case MODE_INSERT:
            return "INSERT";
        case MODE_COMMAND:
            return "CMD";
        default:
            return "NORMAL";
    }
}

// First instance of this function being called should pass nv_mode()
void
state_enter(state_callback s, cmdarg_T *arg)
{
    while (1) {
        // Flush the UI using data from previous state changes
        screen_refresh();
        int key = input_read_key(); // read user keyboard input

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
        // Update current mode
        econfig.mode = MODE_COMMAND;
        // Enter command line mode; MODE_COMMAND
        cmdarg_T cmdlarg;
        cmdlarg.count0 = 0;
        state_enter(command_line_mode, &cmdlarg);
    }
    else if (key == 'i') {
        // Update current mode then print to status bar
        econfig.mode = MODE_INSERT;
        statusbar_set_message("-- INSERT --");
        // Enter insert mode; MODE_INSERT
        state_enter(insert_mode, NULL);
    }
    else if (key == 'g' || key == 'R' || key == 'y' || key == 'd' || key == 'c')
    {
        // Update current mode
        econfig.mode = MODE_OP_PENDING;
        // TODO: Show operator that was pressed
        // Enter operator pending mode; MODE_OP_PENDING
        cmdarg_T oparg;
        oparg.cmdchar = key;
        oparg.count1 = 0;
        state_enter(operator_pending_mode, &oparg);
    }
    else if (isdigit(key) && key != '0') {
        // Update current mode
        econfig.mode = MODE_COUNT_PENDING;
        // Enter count pending mode; MODE_COUNT_PENDING
        cmdarg_T ctarg;
        ctarg.cmdchar = key;
        // count_pending_mode converts the cmdchar (single digit unsigned int)
        // into a character that is appended to the stringified opcount
        // then back to number to be assigned to opcount
        state_enter(count_pending_mode, &ctarg);
    }
    else {
        // Default: Enter normal mode; MODE_NORMAL
        nv_process_key(key);
    }

    econfig.mode = MODE_NORMAL; // return to normal mode

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
        statusbar_set_message("Command_line_mode(:) %d", arg->count0);
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
        statusbar_set_message("operator_pending_mode(%c) %d", arg->cmdchar,
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
        statusbar_set_message("operator_pending_mode(%c) %d", arg->cmdchar,
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
        statusbar_set_message(":");
        screen_refresh();

        // Read keyboard input from user
        int c = input_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) arg->oap[--buflen] = '\0';
        }
        else if (c == '\x1b') {
            // Escape command mode when user presses escape keybind
            statusbar_set_message("");
            free(arg->oap);
            goto end;
        }
        else if (c == 0x0d) {
            // Execute command when user presses enter
            if (buflen != 0 && cmd_is_valid(arg->oap)) {
                statusbar_set_message("");
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
    if (key == CTRL_KEY('[')) {
        statusbar_set_message(""); // remove status bar message
        return false;
    }
    else {
        // Handle keys on insert mode; Most of the keys will be just be inserted
        // as a char to the editor and not a keybind to a different motion
        ins_process_key(key);
    }

    return true; // return to normal mode
}
