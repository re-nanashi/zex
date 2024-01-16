/** includes **/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/** defines **/
#define CTRL_KEY(k) ((k)&0x1f)

/** data **/
struct termios orig_termios;

/** terminal **/
void
clear_screen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor to top
}

void
die(const char *s)
{
    clear_screen();

    perror(s);
    exit(1);
}

void
disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void
enable_raw_mode()
{
    // Disable raw mode at exit
    // if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("hello");
    atexit(disable_raw_mode);

    // This is just a shallow copy
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

// TODO: handle escape sequences
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

/** output **/
void
editor_refresh_screen()
{
    clear_screen();
}

/** input **/
void
editor_process_keypress()
{
    char c = editor_read_key();

    switch (c) {
        case CTRL_KEY('q'):
            clear_screen();
            exit(0);
            break;
    }
}

/** init **/
int
main()
{
    enable_raw_mode();

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
