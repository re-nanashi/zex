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
#include "operations.h"
#include "logger.h"
#include "input.h"

char *
editor_rows_to_string(int *buflen)
{
    int totallen = 0;
    int idx;
    for (idx = 0; idx < econfig.line_count; idx++)
        totallen += econfig.rows[idx].size + 1;
    *buflen = totallen;

    char *buf = malloc(totallen);
    char *tmp = buf;
    for (idx = 0; idx < econfig.line_count; idx++) {
        memcpy(tmp, econfig.rows[idx].chars, econfig.rows[idx].size);
        tmp += econfig.rows[idx].size;
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

    while ((linelen = getline(&line, &linecap, (FILE *)fp)) != -1) {
        while (linelen > 0
               && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        op_insert_row(econfig.line_count, line);
    }

    free(line);
    fclose(fp);
    econfig.dirty = 0;
}

void
file_write()
{
    if (econfig.filename == NULL) {
        econfig.filename = get_user_input_prompt("Save as: %s");
        if (econfig.filename == NULL) {
            sbar_set_status_message("Save aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(&len);

    int fd = open(econfig.filename, O_RDWR | O_CREAT, 0644);
    // Error handling
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                // Free buffer
                free(buf);
                econfig.dirty = 0;
                sbar_set_status_message("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    // Free buffer
    free(buf);
    sbar_set_status_message("File cannot be saved. I/O error: %s",
                            strerror(errno));
}
