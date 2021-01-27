/* paging.S - set up page directory, page table, and pages */

#include "paging.h"
// #include "lib.h"
// #include "terminal.h"

int32_t video_pages[3] = {TERM_1_VIDPAGE, TERM_2_VIDPAGE, TERM_3_VIDPAGE};

/* void init_paging(void) - initializes the page table and page directory
 * Inputs   : none
 * Outputs  : none
 * Side Effects : maps video memory and kernel
 */
void init_paging(void) {

    /* PAGE DIRECTORY INITIALIZATION */
    /* initialize same stuff first (change for vid memory and kernel later) */
    int i;
    for (i = 0; i < NUM_ENTRIES; i++) {
        page_directory[i].P = 0;        // mark as absent
        page_directory[i].R = 1;        // everything should be R/W
        page_directory[i].U = 1;        // page should be able to be accessed by all
        page_directory[i].W = 0;        // write-back is enabled
        page_directory[i].D = 1;        // check descriptors file: should be 1 for kernel and program pages
        page_directory[i].A = 0;        // none of these have been accessed yet
        page_directory[i].S = 1;        // pages are 4 MiB in size (first PDE will eventually be set to 4kB)

        page_directory[i].offset31_12 = 0;  // set to NULL initially since these point to nowhere
    }

    /* PAGE TABLE INITIALIZATION - we need to initialize this first because we need this mapping for PD */
    for (i = 0; i < NUM_ENTRIES; i++) {
        // check descriptors file for reason behind why bits are set this way
        page_table[i].P = 0;            // mark as absent
        page_table[i].R = 1;            // everything should be R/W
        page_table[i].U = 1;            // page should be able to be accessed by all
        page_table[i].W = 0;            // write-back is enabled
        page_table[i].C = 1;            // page-cached disabled should be set to 1??
        page_table[i].A = 0;            // none of these accessed yet
        page_table[i].D = 0;            // hasn't been written to yet

        page_table[i].offset31_12 = 0;  // set to NULL initially
    }

    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].P = 1;
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].U = 0; // vid mem should be set to supervisor only since it is kernel mapping
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].C = 0; // vid mem contains memory mapped I/O and shouldn't be cached
    page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].offset31_12 = VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB; // vid mem contains memory mapped I/O and shouldn't be cached

    /* first PDE should be for video memory */
    page_directory[0].P = 1;        // mark as present
    page_directory[0].S = 0;        // pages are 4 kB ONLY for this first PDE (due to video memory)
    page_directory[0].offset31_12 = ((uint32_t) page_table >> ADDRESS_SHIFT_KB) & 0xFFF;    // set bits 31-12 to address of page table (right shifted)

    
    /* second PDE should be for kernel (at 4MB - 0x400000 for physical and virtual) */
    // page_directory[1].offset31_12 = KERNEL_ADDRESS >> ADDRESS_SHIFT_KB; 
    page_directory[1].offset31_12 = 1024;   // 1024 because 2^10 in bits 31-12 means 1 for bit 22 => points to second PDE (index 1)
    page_directory[1].P = 1;        // mark as present
    page_directory[1].U = 0;        // kernel stuff should be supervisor only

    flush_tlb();

    asm volatile(
        // load page dir address into CR3 
        "movl %0, %%eax;"
        "movl %%eax, %%cr3;"

        // allow mixed page sizes: set PSE, bit 4 in CR4
        "movl %%cr4, %%eax;"
        "orl $0x00000010, %%eax;"
        "movl %%eax, %%cr4;"

        // set bit-31 of CR0 to enable paging
        "movl %%cr0, %%eax;"
        "orl $0x80000001, %%eax;"
        "movl %%eax, %%cr0"
        
        :
        : "r" (page_directory) 
        : "eax", "cc"
    );
}

/* void flush_tlb() - TLB needs to be flushed when CR3 reloaded
 * Inputs : none
 * Outputs: none
 * Clobbers : memory (since we performed memory reads or writes to items other than those listed in the input and output operands)
 *          : cc (modified flag registers)
 */
void flush_tlb(void) {
    asm volatile(
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        : 
        : "eax"     // clobbered
    );
    return;
}

/* void map_user_program()
 * 
 * Inputs   : pid
 * Outputs  : none
 * Side Effects : maps the user program from 128MB - 132MB in virtual memory to either 8MB-12MB, 12MB-16MB, etc. in physical memory
 *                if pid == 0, then map to 8-12MB; if pid == 1, then map to 12-16MB, etc.
 * 
 */
void map_user_program(int pid) {
    /* mapping */
    // index 32 because user program starts at 128MB in virtual memory and each "index" chunk is 4MB. 128MB/4MB => 32
    page_directory[32].P = 1;   // mark as present
    // must be user level access -> but might already be set
    page_directory[32].U = 1;   // accessible by all
    // R/W accessible
    page_directory[32].R = 1;
    page_directory[32].S = 1;
    // might need to set more bits?
    page_directory[32].offset31_12 = (pid + 2) * FOUR_MB_OFFSET;    // we add 2 because the first 0-8MB are taken up already

    /* flush the tlb */
    flush_tlb();
}


/* void map_vidmem()
 * Inputs   : none
 * Outputs  : none
 * Side Effects : always map user program to same location in physical memory
 */
void map_vidmem() {
    // index 33 because we need to place somewhere after user-level process memory (132MB+)
    page_directory[33].P = 1;   // mark as present
    // must be user level access -> but might already be set
    page_directory[33].U = 1;   // accessible by all
    // R/W accessible
    page_directory[33].R = 1;
    page_directory[33].S = 0; 
    page_directory[33].offset31_12 = (uint32_t)vidmap_page_table >> ADDRESS_SHIFT_KB;

    vidmap_page_table[0].P = 1;
    vidmap_page_table[0].U = 1;
    vidmap_page_table[0].R = 1;
    vidmap_page_table[0].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB);

    flush_tlb();
}

/* void scheduling_vidmap(int terminal) - map each video page to its corresponding to backup buffer
 * Inputs   : int terminal - represents which backup buffer to map to
 * Outputs  : none
 * Side Effects : none
 */ 
void scheduling_vidmap(int terminal, int curr_term) {

    // map_vidmem();
    page_directory[33].P = 1;   // mark as present
    // must be user level access -> but might already be set
    page_directory[33].U = 1;   // accessible by all
    // R/W accessible
    page_directory[33].R = 1;
    page_directory[33].S = 0; 
    page_directory[33].offset31_12 = (uint32_t)vidmap_page_table >> ADDRESS_SHIFT_KB;

    if(terminal != curr_term){
        vidmap_page_table[0].P = 1;
        vidmap_page_table[0].U = 1;
        vidmap_page_table[0].R = 1;
        vidmap_page_table[0].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB) + (terminal + 1);
    }
    else{
        vidmap_page_table[0].P = 1;
        vidmap_page_table[0].U = 1;
        vidmap_page_table[0].R = 1;
        vidmap_page_table[0].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB);
    }

    if(terminal != curr_term){
        page_table[video_pages[terminal] >> ADDRESS_SHIFT_KB].P = 1;
        page_table[video_pages[terminal] >> ADDRESS_SHIFT_KB].U = 1;
        page_table[video_pages[terminal] >> ADDRESS_SHIFT_KB].R = 1;
        page_table[video_pages[terminal] >> ADDRESS_SHIFT_KB].offset31_12 = (video_pages[terminal] >> ADDRESS_SHIFT_KB);
        // page_table[video_pages[terminal] >> ADDRESS_SHIFT_KB].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB) + (terminal + 1);
    }
    else{
        page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].P = 1;
        page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].U = 1;
        page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].R = 1;
        page_table[VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB);
    }

    flush_tlb();
}

// **NEW**
/* void map_vidmem() - maps a new 4kB chunk in virtual memory to the original 4kB video memory page in physical address */
void vidmap_term(int term_id) {

    page_table[(VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB)+term_id+1].P = 1;
    page_table[(VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB)+term_id+1].U = 1;
    page_table[(VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB)+term_id+1].R = 1;
    page_table[(VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB)+term_id+1].offset31_12 = (VIDMEM_ADDRESS >> ADDRESS_SHIFT_KB) + (term_id + 1);

    flush_tlb();
}
