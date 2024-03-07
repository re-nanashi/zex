/**
 * @file zex.c
 * @author re-nanashi
 * @brief Zex main program
 */

#include <pthread.h>

#include "config.h"
#include "input.h"
#include "screen.h"
#include "file_io.h"
#include "logger.h"
#include "terminal.h"
#include "state.h"

/* @brief Declare Zex editor configurations */
editor_config_T econfig;

void
init_editor()
{
    // Initialize configurations
    econfig.cx = 0;
    econfig.cy = 0;
    econfig.rx = 0;
    econfig.row_offset = 0;
    econfig.col_offset = 0;
    econfig.line_count = 0;
    econfig.rows = NULL;
    econfig.mode = MODE_NORMAL;
    econfig.dirty = 0;
    econfig.filename = NULL;
    econfig.statusmsg[0] = '\0';
    econfig.statusmsg_time = 0;

    if (term_get_window_sz(&econfig.screenrows, &econfig.screencols) == -1)
        die("get_window_sz");
}

int
main(int argc, char *argv[])
{
    term_enable_raw_mode();
    init_editor();

    // If a file to edit is passed
    if (argc >= 2) {
        file_open(argv[1]);
    }

    // Create new thread for handling terminal resolution changes
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_screen_refresh, NULL) != 0) {
        die("pthread_create");
        return 1;
    }

    // Detach thread
    if (pthread_detach(thread) != 0) {
        die("pthread_detach");
        return 1;
    }

    // Set initial status message
    sbar_set_status_message("HELP: Ctrl-Q = quit");

    // Enter MODE_NORMAL as default state
    state_enter(nv_mode, NULL);

    return 0;
}
