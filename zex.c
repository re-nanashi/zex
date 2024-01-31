/**
 * @file zex.c
 * @author re-nanashi
 * @brief Simple text editor created in C
 */

/** includes **/
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>

/** defines **/
#define ZEX_VERSION    "0.0.1"
#define ZEX_TAB_STOP   4
#define ZEX_QUIT_TIMES 2

/* @brief CTRL + k(key) macro */
#define CTRL_KEY(k) ((k)&0x1f)
#define IS_ZERO(x)  (x == 0)

enum EditorKeys {
    BACKSPACE = 127,
    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN
};

// TODO: Create modes
enum Modes { NORMAL, INSERT };

/** data **/

typedef struct editor_row {
    int size;
    int rsize;
    char *chars;
    char *render;
} e_row;

/* @brief Editor's global state */
struct editor_config {
    int cx, cy;
    int rx;
    int row_offset;
    int col_offset;
    int screenrows;
    int screencols;
    int numrows;
    e_row *rows;
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct termios orig_termios;
};
struct editor_config editor_conf;

/** terminal **/
/**
 * @brief Display error message then quit program
 *
 * @param Error message str
 */
void
die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen; check VT100
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top

    perror(s);
    exit(1);
}

/* @brief Disable raw mode by setting back the state to its original config */
void
disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_conf.orig_termios) == -1)
        die("tcsetattr");
}

/* @brief Enable raw mode */
void
enable_raw_mode()
{
    // Disable raw mode at exit
    // if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    if (tcgetattr(STDIN_FILENO, &editor_conf.orig_termios) == -1) die("hello");
    atexit(disable_raw_mode);

    // This is just a shallow copy
    struct termios raw = editor_conf.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

// TODO: handle escape sequences
/* @brief Read key input */
int
editor_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    // Handle not so ordinary keys
    if (c == '\x1b') {
        char seq[3];

        // Immediately read two more bytes into seq buffer
        // if both reads time out; user just pressed Escape
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                // if there is no '~' in the sequence
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            }
            else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        }
        else if (seq[0] == '0') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }

        return '\x1b';
    }
    else {
        switch (c) {
            case 'k':
                return ARROW_UP;
            case 'j':
                return ARROW_DOWN;
            case 'h':
                return ARROW_LEFT;
            case 'l':
                return ARROW_RIGHT;
            case '0':
                return HOME_KEY;
        }

        return c;
    }
}

int
get_cursor_position(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    // Subtract 2 from row to give space for status bar and cmd line
    *rows -= 2;

    editor_read_key();

    return -1;
}

/**
 * @brief Extract the window size of the terminal
 *
 * @param rows pointer to an integer to store the number of rows in the editor
 * @param cols pointer to an integer to store the number of cols in the editor
 */
int
get_window_size(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return get_cursor_position(rows, cols);
    }
    else {
        // Subtract 2 from rows to give space for status bar and cmd line
        *rows = ws.ws_row - 2;
        *cols = ws.ws_col;
        return 0;
    }
}

/* row operations */
int
editor_row_cx_to_rx(e_row *row, int cx)
{
    int rx = 0;
    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += (ZEX_TAB_STOP - 1) - (rx % ZEX_TAB_STOP);
        rx++;
    }

    return rx;
}

void
editor_update_row(e_row *row)
{
    int tabs = 0;
    int j;
    // get the number of '\t' within the line
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs * (ZEX_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % ZEX_TAB_STOP != 0)
                row->render[idx++] = ' ';
        }
        else {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void
editor_insert_row(int at, char *s, size_t len)
{
    if (at < 0 || at > editor_conf.numrows) return;

    editor_conf.rows =
        realloc(editor_conf.rows, sizeof(e_row) * (editor_conf.numrows + 1));
    memmove(&editor_conf.rows[at + 1], &editor_conf.rows[at],
            sizeof(e_row) * (editor_conf.numrows - at));

    editor_conf.rows[at].size = len;
    editor_conf.rows[at].chars = malloc(len + 1);
    memcpy(editor_conf.rows[at].chars, s, len);
    editor_conf.rows[at].chars[len] = '\0';

    editor_conf.rows[at].rsize = 0;
    editor_conf.rows[at].render = NULL;
    editor_update_row(&editor_conf.rows[at]);

    editor_conf.numrows++;
    editor_conf.dirty++;
}

void
editor_free_row(e_row *row)
{
    free(row->render);
    free(row->chars);
}

void
editor_del_row(int at)
{
    if (at < 0 || at >= editor_conf.numrows) return;
    editor_free_row(&editor_conf.rows[at]);
    memmove(&editor_conf.rows[at], &editor_conf.rows[at + 1],
            sizeof(e_row) * (editor_conf.numrows - at - 1));
    editor_conf.numrows--;
    editor_conf.dirty++;
}

void
editor_row_insert_ch(e_row *row, int at, int c)
{
    if (at < 0 || at > row->size) at = row->size;
    row->chars =
        realloc(row->chars, row->size + 2); // +1 for new char; +1 for null
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editor_update_row(row);
    editor_conf.dirty++;
}

void
editor_row_append_str(e_row *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    editor_conf.dirty++;
}

void
editor_row_del_ch(e_row *row, int at)
{
    if (at < 0 || at >= row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editor_update_row(row);
    editor_conf.dirty++;
}

/* editor operations */
void
editor_insert_ch(int c)
{
    // TODO:
    // if (editor_conf.cy == editor_conf.numrows) {
    //     editor_append_row("", 0);
    // }
    editor_row_insert_ch(&editor_conf.rows[editor_conf.cy], editor_conf.cx, c);
    editor_conf.cx++;
}

void
editor_insert_nline()
{
    if (editor_conf.cx == 0) {
        editor_insert_row(editor_conf.cy, "", 0);
    }
    else {
        e_row *row = &editor_conf.rows[editor_conf.cy];
        editor_insert_row(editor_conf.cy + 1, &row->chars[editor_conf.cx],
                          row->size - editor_conf.cx);
        row = &editor_conf.rows[editor_conf.cy];
        row->size = editor_conf.cx;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }
    editor_conf.cy++;
    editor_conf.cx = 0;
}

// TODO: We can't get past the last ch in a row. Implement modes
void
editor_del_ch()
{
    // can't use since cursor is exactly at the last row
    // if (editor_conf.cy == editor_conf.numrows) return;
    if (editor_conf.cx == 0 && editor_conf.cy == 0) return;

    e_row *row = &editor_conf.rows[editor_conf.cy];
    if (editor_conf.cx > 0) {
        editor_row_del_ch(&editor_conf.rows[editor_conf.cy], editor_conf.cx);
        editor_conf.cx--;
    }
    else {
        // case where cursor is at the beginning of a line
        editor_conf.cx = editor_conf.rows[editor_conf.cy - 1]
                             .size; // put the cursor at the eol
        // append the string to the previous string
        editor_row_append_str(&editor_conf.rows[editor_conf.cy - 1],
                              row->chars, row->size);
        editor_del_row(editor_conf.cy);
        editor_conf.cy--;
    }
}

/* file i/o */
char *
editor_rows_to_string(int *buflen)
{
    int totlen = 0;
    int j;
    for (j = 0; j < editor_conf.numrows; j++)
        totlen += editor_conf.rows[j].size + 1;
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *tmp = buf;
    for (j = 0; j < editor_conf.numrows; j++) {
        memcpy(tmp, editor_conf.rows[j].chars, editor_conf.rows[j].size);
        tmp += editor_conf.rows[j].size;
        *tmp = '\n';
        tmp++;
    }

    // we return the initial address of the buffer
    // to be freed after use
    return buf;
}

void
editor_open(char *filename)
{
    free(editor_conf.filename);
    editor_conf.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    // Loop through all the line rows in the file
    while ((linelen = getline(&line, &linecap, (FILE *)fp)) != -1) {
        while (linelen > 0
               && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editor_insert_row(editor_conf.numrows, line, linelen);
    }

    free(line);
    fclose(fp);
    editor_conf.dirty = 0;
}

/* @brief Functions prototype that sets status message */
void editor_set_status_message(const char *fmt, ...);

void
editor_save()
{
    if (editor_conf.filename == NULL) return;

    int len;
    char *buf = editor_rows_to_string(&len);

    int fd = open(editor_conf.filename, O_RDWR | O_CREAT, 0644);
    // error handling
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                editor_conf.dirty = 0;
                editor_set_status_message("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    // free buffer
    free(buf);
    editor_set_status_message("File cannot be saved. I/O error: %s",
                              strerror(errno));
}

/* append buffer */
struct append_buf {
    char *b;
    int len;
};

/* @brief Default value/initialization for buffer */
#define ABUF_INIT                                                             \
    {                                                                         \
        NULL, 0                                                               \
    }

void
ab_append(struct append_buf *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new; // point the buffer to the address of new
    ab->len += len;
}

void
ab_free(struct append_buf *ab)
{
    free(ab->b);
}

/** output **/
void
editor_scroll()
{
    editor_conf.rx = 0;
    if (editor_conf.cy < editor_conf.numrows) {
        editor_conf.rx = editor_row_cx_to_rx(&editor_conf.rows[editor_conf.cy],
                                             editor_conf.cx);
    }

    if (editor_conf.cy < editor_conf.row_offset) {
        editor_conf.row_offset = editor_conf.cy;
    }

    if (editor_conf.cy >= editor_conf.row_offset + editor_conf.screenrows) {
        editor_conf.row_offset = editor_conf.cy - editor_conf.screenrows + 1;
    }

    if (editor_conf.rx < editor_conf.col_offset) {
        editor_conf.col_offset = editor_conf.rx;
    }

    if (editor_conf.rx >= editor_conf.col_offset + editor_conf.screencols) {
        editor_conf.col_offset = editor_conf.rx - editor_conf.screencols + 1;
    }
}

void
editor_draw_welcome_mes(struct append_buf *ab, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char welcome_mes[80];
    int welcome_mes_len =
        vsnprintf(welcome_mes, sizeof(welcome_mes), format, args);
    va_end(args);

    if (welcome_mes_len > editor_conf.screencols)
        welcome_mes_len = editor_conf.screencols;
    int padding = (editor_conf.screencols - welcome_mes_len) / 2;

    if (padding) {
        ab_append(ab, "~", 1);
        padding--;
    }

    while (padding--)
        ab_append(ab, " ", 1);

    ab_append(ab, welcome_mes, welcome_mes_len);
}

void
editor_draw_rows(struct append_buf *ab)
{
    int y;
    int welcome_mes_row = editor_conf.screenrows / 3;

    for (y = 0; y < editor_conf.screenrows; y++) {
        int filerow = y + editor_conf.row_offset;

        if (filerow >= editor_conf.numrows) {
            // Display welcome message if numrows is zero; no file as arguments
            if (IS_ZERO(editor_conf.numrows) && y == welcome_mes_row) {
                editor_draw_welcome_mes(ab, "ZEX editor v%s", ZEX_VERSION);
            }
            else if (IS_ZERO(editor_conf.numrows) && y == welcome_mes_row + 2)
            {
                editor_draw_welcome_mes(
                    ab, "ZEX is open source and freely distributable");
            }
            else {
                ab_append(ab, "~", 1);
            }
        }
        else {
            int len = editor_conf.rows[filerow].rsize - editor_conf.col_offset;
            if (len < 0) len = 0;
            if (len > editor_conf.screencols) len = editor_conf.screencols;
            ab_append(
                ab, &editor_conf.rows[filerow].render[editor_conf.col_offset],
                len);
        }

        ab_append(ab, "\x1b[K", 3); // Erase part of the current line
        ab_append(ab, "\r\n", 2);
    }
}

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

    if (get_window_size(&editor_conf.screenrows, &editor_conf.screencols)
        == -1)
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

/** input **/
void
editor_move_cursor(int key)
{
    // check if there is text first
    e_row *row = (editor_conf.cy >= editor_conf.numrows)
                     ? NULL
                     : &editor_conf.rows[editor_conf.cy];

    switch (key) {
        case ARROW_LEFT:
            if (editor_conf.cx != 0) editor_conf.cx--;
            break;
        case ARROW_RIGHT:
            if (row && editor_conf.cx < row->size - 1) editor_conf.cx++;
            break;
        case ARROW_UP:
            if (editor_conf.cy < editor_conf.numrows && editor_conf.cy != 0)
                editor_conf.cy--;
            break;
        case ARROW_DOWN:
            if (editor_conf.cy < editor_conf.numrows - 1) editor_conf.cy++;
            break;
    }

    row = (editor_conf.cy >= editor_conf.numrows)
              ? NULL
              : &editor_conf.rows[editor_conf.cy];
    int rowlen = row ? row->size : 0;
    if (editor_conf.cx >= rowlen) {
        editor_conf.cx = rowlen == 0 ? rowlen : rowlen - 1;
    }
}

void
editor_process_keypress()
{
    static int quit_times = ZEX_QUIT_TIMES;

    int c = editor_read_key();

    switch (c) {
        case '\r':
            editor_insert_nline();
            break;

        case CTRL_KEY('q'):
            if (editor_conf.dirty && quit_times > 0) {
                editor_set_status_message("Warning: File has unsaved changes. "
                                          "Press CTRL_Q %d more time to quit.",
                                          quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J",
                  4); // clears the screen; check VT100
            write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top
            exit(0);
            break;

        case CTRL_KEY('s'):
            editor_save();
            break;

        case HOME_KEY:
            editor_conf.cx = 0;
            break;

        case END_KEY:
            if (editor_conf.cy < editor_conf.numrows) {
                editor_conf.cx = editor_conf.rows[editor_conf.cy].size - 1;
            }
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editor_move_cursor(ARROW_RIGHT);
            editor_del_ch();
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                editor_conf.cy = editor_conf.row_offset;
            }
            else if (c == PAGE_DOWN) {
                editor_conf.cy =
                    editor_conf.row_offset + editor_conf.screenrows - 1;
                if (editor_conf.cy > editor_conf.numrows)
                    editor_conf.cy = editor_conf.numrows - 1;
            }

            int times = editor_conf.screenrows;
            while (times--) {
                editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        } break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editor_move_cursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            editor_insert_ch(c);
    }

    // reset if another key is pressed
    quit_times = ZEX_QUIT_TIMES;
}

/** init **/
void
init_editor()
{
    editor_conf.cx = 0;
    editor_conf.cy = 0;
    editor_conf.rx = 0;
    editor_conf.row_offset = 0;
    editor_conf.numrows = 0;
    editor_conf.rows = NULL;
    editor_conf.dirty = 0;
    editor_conf.filename = NULL;
    editor_conf.statusmsg[0] = '\0';
    editor_conf.statusmsg_time = 0;

    if (get_window_size(&editor_conf.screenrows, &editor_conf.screencols)
        == -1)
        die("get_window_size");
}

int
main(int argc, char *argv[])
{
    enable_raw_mode();
    init_editor();
    if (argc >= 2) {
        editor_open(argv[1]);
    }

    pthread_t thread;
    // Create a separate thread for handling terminal resolution changes
    if (pthread_create(&thread, NULL, refresh_screen_thread, NULL) != 0) {
        die("pthread_create");
        return 1;
    }

    if (pthread_detach(thread) != 0) {
        die("pthread_detach");
        return 1;
    }

    editor_set_status_message("HELP: Ctrl-Q = quit");

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
