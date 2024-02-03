#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "config.h"
#include "output.h"
#include "terminal.h"
#include "logger.h"

void
write_to_abuf(a_buf *ab, const char *s, int len)
{
    // Reallocate new size of append buffer
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;

    // Assign new append buffer
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void
abuf_free(a_buf *ab)
{
    free(ab->b);
}

void
editor_scroll_handler()
{
    // Horizontal scrolling
    econfig.rx = 0;

    if (econfig.cy < econfig.numrows) {
        econfig.rx =
            editor_row_convert_cx_to_rx(&econfig.rows[econfig.cy], econfig.cx);
    }

    if (econfig.rx < econfig.col_offset) {
        econfig.col_offset = econfig.rx;
    }

    if (econfig.rx >= econfig.col_offset + econfig.screencols) {
        econfig.col_offset = econfig.rx - econfig.screencols + 1;
    }

    // Vertical scrolling
    if (econfig.cy < econfig.row_offset) {
        econfig.row_offset = econfig.cy;
    }

    if (econfig.cy >= econfig.row_offset + econfig.screenrows) {
        econfig.row_offset = econfig.cy - econfig.screenrows + 1;
    }
}

void
editor_draw_welcome_message(struct append_buf *ab, const char *format, ...)
{
    // Copy string to temporary buffer
    va_list args;
    va_start(args, format);

    char buf[80];
    int buflen = vsnprintf(buf, sizeof(buf), format, args);

    va_end(args);

    // Apply padding
    if (buflen > econfig.screencols) buflen = econfig.screencols;
    int padding = (econfig.screencols - buflen) / 2;

    if (padding) {
        write_to_abuf(ab, "~", 1);
        padding--;
    }
    while (padding--)
        write_to_abuf(ab, " ", 1);

    // Write temp buffer to append buffer
    write_to_abuf(ab, buf, buflen);
}

void
editor_draw_rows(struct append_buf *ab)
{
    int y;
    int welcome_message_row = econfig.screenrows / 3;

    for (y = 0; y < econfig.screenrows; y++) {
        int filerow = y + econfig.row_offset;

        // Length of text file does not exceed editor height
        if (filerow >= econfig.numrows) {
            if (econfig.numrows == 0) {
                if (y == welcome_message_row) {
                    editor_draw_welcome_message(ab, "ZEX editor v%s",
                                                ZEX_VERSION);
                }
                else if (y == welcome_message_row + 2) {
                    editor_draw_welcome_message(
                        ab, "ZEX is open source and freely distributable");
                }
            }
            else {
                write_to_abuf(ab, "~", 1);
            }
        }
        // Draw text from file to editor
        else {
            int len = econfig.rows[filerow].rsize - econfig.col_offset;
            if (len < 0) len = 0;
            if (len > econfig.screencols) len = econfig.screencols;
            write_to_abuf(ab, &econfig.rows[filerow].render[econfig.col_offset],
                          len);
        }

        write_to_abuf(ab, "\x1b[K", 3); // erase to end of current row
        write_to_abuf(ab, "\r\n", 2);
    }
}

void
editor_draw_status_bar(struct append_buf *ab)
{
    // Draw bar by inverting color (see Select Graphic Rendition)
    write_to_abuf(ab, "\x1b[7m", 4); // m command

    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       econfig.filename ? econfig.filename : "[No Name]",
                       econfig.numrows, econfig.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d:%d", econfig.cy + 1,
                        econfig.numrows, econfig.cx + 1);

    // Draw status text to editor
    if (len > econfig.screencols) len = econfig.screencols;
    write_to_abuf(ab, status, len);
    while (len < econfig.screencols) {
        if (econfig.screencols - len == rlen) {
            write_to_abuf(ab, rstatus, rlen);
            break;
        }
        else {
            write_to_abuf(ab, " ", 1); // write space until rstatus len
            len++;
        }
    }

    write_to_abuf(ab, "\x1b[m", 3); // reset to normal formatting
    write_to_abuf(ab, "\r\n", 2);
}

void
editor_draw_cmd_line(struct append_buf *ab)
{
    // Erase to end of current row
    write_to_abuf(ab, "\x1b[K", 3);

    int cmdlen = strlen(econfig.statusmsg);
    if (cmdlen > econfig.screencols) cmdlen = econfig.screencols;
    // Draw message to command line
    if (cmdlen && time(NULL) - econfig.statusmsg_time < 5)
        write_to_abuf(ab, econfig.statusmsg, cmdlen);
}

void
editor_set_status_message(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(econfig.statusmsg, sizeof(econfig.statusmsg), format, args);
    va_end(args);

    econfig.statusmsg_time = time(NULL);
}

void
editor_refresh_screen()
{
    // Apply offsetting when scrolling
    editor_scroll_handler();

    // Get new terminal size
    if (get_window_sz(&econfig.screenrows, &econfig.screencols) != -1)
        die("get_window_sz");

    // Initialize buffer
    a_buf ab = ABUF_INIT;

    write_to_abuf(&ab, "\x1b[?25l", 6); // hide cursor when repainting
    write_to_abuf(&ab, "\x1b[H", 3); // reposition cursor to top

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_cmd_line(&ab);

    char buf[32];
    // Reposition cursor with offset values
    snprintf(buf, sizeof(buf), "\x1b[%d:%dH",
             (econfig.cy - econfig.row_offset) + 1,
             (econfig.cx - econfig.col_offset) + 1);
    write_to_abuf(&ab, buf, strlen(buf));

    write_to_abuf(&ab, "\x1b?25h", 6); // show cursor; VT510

    // Draw buffer to terminal
    write(STDOUT_FILENO, ab.b, ab.len);
    abuf_free(&ab);
}

void *
thread_refresh_screen()
{
    int prev_num_rows = econfig.screenrows, prev_num_cols = econfig.screencols;

    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 10000000;

    while (1) {
        if (get_window_sz(&econfig.screenrows, &econfig.screencols) != -1)
            die("get_window_sz");

        if (prev_num_rows != econfig.screenrows
            || prev_num_cols != econfig.screencols)
        {
            // Assign new values
            prev_num_rows = econfig.screenrows;
            prev_num_cols = econfig.screencols;

            // Refresh screen using new values
            editor_refresh_screen();
        }

        nanosleep(&sleep_time, NULL);
    }

    return NULL;
}
