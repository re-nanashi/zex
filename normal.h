#ifndef NORMAL_H
#define NORMAL_H

#include <stdbool.h>

typedef enum ShiftStatus { SHIFT, SHIFT_NOT_PRESSED } shift_status_T;

void fwd_word(bool flag);
void end_word(bool flag);

void replace_char_at_cur(shift_status_T shift_status);

void nv_process_key(int key);

#endif
