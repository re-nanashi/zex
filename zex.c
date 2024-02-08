/**
 * @file zex.c
 * @author re-nanashi
 * @brief Zex main program
 */

#include <pthread.h>

#include "config.h"
#include "input.h"
#include "output.h"
#include "file_io.h"
#include "logger.h"
#include "terminal.h"

/* @brief Initialize Zex editor configurations */
econf_t econfig;

void
init_editor()
{
    econfig.cx = 0;
    econfig.cy = 0;
    econfig.rx = 0;
    econfig.row_offset = 0;
    econfig.col_offset = 0;
    econfig.numrows = 0;
    econfig.rows = NULL;
    econfig.mode = NORMAL;
    econfig.dirty = 0;
    econfig.filename = NULL;
    econfig.statusmsg[0] = '\0';
    econfig.statusmsg_time = 0;

    if (get_window_sz(&econfig.screenrows, &econfig.screencols) == -1)
        die("get_window_sz");
}

int
main(int argc, char *argv[])
{
    enable_raw_mode();
    init_editor();

    // If a file to edit is passed
    if (argc >= 2) {
        editor_fopen(argv[1]);
    }

    // Create new thread for handling terminal resolution changes
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_refresh_screen, NULL) != 0) {
        die("pthread_create");
        return 1;
    }

    if (pthread_detach(thread) != 0) {
        die("pthread_detach");
        return 1;
    }

    // Set initial status message
    editor_set_status_message("HELP: Ctrl-Q = quit");

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
