//initialized rtc XD
//https://wiki.osdev.org/RTC
#include "i8259.h"
#include "lib.h"
#include "rtc.h"
#include "terminal.h"

volatile int rtc_interrupt_occurred[3] = {0,0,0};    // flag for rtc
volatile int counter;
volatile int freq_ct[3];
/*
 * rtc_init
 *   DESCRIPTION: Initializes rtc for usage 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Enables IRQ on pic, outputs data to RTC data port
 */   
//frequency =  32768 >> (rate-1);
void rtc_init(){
    //disable interrupts
    // cli(); 
    outb(SREG_B, IDX_PORT); //select regB
    char b_val = inb(DATA_PORT); //hold the value of regB
    outb(SREG_B, IDX_PORT); //select regB again
    outb(b_val | 0x40, DATA_PORT); //set the 6th bit of reg B to 1 to turn on periodic interrupts 
    int rate = 0x06; //initialized at 1024hz
    outb(SREG_A, IDX_PORT);  //select regA
	char a_val = inb(DATA_PORT); //hold the value of reg A
	outb(SREG_A, IDX_PORT); //select regA again
	outb((a_val & 0xF0) | rate, DATA_PORT); //set 
    freq_ct[0] = 512;
    freq_ct[1] = 512;
    freq_ct[2] = 512;
    outb(SREG_C, IDX_PORT);	// select register C
    inb(DATA_PORT);		//throw away contents
    enable_irq(8);  //enable irq on pin 8 (first pin on slave)
    
    return;
}


/*
 * rtc_open
 *   DESCRIPTION: open rtc set frequency to default 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS:changes rtc frequency
 */ 
int32_t rtc_open(const uint8_t* filename) {
    set_rtcFrequency(2);
    return 0;
}


/*
 * rtc_close
 *   DESCRIPTION: close rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */ 
int32_t rtc_close(int32_t fd) {
    return 0;
}


/*
 * rtc_read
 *   DESCRIPTION: read from rtc 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: always returns 0 (only after interrupt has occured)
 *   SIDE EFFECTS: waits until interrupt has occured
 */ 

int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    rtc_interrupt_occurred[curr_term] = 0;
    while(!rtc_interrupt_occurred[curr_term]){
    }
    return 0;
}


/*
 * rtc_write
 *   DESCRIPTION: writes a frequency to the rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure 0 on success 
 *   SIDE EFFECTS: modifies rtc interrupt frequency
 */ 
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (buf == NULL || set_rtcFrequency(*(int*)(buf)) == -1)
        return -1;
    return 0;
}


/*
 * set_rtcFrequency
 *   DESCRIPTION: changes the RTC frequency
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure 0 on success
 *   SIDE EFFECTS: changes the frequency of RTC
 */
int set_rtcFrequency (int frequency) {
    // if not within range, return -1
    if (frequency < 2 || frequency > 1024)
        return -1;
    // if (frequency % 2 != 0)
    //     return -1;
    if((frequency == 0) && ((frequency & (frequency - 1)) != 0))
        return -1;
    // determine frequency by powers of 2
    // rate starts at 15, decrement for every power of 2
    if (frequency == 2)
	    freq_ct[curr_term] = 1024/2;
	if (frequency == 4)
		freq_ct[curr_term] = 1024/4;
	if (frequency == 8)
		freq_ct[curr_term] = 1024/8;
	if (frequency == 16)
		freq_ct[curr_term] = 1024/16;
	if (frequency == 32)
		freq_ct[curr_term] = 1024/32;
	if (frequency == 64)
		freq_ct[curr_term] = 1024/64;
	if (frequency == 128)
		freq_ct[curr_term] = 1024/128;
	if (frequency == 256)
		freq_ct[curr_term] = 1024/256;
	if (frequency == 512)
		freq_ct[curr_term] = 1024/512;
	if (frequency == 1024)
		freq_ct[curr_term] = 1024/1024;

/*     cli();
    
	outb(SREG_A, IDX_PORT);  //select regA
	char a_val = inb(DATA_PORT); //hold the value of reg A
	outb(SREG_A, IDX_PORT); //select regA again
	outb((a_val & 0xF0) | rate, DATA_PORT); //set rate (high 4 bits are value of reg A, low 4 are the rate)

	sti();
 */
    return 0;
}


/*
 * rtc_handler
 *   DESCRIPTION: Handler code to be invoked when an RTC interrupt is called 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Enables IRQ on pic, outputs data to RTC data port
 */   
void rtc_handler(){
    //disable interrupts
    //cli(); 
    counter++;
    if(counter >= freq_ct[curr_term]){
        rtc_interrupt_occurred [curr_term]= 1;
        counter = 0;
    }
    
    outb(SREG_C, IDX_PORT);	// select register C
    inb(DATA_PORT);		//throw away contents
    
    //test_interrupts();
    send_eoi(8); //send eoi to RTC, slave pin 1 so 8
    //sti(); //restore
    return;
}
