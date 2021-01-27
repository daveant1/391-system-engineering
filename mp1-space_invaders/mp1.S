
# mp1.S - missile-command version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct missile, 
    # struct missile is defined in rtc.h and mp1.h

	NEXT 	  = 0
	X	  = 4
	Y	  = 8
	VX	  = 12
	VY	  = 16
	DEST_X	  = 20
	DEST_Y	  = 24
	EXPLODED  = 28
	C	  = 32

    # Character to draw for an explosion - '@'

	EXPLOSION = 64

	# Character to draw for crosshair - 'x'

	CROSSHAIR = 88

	# Character to draw for crosshair - ' '

	SPACE = 32

# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:	.long 0x0	# Head of list pointer
base_alive:		.long 0x0	# Array of 3 bytes, plus a padding byte
mp1_score:		.long 0x0	# Player's current score


# Data private to this  file

base_pic:     	    .string "/^^^\\"	# Picture of a live base
dead_base_pic: 	    .string "xxxxx"	# Picture of a dead base
crosshairs_x:		.long 0x0	# X-position of the crosshairs
crosshairs_y:		.long 0x0	# Y-position of the crosshairs
notify_flag:		.long 0x0	# A flag that is set at the beginning and during update_missile to say if we have called notify user for current iteration

.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#	(1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#	(2) Redraw the bases - they may have been overwritten with missiles
#	(3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
	# Stack setup and callee-save
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI

	# Step 1: Update missile list
	CALL update_missiles

	# Step 2: Redraw cities
	# Loop through base_alive to print and draw cities
	# ESI: base counter
	# EDI: char counter
	# EAX: current coordinate to print to
	# EBX: base_pic str start
	# EDX: base_alive array
	MOVL $1, %ESI
	MOVL base_alive, %EDX
	next_base:
		CMPL $3, %ESI
		JG draw_xhair
		MOVL $base_pic, %EBX
		CMPB $0, %DL
		JNE draw_prep
		MOVL $dead_base_pic, %EBX
		
	draw_prep:
		# Calculate start coordinates using midpoint of base drawing
		# x = (base count * 20) - 2, y = 24
		# poke offset: EAX = (y*80+x)*2
		IMULL $20, %ESI, %ECX
		SUBL $2, %ECX	# Subtract 2 to get start x coordinate
		MOV $24, %EAX
		IMULL $80, %EAX
        ADDL %ECX, %EAX
        SALL $1, %EAX
		INCL %ESI	# lower count of bases
		SARL $8, %EDX # Isolate next base status from base_alive
		XORL %EDI, %EDI	  # Init base char count to 0
	draw_loop:
        MOVB (%EBX, %EDI), %CL	  # Load next char 
        PUSHL %EDX
        CALL mp1_poke
        POPL %EDX
		ADDL $2, %EAX	# Add 2 bytes to get address to place next char
		INCL %EDI
		CMPL $5, %EDI	# Check if we have printed all five chars via counter
		JE next_base
		JMP draw_loop

	# Step 3: Redraw crosshairs 
	draw_xhair:
		# Calculate poke coords for xhair (y*80+x)*2
		IMULL $80, crosshairs_y, %EAX	
		ADDL crosshairs_x, %EAX
		SALL $1, %EAX
		MOVB $CROSSHAIR, %CL
		PUSHL %EDX
		CALL mp1_poke
		POPL %EDX 

	# Restore callee-saved and ret
	rtc_done:	
		POPL %EDI
		POPL %ESI
		POPL %EBX
		LEAVE
		RET



# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:
	MOVL 8(%ESP), %EAX		# EAX <- cmd
	CMPL $4, %EAX
	JG invalid_cmd
	CMPL $0, %EAX
	JL invalid_cmd
	JMP *jump_table(,%EAX, 4)

invalid_cmd:
	MOVL $-1, %EAX
	RET
jump_table:
.long mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame



# ----------------- Functions private to this file -------------------
# void update_missiles();
# Iterates through missile list, erases missile from screen, checks explosions,  updates x/y coords
# Removes exploded/out-of-bound missiles from list, redraws missile/explosion
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
# ECX: current missile ptr
# ESI: x coordinate
# EDI: y coordinate
# notify_flag: used to determine if we already called mp1_notify_user for this iteration

update_missiles:
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI
	MOVL mp1_missile_list, %ECX 	# ECX <- HEAD of missile list
	MOVL $0, notify_flag		# Init notify_flag

	new_missile:
		CMPL $0, %ECX	# Check if curr missile NULL
		JE update_done

		# STEP 1: Begin by erasing missile from screen no matter what
		# offset = (row*80+col) * 2
		# IMULL result stored in EAX for mp1_poke function
		MOVL X(%ECX), %ESI
		SARL $16, %ESI
		MOVL Y(%ECX), %EDI
		SARL $16, %EDI
        IMULL $80, %EDI, %EAX
        ADDL %ESI, %EAX
        SALL $1, %EAX
		PUSHL %ECX  	# Save ECX before moving in char
        MOVB $SPACE, %CL	  # Load space char 
        PUSHL %EDX
        CALL mp1_poke
        POPL %EDX
		POPL %ECX
		# CMPL $-1, %EBX
		# JNE mp1_missile_remove

		# STEP 2: Check if exploding, update x and y if not exploded
		CMPL $0, EXPLODED(%ECX)
		JNE dest_or_explode		# exploded != 0
		MOVL X(%ECX), %ESI
		ADDL VX(%ECX), %ESI
		MOVL %ESI, X(%ECX)
		SARL $16, %ESI     		# isolate X[31:16]
		MOVL Y(%ECX), %EDI		# same with y
		ADDL VY(%ECX), %EDI
		MOVL %EDI, Y(%ECX)
		SARL $16, %EDI

		# Our flag registers to determine if we need to rm from list or draw missile/explosion
		# XORL %EBX, %EBX	
		XORL %EDX, %EDX

		
	# Now, check for three cases:

	# Case 1: Out of range (x<0 | x>79 | y<0 | y>24)
	out_of_range:
		CMPL $0, %ESI
		JL mp1_missile_remove
		CMPL $79, %ESI
		JG mp1_missile_remove
		CMPL $0, %EDI
		JL mp1_missile_remove
		CMPL $24, %EDI
		JG mp1_missile_remove

	# Case 2: Missile has reached desination
	dest_check:
		CMPL DEST_X(%ECX), %ESI
		JNE just_moving
		CMPL DEST_Y(%ECX), %EDI
		JNE just_moving

	# Call missile_explode, decrement exploded and check if == 0, then erase from screen, rm_from_list, free with mp1_free
	dest_or_explode:
		PUSHL %ECX
		PUSHL %ECX   # this is struct arg
		CALL missile_explode
		ADDL $4, %ESP # adjust ESP to skip args
		POPL %ECX
		
		# check if non-zero return, notify user
		CMPL $0, %EAX
		JE skip_notify
		CMPL $-1, notify_flag	# Check if already called mp1_notify_user
		JE skip_notify
		PUSHL %EAX
		PUSHL %ECX
		PUSHL %EDX
		CALL mp1_notify_user
		POPL %EDX
		POPL %ECX
		POPL %EAX
		MOVL $-1, notify_flag  # Set notify flag to -1 to indicate we have already called

	skip_notify:
		DECL EXPLODED(%ECX)
		CMPL $0, EXPLODED(%ECX)
		JLE mp1_missile_remove
		# Set EDX flag to signify we are drawing explosion
		MOVL $-1, %EDX

	just_moving:
	# Draw missile/symbol to screen in new coords
	# Use EDX as flag to determine if we are drawing missile or explosion
	draw:
		# change x and y back to updated coords
		MOVL X(%ECX), %ESI
		SARL $16, %ESI
		MOVL Y(%ECX), %EDI
		SARL $16, %EDI
		IMULL $80, %EDI, %EAX
        ADDL %ESI, %EAX
        SALL $1, %EAX
		PUSHL %ECX  	# Save ECX before moving in char
		MOVB C(%ECX), %CL	# Load missile char first
		CMPL $-1, %EDX		# Check if missile or explosion
		JNE skip_explosion	# Don't load EXPLOSION char if not
		MOVB $EXPLOSION, %CL # Else print explosion char
	skip_explosion:
		PUSHL %EDX
        CALL mp1_poke
        POPL %EDX
		POPL %ECX
		JMP fetch_next

	# Block that removes missile from linked list and free ptr memory
	# EAX: ptr to prev missile
	# ECX: missile ptr to be removed
	# EDX: cursor through list (removed once equal to ECX)
	mp1_missile_remove:
		MOVL mp1_missile_list, %EAX
		CMPL %EAX, %ECX		# Account for case where we ECX is head
		JE free_head
		MOVL NEXT(%EAX), %EDX	# Account for case where ECX is second in list
		CMPL %EDX, %ECX
		JE free_second

	list_loop:
		# move prev and curr ptrs up one
		MOVL NEXT(%EAX), %EAX
		MOVL NEXT(%EAX), %EDX
		CMPL %EDX, %ECX
		JNE list_loop
		# reset prev ptr's NEXT to the next ptr
		MOVL NEXT(%EDX), %EDX
		MOVL %EDX, NEXT(%EAX)
		JMP free_mem
	free_second:
		MOVL NEXT(%EDX), %EDX
		MOVL %EDX, NEXT(%EAX)
		JMP free_mem
	free_head:
		MOVL NEXT(%EAX), %EDX
		MOVL %EDX, mp1_missile_list	 # Adjust head missile ptr to next
	free_mem:
		PUSHL %EDX 		# Save ptr to next missile
		PUSHL %ECX
		CALL mp1_free
		ADDL $4, %ESP 	# adjust ESP to skip args
		POPL %EDX
		MOVL %EDX, %ECX 	# update ECX to point to next missile
		JMP new_missile

	fetch_next:
		MOVL NEXT(%ECX), %ECX	# update ECX to point to next missile
		JMP new_missile

	update_done:
		POPL %EDI
		POPL %ESI
		POPL %EBX
		LEAVE
		RET
	


# void mp1_ioctl_startgame (unsigned long ignore)
# Inputs   : unsigned long ignore
# Outputs  : none
# Registers: Standard C calling convention
# Initalizes all variables and crosshair for driver
mp1_ioctl_startgame:
	# callee setup
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI
	# init vars
	MOVL $0, mp1_missile_list
	MOVL $0, mp1_score
	MOVL $0x00FFFFFF, base_alive
	# init xhairs
	MOVL $40, crosshairs_x
	MOVL $12, crosshairs_y
	# callee teardown
	POPL %EDI
	POPL %ESI
	POPL %EBX
	LEAVE
	RET



# int mp1_ioctl_addmissile (struct missile* user_missile)
# Allocates space in kernel for new missile, copies new missile from user space to kernel space
# ECX: hold struct missile ptr
mp1_ioctl_addmissile:
	# callee setup
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI
	# Load ptr and allocate dynamic memory
	MOVL 8(%EBP), %ECX  # Load struct missile ptr
	PUSHL %ECX		# Caller save
	PUSHL $33		# Push on sizeof(struct missile)=33
	CALL mp1_malloc
	ADDL $4, %ESP 	# adjust ESP to skip args
	POPL %ECX
	CMPL $0, %EAX	# Check if returned ptr null
	JE malloc_failed

	# Copy allocated memory from user to kernel space
	PUSHL %EAX
	PUSHL %ECX
	# unsigned long mp1_copy_from_user(void *to, const void *from, unsigned long n)
	PUSHL $33	# arg n
	PUSHL %ECX	# arg *from
	PUSHL %EAX	# arg *to
	CALL mp1_copy_from_user
	ADDL $12, %ESP 	# adjust ESP to skip args
	POPL %ECX
	CMPL $0, %EAX 	# Check if copy successful
	JNE copy_failed
	POPL %EAX

	# Finally, add missile to list
	MOVL mp1_missile_list, %EDX
	MOVL %EDX, NEXT(%EAX)
	MOVL %EAX, mp1_missile_list
	JMP add_finished 

copy_failed: 
	# Free memory that failed to allocate
	POPL %EAX
	PUSHL %EAX
	call mp1_free
	ADDL $4, %ESP

malloc_failed:
	# Prepare -1 return value in case of copy or malloc fail
	MOVL $-1, %EAX

add_finished:
	# callee teardown
	POPL %EDI
	POPL %ESI
	POPL %EBX
	LEAVE
	RET



# int mp1_ioctl_movexhairs (unsigned long xhair_delta_packed)
# Move crosshairs using xhair_delta, erase and redraw
# ECX: holds delta_packed
# ESI: x crosshair coord
# EDI: y crosshair coord
mp1_ioctl_movexhairs:
	# callee setup
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI
erase_xhair:
	# Erase and redraw xhair no matter what
	# Calculate poke coords for xhair (y*80+x)*2
	IMULL $80, crosshairs_y, %EAX	
	ADDL crosshairs_x, %EAX
	SALL $1, %EAX
	MOVB $SPACE, %CL
	PUSHL %EDX
	CALL mp1_poke
	POPL %EDX  

	MOVL 8(%EBP), %ECX	# ECX <- xhair_delta_packed
	MOVL crosshairs_x, %ESI
	MOVL crosshairs_y, %EDI
	ADDW %CX, %SI
	SARL $16, %ECX
	ADDW %CX, %DI

# Check if new components moved off screen
check_x:
	CMPL $0, %ESI
	JL update_xl
	CMPL $79, %ESI
	JG update_xh
check_y:
	CMPL $0, %EDI
	JL update_yl
	CMPL $24, %EDI
	JG update_yh
	JMP draw_xhairs

# x coordinate < 0
update_xl:
	XORL %ESI, %ESI
	JMP check_y
# x coordinate > 79	
update_xh:
	MOVL $79, %ESI
	JMP check_y
# y coordinate < 0
update_yl:
	XORL %EDI, %EDI	
	JMP draw_xhairs
# y coordinate > 24	
update_yh:
	MOVL $24, %EDI

draw_xhairs:
	# update crosshairs values
	MOVL %ESI, crosshairs_x
	MOVL %EDI, crosshairs_y
	# Calculate poke coords for drawing xhair (y*80+x)*2
	IMULL $80, crosshairs_y, %EAX	
	ADDL crosshairs_x, %EAX
	SALL $1, %EAX
	MOVB $CROSSHAIR, %CL
	PUSHL %EDX
	CALL mp1_poke
	POPL %EDX 

finish_xhairs:
	# Always return 0
	XORL %EAX, %EAX
	# callee teardown
	POPL %EDI
	POPL %ESI
	POPL %EBX
	LEAVE
	RET



# int mp1_ioctl_getstatus (unsigned long* user_status)
# Updates user_status pointer with mp1_score and status of three bases
# ECX: holds user_status ptr
# EDX: base_alive array
# ESI: base counter
# EDI: temporary register, eventually copied to user_status
mp1_ioctl_getstatus:
	# callee setup
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI

	MOVL 8(%EBP), %ECX	# ECX <- user_status ptr
	MOVL $3, %ESI		# init base counter to 3
	XORL %EDI, %EDI 	# init temp reg EDI to 0
	MOVL base_alive, %EDX

base_loop:
	CMPL $0, %ESI	# Check base counter
	JLE copy_score
	CMPB $0, %DL	# Check curr base alive/dead
	JE dead_base

	ADDL $1, %EDI 	# Add 1 to set curr base bit to alive

dead_base:
	SALL $1, %EDI	# Make room for next base bit
	DECL %ESI
	SARL $8, %EDX
	JMP base_loop

copy_score:
	SALL $15, %EDI	# Move EDI base bits to positions [18:16]
	MOVL mp1_score, %EBX
	MOVW %BX, %DI
	PUSHL %EDI		# Push EDI to adjust ESP to point to EDI
	MOVL %ESP, %EBX	# Save stack pointer into EBX for copy_to_user function

	# call mp1_copy_to_user
	PUSHL %ECX
	# unsigned long mp1_copy_to_user(void *to, const void *from, unsigned long n)
	PUSHL $4	# arg n (4-byte integer)
	PUSHL %EBX	# arg *from (ptr to EBI register)
	PUSHL %ECX	# arg *to
	CALL mp1_copy_to_user
	ADDL $12, %ESP
	POPL %ECX
	POPL %EDI
	
	# Check for copy success return
	CMPL $0, %EAX
	JNE copy_score_fail
	XORL %EAX, %EAX
	JMP status_finished

copy_score_fail:
	MOVL $-1, %EAX

status_finished:
 	# callee teardown
	POPL %EDI
	POPL %ESI
	POPL %EBX
	LEAVE
	RET



# int mp1_ioctl_endgame (unsigned long ignore)
# Cleans up memory by clearing and deallocating linked list
# ECX: list cursor (ptr to current list node)
mp1_ioctl_endgame:
	# callee setup
	PUSHL %EBP
	MOVL %ESP, %EBP
	PUSHL %EBX
	PUSHL %ESI
	PUSHL %EDI

# Iterate through list and remove missiles node-by-node		
free_list_loop:
	CMPL $0, mp1_missile_list	# If HEAD==null, we have reached end of list
	JE list_freed
	MOVL mp1_missile_list, %ECX
	MOVL NEXT(%ECX), %EBX
	MOVL %EBX, mp1_missile_list	# Point HEAD to next missile
	
	# Free memory occupied by current missile ECX
	PUSHL %ECX
	PUSHL %ECX
	CALL mp1_free
	ADDL $4, %ESP	# Adjust ESP to skip args
	POPL %ECX
	JMP free_list_loop
	
list_freed:
	# restore callee-saved
	POPL %EDI
	POPL %ESI
	POPL %EBX
	LEAVE
	RET
