#ifndef NORMAL_H
#define NORMAL_H

typedef enum ShiftFlag { SHIFT, UNSHIFT } sflag_t;

void cursor_jump_forward_start_word(sflag_t sflag);

void replace_char_at_cur(sflag_t sflag);

void cursor_jump_forward_end_word(sflag_t sflag);

void nvrc_process_key(int c);

#endif
