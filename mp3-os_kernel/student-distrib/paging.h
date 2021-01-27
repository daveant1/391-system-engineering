/* paging.h - Defines for various paging stuff: paging directory, paging table, etc.
 */

#include "x86_desc.h"   // most likely???

/* macros */
#define NUM_ENTRIES         1024        // since we have 2^10 = 1024 entries for both the PD and PT
#define PAGING_ALIGNMENT    4096        // align to the nearest 4 kB (minimal page size)
#define VIDMEM_ADDRESS      0x000B8000
#define KERNEL_ADDRESS      0x00400000

#define ADDRESS_SHIFT_KB    12          // only need top 20 bits (so shift right 12) to find where 4kB pages are
#define ADDRESS_SHIFT_MB    22          // only need top 10 bits (so shift right 22) to find where 4MB pages are

#define FOUR_MB_OFFSET      1024        // 4MB has a difference of 1024 when right shifted by 12 (used in map_user_prog)
#define FOUR_KB             0x1000
#define FOUR_MB             0x400000

#define TERM_1_VIDPAGE 0x000B9000
#define TERM_2_VIDPAGE 0x000BA000
#define TERM_3_VIDPAGE 0x000BB000

/* struct for Page Directory Entries */
typedef struct pde {
    /* using union and struct here gives us the option to use both between 4MB and 4kB pages */
    union {
        //uint32_t entry_value;
        struct {
            uint32_t P              : 1;    // Present bit
            uint32_t R              : 1;    // Read/Write Permissions flag
            uint32_t U              : 1;    // User/Supervisor bit
            uint32_t W              : 1;    // Write Through enabling
            uint32_t D              : 1;    // cache disabled bit
            uint32_t A              : 1;    // accessed bit
            uint32_t bit6           : 1;    // unused bit
            uint32_t S              : 1;    // Page Size (if set, pages are 4MB, else 4kB)
            uint32_t G              : 1;    // ignored bit
            uint32_t offset11_9     : 3;
            uint32_t offset31_12    : 20;
        } __attribute__ ((packed));
    };
} pde_t;

/* struct for each Page Table Entry (only one for the video memory block) */
typedef struct __attribute__((packed)) pte {
    uint32_t P              : 1;    // Present bit
    uint32_t R              : 1;    // Read/Write Permissions flag
    uint32_t U              : 1;    // User/Supervisor bit
    uint32_t W              : 1;    // Write Through enabling
    uint32_t C              : 1;    // 'Cached', is the D bit from PDE (because we use D as dirty flag, see below)
    uint32_t A              : 1;    // accessed bit
    uint32_t D              : 1;    // dirty flag
    uint32_t bit7           : 1;    // unused bit
    uint32_t G              : 1;    // global flag
    uint32_t offset11_9     : 3;
    uint32_t offset31_12    : 20;
} pte_t;

/* global variables */
pte_t page_table[NUM_ENTRIES] __attribute__ ((aligned (PAGING_ALIGNMENT)));     // 0 to 4
pde_t page_directory[NUM_ENTRIES] __attribute__((aligned (PAGING_ALIGNMENT)));
pte_t vidmap_page_table[NUM_ENTRIES] __attribute__ ((aligned (PAGING_ALIGNMENT)));          // 128 to 132

/* functions in paging.c */
void init_paging(void);
extern void flush_tlb(void);
void map_user_program(int pid);
void map_vidmem(void);
void vidmap_term(int term_id);
// void scheduling_vidmap(int terminal);
void scheduling_vidmap(int terminal, int curr_term);
//void map_vidmem(uint8_t** screen_start, int pid);
