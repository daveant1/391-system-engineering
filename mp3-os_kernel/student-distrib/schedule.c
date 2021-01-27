
#include "schedule.h"

/* void init_PIT() - initializes the PIT, called in kernel.c
 * Inputs - none
 * Outputs - none
 */
void init_PIT(){
    //use channel 0 data port (0x40)
    //use Mode/Command register port (0x43) to write instructions
    cli();
    outb(SET_MODE, COMMAND_REG);    //set PIT to generate square waves in 16-bit binary on channel 0 with lo/hi access mode

    // we want 10 to 50 ms
    outb(COUNTER_LO, PIT_DATA);     //send low 8-bits of counter followed by high 8 bits (counter = 59659 because 1193180/counter = 20Hz (50ms))
    outb(COUNTER_HI, PIT_DATA);

    enable_irq(PIT_IRQ);      //Enable IRQ on port 0 (timer chip)
    sti();
}

/* void PIT_handler() - function called upon interrupt fired by PIT
 * Inputs - none
 * Outputs - none
 * Mainly just calls schedule(), which does most of the work
 */
void PIT_handler(){
    send_eoi(PIT_IRQ);

    /*If more than one active terminal running, switch process*/
    schedule();


    /*
    need a way to keep track of list of pids
    current pid is important, as well as order
    i.e. fish1 (T1) -> fish3 (T3) -> fish2 (T2)
    
    how to account for when program is done? do round robin
    */

}

/* initial_boot()
 * Setups terminals and performs initial boot method
 * no input, no output
 * no side effects
 */
void initial_boot() {
    int i, j;
    // inititalize 3 terminals
    for (i = 0; i < 3; i++){
        term_t term;
        term.term_id = i;
        term.term_x = 0;
        term.term_y = 0;
        for(j = 0; j < KEYBOARD_BUF_SIZE; j++){
            term.keyboard_buf[j] = '\0';
        }
        /*Map each of terminals to corresponding vidmem page*/
        vidmap_term(i);
        
        terminals[i] = term;

        terminals[i].term_x = 0;
        terminals[i].term_y = 0;
        terminals[i].key_flag = 0;
        terminals[i].buf_idx = 0;
        terminals[i].ac_repeats = 0;
        terminals[i].history_idx = 0;
        terminals[i].key_flag = 0;

    }

    terminals[0].vidmem = (int32_t) TERM_1_VIDPAGE;
    terminals[1].vidmem = (int32_t) TERM_2_VIDPAGE;
    terminals[2].vidmem = (int32_t) TERM_3_VIDPAGE;

    //Reset curr_term to first shell terminal
    curr_term = 0;

    // set process to be 0 in preparation for the first terminal, and then start round robin
    scheduled_process = 0;

    for (i = 0; i < 3; i++){
        scheduling_array[i] = -1;
    }
}

/* void schedule() - change currently scheduled process to next in scheduling queue or boot up the three terminals if not booted yet
 * Inputs - none
 * Outputs - none
 */
void schedule(){

    int next_scheduling_term;
    int curr_esp;
    int curr_ebp;

    // ---------------------------------------------        BOOT        --------------------------------------------- //
    // BAND-AID SOLUTION DONT FIX WHATS NOT BROKEN

    // need to BOOT first terminal
    if(scheduling_array[0] == -1){
        // if not first terminal, map to backup buffers instead of actual video memory (because at start, we are looking at term 1)

        scheduling_vidmap(scheduled_process, curr_term);
        // send_eoi(PIT_IRQ);
        sys_execute((uint8_t*)"shell");
        return;
    }

    // save current ebp/esp (for the current pcb)
    asm volatile(

        "movl %%ebp, %0;"
        "movl %%esp, %1;"

        : "=r"(curr_ebp), "=r"(curr_esp)
    );

    // get pcb bc first shell has pid 0
    pcb_t* curr_pcb;
    pcb_t* next_pcb;
    curr_pcb = get_pcb_from_pid(scheduling_array[scheduled_process]);
    // curr_pcb->old_ebp = curr_ebp;
    // curr_pcb->old_esp = curr_esp;
    
    curr_pcb->schedule_ebp = curr_ebp;
    curr_pcb->schedule_esp = curr_esp;

    next_scheduling_term = ((1+scheduled_process) % 3);     // mod 3 because 3 terminals at most

    // need to BOOT the SECOND terminal
    if(scheduling_array[1] == -1){
        scheduled_process = 1;
        // switch_coords(0, 1);

        int ebp = 0x800000 - (scheduled_process) * 0x2000;      // 8MB - (pid)*8kB
        int esp = ebp;

        scheduling_vidmap(scheduled_process, curr_term);
        // switch_coords(0, 1);

        asm volatile(
            // literally save ebp and esp into (free to clobber) registers
            "movl %0, %%ebp;"
            "movl %1, %%esp;"
            
            : // no outputs
            : "r"(ebp), "r"(esp)    //we might need way to save esp/ebp after context switch in execute (instead of old parent's esp/ebp)
        );

        sys_execute((uint8_t*)"shell");
        return;
    }

    // BOOT THIRD TERMINAL AHHHHHH
    if(scheduling_array[2] == -1){
        scheduled_process = 2;
        booted_flag = 1;                                        // flag for debugging now
        // switch_coords(1,2);

        int ebp = 0x800000 - (scheduled_process) * 0x2000;      // 8MB - (pid)*8kB
        int esp = ebp;

        scheduling_vidmap(scheduled_process, curr_term);
        // switch_coords(1,2);

        asm volatile(
            // literally save ebp and esp into (free to clobber) registers
            "movl %0, %%ebp;"
            "movl %1, %%esp;"
            
            : // no outputs
            : "r"(ebp), "r"(esp)    //we might need way to save esp/ebp after context switch in execute (instead of old parent's esp/ebp)
            // : "r"(pcb->old_ebp), "r"(pcb->old_esp)    //we might need way to save esp/ebp after context switch in execute (instead of old parent's esp/ebp)
        );

        sys_execute((uint8_t*)"shell");
        return;
    }

    // --------------------------------------------- normal scheduling --------------------------------------------- //

    // video remapping
    // if scheduled_process == curr_term, then don't need to do anything because it's already mapped to physical vid addr (0xB8000), we did this in checkpoint 4
    // if NOT equal, then need to change mapping to map to background buffers in physical memory

    // finally switch it
    scheduled_process = next_scheduling_term;

    next_pcb = get_pcb_from_pid(scheduling_array[next_scheduling_term]);
    curr_pid = next_pcb->curr_pid;

    /* Restore next process' TSS */
    tss.ss0 = KERNEL_DS;
    // tss.esp0 = next_pcb->old_esp0;
    tss.esp0 = 0x800000 - (scheduling_array[next_scheduling_term]) * 0x2000;      // 8MB - (pid)*8kB

    /*Remap user 128MB to new user program*/ 
    map_user_program(scheduling_array[next_scheduling_term]);

    // video remapping 
    scheduling_vidmap(next_scheduling_term, curr_term);

    // switch coords
    // switch_coords(prev_process, next_scheduling_term);

    /* Switch ESP and EBP to next processes kernel stack */
    asm volatile(
        // literally save ebp and esp into (free to clobber) registers
        "movl %0, %%ebp;"
        "movl %1, %%esp;"
        
        : // no outputs
        : "r"(next_pcb->schedule_ebp), "r"(next_pcb->schedule_esp)    //we might need way to save esp/ebp after context switch in execute (instead of old parent's esp/ebp)
    );
    
    // send_eoi(PIT_IRQ);
}
