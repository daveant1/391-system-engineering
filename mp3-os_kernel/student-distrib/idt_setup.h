#ifndef _IDT_SETUP_H
#define _IDT_SETUP_H

#include "x86_desc.h"

/* sets up and fills interrupt descriptor table entries/gates */
void idt_setup(void);

// for assembly linkage for exceptions
void DIVIDE_ERR_EXCEP(void);
void DEBUG_EXCEP(void);
void NMI_EXCEP(void);
void BREAKPOINT_EXCEP(void);
void OVERFLOW_EXCEP(void);
void BOUND_RANGE_EXCEP(void);
void INVALID_OPCODE_EXCEP(void);
void DEVICE_NOT_AVAIL_EXCEP(void);
void DOUBLE_FAULT_EXCEP(void);
void COPROC_SEG_OVERRUN_EXCEP(void);
void INVALID_TSS_EXCEP(void);
void SEG_NOT_PRESENT_EXCEP(void);
void STACK_SEG_FAULT_EXCEP(void);
void GENERAL_PROTECTION_EXCEP(void);
void PAGE_FAULT_EXCEP(void);
void FPU_FLOAT_ERR_EXCEP(void);
void ALIGN_CHECK_EXCEP(void);
void MACHINE_CHECK_EXCEP(void);
void SIMD_FLOAT_ERR_EXCEP(void);

// for assembly linkage for interrupts
void KEYBOARD_INTERRUPT(void);
void RTC_INTERRUPT(void);
void PIT_INTERRUPT(void);

//for assembly linkage for system calls
void SYSTEM_CALL_WRAPPER(void);

#endif
