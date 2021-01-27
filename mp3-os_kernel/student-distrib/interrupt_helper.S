.text

.globl KEYBOARD_INTERRUPT, RTC_INTERRUPT, PIT_INTERRUPT

# RTC_INTERRUPT(void);
# Interrupt called for RTC
# Inputs   : none
# Outputs  : none
RTC_INTERRUPT:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags

        call rtc_handler

        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context


# KEYBOARD_INTERRUPT(void);
# Interrupt called when key is pressed. Pushes all the registers/flags and pops them to save state
# Inputs   : none
# Outputs  : none
KEYBOARD_INTERRUPT:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags

        call keyboard_handler

        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context


# PIT_INTERRUPT(void);
# Interrupt called during PIT interrupt. Pushes all the registers/flags and pops them to save state
# Inputs   : none
# Outputs  : none
PIT_INTERRUPT:
        cli         # begin critical section
        pushal      # pushing all registers
        pushfl      # pushing all flags

        call PIT_handler

        popfl       # popping all flags
        popal       # popping all registers
        sti         # end critical section
        iret        # per osdev, need iret since interrupt context
