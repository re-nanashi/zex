/**
 * @file zex.c
 * @author re-nanashi
 * @brief Simple text editor created in C
 */

/** includes **/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/** defines **/
/* @brief CTRL + k(key) macro */
#define CTRL_KEY(k) ((k)&0x1f)

/** data **/
/* @brief Editor's global state */
struct editor_config {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};
struct editor_config editor_conf;

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
char
editor_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    return c;
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

struct append_buf {
    char *b;
    int len;
};

// Default value/initialization for buffer
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
editor_draw_rows(struct append_buf *ab)
{
    int y;
    for (y = 0; y < editor_conf.screenrows; y++) {
        // Draw the tilde
        ab_append(ab, "~", 1);

        // K command erases part of the current line
        // 0 erases the part of the line right of the cursor
        ab_append(ab, "\x1b[K", 3);
        if (y < editor_conf.screenrows - 1) {
            ab_append(ab, "\r\n", 2);
        }
    }
}

void
editor_refresh_screen()
{
    struct append_buf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[?25l", 6); // hide cursor when repainting
    ab_append(&ab, "\x1b[H", 3); // reposition cursor to top

    editor_draw_rows(&ab);

    ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);
}

/** input **/
void
editor_process_keypress()
{
    char c = editor_read_key();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J",
                  4); // clears the screen; check VT100
            write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top
            exit(0);
            break;
    }
}

/** init **/
void
init_editor()
{
    if (get_window_size(&editor_conf.screenrows, &editor_conf.screencols)
        == -1)
        die("get_window_size");
}

int
main()
{
    enable_raw_mode();
    init_editor();

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
