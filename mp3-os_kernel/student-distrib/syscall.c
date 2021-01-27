
#include "syscall.h"

// initialize the file operations table 
fops_t std_in_table = { bad_open, terminal_read, bad_write, bad_close};
fops_t std_out_table = {bad_open, bad_read, terminal_write, bad_close};
fops_t rtc_table = {rtc_open, rtc_read, rtc_write, rtc_close};
fops_t filesys_table = {file_open, file_read, file_write, file_close};
fops_t filedir_table = {dir_open, dir_read, dir_write, dir_close};
fops_t bad_table = {bad_open, bad_read, bad_write, bad_close};

/* local variables */


/* void handle_system_call()
 * Temporary system call handler 
 * Inputs: None
 * Outputs: None
 * Side Effects: Prints "System call was called."
 */
void handle_system_call(){
    clear();
    printf("System call (vector 0x80) was called.\n");
    while(1);
}

/* void syscall_init() - set up structures and local variables used in passing in parameters for system calls
 * Inputs: None
 * Outputs: None
 * Side Effects: initializes pid_array to unused for all entries
 */
void syscall_init() {
    int i;
    for (i = 0; i < MAX_NUM_PIDS; i++) {
        pid_array[i] = UNUSED;
    }
}


/* int32_t sys_halt(uint8_t status) 
 * Inputs: status - some number 0-255 and indicates status of the executable?
 * Outputs: returns a value to the parent execute system call so that we know how the program ended
 * Side Effects: Closes any relevant FDs, handles exiting first shell(s), 
 *                  performs context switching, restores pcb, esp/ebp, and parent paging
 */
int32_t sys_halt (uint8_t status){
    cli();

    int32_t return_val;
    int parent_esp, parent_ebp;

    // meaning an exception on user side was generated -> squash!
    if(exception_flag == 1){
        exception_flag = 0;
        return_val = 256;       // also need to return 256
    }
    else{
        return_val = (int32_t)status;       // just return normal status
    }

    pcb_t* curr_pcb = get_pcb_from_pid(curr_pid);


    // Set pids to unused
    pid_array[curr_pcb->curr_pid] = NOT_IN_USE;

    int i;
    // close any relevant FDs
    // we start at 2 because of stdin and stdout
    for (i = 2; i < FDA_SIZE; i++) {
        sys_close(i);
    }

    // if first shell, restart a new one
    // original was curr_pid == 0
    if(curr_pid <= 2){
        sys_execute((uint8_t*)"shell");
    }

    parent_ebp = curr_pcb->old_ebp;
    parent_esp = curr_pcb->old_esp;
    
    // restore tss values and EBP/ESP values
    tss.esp0 = 0x800000 - (curr_pcb->parent_pid) * 0x2000;      // 8MB - (pid)*8kB
    tss.ss0 = KERNEL_DS;        // unsure if this is needed? halt worked fine without

    // restore parent paging
    map_user_program(curr_pcb->parent_pid);

    /* Update terminal and pcb's active pid to other settings*/
    terminals[curr_pcb->term_id].active_pid = curr_pcb->parent_pid;
    scheduling_array[curr_pcb->term_id] = curr_pcb->parent_pid;

    // restore pcb
    curr_pid = curr_pcb->parent_pid;
    curr_pcb = get_pcb_from_pid(curr_pid);

    asm volatile(
        // literally save ebp and esp into (free to clobber) registers
        "movl %0, %%eax;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "sti;"
        "jmp RETURN_FROM_HALT;"
        
        : // no outputs
        : "r"((int32_t)return_val), "r"(parent_ebp), "r"(parent_esp)       // we booling now
        : "eax"
    );

    // remember to go back to end of execute and set the return value accordingly (always returns 0 right now)
    return 0;
}
    

/* int32_t sys_execute (const uint8_t command)
 * Attempts to load and execute a new program -> handing off the processor to the new program until it terminates
 * Inputs: uint8_t command - reads command to know what to do
 * Outputs: -1 if command cannot be executed
 *          256 if program dies by an exception
 *          [0,255] if program executes a halt syscall (value returned given by call to halt)
 * Side Effects: Does a lot of stuff
 */
int32_t sys_execute (const uint8_t* command){

    // begin critical section
    cli();

    // STEP 1: parse the command
    int i;
    int return_val;

    if(command == NULL){
        return -1;
    }

    // check if command is \n or \n\n
    if(command[0] == '\n' || command[1] == '\n'){
        return 0;
    }

    int8_t cmd[MAX_CMD_LENGTH];                 // get first command
    uint8_t args[MAX_ARGS_LENGTH];              // get following arguments
    uint8_t idx = 0;                            // to identify spaces (also to index cmd)
    uint8_t cmd_idx = 0;
    uint8_t args_idx = 0;                       // to index args

    // initialize cmd and args
    for(i = 0; i < MAX_CMD_LENGTH; i++){
        cmd[i] = '\0';
        args[i] = '\0';
    }

    // if leading spaces, need to move idx up until we reach a nonchar
    if(command[0] == ' '){
        while(command[idx] == ' '){
            idx++;
        }
    }

    // now proceed to copy command
    while(command[idx] != ' ' && command[idx] != '\0'){
        cmd[cmd_idx++] = command[idx++];        // copy command into cmd until first space
    }

    while(command[idx] == ' '){
        idx++;                          // skip spaces until we reach argument
    }

    while(command[idx] != '\0'){
        args[args_idx++] = command[idx++];      // copy rest of command (nonspace) into args
    }


    // STEP 2: check file validity

    // each process points to a dentry object (which points to inode object)
    dentry_t d;
	if(read_dentry_by_name(cmd, &d) == -1){
        return -1;                              // return -1 since dentry does not exist
    }

    // read header (found at start of ELF file)
    uint8_t read_buf[4];                               // only need 4 because of the magic numbers
    uint32_t inode_idx = d.inode_num;                   // get inode from read_dentry
    read_data(inode_idx, 0, read_buf, 4);               // again, only 4 magic numbers, read into read_buf

    // now check if executable file
    // magic numbers from https://wiki.osdev.org/ELF (0x7F, E, L, F) -> bits 0 to 3
    if(read_buf[0] != 0x7F || read_buf[1] != 'E' || read_buf[2] != 'L' || read_buf[3] != 'F'){
        return -1;      // return -1 since magic numbers dont match what ELF should be
    }

    // now obtain program entry position (bits 24-27) - also 4 bits so we can overwrite read_buf
    read_data(inode_idx, 24, read_buf, 4);              // read 4 bc only 4 bits to read
    uint32_t entry_position = (uint32_t)(read_buf[3] << 24 | read_buf[2] << 16 | read_buf[1] << 8 | read_buf[0]);   // because of little endian format, need to read in data backwards

    // STEP 3: Paging

    // find an unused pid (0-5)
    int pid;
    for (pid = 0; pid < MAX_NUM_PIDS; pid++) {
        if (pid_array[pid] == UNUSED) {
            pid_array[pid] = USED;
            break;
        }
        // if pid_array is full
        if (pid == MAX_NUM_PIDS - 1) {
            return -1;
        }
    }
    // if pid = 0, first shell -> tell keyboard
    shell_flag = 1;

    // tell keyboard that another process is running
    if(pid != 0){
        running_flag = 1;
    }

    // map user program
    map_user_program(pid);

    // STEP 4: user level program loader
    uint8_t* usr_buf = (uint8_t*)0x8048000;                 // memory 
    uint32_t exe_length = ((inode_t*)(fs_start_addr + (inode_idx + 1) * BLOCK_SIZE))->length;       // get length
    read_data(inode_idx, 0, usr_buf, exe_length);       // load data into usr_buf
                                                        // offset of 0 since we want to read in from byte 0 (start of executable)
    // already have entry position from step 2

    // STEP 5: PCB

    // helper function to make/initialize a PCB
    pcb_t* curr_pcb = init_pcb(pid, args);

    /*Initalize term id and update active pid of curr_term*/
    int curr_esp;
    int curr_ebp;
    
    // save current EBP and ESP registers into PCB before we change
    asm volatile(
        // literally save ebp and esp into (free to clobber) registers
        "mov %%esp, %0;"
        "mov %%ebp, %1;"
        
        : "=r" (curr_esp), "=r" (curr_ebp)        // bro is this how we read registers im losing my minddddddd
    );

    // case for booting up
    if(pid <= 2){
        curr_pcb->term_id = scheduled_process;
        terminals[scheduled_process].active_pid = pid;
        scheduling_array[scheduled_process] = pid;

        // shells will never halt -> only important is schedule esp/ebp
        curr_pcb->old_esp0 = tss.esp0;          // dont think this matters
        curr_pcb->schedule_esp = curr_esp;
        curr_pcb->schedule_ebp = curr_ebp;

        curr_pcb->old_ebp = curr_ebp;
        curr_pcb->old_esp = curr_esp;

    }
    // case for normal scheduling
    else{
        curr_pcb->parent_pid = scheduling_array[curr_term];
        pcb_t* parent_pcb = get_pcb_from_pid(scheduling_array[curr_term]); // retrieve parent program's pcb
        parent_pcb->child_pid = pid;

        curr_pcb->term_id = curr_term;
        terminals[curr_term].active_pid = pid;
        scheduling_array[curr_term] = pid;

        // scheduling and execute ebp/esp are DIFFERENT
        curr_pcb->old_esp0 = tss.esp0;      // again this may not matter
        curr_pcb->schedule_esp = curr_pcb->base_kernel_stack;
        curr_pcb->schedule_ebp = curr_pcb->schedule_esp;

        curr_pcb->old_ebp = curr_ebp;
        curr_pcb->old_esp = curr_esp;

    }

    // update current pid
    curr_pid = pid;

    // STEP 6: Context Switch - home stretch ?

    // prepare for context switch: write new process' info to TSS
    tss.ss0 = KERNEL_DS;                       
    tss.esp0 = curr_pcb->base_kernel_stack;     // already calculated

    /* OSDEV Order
    user data segment
    push current esp
    pushf (?)
    user code segment
    */

    asm volatile(
        // look at x86_desc.h macros for the values below. also just follow osdev [order]
        "pushl $0x002B;"            // push user data segement (SS)
        //"pushl %%esp;"              // push ESP -> wrong because we are pushing the kernel stack address. ESP here is 0x7ffd04 -> definitely wrong
                                    // not what we want because we are switching to user stack
                                    // We want the user's stack pointer (virt. mem.) 
                                    // should the user's stack pointer less than 132 MB (132 MB - 4 B) -> so that it doesn't go outside the page
        "movl $0x83FFFFC, %%eax;"          // 132 MB - 4 B => 0x8400000 - 0x4 => 0x83FFFFC
        "pushl %%eax;"

        "pushfl;"                   // push flags (EFLAGS)
        "popl %%edx;"                // save flags into edx
        "orl $0x200, %%edx;"        // or with x200 to enable flags
        "pushl %%edx;"              // finally push register

        "pushl $0x0023;"            // push user code segment (CS)
        "movl %0, %%eax;"
        "pushl %%eax;"              // push the entry position (EIP) -> must be somewhere around 128 MB

        "iret;"                     // iret has to happen
        
        "RETURN_FROM_HALT:;"         // this is where we jump from halt (per discussion)
        "movl %%eax, %1;"
        
        : "=r" (return_val)
        : "r" (entry_position)
        : "eax", "edx"
    );


    /* this is THE approach */
    /* set up correct values for user-level EIP, CS, EFLAGS, ESP, and SS registers on the kernel-mode stack */
    /* then execute iret (allows for kernel -> user) instruction */

    return return_val;
}

/* int32_t sys_read(int32_t fd, void* buf, int32_t nbytes)
 * Checks for valid fd input and reads by calling proper read() function ptr
 * Inputs:  int32_t fd - index of fd to use to read
 *          void* buf - ptr to buffer we read from
 *          int32_t nbytes - number of bytes to read
 * Outputs: -1 if invalid argument or fd not initialized
 */
int32_t sys_read (int32_t fd, void* buf, int32_t nbytes){
    //I need access to curr_pcb
    pcb_t* curr_pcb;
    /* Check bounds of fd idx */
    if (fd < 0 || fd >= FDA_SIZE)
        return -1;
    
    // get pcb_ptr
    curr_pcb = get_pcb_ptr();

    /* Check valid buf input */
    if (buf == NULL)
        return -1;
    /* Check if fd index corresponds to valid fd */
    if (curr_pcb->fda[fd].flags == NOT_IN_USE)
        return -1;

    return curr_pcb->fda[fd].fops_ptr.read(fd, buf, nbytes);
}

/* int32_t sys_write(int32_t fd, void* buf, int32_t nbytes)
 * Checks for valid fd input and writes by calling proper write() function ptr
 * Inputs:  int32_t fd - index of fd to use to write
 *          void* buf - ptr to buffer we write from
 *          int32_t nbytes - number of bytes to write
 * Outputs: -1 if invalid argument or fd not initialized
 */
int32_t sys_write (int32_t fd, void* buf, int32_t nbytes){
    //I need access to curr_pcb
    pcb_t* curr_pcb;
    /* Check bounds of fd idx */
    if (fd < 0 || fd >= FDA_SIZE)
        return -1;

    // get pcb_ptr
    curr_pcb = get_pcb_ptr(); // this might be wrong -> nvm, I stepped through and values look correct

    /* Check valid buf input */
    if (buf == NULL)
        return -1;
    /* Check if fd index corresponds to valid fd */
    if (curr_pcb->fda[fd].flags == NOT_IN_USE)
        return -1;

    return curr_pcb->fda[fd].fops_ptr.write(fd, buf, nbytes);
}

/* int32_t sys_open (const uint8_t* filename)
 * Checks for valid filename input; Find empty entry in fda for pcb and initialize fd entry; call proper open() function ptr
 * Inputs:  const uint8_t* filename - name of file to open and create fd entry for
 * Outputs: -1 if invalid argument/entry does not exist, no empty entry in fda, or unsuccessful open() fptr call
 *           return fd index of new fd entry opened otherwise
 * Side Effects: Opens filename and sets up fda entry for pcb for this file descriptor
 */
int32_t sys_open (const uint8_t* filename){
    // need to return -1 if (File Descriptor) array is full - see appendix A 8.2 last sentence 
    //I need access to curr_pcb
    pcb_t* curr_pcb;
    dentry_t test_dentry;
    /*Check if named file exists*/
    if(read_dentry_by_name((int8_t*)filename, &test_dentry) == -1)
        return -1;

    // get pcb_ptr
    curr_pcb = get_pcb_ptr();
    /*Check if any fd entries are free*/
    int32_t fd_idx;

    // start at 2 since first 2 entries are occupied
    for(fd_idx = 2; fd_idx < FDA_SIZE; fd_idx++){
        /* Check for empty fda entry */
        if(curr_pcb->fda[fd_idx].flags == NOT_IN_USE)
            break;
        if (fd_idx == FDA_SIZE - 1)
            return -1;   
    }
    
    /* Declare and initialize file descriptor to be placed in fda */
    files_t fd_entry;
    switch (test_dentry.filetype){
        case 0:     //RTC filetype
            fd_entry.fops_ptr = rtc_table;
            break;
        case 1:     //Directory filetype
            fd_entry.fops_ptr = filedir_table;
            break;
        case 2:     //Traditional file filetype
            fd_entry.fops_ptr = filesys_table;
            break;
    }
    
    /* Make actual open call and return */
    if(0 != fd_entry.fops_ptr.open(filename)) return -1;

    /* Set up remaining fields for our fda entry */
    fd_entry.inode = test_dentry.inode_num;
    fd_entry.file_position = 0;
    fd_entry.flags = IN_USE;

    /* Finally set entry in fda for curr_pcb */
    curr_pcb->fda[fd_idx] = fd_entry;
    return fd_idx;
}

/* int32_t sys_close (int32_t fd)
 * Checks for valid fd input and closes corresponding fd by calling proper close() function ptr
 * Inputs:  int32_t fd - index of fd to close 
 * Outputs: -1 if invalid fd argument or close does not complete successfully
 * Side Effects: Closes file descriptor input
 */
int32_t sys_close (int32_t fd){
    pcb_t* curr_pcb;

    /* Check bounds of fd idx (must be between 2 and 7)*/
    if (fd < 2 || fd >= FDA_SIZE)
        return -1;

    // get pcb_ptr
    curr_pcb = get_pcb_ptr();

    /* Check if fd already invalid */
    if (curr_pcb->fda[fd].flags == NOT_IN_USE)
        return -1;
    /* Make sure file closes successfully */
    if(curr_pcb->fda[fd].fops_ptr.close(fd) != 0)
        return -1;
    /* Finally set fd entry to not in use */
    curr_pcb->fda[fd].flags = NOT_IN_USE;
    return 0;
}

/* int32_t sys_getargs (uint8_t* buf, int32_t nbytes)
 * Reads the program's command line arguments into a user-level buffer
 * Inputs: uint8_t* buf - user buffer to read arguments into
 *         int32_t nbytes - number of bytes to read
 * Outputs: -1 if no arguments or if arguments and a terminal null do not fit in the buffer
 *          0 on success
 * Side Effects: Does a lot of stuff
 */
int32_t sys_getargs (uint8_t* buf, int32_t nbytes){
    // we will prob need to interact with PCB for this
    if (buf == NULL) return -1;

    if (nbytes <= 0) return -1;

    pcb_t *curr_pcb = get_pcb_ptr();

    if(curr_pcb->args[0] == '\0') return -1;

    // https://stackoverflow.com/questions/38255212/is-it-bad-practice-to-use-memcpy-over-strncpy-or-similar?noredirect=1&lq=1
    strncpy((int8_t*)buf, (int8_t*)(curr_pcb->args), nbytes);

    return 0;
}

/* int32_t sys_vidmap (uint8_t** screen_start)
 * Mps the text-mode video memory into user space at a pre-set virtual address
 * Inputs: uint8_t** screen_start - ptr to ptr to video memory
 * Outputs: address
 *          -1 if location is invalid
 * Side Effects: Does a lot of stuff
 */
int32_t sys_vidmap (uint8_t** screen_start){
    // just check whether the address falls within the address range covered by the single user-level page
    // NOTE: requires us to add anohter page mapping for the program (4kB page)
    /* Make sure screen_start is within virtual addr range for user-level page (128-132MB) */
    if(screen_start < (uint8_t**) ONE28_MB || screen_start >= (uint8_t**) ONE32_MB)  return -1;
    map_vidmem();

    *screen_start = (uint8_t*) ONE32_MB;
    return 0;
}


/* int32_t bad call functions (args depend on function type)
 * Is a bad call function because function pointer does not exist
 * Inputs: none
 * Outputs: -1 by default since bad call
 * Side Effects: none
 */
int32_t bad_open(const uint8_t* filename){
    return -1;
}
int32_t bad_read(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}
int32_t bad_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}
int32_t bad_close(int32_t fd){
    return -1;
}


/* void init_pcb(void)
 * Initializes a given pcb for use
 * Inputs:  pcb_t curr_pcb - pcb to initialize
 *          int pid - identify the process
 *          args - keep track of the arguments saved
 * Outputs: a pointer to the initialized pcb
 * Side Effects: initializes all the attributes of the pcb
 */
pcb_t* init_pcb(int pid, uint8_t* args){
    // initialize all entires to be bad at first
    int i;
    pcb_t* curr_pcb = get_pcb_from_pid(pid);
    for(i = 0; i < FDA_SIZE; i++){
        curr_pcb->fda[i].fops_ptr = bad_table;       // FIXED: CAUSES PAGE FAULT EXCEPTION: because of how we initialized curr_pcb
        
        curr_pcb->fda[i].inode = -1;
        curr_pcb->fda[i].flags = NOT_IN_USE;
        curr_pcb->parent_pid = NULL;
        curr_pcb->child_pid = NULL;
    }

    // initialize first two entries of fda to STDIN and STDOUT
    curr_pcb->fda[0].fops_ptr = std_in_table;
    curr_pcb->fda[0].flags = IN_USE;
    curr_pcb->fda[1].fops_ptr = std_out_table;
    curr_pcb->fda[1].flags = IN_USE;

    // set base kernel stack (depends on the pid)
    curr_pcb->base_kernel_stack = 0x800000 - (pid) * 0x2000;      // 8MB - (pid)*8kB
    curr_pcb->curr_pid = pid;
    curr_pcb->args = args;
    
    curr_pcb->term_id = curr_term;

    return curr_pcb;
}

/* pcb_t* get_pcb_ptr()
 * Gets a pointer to the current pcb based on esp pointer
 * Inputs:  none
 * Outputs: pcb_t pointer to current pcb
 * Side Effects: none
 */
pcb_t* get_pcb_ptr(void){
    pcb_t* curr_ptr;

    asm volatile(
        "movl %0, %%eax;"
        "andl %%esp, %%eax;"      // load 0x2B into task state register
        "movl %%eax, %1;"
        : "=r" (curr_ptr)
        : "r" (PCB_MASK)
    );

    return curr_ptr;
}

/* pcb_t - write a get pcb from pid function. also, when setting flags in line 444, not the same as the pcb addr in sys write */
/* get_pcb_from_pid(int pid)
 * Gets a pointer to the pcb corresponding to a pid
 * Inputs   : pid
 * Outputs  : a pcb pointer
 */
pcb_t* get_pcb_from_pid(int pid) {
    pcb_t* pcb_ptr;
    pcb_ptr = (pcb_t*)(EIGHT_MB - (pid + 1) * EIGHT_KB);
    return pcb_ptr;
}
