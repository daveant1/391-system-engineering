#include "exceptions.h"
#include "syscall.h"

/* array of exception messages indexed by vector created 
*  based on titles of exceptions in IA-32 manual section 5.14 - 20 of them
*/
const char* exception_msgs[20] = {
    "Divide Error Exception",
    "Debug Exception",
    "Non-maskable Interrupt",
    "Breakpoint Exception",
    "Overflow Exception",
    "BOUND Range Exceeded Exception",
    "Invalid Opcode Exception",
    "Device Not Available Exception",
    "Double Fault Exception",
    "Coprocessor Segment Overrun",
    "Invalid TSS Exception",
    "Segment Not Present Exception",
    "Stack Fault Exception",
    "General Protection Exception",
    "Page-Fault Exception",
    "Reserved",
    "x87 FPU Floating-Point Error",
    "Alignment Check Exception",
    "Machine-Check Exception",
    "SIMD Floating-Point Exception"
};

/* void handle_exception()
 * Central exception handler for all exception
 * Inputs: vec - vector number used to call interrupt handler
 * Outputs: None
 * Side Effects: Prints message containing vector and corresponding int message
 */
void handle_exception(unsigned vec){
    // switch_terminals(0);        // hopefully works?
    clear();
    printf("Interrupt vector 0x%x -- %s\n", vec, exception_msgs[vec]);

    // if page fault, I want to print out the stack and CR2
    int ESP;
    int CR2;

    // save registers into variables
    asm volatile(
        "movl %%esp, %0;"
        "movl %%cr2, %1;"
        
        : "=r" (ESP), "=r" (CR2)
    );

    printf("ESP: %x\n", ESP);
    printf("CR2: %x\n", CR2);

    // check if user exception - if so, must halt
    // 3 sig bit represents U/S (from ESP, per Intel Manual page 188) - if this bit = 1, its user mode
    // need to mask by AND 0x4, rshifting twice
    int test = ((ESP & 0x4) >> 2);
    if(test == 1){
        exception_flag = 1;     // make sys_halt aware that this was an exception
        sys_halt(255);          // random number, won't matter since exception flag
    }

    // exception_flag = 1;     // make sys_halt aware that this was an exception
    // sys_halt(255);          // random number, won't matter since exception flag

    while(1);
}

/*Definition of exception handlers - just routed through handle_exception function*/
void divide_err_excep() { handle_exception(0); }
void debug_excep() { handle_exception(1); }
void nmi_excep() { handle_exception(2); }
void breakpoint_excep() { handle_exception(3); }
void overflow_excep() { handle_exception(4); }
void bound_range_excep() { handle_exception(5); }
void invalid_opcode_excep() { handle_exception(6); }
void device_not_avail_excep() { handle_exception(7); }
void double_fault_excep() { handle_exception(8); }
void coproc_seg_overrun_excep() { handle_exception(9); }
void invalid_tss_excep() { handle_exception(10); }
void seg_not_present_excep() { handle_exception(11); }
void stack_seg_fault_excep() { handle_exception(12); }
void general_protection_excep() { handle_exception(13); }
void page_fault_excep() { handle_exception(14); }
void fpu_float_err_excep() { handle_exception(16); }
void align_check_excep() { handle_exception(17); }
void machine_check_excep() { handle_exception(18); }
void simd_float_err_excep() { handle_exception(19); }

/* System call handler definition */
void system_call() { handle_system_call(); }
