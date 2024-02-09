/**
 * @file terminal.c
 * @author re-nanashi
 * @brief Source file containing function definitions of terminal operations
 */

#include "terminal.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"

void
term_disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &econfig.orig_termios) == -1)
        die("tcsetattr");
}

void
term_enable_raw_mode()
{
    // Disable raw mode at exit
    if (tcgetattr(STDIN_FILENO, &econfig.orig_termios) == -1) die("tcgetattr");
    atexit(term_disable_raw_mode);

    // Set terminal attributes
    struct termios raw = econfig.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int
term_get_cursor_pos(int *rows, int *cols)
{
    char buf[32];
    unsigned int idx = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1; // no cursor position reported

    // Read escape code to buffer
    while (idx < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[idx], 1) != 1) break;
        if (buf[idx] == 'R') break;
        idx++;
    }
    buf[idx] = '\0';

    // Parse the buffer
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d:%d", rows, cols) != 2) return -1;
    *rows -= 2; // subtract 2 to give space to  status bar and cmd line

    return -1;
}

int
term_get_window_sz(int *rows, int *cols)
{
    struct winsize ws;

    // Check if we can get the terminal dimensions
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // Move the cursors to the edge of the terminal
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return term_get_cursor_pos(rows, cols);
    }
    else {
        // Subtract 2 from rows to give space to status bar and command line
        *rows = ws.ws_row - 2;
        *cols = ws.ws_col;
        return 0;
    }
}
