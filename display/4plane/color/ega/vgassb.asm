        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	VGASSB.ASM
;
;   This module contains the SaveScreenBitmap routine.
;
; Created: 19-May-1987
; Author:  Bob Grudem [bobgru]
;
; Adapted for VGA: 09-Jan-1989
; -by-  Amit Chatterjee [amitc]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	SaveScreenBitmap
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
;	The subroutine SaveScreenBitmap saves a single bitmap from
;	the display to unused display memory, or restores or discards
;	a bitmap from otherwise unused memory to the display.  It
;	returns an error if the memory is either nonexistent, already
;	in use, or has been changed since the last save.
;
;	If the extra memory is being used by another program, the 
;	"SHADOW_EXISTS" bit will be turned off.  When it becomes free
;	again, the "SHADOW_TRASHED" and "SHADOW_EXISTS" bits will be set.
;	Thus, whenever the "SHADOW_EXISTS" bit is set, the memory is
;	available for use by this function.
;
; Restrictions:
;
;	Only one bitmap can be saved at a time.
;       The size of the shadow area on the VGA is less than half the size
;	of the screen and so only small bitmaps can be saved.
;
;-----------------------------------------------------------------------;

;	This function will perform private stack checking.  In order for
;	private stack checking to occur, two symbols must be defined
;	prior to the inclusion of cmacros.inc.  ?CHKSTK must be defined
;	if the cmacros are to perform stack checking on procedures with
;	local parameters.  ?CHKSTKPROC must be defined if private stack
;	checking will be used.
;
;	The actual macro body for ?CHKSTKPROC will be defined later.



?CHKSTK = 1
?CHKSTKPROC	macro
		endm


	.xlist
	include	cmacros.inc
	include	gdidefs.inc
	include	display.inc
	include	egamem.inc
	include	macros.mac
	.list


	externA	ScreenSelector		;selector to the screen
	externA	SSB_START_SCAN
	externA SSB_TOTAL_SCANS
	externA SCREEN_WIDTH

	externFP bitblt 		;bitblt function


;	Allowed values for the function (cmd) SaveScreenBitmap is to
;	perform.

SSB_SAVE	equ	0
SSB_RESTORE	equ	1
SSB_IGNORE	equ	2


sBegin	Code
assumes cs,Code
assumes ds,Data
assumes es,nothing

	externW ssb_device

;--------------------------Exported-Routine-----------------------------;
; SaveScreenBitmap
;
; Move a bitmap between active and unused display memory.
;
; Entry:
;	EGA registers in default state
; Returns:
;	AX = positive if no error
; Error Returns:
;	AX = 0 if error occured
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	bitblt
; History:
;	Tue 19-May-1987 17:22:59 -by-  Bob Grudem [bobgru]
;	Created.
;	Thu 06-Aug-1987          -by-  Bob Grudem [bobgru]
;	Replaced some magic numbers with equates; cleanup up style.
;-----------------------------------------------------------------------;


cProc	SaveScreenBitmap,<FAR,PUBLIC>,<si,di>

	parmD	lprect			;--> rectangle to operate on
	parmW	cmd			;0 = save, 1 = restore, 2 = ignore

	localB	status			;shadow memory status byte
	localW	x1			;lower x boundary of rectangle
	localW	y1			;lower y boundary
	localW	y2			;upper y boundary
	localW	xExt			;width of rectangle in pixels
	localW	yExt			;height of rectangle in scan lines
	localW	xStart			;start of x in case of banding
	localW	yExtLeft		;for banding
	localW	xExtB			;byte alligned extent

cBegin


ife	???				;if no locals
	xor	ax,ax			;  check anyway
	call	my_check_stack
endif
	jc	ssb_exit_error_vect	;no room, abort



;	Read the flag byte into AL, then check to see if any operation
;	is possible.  We cannot continue if:
;
;	shadow mem does not exist
;	we are to save to shadow mem already in use
;	we are to restore from shadow mem not in use
;	we are to restore from shadow mem stolen (trashed)



ssb_chk_mem_exists:
	mov	dx,ScreenSelector
	mov	es,dx
	assumes es,EGAMem

	mov	al,shadow_mem_status	;get status byte
	test	al,SHADOW_EXISTS	;see if mem is there
	jz	ssb_exit_error_vect	;no --> get out now

	mov	status,al		;store status for future

	cmp	cmd,SSB_SAVE
	jne	ssb_restore_check

	test	al,SHADOW_IN_USE	;see if mem is already used
	jz	ssb_get_extents		; yes --> exit


ssb_exit_error_vect:
	clc
	jmp	ssb_exit_error		;jump a really long way



;	The function code was SSB_IGNORE, so clear out the SHADOW_IN_USE
;	bit and exit successfully.  Clear the SHADOW_TRASHED bit.

ssb_ignore_it:
	and	al,not (SHADOW_IN_USE or SHADOW_TRASHED)
	jmp	ssb_exit_ok


ssb_restore_check:
	cmp	cmd,SSB_RESTORE
	jne	ssb_ignore_it

	test	al,SHADOW_IN_USE	;see if mem already being used
	jz	ssb_exit_error_vect	;no --> nothing to restore

	test	al,SHADOW_TRASHED	;see if mem trashed
	jnz	ssb_exit_error_vect	;yes --> who knows what's there?


ssb_get_extents:
	lds	bx,lprect		;DS:BX --> rectangle boundaries
	assumes ds,nothing

	mov	cx,[bx]			;get x1
	mov	dx,[bx+4]		;get x2
	sub	dx,cx			;compute xExt
	mov	x1,cx			;store results
	mov	xExt,dx

	mov	cx,[bx+2]		;get y1
	mov	dx,[bx+6]		;get y2
	sub	dx,cx			;compute yExt
	mov	y1,cx			;store results
	mov	yExt,dx

;	Check for real whether we are saving or restoring.

	cmp	cmd,SSB_SAVE
	jne	ssb_do_restore

		    
;	Saving.  Align the rectangle on byte boundaries to speed up
;	bitblt.	The rop is "source copy" (S). The lpBrush and lpDrawMode
;	are not used. The physical device is the same for source and
;	destination. The point (x1,y1) is the source, and (x1,y2) is
;	the destination, where  y2 = y1 + SSB_EXTRA_SCANS.

	mov	al,byte ptr x1		;align rectangle on byte boundaries
	and	ax,00007h		;by moving x to the left, and 
	add	ax,xExt			;adding extra pixels to the
	and	byte ptr x1,0F8h	;rectangle's width

	add	ax,00007h		;round width up to the next byte
	and	al,0F8h			;to be on byte boundaries on both
	mov	xExt,ax			;sides of the rectangle


; if the yExt is more than what we have space for, try the iterative one .

	cmp	dx,SSB_TOTAL_SCANS	;more than what we can deal with ?
	jbe	@f			;no.
	jmp	ssb_iterative		;try iterative one.
@@:

	mov	y2,SSB_START_SCAN

	xor	ax,ax			;setup for passing parameters
	mov	bx,00CCh		;rop goes in BX:CX
	mov	cx,0020h
	mov	dx,CodeOFFSET ssb_device

	farPtr	dvc,<cs>,<dx>
	farPtr	rop,<bx>,<cx>
	farPtr	lpBrush,<ax>,<ax>
	farPtr	lpDrawMode,<ax>,<ax>
	arg	<dvc,x1,y2,dvc,x1,y1,xExt,yExt,rop,lpBrush,lpDrawMode>

	cCall	bitblt			;delegate

	mov	al,status		;get status byte
	or	al,SHADOW_IN_USE	;indicate mem in use
	and	al,not SHADOW_TRASHED	;indicate mem not trashed

	jmp	ssb_exit_ok


;	Restoring. The rop is "source copy" (S).  The lpBrush and
;	lpDrawMode are not used. The physical device is the same for
;	source and destination. The point (x1,y1) is the destination,
;	and (x1,y2) is the source, where  y2 = SSB_START_SCAN

ssb_do_restore:

; if the yExt is more than what we have space for, try the iterative one .

	cmp	dx,SSB_TOTAL_SCANS	;more than what we can deal with ?
	jbe	@f			;no.
	jmp	ssb_do_iterative_restore
@@:

	mov	y2,SSB_START_SCAN

	xor	ax,ax			;setup for passing parameters
	mov	bx,00CCh		;rop goes in BX:CX
	mov	cx,0020h
	mov	dx,CodeOFFSET ssb_device

	farPtr	dvc,<cs>,<dx>
	farPtr	rop,<bx>,<cx>
	farPtr	lpBrush,<ax>,<ax>
	farPtr	lpDrawMode,<ax>,<ax>
	arg	<dvc,x1,y1,dvc,x1,y2,xExt,yExt,rop,lpBrush,lpDrawMode>

	cCall	bitblt			;resurrect

	mov	al,status		;get status byte, say mem not in use
	and	al,not SHADOW_IN_USE
	jmp	ssb_exit_ok

ssb_iterative:


; check to see if we can break up the bitmap horizonatally and stack up
; pieces side by side to fit them.

	xor	ax,ax			;start at left edge
	mov	bx,yExt			;total height 

check_for_fit:

	cmp	ax,SCREEN_WIDTH		;within the horizontal width ?
	jb	@f			;yes
	jmp	ssb_exit_error		;can't fit this one
@@:
	or	bx,bx			;still some left
	js	ssb_iterative_fits	;no.
	sub	bx,SSB_TOTAL_SCANS	;make one more slice
	add	ax,xExt			;position for the next one
	jmp	short check_for_fit	;keep checking.


ssb_iterative_fits:

;	Check for real whether we are saving or restoring.

	cmp	cmd,SSB_SAVE
	jne	ssb_do_iterative_restore



;	The rop is "source copy" (S). The lpBrush and lpDrawMode
;	are not used. The physical device is the same for source and
;	destination. 
;       We have to brek up the source bitmap into bands of hgt SSB_TOTAL_SCANS
;	and start from the left edge till be have done all.


	mov	y2,SSB_START_SCAN	;destination start is fixed
	mov	ax,yExt			;total height
	mov	yExtLeft,ax		;amount to do.
	xor	ax,ax			;start at left edge
	mov	xStart,ax		;for first band

do_iterative_save:

	mov	di,SSB_TOTAL_SCANS	;assume this is height of blt
	cmp	di,yExtLeft		;is it the last blt ?
	jbe	@f			;no.
	mov	di,yExtLeft		;get the last part
@@:
	sub	yExtLeft,di		;update it
	xor	ax,ax			;setup for passing parameters
	mov	bx,00CCh		;rop goes in BX:CX
	mov	cx,0020h
	mov	dx,CodeOFFSET ssb_device

	farPtr	dvc,<cs>,<dx>
	farPtr	rop,<bx>,<cx>
	farPtr	lpBrush,<ax>,<ax>
	farPtr	lpDrawMode,<ax>,<ax>
	arg	<dvc,xStart,y2,dvc,x1,y1,xExt,di,rop,lpBrush,lpDrawMode>
	cCall	bitblt			;delegate

; check to see if we are done 

	cmp	yExtLeft,0		;all done ?
	jz	@f			;yes.
	mov	ax,xExt			;get the width
	add	xStart,ax		;position for next band
	add	y1,di			;position for next band
	jmp	short do_iterative_save
@@:

; we are all done.

	mov	al,status		;get status byte
	or	al,SHADOW_IN_USE	;indicate mem in use
	and	al,not SHADOW_TRASHED	;indicate mem not trashed

	jmp	ssb_exit_ok


;	Restoring. The rop is "source copy" (S).  The lpBrush and
;	lpDrawMode are not used. The physical device is the same for
;	source and destination. The point (x1,y1) is the destination,
;	and (x1,y2) is the source, where  y2 = SSB_START_SCAN

ssb_do_iterative_restore:
	
	xor	ax,ax			;zero out
	mov	al,byte ptr x1		;align rectangle on byte boundaries
	and	ax,00007h		;by moving x to the left, and 
	mov	xStart,ax		;start for X in shadow
	add	ax,xExt			;adding extra pixels to the
	add	ax,00007h		;round width up to the next byte
	and	al,0F8h			;to be on byte boundaries on both
	mov	xExtB,ax		;x offset to next band start


	mov	y2,SSB_START_SCAN	;destination start is fixed
	mov	ax,yExt			;total height
	mov	yExtLeft,ax		;amount to do.


do_iterative_restore:

	mov	di,SSB_TOTAL_SCANS	;assume this is height of blt
	cmp	di,yExtLeft		;is it the last blt ?
	jbe	@f			;no.
	mov	di,yExtLeft		;get the last part
@@:
	sub	yExtLeft,di		;update it

	xor	ax,ax			;setup for passing parameters
	mov	bx,00CCh		;rop goes in BX:CX
	mov	cx,0020h
	mov	dx,CodeOFFSET ssb_device

	farPtr	dvc,<cs>,<dx>
	farPtr	rop,<bx>,<cx>
	farPtr	lpBrush,<ax>,<ax>
	farPtr	lpDrawMode,<ax>,<ax>
	arg	<dvc,x1,y1,dvc,xStart,y2,xExt,di,rop,lpBrush,lpDrawMode>

	cCall	bitblt			;resurrect

; check to see if we are done 

	cmp	yExtLeft,0		;all done ?
	jz	@f			;yes.
	mov	ax,xExtB		;get the width to next band
	add	xStart,ax		;position for next band
	add	y1,di			;position for next band
	jmp	short do_iterative_restore
@@:

; we are all done.

	mov	al,status		;get status byte, say mem not in use
	and	al,not SHADOW_IN_USE


;	Store new value of shadow_mem_status in EGA memory.
;
;	AL = new value of shadow_mem_status
;
;	Note that we don't have to program any of the EGA registers
;	because bitblt has done this.

ssb_exit_ok:

	mov	dx,ScreenSelector
	mov	es,dx
	assumes es,EGAMem

	mov	shadow_mem_status,al	;store new shadow mem status

	stc				;prepare for "successful" exit	    
	jmp	ssb_exit_error		;no error here though.



;	Carry flag = 0 --> error, so exit with AX = 0
;		   = 1 --> successful, so exit with AX nonzero

ssb_exit_error:
	sbb	ax,ax

ssb_exit:

cEnd
sEnd	Code
	end

