        page    ,132
;
;-----------------------------Module-Header-----------------------------;
; Module Name:	ENABLE.ASM
;
; This module contains the routine which is called when the device
; is to either enable itself or return it's GDIINFO.
;
; Created: 16-Jan-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1983-1987 Microsoft Corporation
;
; Exported Functions:	Enable
;
; Public Functions:	none
;
; Public Data:		_cstods
;
; General Description:
;
;	The Display is called to enable itself on one of two occasions.
;
;	The first situation where the Disable routine is called is
;	when Windows is starting the session.  For this situation,
;	the driver will also be asked to return information about
;	the device hardware (e.g. resolution, etc).
;
;	The second is when an old application was run (e.g. WORD).
;	In this instance, Enable will be called to enable the display
;	hardware after the old application ran.
;
;	Unfortunately, there is no way to distinguish these two modes.
;
; Restrictions:
;
; History:
;
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.

incDevice	= 1

	.xlist
	include cmacros.inc
	include macros.mac
	include gdidefs.inc
	include display.inc
	include rt.mac
        .list

	externNP hook_int_2Fh		;Hook into multiplexed interrupt
	externA  ScreenSelector 	; an import from the kernel
	externA  PHYS_DEVICE_SIZE	;Size of physical device
	externA  __WinFlags		;Windows info bit
	externFP AllocSelector		; allocate a new selector
	externFP PrestoChangeoSelector	; CS <--> DS conversion
	externFP FreeSelector		; free an allocated selector
	externFP AllocCSToDSAlias	; change a CS selector to DS

ifdef PALETTES
	externFP SetPaletteTranslate    ;in color\ega\vga\palette.asm
endif

sBegin	Data

	externW ScratchSel		; the free selector
        externW ssb_mask                ;Mask for save save screen bitmap bit

sEnd	Data


sBegin	Code
        externB ??BigFontFlags
sEnd    Code


	externA __NEXTSEG		;WINDOWS runtime segment selector

createSeg _INIT,InitSeg,word,public,CODE
sBegin	InitSeg
assumes cs,InitSeg


	externNP physical_enable	;Enable routine
	externB  physical_device	;Device physical data
	externB  info_table_base	;GDIInfo table
page
;--------------------------Exported-Routine-----------------------------;
; INT Enable(lpDevice,style,lpDeviceType,lpOutputFile,lpStuff)
; DEVICE lpDevice;		//device block or GDIInfo destination
; INT	 style; 		//Style of initialization
; LPSTR  lpDeviceType;		//Device type (i.e FX80, HP7470, ...)
; LPSTR  lpOutputFile;		//DOS output file name (if applicable)
; LPSTR  lpStuff;		//Device specific information
;
; Enable - Enable Device
;
; The given device is either initialized or the GDI information
; for the given device is returned.
;
; If style=InquireInfo, then GDI is asking that the parameters
; passed be interpreted and the appropriate GDI information
; for the device be returned in lpDevice.
;
; If style=EnableDevice, then GDI is requesting that the device
; be initialized and lpDevice be initialized with whatever
; data is needed by the device.
;
; The three other pointers passed in will be the same for both
; calls, allowing for the device to request only the minimum
; required for a device that is supported.  These will be
; ASCIIZ strings or NULL pointers if no parameter was given.
; These strings are ignored by the display drvier.
;
; For the inquire function, the number of bytes of GDIINFO placed
; into lpDevice is returned.  For the enable function, non-zero is
; returned for success.  In both cases, zero is returned for an error.
;
;
; Warnings:
;	Destroys AX,BX,CX,DX,ES,FLAGS
; Effects:
;	none
; Calls:
;	PhysicalEnable
; History:
;  Mon 23-Jan-1989 1pm -by- David Miller
; Modified to support 256 color modes of VRAM VGA.
;
;  Thu 03-Nov-1988 14:59:15 -by-  Amit Chatterjee [amitc]
; Added a call to initialize the palette translation table
;
;  Mon 21-Sep-1987 00:20:57 -by-  Walt Moore [waltm]
; Added call to hook_int_2Fh
;
;  Wed 12-Aug-1987 17:16:37 -by-  Walt Moore [waltm]
; Made non-resident.
;
;  Tue 19-May-1987 22:01:34 -by-  Bob Grudem [bobgru]
; Added code to modify GDI info table if EGA doesn't have enough
; memory to make use of save_screen_bitmap
;
;  Fri 26-Jun-1987 15:00:00 -by-  Bob Grudem [bobgru]
; Removed code mentioned above and put it in EGAINIT.ASM, in an INIT
; segment.  This restores the integrity of the device-dependence levels
; within the Mondo Tree Structure of Death.
;
;  Fri 16-Jan-1987 17:52:12 -by-  Walt Moore [waltm]
; Initial version
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; INT Enable(lpDevice,style,lpDeviceType,lpOutputFile,lpStuff)
; DEVICE lpDevice;		//device block or GDIInfo destination
; INT	 style; 		//Style of initialization
; LPSTR  lpDeviceType;		//Device type (i.e FX80, HP7470, ...)
; LPSTR  lpOutputFile;		//DOS output file name (if applicable)
; LPSTR  lpStuff;		//Device specific information
; {
;   if (style == inquire)
;   {
;	*(GDIINFO)lpDevice = (GDIINFO)info_table_base; //copy GDIINFO
;	return (sizeof(GDIINFO));
;   }
;
;   *lpDevice = (DEVICE)physical_device;    //Initialize Physical device
;   hook_int_2Fh();
;   return(physical_enable(lpDevice));	    //Initialize hardware
; }
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing

cProc	Enable,<FAR,PUBLIC,WIN,PASCAL>,<ds,es,si,di>

	parmD	lp_device		;Physical device or GDIinfo destination
	parmW	style			;Style, Enable Device, or Inquire Info
	parmD	lp_device_type		;Device type (i.e FX80, HP7470, ...)
	parmD	lp_output_file		;DOS output file name (if applicable)
	parmD	lp_stuff		;Device specific information

cBegin
	WriteAux <'Enable'>

;----------------------------------------------------------------------------;
; initialize the palette translation table by invoking SetPaletteTranslate   ;
; with a NULL pointer. Do this only if the palette manager is supported      ;
;----------------------------------------------------------------------------;

ifdef	PALETTES
	xor	ax,ax
	farPtr  <lpNULL>,ax,ax		; set up a null pointer

	arg	lpNULL
	cCall	SetPaletteTranslate     ; initialize the palette trans. table
endif

;----------------------------------------------------------------------------;

        push    ds
	mov	ax,cs			;Set up ds=cs
	mov	ds,ax
	assumes ds,InitSeg

	cld
	les	di,lp_device		;--> device structure or GDIinfo dest.
	assumes es,nothing

	and	style,InquireInfo	;Is this the inquire function?
	jnz	inquire_gdi_info	;  Yes, return GDIinfo
	errnz	InquireInfo-00000001b
	errnz	EnableDevice-00000000b
	errnz	InfoContext-8000h	;Ignore infomation context flag

; Initialize passed device block
; also change the slector in physical_device at this point

	push	es
	cCall	AllocCSToDSAlias, <cs>
	mov	es, ax
	push	ax			; save a copy on the stack
	assumes es, InitSeg
	mov	ax,ScreenSelector
	mov	word ptr es:[physical_device.bmType],	ax
	mov	word ptr es:[physical_device.bmBits+2], ax
	xor	bx,bx
	mov	es,bx			;invalidate es before freeing it
	cCall	FreeSelector
	pop	es
;
	lea	si,physical_device	;DS:SI --> physical device to copy
	mov	cx,PHYS_DEVICE_SIZE	;Set move count
	rep	movsb
	pop	ds
	assumes ds,Data

	call	hook_int_2Fh		;Hook into multiplexed interrupt
	call	physical_enable 	;Enable device
	jmp	short exit_enable
page

;	inquire_gdi_info - Inquire Device Specific Information
;
;	The GDI device specific information is returned to the caller
;
;	The information is based on the three pointers passed in.
;	Normally this data would be interpreted and the correct
;	GDINFO returned.  This allows for dynamically returning
;	the info based on the specifics of the device actually
;	being used (i.e. a driver supporting two similar plotters
;	could return the extents of the actual plotter in use).
;
;	These parameters are ignored for display drivers.
;
;	Currently:
;		ES:DI --> where GDIINFO goes
;		DS    =   CS

public	inquire_gdi_info
inquire_gdi_info:
        mov     si,InitSegOFFSET info_table_base

	mov	cx,size GDIINFO
	mov	ax,cx				; return size of GDIInfo
	rep	movsb

	pop	ds
	assumes ds,Data
	mov	bx,ssb_mask
	and	es:[di].dpRaster[-size GDIINFO],bx

	push	ax
	mov	ax,__WinFlags			; setup for 3.0 fonts
	and	ax,WF_CPU386 OR WF_PMODE
	cmp	ax,WF_CPU386 OR WF_PMODE	; 386 protected mode?
	jnz	@F
	or	wptr es:[di].dpRaster[-size GDIINFO],RC_BIGFONT ; set the big fonts bit
;
; change our code segment based variable to reflect the mode
;
	push	es
;
	xor	ax,ax
	push	ax
	cCall	AllocSelector		; get a free selector
	mov	bx,_TEXT
	cCall	PrestoChangeoSelector,<bx,ax>
	mov	es,ax
	assumes es,Code
	mov	byte ptr es:[??BigFontFlags],-1

;
        pop     es
	assumes es,nothing
;
@@:
	pop	ax
exit_enable:

cEnd

sEnd	InitSeg

	ifdef	PUBDEFS
	public	inquire_gdi_info
	public	exit_enable
        endif

end
