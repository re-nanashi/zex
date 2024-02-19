#include "buffer.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"

#define INIT_SIZE 128

void
rbuf_init(editor_row_T *row)
{
    // Initialize both bufsize and gapsize to equal len
    row->size = row->gap = INIT_SIZE;
    row->front = 0;
    // A row buffer's initial size would be 1024
    row->chars = malloc(INIT_SIZE + 1);
}

void
rbuf_destroy(editor_row_T *row)
{
    free(row->chars);
    free(row->render);

    row->chars = NULL;
    row->render = NULL;
}

// TODO(2): Update row to render
void
rbuf_insert(editor_row_T *row, int c)
{
    // There is no more gap so create new gap
    if (!row->gap) {
        // Get the tail position of the gap buffer
        size_t tail = row->size - row->front;
        row->gap = row->size;
        row->size *= 2; // increase total size
        // Reallocate new size then move the character string to new address
        row->chars = realloc(row->chars, row->size);
        memmove(row->chars + row->front + row->gap, row->chars + row->front,
                tail);
    }

    // Insert new char then update sizes
    row->chars[row->front] = c;
    row->front++; // offset gap pos by one; to the right
    row->gap--; // decrement size of the gap
}

// When we leave a row, we make sure that we leave the front of the gap at
// exactly the last index of the cursor. So that when we go back, we will
// be able to put the cursor at the front of the gap (index where we left at)
void
rbuf_insertstr(editor_row_T *row, const char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        len--;

    size_t tail = row->size - row->front - row->gap;

    while (row->gap < len) {
        row->gap = 0; // make gap zero so we can utilize rbuf_insert function's
                      // resize side-effect
        rbuf_insert(row, 0);
        row->front--;
        row->gap++; // increment gap since we decremented at rbuf_insert
    }

    memcpy(row->chars + row->front, s, len);
    row->front += len;
    row->gap = row->size - row->front - tail;
}

// Will only be used when in insert mode
// Move the gap buffer to the left
void
rbuf_backward(editor_row_T *row)
{
    if (row->front > 0) {
        row->chars[row->front + row->gap - 1] = row->chars[row->front - 1];
        row->front--;
    }
}

// Will only be used when in insert mode
// Move the gap buffer to the right
void
rbuf_forward(editor_row_T *row)
{
    size_t tail = row->size - row->front - row->gap;
    // if there are chars after gap
    if (tail > 0) {
        row->chars[row->front] = row->chars[row->front + row->gap];
        row->front++;
    }
}

// Negative amt moves the front of gap to the left
// Positive amt moves the front of gap to the right
void
rbuf_move(editor_row_T *row, ptrdiff_t amt)
{
    size_t len = 0;
    char *dest, *src;

    if (amt < 0) {
        len -= amt; // abs value
        // Prevent the amt of movement to get past the front of the buffer
        if (len > row->front) len = row->front;
        dest = row->chars + row->front + row->gap - len;
        src = row->chars + row->front - len;
        row->front -= len;
    }
    else {
        size_t tail = row->size - row->front - row->gap;
        len = amt;
        // Prevent the amt of movement to get past the end of the buffer
        if (len > tail) len = tail;
        dest = row->chars + row->front;
        src = row->chars + row->front + row->gap;
        row->front += len;
    }

    // Move the substring to new address
    memmove(dest, src, len);
}

void
rbuf_delete(editor_row_T *row)
{
    if (row->size > row->front + row->gap) row->gap++;
}

void
rbuf_backspace(editor_row_T *row)
{
    if (row->front) {
        row->front--;
        row->gap++;
    }
}
