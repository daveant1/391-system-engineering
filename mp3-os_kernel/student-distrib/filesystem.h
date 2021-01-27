/* filesystems.h - Definitions for various structures and variables used in our filesystem */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "types.h"

/* macros */
#define DENTRY_B_RES        24      // 24 bytes reserved after first three elements of dir. entries
#define BOOTBLOCK_B_RES     52      // 52 bytes reserved after first three elements of boot block
#define FILENAME_LEN        32
#define NUM_DATA_BLOCKS     1023    // 1023 because each block is 4kB
                                    // each data block index is stored in 4B
                                    // therefore, we have room for 4kB/4B => 1024
                                    // but subtract 1 because length takes up first entry
#define NUM_DIRENTRIES      63      // boot block is 4kB total (each block is 4kB), 4 kB = 2^12 B
                                    // each dir entry is 64 B
                                    // therefore, we have room for 2^12/2^6 => 2^6 = 64 files
                                    // however, first 64B is reserved for some data (see struct below),
                                    // so we only have room for 63 dir entries
                                    // (first dir entry is for itself, ".", so technically only 62 dir entries)
#define BLOCK_SIZE          4096    // each block is 4kB
#define FIRST_BYTE_SHIFT        24
#define SECOND_BYTE_SHIFT       16
#define THIRD_BYTE_SHIFT        8



/* data block struct */
typedef struct datablock {
    uint32_t data[NUM_DATA_BLOCKS];
} datablock_t;

/* directory entry struct (used in boot block struct) */
typedef struct dentry {
    int8_t filename[FILENAME_LEN];
    uint32_t filetype;
    uint32_t inode_num;
    uint8_t reserved[DENTRY_B_RES];
} dentry_t;

/* boot block struct */
typedef struct boot_block {
    uint32_t dir_entry_count;
    uint32_t inode_count;       // N
    uint32_t data_block_count;  // D
    uint8_t data_reserved[BOOTBLOCK_B_RES];
    dentry_t direntries[NUM_DIRENTRIES];
} boot_block_t;

/* index node struct */
typedef struct inode {
    uint32_t length;    // length in bytes
    uint32_t data_block_num[NUM_DATA_BLOCKS];      

} inode_t;

/* the filesystem, needed?? */
uint32_t filesystem_start;   //used to save start addr of filesystem
int global_inode_index;     // used for file_read
int global_file_index;      // used for dir_read

/* Helper functions - utilized by local functions below and system calls */
uint32_t read_dentry_by_name (const int8_t* fname, dentry_t* dentry);
uint32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
uint32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/* local functions - function params based on declarations in ece391syscall.h */
uint32_t init_file_system(uint32_t fs_start, uint32_t fs_end);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

#endif
