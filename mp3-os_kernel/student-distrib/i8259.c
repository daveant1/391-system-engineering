/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

// CITATION: OSDEV 8259_PIC

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
// initialize these to all 1's to default disable
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/*
 * i8259_init
 *   DESCRIPTION: Initializes the 8259 PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends ICW1-4 to master and slave PICs
 */   
void i8259_init(void) {
    // cli(); - initializing func must be a crit sect BUT this is covered by cli in boot.S and sti in kernel.c

    // send ICWs to both master AND slave

    // sending ICW1-4 to master PIC, use data port (port + 1) for ICW2-4 per lecture

    // ICW1: start init, edge-triggered inputs, cascase mode, 4 ICWs
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    // ICW2: high bits of vector# (master and slave respectively)
    outb(ICW2_MASTER, MASTER_8259_PORT+1);
    outb(ICW2_SLAVE, SLAVE_8259_PORT+1);

    // ICW3: bit vector of slaves
    outb(ICW3_MASTER, MASTER_8259_PORT+1);      // bit vector of slaves
    outb(ICW3_SLAVE, SLAVE_8259_PORT+1);        // input pin on master

    // ICW4: ISA=x86, normal/auto EOI
    outb(ICW4, MASTER_8259_PORT+1);
    outb(ICW4, SLAVE_8259_PORT+1);


    // mask all of master and slave, use data port
    outb(master_mask, MASTER_8259_PORT+1);
    outb(slave_mask, SLAVE_8259_PORT+1);

    // need to enable slave PIC - IRQ2 on master (otherwise slave is always masked)
    enable_irq(2);
}

/*
 * enable_irq
 *   DESCRIPTION: Enable (unmask) the specified IRQ 
 *   INPUTS: irq_num - the irq coming from master/slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: PIC ignores request and continues normal operation when bit is set
 */   
void enable_irq(uint32_t irq_num) {
    uint16_t port;
    uint8_t value;

    // IRQ bit must be 0 -> start w/ 1111 1110, shift by irq_num, mask, then flip bits
    // value = 0x00;

    // OOB edge case (must be btw 0 and 15)
    if(irq_num < 0 || irq_num > 15) return;

    // if master (mapped from 0 to 7)
    if(irq_num < 8 && irq_num >= 0){
        port = MASTER_8259_PORT+1;
    }
    // if slave, >8 (mapped from 8 to 15)
    else{
        irq_num -= 8;                   // to get w/in proper bounds for shifting
        port = SLAVE_8259_PORT+1;
    }
    
    value = inb(port) & ~(1 << irq_num);
    outb(value, port);
}


/*
 * disable_irq
 *   DESCRIPTION: Disable (mask) the specified IRQ
 *   INPUTS: irq_num - the irq coming from master/slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: masks IRQ2 from master PIC if slave PIC is disabled
 */   
void disable_irq(uint32_t irq_num) {
    uint16_t port;
    uint8_t value;

    // need to set IRQ bit

    // OOB edge case (must be btw 0 and 15)
    if(irq_num < 0 || irq_num > 15) return;

    // if master (mapped from 0 to 7)
    if(irq_num < 8 && irq_num >= 0){
        port = MASTER_8259_PORT+1;
    }
    // if slave, >8 (mapped from 8 to 15)
    else{
        irq_num -= 8;                   // to get w/in proper bounds for shifting
        port = SLAVE_8259_PORT+1;
    }

    value = inb(port) | (1 << irq_num);     // lshift 1 accordingly to determine 
    outb(value, port);
}


/*
 * send_eoi
 *   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *   INPUTS: irq_num - the irq coming from master/slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: issues an EOI to PIC chips at tend of IRQ-based interrupt routine 
 */   
void send_eoi(uint32_t irq_num) {
    // OOB edge case (must be between 0 and 15)
    if(irq_num < 0 || irq_num > 15) return;

    // if master
    if(irq_num < 8 && irq_num >= 0){
        outb(EOI | irq_num, MASTER_8259_PORT);        // This gets OR'd with the interrupt number and sent out to the PIC
    }
    // if slave
    else{
        irq_num -= 8;               // to get w/in proper bounds for shifting
        outb(EOI | irq_num, SLAVE_8259_PORT);

        // also need to mask IR2 on master per lecture, so OR w/ 2 (0x2)
        outb(EOI | 0x2, MASTER_8259_PORT);
    }
}
