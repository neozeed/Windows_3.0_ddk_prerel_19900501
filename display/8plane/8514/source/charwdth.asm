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
.286c

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include macros.mac
	include fontseg.inc
	.list

externA __WinFlags

sBegin  code
assumes cs,code
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


assumes ds,code
assumes es,nothing



cProc	GetCharWidth, <FAR, PUBLIC, WIN, PASCAL>, <si, di>

	parmD	lp_device
	parmD	lp_buffer
	parmW	first_char
	parmW	last_char
	parmD	lp_phys_font
	parmD	lp_draw_mode
	parmD	lp_font_trans

cBegin
	xor	cx,cx			;0 = error return code
	mov	si,first_char		;Keep first character in si
	mov	ax,last_char		;Compute number of widths to return
	sub	ax,si
	jl	exit_get_widths_node	;Negative range, treat as an error
	inc	ax			;Inclusive of the last character
	cld
	les	di,lp_buffer		;Widths will be returned here
	assumes es,nothing

	mov	ds,seg_lp_phys_font	;Width table is here
	assumes ds,FontSeg

	mov	cx,fsPixWidth		;If width field is non-zero, then
	jcxz	get_prop_widths 	;  this is a fixed pitch font
	xchg	ax,cx			;CX = # widths, AX = width
	jmp	rep_store_and_exit
exit_get_widths_node:
	jmp	exit_get_widths

get_prop_widths:
	mov	dx, __WinFlags
	test	dl, 1			;are we in protect mode?
	jz	RealGetPropWidth
	test	dl, 4			;are we on a 386?
	jz	RealGetPropWidth
	xor	dx, dx			;Get the width of the default character
	mov	dl,fsDefaultChar
	shl	dx, 1
	mov	bx, dx
	shl	dx, 1
	add	bx, dx			;Width table consists of 6 bytes/entry
	mov	cx,wptr fsCharOffset[bx][0]
	xchg	ax,bx			;Save buffer size/count in BX


;	Compute the number of characters outside the range of the first
;	character and return the default character's width for them.

p_first_default_widths:
	xor	ax,ax
	mov	al,fsFirstChar		;If caller is asking for characters
	sub	si,ax			;  before first valid character, then
	jge	p_proceess_valid_chars	  ;  give him that many default widths
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

p_proceess_valid_chars:
	xor	ax,ax			;The number of valid widths is
	mov	al,fsLastChar		;  whatever remains in the font or
	sub	ax,si			;  however much room is left in the
	jl	p_last_default_widths	;  buffer
	inc	ax			;Inclusive of last character
	min_ax	bx
	sub	bx,ax			;Set number of last defaults
	xchg	ax,cx			;CX = # to move, AX = default width
	jcxz	p_done_with_valid_widths
	shl	si, 1
	mov	dx, si
	shl	dx, 1
	add	si, dx
	add	si,offset fsCharOffset

p_get_widths_loop:
	movsw				;Move next width
	add	si, 4			;Skip the bits offset
	loop	p_get_widths_loop

p_done_with_valid_widths:
	xchg	ax,cx

p_last_default_widths:
	xchg	ax,cx			;AX = default width
	mov	cx,bx			;Set remaining buffer slots
	jmp	short rep_store_and_exit

RealGetPropWidth:
	xor	bx,bx			;Get the width of the default character
	mov	bl,fsDefaultChar
	shiftl	bx,2			;Width table consists of dwords
	mov	cx,wptr fsFlags[bx][0]	;we should be using rlfntseg.inc
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
	min_ax	bx
	sub	bx,ax			;Set number of last defaults
	xchg	ax,cx			;CX = # to move, AX = default width
	jcxz	done_with_valid_widths
	shiftl	si,2
	add	si,offset fsFlags	;in rlfntseg.inc, fsCharOffset is in
					;place of fsFlags as defined in fontseg
get_widths_loop:
	movsw				;Move next width
	inc	si			;Skip the bits offset
	inc	si
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

sEnd    code
end
