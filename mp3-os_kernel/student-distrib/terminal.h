/* terminal.h - Defines used for terminal.c
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#define MAX_TERMINALS 3

#include "keyboard.h"
#include "types.h"
#include "lib.h"
#include "schedule.h"

// terminal driver functions below

// terminal open function
int32_t terminal_open(const uint8_t* filename);

// terminal close function
int32_t terminal_close(int32_t fd);

// terminal read function
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

// terminal write function
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

// boot terminals
void boot_terminals(void);

// switch terminals
void switch_terminals(int next_term);

// saves/restores screen_x and screen_y
// void switch_coords(int scheduled_term, int next_term);


// current terminal we are viewing (0, 1, or 2) corresponds to T1, T2, T3
int curr_term;

typedef struct term{
    int term_x;
    int term_y;
    int term_id;
    int active_pid;             // we might need this instead of term_pid array and just keep track of currently executing process on terminal

    char keyboard_buf[128];     // keyboard buf size = 128
    int buf_idx;                // index for keyboard buf
    
    // int term_pid[4];            // any given terminal can have at most 4 processes runnning (per discussion) - may not need it
    int32_t vidmem;             // store vidmem address
    volatile int key_flag;      // key for pressing enter

    int ac_repeats;             // count for #times stuck at autocomplete
    int history_idx;            // idx for buffer history
    int absolute_history_idx;   // absolute idx so we can return here
    char history[100][128];     // history of 100 keyboard buffers

} term_t;

term_t terminals[MAX_TERMINALS];


#endif /* _TERMINAL_H */
