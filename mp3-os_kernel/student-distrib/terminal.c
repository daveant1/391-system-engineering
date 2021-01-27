/* terminal.c - Functions for terminal driver
 */

#include "terminal.h"

/* terminal_open(const uint8_t* command)
 * Inputs: filename - not relevant to terminal
 * Return Value: int32_t (0 on success, -1 on failure)
 * Function: not relevant to terminal */
int32_t terminal_open(const uint8_t* filename){
    return 0;
}


/* terminal_close(int32_t fd)
 * Inputs: fd - not relevant to terminal
 * Return Value: int32_t (0 on success, -1 on failure)
 * Function: not relevant to terminal */
int32_t terminal_close(int32_t fd){
    return 0;
}


/* terminal_read(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: fd - not relevant to terminal
 *          buf - data to read to
 *          nbytes - number of bytes to read
 * Return Value: int32_t (number of bytes written on success, -1 on failure?)
 * Function: Reads from keyboard buffer to buf until a newline '\n' */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    int bytes;

    if(nbytes <= 0 || buf == NULL){
        return -1;
    }

    // wait until '\n' is pressed
    while(!terminals[scheduled_process].key_flag);      // new for terminals

    // bytes should be the minimum between keyboard buffer length and nbytes to read
    bytes = (strlen(terminals[scheduled_process].keyboard_buf) < nbytes) ? strlen(terminals[scheduled_process].keyboard_buf) : nbytes;

    // use memcpy from lib.c to copy from keyboard_buf to buf
    memcpy(buf, terminals[scheduled_process].keyboard_buf, bytes);

    clear_keyboard_buf(scheduled_process);       // need to clear the buffer at the end

    // key_flag = 0;                            // reset key_flag to prevent reading
    terminals[scheduled_process].key_flag = 0;  // reset key_flag to prevent reading


    return bytes;               // return size of buffer
}


/* terminal_write(int32_t fd, const void* buf, int32_t nbytes)
 * Inputs: fd - not relevant to terminal
 *          buf - data to write
 *          nbytes - number of bytes
 * Return Value: int32_t (number of bytes written on success, -1 on failure?)
 * Function: Writes n bytes from buf to screen */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    cli();

    int i;      // for looping
    int bytes;

    if(nbytes <= 0 || buf == NULL){
        return -1;
    }

    // bytes should be the minimum between keyboard buffer length and nbytes to read
    bytes = (strlen((char*)buf) < nbytes) ? strlen((char*)buf) : nbytes;

    for(i = 0; i < nbytes; i++){
        char letter = ((char*)buf)[i];      // converting void pointer to char pointer -> need to index by array (ea char = 1 byte)
        if(letter != '\0'){                 // do not print null bytes to screen
            putc(letter, scheduled_process);
        }
    }

    sti();

    return nbytes;
}


/* void switch_terminals(int next_term)
 * Inputs: int next_term
 *          buf - data to write
 *          nbytes - number of bytes
 * Return Value: int32_t (number of bytes written on success, -1 on failure?)
 * Function: Writes n bytes from buf to screen */
void switch_terminals(int next_term) {
    if (curr_term == next_term) return;

    // realized we should reset vidmem to point directly to vidmem -> needs to do it anyway (we werent calling scheduling_vidmem equivalent here)
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].P = 1;
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].U = 1;
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].R = 1;
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB);
    flush_tlb();

    // save video mem into backup buffer,
    memcpy((char*)terminals[curr_term].vidmem, (uint32_t*)VIDMEM_ADDRESS, 80*25*2);     //  rows*cols*2 since each "pixel" is 2 bytes

    // restore next state into vidmem
    memcpy((char*)VIDMEM_ADDRESS, (char*)terminals[next_term].vidmem, 80*25*2);         // rows*cols*2 since each "pixel" is 2 bytes

    // finally switch terms
    curr_term = next_term;
    update_cursor(terminals[next_term].term_x, terminals[next_term].term_y);
}

