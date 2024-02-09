/**
 * @file logger.h
 * @author re-nanashi
 * @brief logger functions and macros for logging messages and events in zex
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

void
die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen; refer to VT100
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top

    perror(s);
    exit(1);
}
