	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	INPORT.ASM
;
; Windows mouse driver data and initialization routines for using an
; InPort mouse for Windows
;
; Created: 21-Aug-1987
; Author:  Mr. Mouse [mickeym], Walt Moore [waltm]
;
;  10-Jan-1990. -by-  Amit Chatterjee.
;  If real mode, if INT 33 driver is loaded, we will try to get the mouse
;  IRQ using an int 33 call. 
;
; Copyright (c) 1986,1987  Microsoft Corporation
;
; Exported Functions:
;	None
; Public Functions:
;	inport_enable
;	inport_disable
;	inport_init
; Public Data:
;	None
; General Description:
;	This module contains the functions to find, enable, disable,
;	and process interrupts for an InPort mouse
;-----------------------------------------------------------------------;


	title	Microsoft Windows InPort Mouse Dependant Code

	.xlist
	include cmacros.inc
	include mouse.inc
	include inport.inc
	.list

	??_out	InPort


	externNP hook_us_in		;Hook us into our interrupt
	externNP unhook_us		;Hook us out of our interrupt
	externNP enable_our_int 	;Enable us at the 8259

INPORT_IO_HIGH	equ	23Ch		;Highest possible base port address
INPORT_IO_LOW	equ	230h		;Lowest  possible base port address
INPORT_IO_SPAN	equ	4		;Spacing between base port addresses

IRQ_PC_MASK_BITS equ	10111100b	;Search IRQs 7,5,4,3, and 2


IRQ_AT_M_MASK_BITS equ	10111000b	;Search IRQs 7,5,4,3,
IRQ_AT_S_MASK_BITS equ	00000010b	;Search IRQ  9

sBegin	Data

externB vector				;Vector # of mouse interrupt
externB mask_8259			;8259 interrupt enable mask
externB mouse_flags			;Various flags as follows
externW io_base 			;Mouse port base address
externW enable_proc			;Address of routine to	enable mouse
externW disable_proc			;Address of routine to disable mouse
externW WinFlags			;Windows exported flags
externB device_int			;Start of mouse specific int handler
externD event_proc			;Mouse event procedure when enabled
OriginalCommPortAddr	dw	0	;addr of port whose int we will use
OriginalCommAddrLoc	dw	0	;location in 40: for above address

sEnd	Data


sBegin	Code
assumes cs,Code
page

;	This is the start of the data which will be copied into
;	the device_int area reserved in the data segment.

INPORT_START	equ	this word


;--------------------------Interrupt-Routine----------------------------;
; inport_int Mouse Interrupt Handler for the InPort Mouse
;
; This is the handler for the interrupt generated by the InPort
; mouse.  It will reside in the Data segment starting at the
; location device_int.
;
; Entry:
;	None
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	ALL
; Registers Destroyed:
;	None
; Calls:
;	event_proc if a mouse event occured
; History:
;	Fri 21-Aug-1987 11:43:42 -by-  Walt Moore [waltm] & Mr. Mouse
;	Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes cs,Code
	assumes ds,nothing
	assumes es,nothing
	assumes ss,nothing

INPORT_PROC_START equ	$-INPORT_START	    ;Delta to this procedure
		  .errnz INPORT_PROC_START   ;Must be first

inport_int	proc	far

	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	mov	ax,_DATA
	mov	ds,ax
	assumes ds,Data

	mov	dx,io_base		;--> Address register
	mov	al,INPORT_MODE		;Get access to mode register
	out	dx,al			;  and set the HOLD bit
	inc	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	in	al,dx
	or	al,INPORT_HOLD
	io_delay
	out	dx,al

	mov	al,INPORT_DATA_1	;Get delta X and save it
	dec	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	out	dx,al
	inc	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	in	al,dx
	cbw
	xchg	bx,ax

	mov	al,INPORT_DATA_2	;Get delta Y and save it
	dec	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	out	dx,al
	inc	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	in	al,dx
	cbw
	xchg	cx,ax

	mov	al,INPORT_MOUSE_STAT	;Get mouse status register
	dec	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	out	dx,al
	inc	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	in	al,dx
	xchg	al,ah			;Save it here

	mov	al,INPORT_MODE		;Have all the info needed, clear
	dec	dx			;  the HOLD bit
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	out	dx,al
	inc	dx
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	io_delay
	in	al,dx
	and	al,not INPORT_HOLD
	io_delay
	out	dx,al


;	The mouse is back and running.	Process the status to see what
;	changed, and set up the return value.  Unlike the other mouse
;	handlers which use a table lookup to determine if a button
;	has changed state, in the general case it will be quicker
;	to just test the delta flags and jump into the button code
;	if a button changed.  Space costs are about the same.


	xchg	al,ah
	xor	ah,ah			;AH will accumulate delta state
	shl	ax,1			;Align movement bit
	shl	ax,1
	.errnz	INPORT_MOVEMENT-01000000b
	.errnz	SF_MOVEMENT-00000001b

	test	al,(INPORT_DELTA_B1+INPORT_DELTA_B3) shl 2
	jnz	inport_button_changed	;At least one changed

inport_eoi:
	mov	al,EOI			;ACK the interrupt
	test	WinFlags,WF_PMODE
	jz	no_slave

;----------------------------------------------------------------------------;
; CAUTION: It might not be a very good idea to EOI the slave if the mouse is ;
; not on the slave.!!!							     ;
;----------------------------------------------------------------------------;

	out	ACK_SLAVE_PORT,al
no_slave:
	out	ACK_PORT,al

	xchg	al,ah			;Return delta and state in AX
	cbw
	or	ax,ax
	jz	inport_no_data
	mov	dx,NUMBER_BUTTONS
	sti
	call	event_proc

inport_no_data:
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax

inport_int_exit:
	iret

inport_button_changed:			;At least one changed
	test	al,INPORT_DELTA_B1 shl 2;Has button 1 changed?
	jz	inport_button_2_changed ;  No, must have been other button
	or	ah,SF_B1_DOWN		;  Yes, assume it is down
	test	al,INPORT_STAT_B1 shl 2 ;  Is button 1 down?
	jnz	check_inport_button_2	;    Yes
	xor	ah,SF_B1_DOWN+SF_B1_UP	;    No, it's up

check_inport_button_2:
	test	al,INPORT_DELTA_B3 shl 2;Has button 2 (SW3) changed?
	jz	inport_eoi		;  No

inport_button_2_changed:
	or	ah,SF_B2_DOWN		;  Yes, assume it is down
	test	al,INPORT_STAT_B3 shl 2 ;  Is button 3 down?
	jnz	inport_eoi		;    Yes
	xor	ah,SF_B2_DOWN+SF_B2_UP	;    No, it's up
	jmp	inport_eoi

inport_int	endp

INPORT_INT_LENGTH = $-INPORT_START	;Length of code to copy
	.errnz	INPORT_INT_LENGTH gt MAX_INT_SIZE

display_int_size  %INPORT_INT_LENGTH
page

;---------------------------Public-Routine------------------------------;
; inport_search - Search for an InPort Pointer
;
; A search will be made for an InPort pointer.
;
; If an adapter is found but no interrupt is found, disable
; searching for the bus mouse since this could cause the InPort
; adapter to enter test mode, causing data contention on the bus.
;
; Entry:
;	None
; Returns:
;	'C' set if found
;	  CX = 0 if single interrupt vector was not found
;	  CX = size of interrupt routine if interrupt vector found
;	    AX = address of interrupt routine if interrupt vector found
;	    SI = offset within the Code segment of the handler
; Error Returns:
;	'C' clear if not found
; Registers Preserved:
;	DS,ES,BP
; Registers Destroyed:
;	AX,BX,DX,FLAGS
; Calls:
;	inport_find_int
; History:
;	Fri 21-Aug-1987 11:43:42 -by-  Walt Moore [waltm] & Mr. Mouse
;	Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes cs,Code
	assumes ds,Data

		public	inport_search
inport_search	proc	near

	mov	dx,INPORT_IO_HIGH+INPORT_ID

inport_search_next:
	mov	ah,INPORT_CODE		;Don't want ID on the bus at all!
	in	al,dx			;Read identification register
	cmp	al,ah			;Check for inport ID code
	je	possible_inport 	;Jump if might have found it
	in	al,dx			;Port toggles between ID and
	cmp	al,ah			;  version number
	je	possible_inport

inport_next_port:
	sub	dx,INPORT_IO_SPAN	;Try next port, if there is one
	cmp	dl,low (INPORT_IO_LOW-INPORT_IO_SPAN+INPORT_ID)
	jne	inport_search_next	;More ports exist to be checked
;	clc				;('C' clear from compare)
	jmp	short inport_end_search ;'C' clear to show not found


;	The correct response to the first inquiry was found.  Get the
;	version/revision number.  Once that is done, then do it again
;	to make sure this really is the chip we're looking at.


possible_inport:
	in	al,dx			;Read version/revision #
	mov	bl,al			;Save version/revision # in BL
	io_delay
	in	al,dx			;Read type code
	cmp	al,ah			;Check if type code still there
	jne	inport_next_port	;Not really an inport device
	in	al,dx			;Read version/revision #
	cmp	al,bl			;Check version/revision #
	jne	inport_next_port	;Not really an inport device


;	Seems that a real Microsoft InPort Mouse has been found.
;	Try to determine which interrupt vector its sitting on.

	sub	dx,INPORT_ID		;--> base IO address for chip
	push	dx			;Save base port address
	call	inport_find_int
	pop	dx
	mov	cx,0			;Don't destroy 'C'
	jc	inport_end_search	;None or multiple IRQs.  Show unusable

	mov	io_base,dx		;Save base IO address
	mov	al,INPORT_RESET 	;Reset chip, leaving interrupts
	out	dx,al			;  low

	mov	enable_proc,CodeOFFSET inport_enable
	mov	disable_proc,CodeOFFSET inport_disable
	mov	si,CodeOFFSET inport_int
	mov	cx,INPORT_INT_LENGTH

show_inport_found:
	stc				;Show inport device was found

inport_end_search:
	ret

inport_search	endp
page

;---------------------------Private-Routine-----------------------------;
; inport_find_int - Find InPort Interrupt Vector
;
; An attempt will be made to find the interrupt vector that the
; InPort adapter is set for.  The allowable vectors are 2,3,4,5, and 7.
; If no interrupt vector is found, or more than one is found, then
; an error will returned.
;
; Entry:
;	DX = base port address
; Returns:
;	'C' clear if interrupt vector found
;	 mask_8259 = interrupt mask
;	 vector    = interrupt vector number
; Error Returns:
;	'C' set   if interrupt vector not found
; Registers Preserved:
;	SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	None
; History:
;	Fri 21-Aug-1987 11:43:42 -by-  Walt Moore [waltm] & Mr. Mouse
;	Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing
	assumes ss,nothing

		public	inport_find_int ;Public for debugging
inport_find_int proc	near

; if this is a INT 33 type of mouse then try to get the IRQ number from 
; the INT 33 driver.

	test	mouse_flags,MF_INT33H
	jz	int33_not_present	;it is not present

	mov	ax,INT33H_GETINFO	;get info about mouse
	int	MOUSE_SYS_VEC		;CH has type & CL has IRQ
	cmp	ch,INT33H_INPORT	;make sure it is inport
	jne	int33_not_present	;may be old driver
	or	cl,cl			;do not deal with IRQ = 0
	jz	int33_not_present	;do alternate search
	mov	al,cl			;get the IRQ into AL

	cmp	al,2			;if it's IRQ 2 & we're in pMode
	jne	@f			;  then it's gotta be on the slave
	test	WinFlags,WF_PMODE	;  8259.  Use IRQ 9 vector in
	jz	@f			;  that case.
	mov	al,71h - 8
@@:
	add	al,08h			;map IRQ to INT vector number
	mov	bh,1			;initial position for IRQ mask
	shl	bh,cl			;get inverse of the mask
	not	bh			;get the correct mak
	mov	vector,al		;Save interrupt vector #
	mov	mask_8259,bh		;Save 8259 interrupt mask
	in	al,MASK_PORT		;--> old 8259 interrupt mask
	push	ax			;Save old interrupt mask on stack
	jmp	inport_search_done	;merge with alternate code

int33_not_present:

	cli				;Must leave ints off if we're
					;  going to restore the 8259
	in	al,MASK_PORT		;--> old 8259 interrupt mask
	push	ax			;Save old interrupt mask on stack
	mov	al,INPORT_RESET 	;Reset chip
	out	dx,al
	io_delay
	io_delay
	io_delay
	mov	al,INPORT_MODE		;Select mode register
	out	dx,al			;  as the current data register
	inc	dx			;Point to data register
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1

	mov	bl,IRQ_PC_MASK_BITS	;IRQ bits to search
	test	WinFlags,WF_PMODE	;Check for master and slave
	jz	inport_no_protect
	mov	bl,IRQ_AT_M_MASK_BITS	;IRQ bits to search
inport_no_protect:
	mov	al,bl			;Disable possible mouse IRQs
	out	MASK_PORT,al
	io_delay
	mov	al,INT_REQUEST		;Select 8259 interrupt request reg
	out	ACK_PORT,al

	mov	ah,bl			;Possible interrupt bits
	mov	cx,10			;Test 10 times max

inport_find_int_loop:
	mov	al,INPORT_TIMER_IE+INPORT_0
	out	dx,al			;Generate 0 on mouse IRQ
	io_delay
	in	al,ACK_PORT		;Read interrupt bits
	and	al,bl			;Make sure only bits 7,5,4,3,2
	xor	al,bl			;Flip bits (0's-->1's & 1's-->0's)
	and	ah,al			;Eliminate non-functional irq's
	jz	inport_no_irq		;All IRQ's eliminated
	mov	al,INPORT_TIMER_IE+INPORT_1
	out	dx,al			;Generate 1 on mouse IRQ
	io_delay
	in	al,ACK_PORT		;Read interrupt bits
	and	al,bl			;Make sure only bits 7,5,4,3,2
	and	ah,al			;Eliminate non-functional IRQ's
	jz	inport_no_irq		;All IRQ's eliminated
	loop	inport_find_int_loop

	xor	bl,bl			;BL will end up with the # of IRQ's
	mov	bh,ah			;--> IRQ bit
	mov	cx,7			;Loop counter and IRQ #

inport_count_irqs:
	shl	ah,1			;Set 'C' if IRQ responded correctly
	jnc	inport_next_irq
	mov	al,cl			;Save IRQ #
	inc	bl			;Increment # of IRQ's

inport_next_irq:
	loop	inport_count_irqs
	dec	bl			;More than 1 IRQ ?
	jz	inport_found_irq	;Only 1 IRQ

inport_no_irq:
	test	WinFlags,WF_PMODE
	jnz	inport_check_slave
	stc				;Set 'C' to indicate no interrupt
	jmp	short inport_search_done

inport_check_slave:
	in	al,MASK_SLAVE_PORT	;--> old 8259 interrupt mask
	push	ax			;Save old interrupt mask on stack

	mov	bl,IRQ_AT_S_MASK_BITS	;IRQ bits to search
	mov	al,bl			;Disable possible mouse IRQs
	out	MASK_SLAVE_PORT,al
	io_delay
	mov	al,INT_REQUEST		;Select 8259 interrupt request reg
	out	ACK_SLAVE_PORT,al

	mov	ah,bl			;Possible interrupt bits
	mov	cx,10			;Test 10 times max

inport_find_slave_int_loop:
	mov	al,INPORT_TIMER_IE+INPORT_0
	out	dx,al			;Generate 0 on mouse IRQ
	io_delay
	in	al,ACK_SLAVE_PORT	;Read interrupt bits
	and	al,bl			;Make sure only bits 7,5,4,3,2
	xor	al,bl			;Flip bits (0's-->1's & 1's-->0's)
	and	ah,al			;Eliminate non-functional irq's
	jz	inport_no_slave_irq	;All IRQ's eliminated
	mov	al,INPORT_TIMER_IE+INPORT_1
	out	dx,al			;Generate 1 on mouse IRQ
	io_delay
	in	al,ACK_SLAVE_PORT	;Read interrupt bits
	and	al,bl			;Make sure only bits 7,5,4,3,2
	and	ah,al			;Eliminate non-functional IRQ's
	jz	inport_no_slave_irq	;All IRQ's eliminated
	loop	inport_find_slave_int_loop

	clc
	jmp	short inport_found_slave_irq

inport_no_slave_irq:
	stc				;Set 'C' to indicate no interrupt
inport_found_slave_irq:
	mov	al,IN_SERVICE		;Restore 8259 slave
	io_delay
	out	ACK_SLAVE_PORT,al
	pop	ax			;Restore old 8259 interrupt mask
	out	MASK_SLAVE_PORT,al
	jc	short inport_search_done
	mov	bh,00000100b
	mov	al,71h - 8

inport_found_irq:
	add	al,8			;Compute interrupt vector #
	mov	vector,al		;Save interrupt vector #
	not	bh			;Want an enable mask
	mov	mask_8259,bh		;Save 8259 interrupt mask
	clc

inport_search_done:
	mov	al,0			;Must preserve 'C'
	out	dx,al			;Tri-state the IRQ line
	mov	al,IN_SERVICE		;Restore 8259
	io_delay
	out	ACK_PORT,al
	pop	ax			;Restore old 8259 interrupt mask
	out	MASK_PORT,al
	sti				;Allow interrupts now
	ret

inport_find_int endp
page

;---------------------------Private-Routine-----------------------------;
; inport_enable - Enable InPort Mouse
;
; The InPort mouse will be initialized, the interrupt vector hooked,
; the old interrupt mask saved, and our interrupt enabled at the 8259.
;
; Entry:
;	None
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	hook_us_in
;	enable_our_int
; History:
;	Fri 21-Aug-1987 11:43:42 -by-  Walt Moore [waltm] & Mr. Mouse
;	Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing
	assumes ss,nothing

		public	inport_enable	;Public for debugging
inport_enable	proc	near

	call	hook_us_in		;Hook us into the interrupt

; if the vector that we are using for the Mouse INT matches either of the 
; serial int vectors 0bh or 0ch, we will then have to NULL out the 
; corresponding serial port address in the BIOS data area so that no one
; will try to use those ports.

	push	es			;save
	mov	bx,BIOSDataSeg		;segment for BIOS data
	mov	es,bx			;es: points to BIOS data
	assumes es,BIOSDataSeg
	xor	ax,ax			;will put in zero there
	mov	bx,offset rs232_data	;Start of serial ports
	cmp	vector,0ch		;using COMM 1's int ?
	jz	zero_out_comm_port_addr	;yes
	add	bx,2			;may be comm 2
	cmp	vector,0bh		;COMM 2's int ?
	jnz	save_comm_port_addr	;do not do anything

zero_out_comm_port_addr:

	xchg	es:[bx],ax		;get previous port addr too.

save_comm_port_addr:

	mov	OriginalCommPortAddr,ax	;if zero then was not swapped
	mov	OriginalCommAddrLoc,bx	;also save the location in 40:
	pop	es			;restore
	assumes	es,nothing

	mov	dx,io_base		;Initialize the InPort Mouse
	mov	al,INPORT_RESET 	;Reset chip
	out	dx,al
	io_delay
	io_delay
	io_delay

	mov	al,INPORT_MODE		;Select mode register
	out	dx,al			;  as the current data register
	inc	dx			;Point to data register
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1

	io_delay
	mov	al,INPORT_TIMER_IE+INPORT_HZ_30+INPORT_QUAD
	out	dx,al

	call	enable_our_int
	ret

inport_enable	endp
page

;---------------------------Private-Routine-----------------------------;
; inport_disable - Disable InPort Mouse
;
; The interrupt vector will be restored, the old interrupt mask
; restored at the 8259.  If the old mask shows that the mouse was
; previously enabled, it will remain enabled, else we will disable
; interrupts at the mouse itself.
;
; Entry:
;	None
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	unhook_us
; History:
;	Fri 21-Aug-1987 11:43:42 -by-  Walt Moore [waltm] & Mr. Mouse
;	Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing
	assumes ss,nothing

		public	inport_disable	;Public for debugging
inport_disable	proc	near

;	Disable interrupts first so no interrupt will hit if
;	only the BIOS default handler is around.

	mov	dx,io_base		;Disable IRQs at the mouse
	mov	al,INPORT_MODE		;Select mode register
	out	dx,al			;  as the current data register
	inc	dx			;Point to data register
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1
	mov	al,INPORT_QUAD+INPORT_0 ;Just leave it in quadrature mode
	out	dx,al			;  with IRQ low

; now if we had zeroed out one of the comm port addresses in the bios data
; area, restore it.

	mov	ax,OriginalCommPortAddr	;get the original address ?
	or	ax,ax			;was it zero or not put at all ?
	jz	@f			;yes,no need to put it back
	push	es			;save
	mov	bx,BIOSDataSeg		;seg value for BIOS data area
	mov	es,bx			;es: points to BIOS data area
	assumes es,BIOSDataSeg
	mov	bx,OriginalCommAddrLoc	;location of the address
	mov	es:[bx],ax		;restore it
	pop	es			;restore
	assumes	es,nothing
@@:

	call	unhook_us		;Restore everything to what it was
	jnz	inport_disable_exit	;IRQ was previously disabled


	mov	dx,io_base		;Initialize the InPort Mouse
	mov	al,INPORT_RESET 	;Reset chip
	out	dx,al
	io_delay
	io_delay
	io_delay

	mov	al,INPORT_MODE		;Select mode register
	out	dx,al			;  as the current data register
	inc	dx			;Point to data register
	.errnz	INPORT_DATA_REG-INPORT_ADDR_PTR-1

	io_delay
	mov	al,INPORT_TIMER_IE+INPORT_HZ_30+INPORT_QUAD
	out	dx,al

inport_disable_exit:
	ret

inport_disable	endp


sEnd	Code
end
