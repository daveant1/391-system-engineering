#include "idt_setup.h"

/* void idt_setup()
 * Sets up interrupt descriptor table for inital boot
 * Inputs: None
 * Outputs: None
 * Side Effects: Fills IDT entries in table
 */
void idt_setup() {
    unsigned i;
    for(i = 0; i < NUM_VEC; i++) {  //offset_15_00 and offset_31_16 missing bc set by SET_IDT_ENTRY
        idt[i].seg_selector = KERNEL_CS;            // set all to ints to be run via kernel code seg
        idt[i].reserved4 = 0;                       // reserved fields specify gate type (reserved4 is 3-bit field set to 0 for interrupts)
        idt[i].reserved3 = (i < 32) ? 1 : 0;        // 01110b = interrupt gate, 01111b = trap gate (if <32, must set to 1 since exception)
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        idt[i].size      = 1;                       // =1 for 32-bit gate size
        idt[i].reserved0 = 0;
        idt[i].dpl       = (i == 0x80) ? 3 : 1;     // if executing system call (vector 0x80) we must be in user privilege (3)
        idt[i].present   = 1;                       // all idt desc slots are empty before SET_IDT_ENTRY called
    }

    //str for SET_IDT_ENTRY is idt_desc_t struct
    SET_IDT_ENTRY(idt[0x00], DIVIDE_ERR_EXCEP);
    SET_IDT_ENTRY(idt[0x01], DEBUG_EXCEP);
    SET_IDT_ENTRY(idt[0x02], NMI_EXCEP);
    SET_IDT_ENTRY(idt[0x03], BREAKPOINT_EXCEP);
    SET_IDT_ENTRY(idt[0x04], OVERFLOW_EXCEP);
    SET_IDT_ENTRY(idt[0x05], BOUND_RANGE_EXCEP);
    SET_IDT_ENTRY(idt[0x06], INVALID_OPCODE_EXCEP);
    SET_IDT_ENTRY(idt[0x07], DEVICE_NOT_AVAIL_EXCEP);
    SET_IDT_ENTRY(idt[0x08], DOUBLE_FAULT_EXCEP);
    SET_IDT_ENTRY(idt[0x09], COPROC_SEG_OVERRUN_EXCEP);
    SET_IDT_ENTRY(idt[0x0A], INVALID_TSS_EXCEP);
    SET_IDT_ENTRY(idt[0x0B], SEG_NOT_PRESENT_EXCEP);
    SET_IDT_ENTRY(idt[0x0C], STACK_SEG_FAULT_EXCEP);
    SET_IDT_ENTRY(idt[0x0D], GENERAL_PROTECTION_EXCEP);
    SET_IDT_ENTRY(idt[0x0E], PAGE_FAULT_EXCEP);
    //0x0F entry intel-reserved
    SET_IDT_ENTRY(idt[0x10], FPU_FLOAT_ERR_EXCEP);
    SET_IDT_ENTRY(idt[0x11], ALIGN_CHECK_EXCEP);
    SET_IDT_ENTRY(idt[0x12], MACHINE_CHECK_EXCEP);
    SET_IDT_ENTRY(idt[0x13], SIMD_FLOAT_ERR_EXCEP);

    // interrupt entries
    SET_IDT_ENTRY(idt[0x20], PIT_INTERRUPT);
    SET_IDT_ENTRY(idt[0x21], KEYBOARD_INTERRUPT);
    SET_IDT_ENTRY(idt[0x28], RTC_INTERRUPT);
    
    //System call entry
    SET_IDT_ENTRY(idt[0x80], SYSTEM_CALL_WRAPPER);
}

