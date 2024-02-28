#ifndef NORMAL_H
#define NORMAL_H

typedef enum ShiftStatus { SHIFT, SHIFT_NOT_PRESSED } shift_status_T;

void cursor_jump_forward_start_word(shift_status_T shift_status);

void replace_char_at_cur(shift_status_T shift_status);

void cursor_jump_forward_end_word(shift_status_T shift_status);

void nv_process_key(int c);

#endif
