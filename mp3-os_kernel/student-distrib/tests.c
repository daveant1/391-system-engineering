#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"
#include "filesystem.h"
#include "rtc.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 15; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	for (i = 16; i < 20; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Divide-by-zero Test
*
* Asserts that division by zero prints correct exception message
* Inputs: None
* Outputs: None
* Side Effects: Prints exception message
* Coverage: idt_setup(), handle_exception()
* Files: exceptions.c/h, idt_setup.c/h
*/
void divide_by_zero_test(){
	TEST_HEADER;

	int result;
	int op = 1;
	int div = 0;
	result = op/div;
}

/* System call Test
*
* Asserts system call goes through
* Inputs: None
* Outputs: None
* Side Effects: Prints "exception message"
* Coverage: idt_setup(), handle_system_call()
* Files: exceptions.c/h, idt_setup.c/h
*/

void system_call_test(){
	TEST_HEADER;

	asm volatile ("int $0x80");
}

/* Paging Tests */
/* Paging Test 1 - tests that inaccessible locations are inaccessible
 * Inputs	:	None
 * Outputs	:	None
 * Side Effects	:	Freeze the kernel
 */

void paging_test(){
	TEST_HEADER;

	int * ptr = (int*) (0xB8000);
	int a;
	a = *(ptr);
}


/* Tests if Page Fault Exception fired on dereferencing null ptr
 * 
 * Inputs	: None
 * Outputs	: None
 * Side Effects	: Prints Page Fault exception message
 */
void null_test(){
	TEST_HEADER;

	int* ptr = NULL;
	int test;
	test = *ptr;
}

/* Checkpoint 2 tests */

/* Tests if terminal_read/write properly reads data from keyboard buffer
	and writes data to terminal correctly
 * 
 * Inputs	: None
 * Outputs	: None
 * Side Effects	: Echos whatever was typed, still retains terminal buffer when clearing screen (CTRL+l/L)
 */
void terminal_test1(){
	int32_t cnt;
	char buf[KEYBOARD_BUF_SIZE];

	while(1){
		cnt = terminal_read(1, buf, KEYBOARD_BUF_SIZE);
		// printf("(%d)\n", cnt);
		terminal_write(1, buf, cnt);
	}
}


/* Testing to see if terminal_write does not stop writing at a null byte
 * Inputs	: None
 * Outputs	: None
 * Side Effects	: Prints the number of bytes written at end (should be KEYBOARD_BUF_SIZE=128)
 * 					Also does not print null bytes
 */
void terminal_test2(){
	int i;
	int32_t cnt;
	char* buf[KEYBOARD_BUF_SIZE];

	printf("Original characters: ");
	// for loop will only go up to 15 characters, meaning rest are null bytes
	for(i = 0; i < 15; i++){
		((char*)buf)[i] = i + 65;		// 65 for start of capital letters
		// putc(((char*)buf)[i]);			// print out original 15 characters
	}

	// new line to compare
	printf("\nTesting terminal_write: ");
	cnt = terminal_write(1, buf, KEYBOARD_BUF_SIZE);
	printf(" (%d)", cnt);									// prints number of bytes written, should not stop at nulll byte!
}


/* Tests if terminal_read handles case where size sent by user does not match size of buffer
 *
 * Inputs	: None
 * Outputs	: None
 * Side Effects	: Should only read at most 2 characters, and print 2 characters
 */
void terminal_test3(){
	int i;
	int32_t cnt;
	char* buf[KEYBOARD_BUF_SIZE];

	cnt = terminal_read(1, buf, 2);		// 2 is definitely less than average word length		
	printf("terminal_read result: ");
	for(i = 0; i < cnt; i++){
		// putc(((char*)buf)[i]);
	}

}


/* Tests terminal_open/close
 *
 * Inputs	: None
 * Outputs	: None
 * Side Effects	: Should print PASS (both open and close return 0, so see if it matches)
 */
void terminal_test4(){
	int32_t open;
	int32_t close;
	const uint8_t* filename;		// points to null

	open = terminal_open(filename) == 0 ? PASS : FAIL;
	close = terminal_close(1) == 0 ? PASS : FAIL;
	TEST_OUTPUT("terminal_open", open);
	TEST_OUTPUT("terminal_close", close);
}


/*fs_test_1() - Tests if helper function can read a directory entry by name
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage : read_dentry_by_name()
 * Side Effects	: Prints PASS if we fetch correct dentry
 */
int fs_test_1(){
	TEST_HEADER;

	dentry_t test_dentry;
	read_dentry_by_name("verylargetextwithverylongname.txt", &test_dentry);
	if(strncmp(test_dentry.filename, "verylargetextwithverylongname.tx", 32) != 0)
		return FAIL;
	/* "verylargetextwithverylongname.tx" should correspond to inode 44, filetype 2*/
	if(test_dentry.inode_num != 44 || test_dentry.filetype != 2)
		return FAIL;	
	else
		return PASS;
	
}

/*fs_test_2() - Tests if helper function can read a directory entry by index
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage : read_dentry_by_index()
 * Side Effects	: Prints PASS if we fetch correct dentry
 */
int fs_test_2(){
	TEST_HEADER;

	dentry_t test_dentry;
	read_dentry_by_index(7, &test_dentry);
	if(strncmp(test_dentry.filename, "counter", 7) != 0)
		return FAIL;
	/* "counter" should correspond to inode 42, filetype 2 */
	if(test_dentry.inode_num != 42 || test_dentry.filetype != 2)
		return FAIL;
	else
		return PASS;
	
}

/* fs_test_list_files - Tests dir_read (and therefore, read_dentry_by_index) by printing out all the files
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage	: dir_read and read_dentry_by_index
 * Side Effects	: lists out all files (check discord general chat for what it should look like)
 */
void fs_test_list_files(){
 	TEST_HEADER;

	dentry_t cur_dentry;
	int i, j;
	for (i = 0; i < 17; i++) {
		read_dentry_by_index(i, &cur_dentry);

		/* Create name buffer to copy fname into + one extra space for null '\0' char */
		uint8_t fname[FILENAME_LEN + 1];
		for(j = 0; j < FILENAME_LEN; j++){
			fname[j] = cur_dentry.filename[j];
		}
		fname[32] = '\0';	//terminate any string over 32 chars with null char

		uint32_t inode_idx = cur_dentry.inode_num;
		inode_t* inode_addr = (inode_t*) (filesystem_start + (4096 * (inode_idx+1))); //calculate inode struct addr using inode number and start addr of filesystem
		printf("file_name: %s, file_type: %d, file_size: %d\n", fname, cur_dentry.filetype, inode_addr->length);
	}
	
}

/* fs_test_read_small_file - Tests file_read (and thus, read_data) by printing out contents of a file
 * works for frame0.txt and frame1.txt (small files)
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage	: file_read and read_data
 * Side Effects	: prints out the contents of the specified file
 */
void fs_test_read_small_file(){
	/*
	TEST_HEADER;

	uint32_t fd;		// unused here
	char buffer[1600];	// 1600 is more than enough
	
	file_open((uint8_t*)"frame0.txt");
	file_read((uint32_t)&fd, &buffer, 1600);
	int i;
	for (i = 0; i < 1600; i++) {
		// printf("%c", buffer[i]);
		putc(buffer[i]);
	}
	printf("\nfile_name: frame0.txt");
	*/

	TEST_HEADER;

	uint32_t fd;		// unused here

	// calculate size of file we're reading
	dentry_t d;
	read_dentry_by_name((int8_t*) "frame0.txt", &d);
	uint32_t inode_index = d.inode_num;
	uint32_t file_size = ((inode_t*)(fs_start_addr + (inode_index + 1) * BLOCK_SIZE))->length;
	char buffer[file_size];

	// fill buffer
	file_open((uint8_t*)"frame0.txt");
	file_read((uint32_t)&fd, &buffer, file_size);

	// print out buffer (contents of file)
	int i;
	for (i = 0; i < file_size; i++) {
		// eliminate NULLs
		if (buffer[i] == 0 || buffer[i] == 1 || buffer[i] == 127) {
			continue;
		}
		// putc(buffer[i]);
	}
	printf("\nfile_name: frame0.txt");
}


/* fs_test_read_executable - Tests file_read (and thus, read_data) by printing out contents of an executable
 * works for grep and ls (executables)
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage	: file_read and read_data
 * Side Effects	: prints out the contents of the specified file
 */
void fs_test_read_executable(){
	TEST_HEADER;

	uint32_t fd;		// unused here

	// calculate size of file we're reading
	dentry_t d;
	read_dentry_by_name((int8_t*) "grep", &d);
	uint32_t inode_index = d.inode_num;
	//uint32_t file_size = ((inode_t*)(fs_start_addr + (inode_index + 1) * BLOCK_SIZE/sizeof(inode_t)))->length;
	uint32_t file_size = ((inode_t*)(fs_start_addr + (inode_index + 1) * BLOCK_SIZE))->length;
	// uint32_t file_size = 100;
	char buffer[file_size];

	// fill buffer
	file_open((uint8_t*)"grep");
	file_read((uint32_t)&fd, &buffer, file_size);

	// print out buffer (contents of file)
	int i;
	for (i = 0; i < file_size; i++) {
		if (buffer[i] == 0 || buffer[i] == 1 || buffer[i] == 127) {
			continue;
		}
		// putc(buffer[i]);
	}
	printf("\nfile_name: grep");
}


/* rtc_test
* Description: tests rtc driver
* Inputs: None
* Outputs: None
* Side Effects: print to screen, open rtc.
*/
void rtc_test()
{
	int i, j;
	rtc_open(0);

	// go up by powers of two
 	for (i = 1; i < 2048; i*=2) {
		printf("Frequency: %d test:", i);
		// go up 10 numbers at most
 		for(j = 0; j < 10; j++) {
 			rtc_read(0, 0, 0);
 			printf("%d ", i);
 		}
 		if (rtc_write(0, (void*) &i, sizeof(int)) == -1)
		 	printf("failed");
 		printf("\n");
	}

 	rtc_close(0);
	rtc_open(0);

	// again, go up 10 numbers at most
	for (i = 0; i < 10; i++){
		rtc_read(0, 0, 0);
 		printf("%d ", i);
	}

	rtc_close(0);
}


/* fs_test_read_large_file - Tests file_read (and thus, read_data) by printing out contents of a file
 * works for  verylargetextwithverylongname.tx(t) (large files)
 * 
 * Inputs	: None
 * Outputs	: None
 * Coverage	: file_read and read_data
 * Side Effects	: prints out the contents of the specified file
 */
void fs_test_read_large_file(){
	TEST_HEADER;

	uint32_t fd;		// unused here

	// calculate size of file we're reading
	dentry_t d;
	// char buffer[6000];		// more than enough

	read_dentry_by_name((int8_t*) "verylargetextwithverylongname.txt", &d);
	uint32_t inode_index = d.inode_num;
	uint32_t file_size = ((inode_t*)(fs_start_addr + (inode_index + 1) * BLOCK_SIZE))->length;
	char buffer[file_size];

	// fill buffer
	file_open((uint8_t*)"verylargetextwithverylongname.txt");
	file_read((uint32_t)&fd, &buffer, file_size);

	// print out buffer (contents of file)
	// 4029 starts lower case
	// 4043 = o
	// 4044 = p
	int i;
	for (i = 0; i < file_size; i++) {
		// putc(buffer[i]);
	}

	printf("\nfile_name: verylargetextwithverylongname.txt");
}


/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* launch_tests
* Description: Test suite entry point
* Inputs: None
* Outputs: None
* Side Effects: Prints messages to screen if helper test involves printing
*/
/* Test suite entry point */
void launch_tests(){
	// launch your tests here
	// terminal_test1();
	// terminal_test2();
	// terminal_test3();
	// terminal_test4();
	// rtc_test();
	// divide_by_zero_test();
	// system_call_test();
	// paging_test1();
	// null_test();
	// TEST_OUTPUT("fs_test_1", fs_test_1());
	// TEST_OUTPUT("fs_test_2", fs_test_2());
	// fs_test_list_files();
	// fs_test_read_small_file();
	// fs_test_read_executable();
	// fs_test_read_large_file();
}
