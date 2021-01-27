/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)


/*******************************Local Vars*********************************/
static int local_ack;			//variable used to check state of tux (has it already acknowledged command?)
static spinlock_t button_lock;  	//spinlock for copying button status safety
static unsigned long curr_buttons; 		//8-bit char holding our current button status
static unsigned long curr_leds = 0x0; 	//long int holding our current LED status

//array indexed by hex values that contains proper format for mapping to seven-segment display
const static unsigned char hex_to_seven[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

/*******************************Local Functions*********************************/
/* These are all helper functions called from the switch statement in the
 * tuxctl_ioctl function below. Interfaces and associated desriptions are below*/
static int ioctl_tux_init(struct tty_struct* tty);
static int ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg);
static int ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg);

/* Helper function for bioc event to avoid unsigned long flag error */
static void mtcp_bioc_event(unsigned char b, unsigned char c);

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 *   DESCRIPTION: handles and parses packets collected from tty line discipline
 *   INPUTS: tty: line discipline struct
 * 			 packet: array of packets collected from tty port
 *   OUTPUTS: none
 *   RETURN VALUE: none
 * 	 SIDE EFFECTS: parses packets and collects data or executes ioctls based on opcode
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    // printk("packet : %x %x %x\n", a, b, c);

	switch (a)	//Check byte 0 for tux response opcode
	{	
		//Opcode response on button press or release
		case MTCP_BIOC_EVENT: 
			mtcp_bioc_event(b, c);

			// printk("curr_buttons: %x\n", curr_buttons);
			break;

		//Opcode response on successful execution of command
		case MTCP_ACK:
			local_ack = 1;
			break;

		//Opcode response on device re-initialization or RESET button press
		case MTCP_RESET:
			ioctl_tux_init(tty);		//Restore to init settings
			if(local_ack){
				ioctl_tux_set_led(tty, curr_leds); //Restore last saved led config
			}
			break;

		default:
			break;
	}
	return;
}

/* mtcp_bioc_event()
 *   DESCRIPTION: called in case of button interrupt-on-change event from tux; sets current button status
 *   INPUTS: b and c: packets from tux; data arguments
 *   OUTPUTS: none
 *   RETURN VALUE: none
 * 	 SIDE EFFECTS: sets curr_buttons status to button press received from packet
 */
static void mtcp_bioc_event(unsigned char b, unsigned char c){
	unsigned long flags;

	unsigned char b1 = (b & 0x0F);  //isolate low four bits of byte 1 (b)
	unsigned char left = (c & 0x02) << 5; //isolate left button (bit 1) and shift to position 6
	unsigned char down = (c & 0x04) << 3; //isolate down button (bit 2) and shift to position 7
	unsigned char b2 = (c & 0x09) << 4;  //isolate bits 3 and 0 of byte 2 (c) and shift to high four bits

	//protect curr_buttons via spinlock
	spin_lock_irqsave(&button_lock, flags);			
	curr_buttons = (b1 | b2 | left | down); 	//add all our button subsets to form compact char
	spin_unlock_irqrestore(&button_lock, flags);
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
/* tuxctl_ioctl()
 *   DESCRIPTION: allows user space game to interact with driver and calls ioctl functions
 *   INPUTS: tty: line discipline struct
 * 			 file: where to access driver
 * 			 cmd: ioctl to call
 * 			 arg: utilized by some of the ioctls
 *   OUTPUTS: none
 *   RETURN VALUE: 0 or -EINVAL based on cmd and whether it fails
 * 	 SIDE EFFECTS: calls corresponding ioctl function based on cmd
 */
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) 
	{
		case TUX_INIT:
			return ioctl_tux_init(tty);
			break;

		case TUX_BUTTONS:
			return ioctl_tux_buttons(tty, arg);
			break;

		case TUX_SET_LED:
			return ioctl_tux_set_led(tty, arg);
			break;

		case TUX_LED_ACK:
			return 0;
			break;

		case TUX_LED_REQUEST:
			return 0;
			break;

		case TUX_READ_LED:
			return 0;
			break;

		default:
			return -EINVAL;
			break;
    }
}


/* ioctl_tux_init()
 *   DESCRIPTION: called in case of initialization or reset of tux controller
 *   INPUTS: tty: line discipline struct
 *   OUTPUTS: none
 *   RETURN VALUE: none
 * 	 SIDE EFFECTS: set initial state of tux controller; initalize all locals for driver
 */
static int ioctl_tux_init(struct tty_struct* tty) {
	/* Set up initial modes for tux controller */
	unsigned char buf[6];
	buf[0] = MTCP_LED_USR;		//Enable LED usr mode
	buf[1] = MTCP_BIOC_ON;		//Enable button interrup-on-change
	
	tuxctl_ldisc_put(tty, buf, 2); 	//Send initialization packets to controller

	/* Initialize local variables */
	button_lock = SPIN_LOCK_UNLOCKED;	//initialize spin lock unlocked
	local_ack = 0;
	curr_buttons = 0xFF; 	//Set all buttons to high (inactive)
	return 0;
}

/* ioctl_tux_buttons()
 *   DESCRIPTION: copies current button status into check
 *   INPUTS: tty: line discipline struct
 * 			 arg: ptr to 32-bit long user space button status
 *   OUTPUTS: none
 *   RETURN VALUE: -EINVAL if invalid ptr; 0 if success
 * 	 SIDE EFFECTS: copy local button_status into user space for access by game
 */
static int ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg){
	unsigned long flags;
	int failure;		//specifies whether copy to user space failed
	unsigned long * pass_buttons;
	pass_buttons = &curr_buttons; //create pointer that points to our current buttons

	if(!arg)	// Check if arg is invalid memory address
		return -EINVAL;
	
	//Protect copy to user space using spinlock
	spin_lock_irqsave(&button_lock, flags);
	failure = copy_to_user((void*) arg, (void*) pass_buttons, sizeof(long));
	spin_unlock_irqrestore(&button_lock, flags);

	if (failure != 0){	//copy_to_user failed
		return -EINVAL;
	}
	else
		return 0;
	
}

/* ioctl_tux_set_led()
 *   DESCRIPTION: copies current button status into check
 *   INPUTS: tty: line discipline struct
 * 			 arg: 32-bit long unsigned specifying led state to set
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 * 	 SIDE EFFECTS: send commands and data to tux to set LEDS based on arg
 */
static int ioctl_tux_set_led (struct tty_struct* tty, unsigned long arg){
	//parse 32-bit led configuration in arg
	
	unsigned char values[4]; 	//array of four values for four displays on tux
	unsigned i;
	unsigned char leds_on, dps_on;  //both 4-bit chars specifiying which leds and decimal points should be turned on
	unsigned char buf[6];
	int buf_len = 2;
	unsigned long bytemask = 0x0000000F; 		//used to mask bytes to retrieve various data and format it for LED display
	unsigned char bitmask = 0x01;				//same as above but for bits
	
	if(!local_ack){ return 0; }		//Check if LEDs or other instruction currently being set

	//Set local_ack to 0 before setting LEDs
	local_ack = 0;

	//Collect four hex values from low 16-bits of arg
	for (i = 0; i < 4; i++){
		values[i] = (arg & (bytemask << (4*i))) >> (4*i);
	} 
	
	leds_on = (arg & (bytemask << 16)) >> 16; 	//Isolate low four bits of the third byte
	dps_on = (arg & (bytemask << 24)) >> 24; 	//Isolate low four bits of fourth byte

	//Format into packets in buffer to send via SET_LED command
	buf[0] = MTCP_LED_SET;  //Send LED_SET opcode
	buf[1] = leds_on;	//Send which leds are being set

	//set remaining buffers to values data
	for(i=0; i<4; i++){
		//We now edit values array to include decimal points
		if (leds_on & (bitmask << i)){		//check if led should be turned on
			values[i] = hex_to_seven[values[i]]; 	//convert hex value to seven-segment display value 
			values[i] = values[i] | ((dps_on & (bitmask << i)) << (4-i));  //isolate each bit of dps_on array and add to bit 4 (dp bit) in each seven segment hex value		
			buf[i+2] = values[i];	//set buffer
			++buf_len;
		} else {
			buf[i+2] = 0x00;
		}
	}
	tuxctl_ldisc_put(tty, buf, buf_len);	//send buf_len packets to tux to set specific leds
	curr_leds = arg;	//save current LED status stored in arg
	return 0;
}
