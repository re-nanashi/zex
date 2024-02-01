#include <unistd.h>
#include "editor_config.h"

/* @brief Editor configurations */
econf_t econfig;

/* @brief Disable raw mode/non-canonical mode */
void
disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &econfig.orig_termios) == -1)
        die("tcsetattr");
}

/* @brief Enable raw mode/non-canonical mode */
void
enable_raw_mode()
{
    // disable raw mode at exit
    if (tcgetattr(STDIN_FILENO, &econfig.orig_termios) == -1) die("tcgetattr");
    atexit(disable_raw_mode);
}

int
main(int argc, char *argvp[])
{
    enable_raw_mode();

    return 0;
}
