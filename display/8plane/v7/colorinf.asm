	page	,132
;
;-----------------------------Module-Header-----------------------------;
; Module Name:	COLORINF.ASM
;
; This module contains the color information routine.
;
; Created: 16-Feb-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1983-1987 Microsoft Corporation
;
; Exported Functions:	Control
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
;   ColorInfo is called by GDI to either convert a logical color
;   (an RGB triplet) to a physical color, or a phsyical color to
;   a logical color.
;
; Restrictions:
;
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.


	.xlist
	include cmacros.inc
	include gdidefs.inc
	include display.inc
	include macros.mac
	.list


	??_out	colorinf


	ifdef	GEN_COLOR
	externA COLOR_FORMAT		;Color format (0103h or 0104h)
	endif

	externD adPalette		;initial color table
	externB Palette			; VGA palette stuff

sBegin	Code
assumes cs,Code

	externNP sum_RGB_colors_alt

page

;--------------------------Exported-Routine-----------------------------;
;
; ColorInfo
;
;   ColorInfo accepts a logical RGB color value and returns the
;   logical RGB color value that the device can most closely represent.
;   ColorInfo also returns the device dependent, physical representation
;   of bits necessary to display the specified color on the device.
;   This information will be passed back into the driver by GDI.  GDI
;   will do no interpreting of the physical color.
;
;   Colorinfo may also be requested to convert a physical color into
;   a logical RGB color.  This is indicated by a NULL lpPColour.
;   Since this driver maintains logical and physical colors as one
;   in the same (i.e. logical white is xxFFFFFFH and physical white
;   is xxFFFFFFH), we can just pass the physical color back as the
;   logical color!  GDI will only call us to convert our own physical
;   color that was returned by this driver, so this is safe.
;
; Entry:
;	None
; Returns:
;	DX:AX = physical color or logical color as appropriate
; Error Returns:
;	None
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	sum_RGB_colors_alt
; History:
;	Mon 16-Feb-1987 18:09:09 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


assumes ds,Data
assumes es,nothing


cProc	ColorInfo,<FAR,PUBLIC,WIN,PASCAL>,<di,es>

	parmD	lpDevice		;Pointer to device
	parmD	ColorIn 		;Input color (RGB or physical)
	parmD	lpPColour		;Pointer to physical color index

cBegin
WriteAux <'ClrInf'>
	cmp	seg_lpPColour,0 	;are wt to return the RGB colour for
	mov	bx,off_ColorIn		; the indexed passed?
	je	CIReturnRGBColour	;yes! return RGB DX:AX

	;No, return closest matching index at lpPColour
	; and RGB of closest matching index in DX:AX

CIReturnPhysicalColour:
	les	di,lpPColour		;now ES:DI points to PCOLOR data Struc
	mov	ax,off_ColorIn		; Get passed RGB into DX:AX
	mov	dx,seg_ColorIn
ifdef DEBUG
	cmp	ax,083ffh
	jnz	@F
	int 1
@@:
endif

	or	dh,dh			;is this already a physical color?
	js	CIWritePhysicalColorIndex

	;Convert RGB to nearest matching index
	call	sum_RGB_colors_alt
CIWritePhysicalColorIndex:
;;;;	stosb				; save index in Pcolor
	stosw				; save index in Pcolor
	mov	bx,ax
	mov	ax,0ff00h
        stosw

; the color value we want to return is in Palette (the VGA simulated
; colors).  so convert the index to the 20 entry table of TRIPLES and
; get the color.

	xor	bh,bh
	cmp	bx,245
	jl	lower_end
	sub	bx,236			; shift upper-256 indices to upper-20

lower_end:
	mov	ax,bx
	shl	bx,1
	add	bx,ax			; index into a TRIPLEs table
	mov	ax,word ptr Palette[bx+0]
	mov	dx,word ptr Palette[bx+2]
	jmp	short CIExit

;;;if 1
;;;	   mov	   bx,ax
;;;else
;;;	   mov	   ax,off_ColorIn
;;;	   mov	   dx,seg_ColorIn
;;;	   jmp	   short CIExit 	   ; get out
;;;endif

CIReturnRGBColour:
	cmp	bl	,10
	jl	CIReturnSystemClr
	cmp	bl	,245
	ja	CIReturnSystemClr

	mov	ax	,bx
	mov	dx	,0FF00H
	jmp	CI_really_exit

CIReturnSystemClr:
        sub     bh      ,bh
	shiftl	bx,2
	mov	ax,word ptr adPalette[bx+0]
	mov	dx,word ptr adPalette[bx+2]

CIExit:
;   At this time:
;	al has Red
;	ah has green
;	dl has blue
;
	xor dh,dh			; make sure it is an RGB flag

CI_really_exit:
	cEnd


sEnd	Code
end
