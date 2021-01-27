//RTC header + definitions
#ifndef _RTC_H
#define _RTC_H

#define	IDX_PORT	    0x70
#define DATA_PORT	    0x71
#define SREG_A		    0x8A
#define SREG_B		    0x8B
#define SREG_C		    0x8C
#define IRQ_PORT        0x08

//used to initialize the RTC
void rtc_init();

//simple handler for rtc checkpoint one test
extern void rtc_handler();
int set_rtcFrequency(int frequency);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);


#endif /*_RTC_H*/
