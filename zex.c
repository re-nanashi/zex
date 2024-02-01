#include <unistd.h>
#include "editor_config.h"

/* @brief Editor configurations */
econf_t econfig;

/**
 * @brief Log err message then exit program
 *
 * @param s Error message string
 */
void
die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen; refer to VT100
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top

    perror(s);
    exit(1);
}

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
