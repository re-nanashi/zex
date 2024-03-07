/**
 * @file file_io.h
 * @author re-nanashi
 * @brief File I/O operations
 */

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "screen.h"
#include "edit.h"
#include "logger.h"
#include "input.h"

char *
editor_rows_to_str(int *buflen)
{
    int totallen = 0;
    size_t i;
    for (i = 0; i < econfig.line_count; i++)
        totallen += econfig.rows[i].size + 1;
    *buflen = totallen;

    // Copy all the strings of the editor to one big string; The resulting
    // string address will be printed on the screen
    char *buf = malloc(totallen);
    char *tmp = buf;
    for (i = 0; i < econfig.line_count; i++) {
        memcpy(tmp, econfig.rows[i].chars, econfig.rows[i].size);
        tmp += econfig.rows[i].size;
        *tmp = '\n';
        tmp++;
    }

    // Return the initial address of the buffer; remember to free after use
    return buf;
}

void
file_open(char *filename)
{
    // Free filename from config when opening new file
    free(econfig.filename);
    econfig.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    // Copy the text from the file to the editor rows by creating one row per
    // line; Lines are separated by '\n' to each other.
    while ((linelen = getline(&line, &linecap, (FILE *)fp)) != -1) {
        while (linelen > 0
               && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        row_new(econfig.line_count, line);
    }

    free(line);
    fclose(fp);
    econfig.dirty = 0; // No changes are made
}

void
file_write()
{
    if (econfig.filename == NULL) {
        econfig.filename =
            get_user_input_prompt("Save as: %s"); // Get name from user
        if (econfig.filename == NULL) {
            statusbar_set_message("Save aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_str(&len);

    int fd = open(econfig.filename, O_RDWR | O_CREAT, 0644);
    // Error handling
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) { // write the file
                close(fd);
                // Free buffer
                free(buf);
                econfig.dirty = 0;
                statusbar_set_message("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    // Free buffer
    free(buf);
    statusbar_set_message("File cannot be saved. I/O error: %s",
                          strerror(errno));
}
