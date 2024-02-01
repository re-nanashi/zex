/**
 * @file editor_display.h
 * @author re-nanashi
 * @brief Functions responsible for managing terminal display in a
 *        terminal-based text editor.
 */

// TODO
#ifndef EDITOR_DISPLAY_H
#define EDITOR_DISPLAY_H

void editor_scroll();

void editor_draw_welcome_mes(struct append_buf *ab, const char *format, ...);

void editor_draw_rows(struct append_buf *ab);

void
editor_draw_status_bar(struct append_buf *ab)
{
    ab_append(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len =
        snprintf(status, sizeof(status), "%.20s - %d lines %s",
                 editor_conf.filename ? editor_conf.filename : "[No Name]",
                 editor_conf.numrows, editor_conf.dirty ? "(modified)" : "");
    int rlen =
        snprintf(rstatus, sizeof(rstatus), "%d/%d:%d", editor_conf.cy + 1,
                 editor_conf.numrows, editor_conf.cx + 1);

    if (len > editor_conf.screencols) len = editor_conf.screencols;
    ab_append(ab, status, len);
    while (len < editor_conf.screencols) {
        if (editor_conf.screencols - len == rlen) {
            ab_append(ab, rstatus, rlen);
            break;
        }
        else {
            ab_append(ab, " ", 1);
            len++;
        }
    }
    ab_append(ab, "\x1b[m", 3);
    ab_append(ab, "\r\n", 2);
}

void
editor_draw_message_bar(struct append_buf *ab)
{
    ab_append(ab, "\x1b[K", 3);
    int msglen = strlen(editor_conf.statusmsg);
    if (msglen > editor_conf.screencols) msglen = editor_conf.screencols;
    if (msglen && time(NULL) - editor_conf.statusmsg_time < 5)
        ab_append(ab, editor_conf.statusmsg, msglen);
}

void
editor_refresh_screen()
{
    editor_scroll();

    if (get_window_size(&editor_conf.screenrows, &editor_conf.screencols) == -1)
        die("get_window_size");

    // Buffer init
    struct append_buf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6); // hide cursor when repainting
    ab_append(&ab, "\x1b[H", 3); // reposition cursor to top

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_message_bar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
             (editor_conf.cy - editor_conf.row_offset) + 1,
             (editor_conf.rx - editor_conf.col_offset) + 1);
    ab_append(&ab, buf, strlen(buf));

    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);
}

void
editor_set_status_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(editor_conf.statusmsg, sizeof(editor_conf.statusmsg), fmt, ap);
    va_end(ap);
    editor_conf.statusmsg_time = time(NULL);
}

/* @brief Handle screen resolution changes */
void *
refresh_screen_thread()
{
    int prev_num_of_rows = editor_conf.screenrows,
        prev_num_of_cols = editor_conf.screencols;

    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 10000000;

    while (1) {
        int result =
            get_window_size(&editor_conf.screenrows, &editor_conf.screencols);

        if (result == -1) die("get_window_size");

        if (prev_num_of_rows != editor_conf.screenrows
            || prev_num_of_cols != editor_conf.screencols)
        {
            prev_num_of_rows = editor_conf.screenrows;
            prev_num_of_cols = editor_conf.screencols;

            editor_refresh_screen();
        }

        nanosleep(&sleep_time, NULL);
    }

    return NULL;
}

#endif /* EDITOR_DISPLAY_H */
