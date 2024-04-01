	page	,132
;----------------------------Module-Header------------------------------;
; Module Name: EGAINIT.ASM
;
; EGA initialization code.
; 
; Created: 26 June 1987
; Author: Bob Grudem
;
; Copyright (c) 1985, 1986, 1987  Microsoft Corporation
;
; This module handles disabling of the RC_SAVEBITMAP raster capability
; if the EGA doesn't have 256Kbytes of display memory.  This operation
; happens at run time, because the bit will always be set at assembly-
; time if the SaveScreenBitmap code is present in the driver, though
; a particular EGA board may not be able to use it.
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.

	title	EGA Initialization Code

incDevice = 1				;allow assembly of needed constants

	.xlist
	include cmacros.inc
	include gdidefs.inc
	.list

	??_out	egainit

	externA		ScreenSelector	; the selector for display memory
	externFP      	AllocCSToDSAlias; get a data seg alias for CS
	externFP	FreeSelector	; free the selector
	

sBegin	Data

	externW ssb_mask	;Mask for save screen bitmap bit

sEnd	Data

createSeg _INIT,InitSeg,byte,public,CODE
sBegin	InitSeg
assumes cs,InitSeg


	externW	physical_device

page

;--------------------------Public-Routine-------------------------------;
; dev_initialization - device specific initialization
;
; Any device specific initialization is performed.
;
; Entry:
;	None
; Returns:
;	AX = 1
; Registers Preserved:
;	SI,DI,BP,DS
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	int 10h
; History:
;	Mon 23-Jan-1989 13:01:00 -by-  Dave Miller
;	Changed to support 256 color modes on VRAM VGA by
;	Video Seven Inc.
;
;	Mon 21-Sep-1987 00:34:56 -by-  Walt Moore [waltm]
;	Changed it to be called from driver_initialization and
;	renamed it.
;
;	Fri 26-Jun-1987 -by- Bob Grudem    [bobgru]
;	Creation.
;-----------------------------------------------------------------------;

;----------------------------Pseudo-Code--------------------------------;
;	load registers to request amount of memory on EGA
;	call EGA BIOS
;	if less than 256Kbytes on EGA board
;		clear RC_SAVEBITMAP bit in GDI info table of driver
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing

cProc	dev_initialization,<NEAR,PUBLIC>

cBegin

;	This prevents USER from calling save_screen_bitmap when there
;	is probability zero (0) of success.

	and	ssb_mask,not RC_SAVEBITMAP

ega_init_exit:

;----------------------------------------------------------------------------;
; at this point we will fill in the  video memory segment addresses into the ;
; pdevice data steucture.					             ;
;----------------------------------------------------------------------------;

	push	es			; save
	cCall	AllocCSToDSAlias,<cs>	; get a data segment alias for cs
	mov	es,ax			; es has this code segment
	push	ax			; save a copy for FreeSelector call
	assumes es,InitSeg

	mov	ax,ScreenSelector	; get the address of memory
	mov	word ptr es:[physical_device.bmType],ax
	mov	word ptr es:[physical_device.bmBits+2],ax

	cCall	FreeSelector		; .. note 'push ax' after alloc

	pop	es			; restore es

;----------------------------------------------------------------------------;

	mov	ax,1			;no way to have error

cEnd

sEnd	InitSeg
end

