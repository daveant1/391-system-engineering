.text

.globl  DIVIDE_ERR_EXCEP, DEBUG_EXCEP, NMI_EXCEP, BREAKPOINT_EXCEP, OVERFLOW_EXCEP, BOUND_RANGE_EXCEP, INVALID_OPCODE_EXCEP, DEVICE_NOT_AVAIL_EXCEP, DOUBLE_FAULT_EXCEP 
.globl  COPROC_SEG_OVERRUN_EXCEP, INVALID_TSS_EXCEP, SEG_NOT_PRESENT_EXCEP, STACK_SEG_FAULT_EXCEP, GENERAL_PROTECTION_EXCEP, PAGE_FAULT_EXCEP, FPU_FLOAT_ERR_EXCEP
.globl  ALIGN_CHECK_EXCEP, MACHINE_CHECK_EXCEP, SIMD_FLOAT_ERR_EXCEP

# All functions included below are simply assembly linkage functions that call corresponding handlers in exceptions.c/h
# I have only included a header for exception #0 as it should be obvious that each function performs the same thing

# DIVIDE_ERR_EXCEP(void);
# Assembly linkage for exception handler triggered by divide error
# Inputs   : none
# Outputs  : none
DIVIDE_ERR_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags

        call divide_err_excep

        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

DEBUG_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call debug_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

NMI_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call nmi_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

BREAKPOINT_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call breakpoint_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

OVERFLOW_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call overflow_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

BOUND_RANGE_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call bound_range_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

INVALID_OPCODE_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call invalid_opcode_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

DEVICE_NOT_AVAIL_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call device_not_avail_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

DOUBLE_FAULT_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call double_fault_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

COPROC_SEG_OVERRUN_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call coproc_seg_overrun_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

INVALID_TSS_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call invalid_tss_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

SEG_NOT_PRESENT_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call seg_not_present_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

STACK_SEG_FAULT_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call stack_seg_fault_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

GENERAL_PROTECTION_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call general_protection_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

PAGE_FAULT_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call page_fault_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

FPU_FLOAT_ERR_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call fpu_float_err_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

ALIGN_CHECK_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call align_check_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

MACHINE_CHECK_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call machine_check_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context

SIMD_FLOAT_ERR_EXCEP:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags
        call simd_float_err_excep
        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context
