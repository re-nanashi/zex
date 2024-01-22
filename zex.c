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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>

/** defines **/
#define ZEX_VERSION         "0.0.1"
#define INITIAL_BUFFER_SIZE 2048

/* @brief CTRL + k(key) macro */
#define CTRL_KEY(k) ((k)&0x1f)

enum EditorKeys {
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
    char *chars;
} e_row;

/* @brief Editor's global state */
struct editor_config {
    int cx, cy;
    int screenrows;
    int screencols;
    int numrows;
    e_row *rows;
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
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

/* file i/o */
void
editor_open(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char line[INITIAL_BUFFER_SIZE];
    int i = 0;
    while (fgets(line, INITIAL_BUFFER_SIZE, fp)) {
        int len = strlen(line);
        editor_conf.rows[i].size = len;

        editor_conf.rows[i].chars = (char *)malloc(len + 1);
        memcpy(editor_conf.rows[i].chars, line, len);
        editor_conf.rows[i].chars[len] = '\0';

        editor_conf.numrows++;
        i++;
    }

    fclose(fp);
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
        // Only display welcome message if numrows is zero; no file as
        // arguments
        if (editor_conf.numrows == 0 && y >= editor_conf.numrows) {
            if (y == welcome_mes_row) {
                editor_draw_welcome_mes(ab, "ZEX editor v%s", ZEX_VERSION);
            }
            else if (y == welcome_mes_row + 2) {
                editor_draw_welcome_mes(
                    ab, "ZEX is open source and freely distributable");
            }
            else {
                ab_append(ab, "~", 1);
            }
        }
        else {
            int len = editor_conf.row.size;
            if (len > editor_conf.screencols) len = editor_conf.screencols;
            ab_append(ab, editor_conf.row.chars, len);
        }

        ab_append(ab, "\x1b[K", 3); // Erase part of the current line

        if (y < editor_conf.screenrows - 1) {
            ab_append(ab, "\r\n", 2);
        }
    }
}

void
editor_refresh_screen()
{
    if (get_window_size(&editor_conf.screenrows, &editor_conf.screencols)
        == -1)
        die("get_window_size");

    // Buffer init
    struct append_buf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6); // hide cursor when repainting
    ab_append(&ab, "\x1b[H", 3); // reposition cursor to top

    editor_draw_rows(&ab);

    // Move the cursor to the position stored in editor_conf
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", editor_conf.cy + 1,
             editor_conf.cx + 1);
    ab_append(&ab, buf, strlen(buf));

    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);
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
    switch (key) {
        case ARROW_LEFT:
            if (editor_conf.cx != 0) editor_conf.cx--;
            break;
        case ARROW_RIGHT:
            if (editor_conf.cx != editor_conf.screencols - 1) editor_conf.cx++;
            break;
        case ARROW_UP:
            if (editor_conf.cy != 0) editor_conf.cy--;
            break;
        case ARROW_DOWN:
            if (editor_conf.cy != editor_conf.screenrows - 1) editor_conf.cy++;
            break;
    }
}

void
editor_process_keypress()
{
    int c = editor_read_key();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J",
                  4); // clears the screen; check VT100
            write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top
            exit(0);
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            int times = editor_conf.screenrows;
            while (times--) {
                editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        } break;
        case HOME_KEY:
            editor_conf.cx = 0;
            break;
        case END_KEY:
            editor_conf.cx = editor_conf.screencols - 1;
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editor_move_cursor(c);
            break;
    }
}

/** init **/
void
init_editor()
{
    editor_conf.cx = 0;
    editor_conf.cy = 0;
    editor_conf.numrows = 0;

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

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
