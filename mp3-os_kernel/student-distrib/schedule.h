#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "i8259.h"
#include "syscall.h"

// all from OSDEV
#define PIT_DATA        0x40
#define COMMAND_REG     0x43
#define SET_MODE        0x36

#define PIT_IRQ         0
#define COUNTER_LO      0x0B 
#define COUNTER_HI      0xE9

// the terminal that is currently executing its process (in the round robin). This is an index into scheduling_array
int scheduled_process;

//Flag to determine if we've booted all three terminals and then must reset curr_term
int booted_flag;

// scheduling array - at most 3 processes running (per Aamir). These are the pids
int scheduling_array[3];

// BOOT
void initial_boot(void);

// initialize the PIT
void init_PIT(void);

// PIT handler
extern void PIT_handler();

/*Change currently scheduled process to next in scheduling queue*/
void schedule(void);


#endif /* _SCHEDULE_H */
