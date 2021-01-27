.globl  SYSTEM_CALL_WRAPPER

# SYSTEM_CALL_WRAPPER(void);
# Assembly linkage wrapper for system call handler; triggered by "int 0x80"
# Inputs   : none
# Outputs  : none
SYSTEM_CALL_WRAPPER:
    cli             # begin critical section
    pushfl          # pushing all flags
    pushl %edx      # push argument registers for system call
    pushl %ecx
    pushl %ebx
    sti
    
    cmpl $1, %eax   # check system call index is between 1 and 8 (for 8 system calls total) 
    jb invalid_idx
    cmpl $8, %eax
    ja invalid_idx

    call *jump_table(, %eax, 4) # call corresponding system call from jump table
                                # (index stored in eax * 4 bytes per entry in table)
    jmp cleanup_call
    
invalid_idx: 
    orl $0xFFFFFFFF, %EAX       # store -1 in EAX if invalid index to system call

cleanup_call:
    popl %ebx   # popping all caller-saved registers
    popl %ecx
    popl %edx
    popfl       # popping all flags
    # sti         # end critical section
    iret        # per osdev, need iret since interrupt context

jump_table:     # jump table of system call handlers
    .long 0x0   # empty entry since we want system call index to start at 1, not 0
    .long sys_halt
    .long sys_execute
    .long sys_read
    .long sys_write
    .long sys_open
    .long sys_close
    .long sys_getargs
    .long sys_vidmap
