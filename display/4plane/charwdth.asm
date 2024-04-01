	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	CHARWDTH.ASM
;
;   This module contains the function GetCharWidth which returns the
;   widths of a range of characters in a font
;
; Created: Thu 30-Apr-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1987 Microsoft Corporation
;
; Exported Functions:	GetCharWidth
;
; Public Functions:	None
;
; Public Data:		None
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;

FIREWALLS = 0

	.286

	incFont = 1

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include macros.mac
	include fontseg.inc
	include	color\ega\njmp.mac
	.list


	externA	__WinFlags

EXTRA_HEADER_STUFF  equ     (offset fsCharOffset)-(offset fsFlags)
LWINP 	equ    00000001b		;protected mode
LWIN386 equ	0000100b		;386 cpu
BOLD	equ	700			;weight of a bold font
;
;  Monday 3-October-1988 13:52   -by-   Ron Gery [rong]
; moved into fixed code segment for fonts-in-EMS

;*createSeg _TEXTSTUFF,TextSeg,word,public,CODE
;*sBegin	TextSeg
;*assumes cs,TextSeg
sBegin Code
assumes	cs,Code
page
;--------------------------Exported-Routine-----------------------------;
; GetCharWidth
;
;   Get Widths of a Range of Character
;
;   The widths of characters within the given range are returned to
;   the caller.  Characters outside of the font's range will be given
;   the width of the default character.
;
;
; Entry:
;	None
; Returns:
;	AX = 1 if success
; Error Returns:
;	AX = 0 if error
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	None
; History:
;	Thu 30-Apr-1987 20:40:21 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


assumes ds,Data
assumes es,nothing



cProc	GetCharWidth,<FAR,PUBLIC,WIN,PASCAL>,<si,di>

	parmD	lp_device
	parmD	lp_buffer
	parmW	first_char
	parmW	last_char
	parmD	lp_phys_font
	parmD	lp_draw_mode
	parmD	lp_font_trans

	localW	embolden
	localW  CPUtype
cBegin

;----------------------------------------------------------------------------;
; get and keep a copy of __WinFlags. This teels us the type of CPU. For 386  ;
; in protected mode we support > 64k fonts, else its normal font.            ;
;----------------------------------------------------------------------------;

	mov	cx,__WinFlags		;load the CPU type
	and	cx,LWINP + LWIN386	;the flags that we are interested in
	mov	CPUtype,cx

;----------------------------------------------------------------------------;
; in case of BOLD fonts the width of each character is to be reportd a 2     ;
; more. 1 seems to be obvious but WRITE expects 2.			     ;
;----------------------------------------------------------------------------;
	les	si,lp_font_trans	;get the text transform pointer
	mov	ds,seg_lp_phys_font	;Get segment of PFont. assume that
	assumes ds,FontSeg		;PFont starts at offset=0 in memory
	mov	embolden,0		;zero out the overhang
	cmp	es:[si].ftWeight,BOLD	;is it bold font ?
	jb	normal_weight		;no
	cmp	fsWeight,BOLD		;don't add embolding simulation stuff
	jnb	normal_weight		;if font is bold by nature.
	cmp	CPUtype, LWINP+LWIN386	;embolding done by driver only in
	jne	normal_weight		;386 protect mode
	mov	embolden,2		;overhang = 2 (WRITE expects 2, not 1)
normal_weight:
;-----------------------------------------------------------------------------;
	xor	cx,cx			;0 = error return code
	mov	si,first_char		;Keep first character in si
	mov	ax,last_char		;Compute number of widths to return
	sub	ax,si
	njl	exit_get_widths 	;Negative range, treat as an error
	inc	ax			;Inclusive of the last character
	cld
	les	di,lp_buffer		;Widths will be returned here
	assumes es,nothing

	mov	cx,fsPixWidth		;If width field is non-zero, then
	jcxz	get_prop_widths 	;  this is a fixed pitch font
	xchg	ax,cx			;CX = # widths, AX = width
	add	ax,embolden		;include overhang if any
	jmp	short rep_store_and_exit


get_prop_widths:
	xor	cx,cx			;Get the width of the default character
	mov	cl,fsDefaultChar
;----------------------------------------------------------------------------;
; header table has 6 byte entries for protectd mode 386, 4 byte entries elsew;
;----------------------------------------------------------------------------;

	shl	cx, 1			;multiply by two
	mov	bx, cx
	cmp	CPUtype, LWINP + LWIN386	;is it protected mode 386 ?
	jnz	four_byte_entries_1		;no

; we have 6 byte entries

	shl	cx,1			;multiply by 2 once again
	add	bx, EXTRA_HEADER_STUFF	;use big header format

four_byte_entries_1:

	add	bx,cx			;add together


	mov	cx,wptr fsFlags[bx][0]	;see definition of EXTRA_HEADER_STUFF
	add	cx,embolden		;add in overhang if any
	xchg	ax,bx			;Save buffer size/count in BX


;	Compute the number of characters outside the range of the first
;	character and return the default character's width for them.

first_default_widths:
	xor	ax,ax
	mov	al,fsFirstChar		;If caller is asking for characters
	sub	si,ax			;  before first valid character, then
	jge	proceess_valid_chars	;  give him that many default widths
	xor	ax,ax			;First real char will be index 0
	xchg	ax,si			;Get number of default widths to
	neg	ax			;  return
	min_ax	bx
	sub	bx,ax			;Update room left in buffer
	xchg	ax,cx			;AX = default width, CX = count
	rep	stosw
	xchg	ax,cx			;CX = default width


;	Compute the number of characters which reside in the font
;	that are to be returned to the caller, and return them.

proceess_valid_chars:
	xor	ax,ax			;The number of valid widths is
	mov	al,fsLastChar		;  whatever remains in the font or
	sub	ax,si			;  however much room is left in the
	jl	last_default_widths	;  buffer
	inc	ax			;Inclusive of last character
	min_ax	bx			;destroys DX
	sub	bx,ax			;Set number of last defaults
	xchg	ax,cx			;CX = # to move, AX = default width
	jcxz	done_with_valid_widths
;----------------------------------------------------------------------------;
; header table has 6 byte entries for 386 protected mode and 4 byte entries  ;
; for all other mode.							     ;
;----------------------------------------------------------------------------;
        
	xor	ax,ax
	mov	dx, 2			;# of bytes for BitsOffset
	cmp	CPUtype, LWINP + LWIN386	;is it protected mode 386 ?
	jnz	four_byte_entries_2	;no

	shl	dx, 1			;make that 4 bytes for BitsOffset
	mov	ax,si
	shl	ax,1			;multiply by 2
	add	ax, EXTRA_HEADER_STUFF	;use big font header

four_byte_entries_2:

	shiftl	si,2
	add	si,ax
	add	si,offset fsFlags

get_widths_loop:
	lodsw				;get the width
	add	ax,embolden		;add in overhang if any
	stosw				;store it
	add	si, dx			;skip over fsBitsOffset
	loop	get_widths_loop

done_with_valid_widths:
	xchg	ax,cx

last_default_widths:
	xchg	ax,cx			;AX = default width
	mov	cx,bx			;Set remaining buffer slots

rep_store_and_exit:
	rep	stosw			;Leaves CX = 0
	inc	cx			;CX = 1 to show success

exit_get_widths:
	xchg	ax,cx			;Return code was in CX


cEnd

;*sEnd	TextStuff
sEnd	Code
end
