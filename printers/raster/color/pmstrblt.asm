;----------------------------------------------------------------------------;
; Copyright (C) Microsoft Corporation 1985-1990. All Rights Reserved.        ;
;----------------------------------------------------------------------------;

 	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	PStrBlt.ASM
;
;   This module contains the PStrBlt function and the ExtendedTextOut
;   function.
;
; Created: 17-Mar-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	PStrBlt
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;


FIXED_PITCH_ON = 0			;en/disables smartfix code


;	This function will perform private stack checking.  In order for
;	private stack checking to occur, two symbols must be defined
;	prior to the inclusion of cmacros.inc.	?CHKSTK must be defined
;	if the cmacros are to perform stack checking on procedures with
;	local parameters.  ?CHKSTKPROC must be defined if private stack
;	checking will be used.
;
;	The actual macro body for ?CHKSTKPROC will be defined later.


?CHKSTK = 1				;Must be in here
?CHKSTKPROC macro lvs
extrn dmCheckStack:near
mov	ax, lvs
call	dmCheckStack
endm


COLOR_FORMAT = 103H			;3 plane device	1 bit per pel
PROTECTEDMODE = 1

	.xlist
	include cmacros.inc

incFont 	= 1			;Include control for gdidefs.inc
incDrawMode	= 1			;Include control for gdidefs.inc
	include gdidefs.inc
	include macros.mac
	include pmstrblt.inc
	include pmfntseg.inc
	include color.inc
	include	pmstrblt.pub
	.list


	??_out	strblt


	externNP p_build_string		;All other font code


sBegin	Data

sEnd	Data

sBegin	Code
assumes cs,Code


	.386p

page
;--------------------------Exported-Routine-----------------------------;
; PdmStrBlt
;
; This is the old PdmStrBlt entry point.  Null parameters are pushed
; for the ExtTextOut's extra parameters, and control given to
; ExtTextOut.
;
; Entry:
; Returns:
;	DX = Y extent of string if extent call
;	AX = X extent of string if extent call
; Error Returns:
;	DX:AX = 8000:0000H
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	See ExtTextOut
; History:
;
;       Mon 04-Dec-1989 17-32-00 -by-  Amit Chatterjee [amitc]
;       Ported this routine to work with IBMCOLOR printers. To do this a few
;       changes had to be done:
;	     (a) Adjusted to work with 3 color planes rather than 4
;            (b) Deals only with bitmaps, so all the device specific code
;	         has been yanked out.
;	     (c) The format for small color bitmaps was changed back to be in
;         	 the old style, that is, a red plane followed by a green plane 
;		 and followed by a blue plane.
;            (d) The was the physical color structure is interpreted was  
;		 changed to be appropriate for the printer driver.
;	     (e) The main entry points for dmStrBlt & dmExtTextOut are in the
;		 file RLSTRBLT.ASM.
;	Note:
;	     Support for huge bitmaps and ExtTextOut is still in although they
;	     will not currently be utilized.
;	*********************************************************************
;       AS THIS FILE IS BASED ON STRBLT.ASM IN THE DISP30 TREE OF 4 PLANE 
;	EGA/VGA DRIVERS, ANY BUG FIX THAT YOU DO IN THIS FILE WOULD PROBABLY
;       BE HAVE TO DONE FOR THE DISP30 TREE EGA/VGA DISPLAY DRIVERS TOO.
;       ********************************************************************
;
;       Thu 06-Apr-1989 09:35:55 -by-  Amit Chatterjee [amitc]
;	Modified.
;          This TextOut function has 80386 soecific code in it, so
;          will non run on 8086 or 80286 protected mode. So now all
;          the text function is being put in a separate fixed segment
;          with the 8086 TextOut appearing in another fixed segment and
;          one of these two segments chosen at enable time.	
;		. Moved Textout to _PROTECT segment
;	        . prefixed a 'P' to entry points
;		. prefixed a 'p_' to all public labels
;
;	Fri 27-Dec-1988 14:12:19 -by-  Amit Chatterjee [amitc]
;       Modified code to support >64k fonts. 
;                . The header in the font segment now has 6 byte entries
;                  per character, with the last 4 bytes being a 32 bit 
;                  pointer to the bits in the same segment.	
;		
;		 . At this point 16 bit code and 16 bit data is still being
;                  used, however where ever necessary we have used the 
;		   extended register set and the address override to take
;		   advantage of 32 bit code and data capabilities.
;
;	Wed 18-Jan-1989 15:32:50 -by-  Amit Chatterjee [amitc]
;       modified code for 16 bit gathering and included embolding code
;       in the driver
;
;	Fri 21-Oct-1988 09:10:15 -by-  Amit Chatterjee [amitc]
;       Adopted interleaved scan line format for small bitmaps as well
;
;	Thu 09-Apr-1987 13:36:08 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,Data
	assumes es,nothing


cProc	PdmStrblt,<FAR,PUBLIC,WIN,PASCAL>

	parmD	lp_device		;Destination device
	parmW	x			;Left origin of string
	parmW	y			;Top  origin of string
	parmD	lp_clip_rect		;Clipping rectangle
	parmD	lp_string		;The string itself
	parmW	count			;Number of characters in the string
	parmD	lp_font 		;Font to use
	parmD	lp_draw_mode		;Drawmode structure to use
	parmD	lp_xform		;Current text transform

;	ExtTextOut parameters we have to dummy up.
;
;	parmD	lp_dx			;Widths for the characters
;	parmD	lp_opaque_rect		;Opaquing rectangle
;	parmW	eto_options		;ExtTextOut options

cBegin	<nogen>

	pop	cx			;Save caller's return address
	pop	bx
	xor	ax,ax
	push	ax			;Push null for lp_dx
	push	ax
	push	ax			;Push null for lp_opaque_rect
	push	ax
	push	ax			;Push null for options
	push	bx			;Restore return address
	push	cx
;	jmp	ExtTextOut
	errn$	PdmEXTTextOut

cEnd	<nogen>
	page
;--------------------------Exported-Routine-----------------------------;
; ExtTextOut
;
; Entry:
; Returns:
;	DX = Y extent of string if extent call
;	AX = X extent of string if extent call
; Error Returns:
;	DX:AX = 8000:0000H
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	exclude_far
;	unexclude_far
;	......
; History:
;	Tue 18-Aug-1987 16:30:43 -by-  Walt Moore [waltm]
;	Added saving enabled_flag.
;
;	Thu 09-Apr-1987 13:36:08 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,Data
	assumes es,nothing

	define_frame PdmEXTTextOut 	;Define frame

cBegin
	push	esi			
	push	edi
	push	ebp			;save the extended register set
	if	???			;Do test only if locals
	jc	p_exit_strblt_error	;No room for locals, return now
	endif


;----------------------------------------------------------------------------;
; save the embolding flag
;----------------------------------------------------------------------------;

	push	es
	les	di,lp_xform		;the TEXTTRANSFORM structure
	mov	fontweight,0		;assume normal text
	cmp	word ptr es:[di].ftWeight,700	;bold text ?
	jb	not_bold_text		;no
	les	di,lp_font		;test the weight in the font
	cmp	word ptr es:[di].dfWeight,700
	jae	not_bold_text		;font is already bold
	mov	fontweight,1		;set bold flag
not_bold_text:
	pop	es

;----------------------------------------------------------------------------;
	call	p_get_mode		;Get data from drawmode, set flags
	call	p_get_font		;Get font data (aborts if invalid)


;	If the character count is negative, then the extent of the
;	string should be calculated.  If positive, then the string
;	should be drawn.

	mov	ax,count
	or	ax,ax
	jg	p_draw_char_string	;Positive count, output string
	jz	p_only_draw_o_rect	;No chars, might have opaque rect
	mov	accel,bh		;Need to save flags for extent code
	mov	excel,bl
	call	p_comp_extent		;Compute extents, update BrkErr
	jmp	p_exit_strblt

p_exit_strblt_error:
	xor	ax,ax			;Return DX:AX = 8000:0000
	mov	dx,8000h		;  to show an error
	jmp	p_exit_strblt

v_exit_strblt_null:
	jmp	p_exit_strblt_null


;	There are no characters to output.  If there is an opaquing
;	rectangle to be filled with the background color, we still
;	must do that.

p_only_draw_o_rect:
	call	p_get_clip		;Aborts if NULL clip region

p_text_completely_clipped:
	test	bl,OPAQUE_RECT
	jz	v_exit_strblt_null	;No opaque rectangle
	mov	accel,bh
	mov	excel,bl
	call	p_get_device_data 	;Get device specific parameters
	jmp	p_maybe_output_o_rect


;	Normal text output is to occur.  Perform a quick rejection
;	on the string.	If the string is clipped then we can abort
;	now.

p_draw_char_string:
	call	p_get_clip		;Aborts if NULL clip region
	call	p_quick_clip
	test	bl,TEXT_VISIBLE
	jz	p_text_completely_clipped ;Nothing shows, see about opaque rect
	mov	accel,bh
	mov	excel,bl
	call	p_get_device_data 	;Get device specific parameters


;	Based on the characteristics of the strblt operation,
;	dispatch to the correct handler.

	mov	bl,accel		;If transparent mode and
	test	bl,IS_OPAQUE		;  there is an opaque rectangle
	jnz	p_draw_dispatch		;  then it must be output now.
	mov	al,excel
	test	al,OPAQUE_RECT
	jz	p_draw_dispatch
	and	al,not OPAQUE_RECT
	mov	excel,al
	xchg	ax,bx			;Need excel in BL
	call	p_output_o_rect
	mov	bl,accel

p_draw_dispatch:
	mov	ax,CodeOFFSET p_build_string
	xor	bl,WIDTH_IS_8
	test	bl,WEIRD_SPACING+WIDTH_IS_8
	jnz	p_invoke_string_code
if FIXED_PITCH_ON
	mov	ax,CodeOFFSET fixed_pitch_strblt
endif

p_invoke_string_code:
   	assumes ss,StrStuff
if 0
;	This code shows whats really happening, but is larger and
;	takes longer to execute than the rep mosvw below.

   	push	ss_height		;Save those stack locations which
   	push	ss_next_scan		;  will be trashed
   	push	ss_draw
   	push	ss_draw_clipped
   	push	ss_phases
   	push	word ptr ss_clip_mask
   	errnz	ss_num_chars-ss_clip_mask-1
 	push	ss_colors
	push	ss_p_frame
endif
   
	mov	dx,ds			;save low paragraph of stack seg
	mov	bx,ss
	mov	ds,bx
	mov	es,bx
	xor	si,si
	lea	di,ss_save_area

	mov	cx,8
	cld
	rep	movsw
	mov	cx,fontweight		;get the bold/normal indicator
	mov	ss_boldflag,cl		;save in in place of 'num_chars' too

	mov	ds,dx

   	call	ax			;This guy does the real work
   
	mov	dx,ds			;restore low paragraph of stack seg
	mov	bx,ss
	mov	ds,bx
	mov	es,bx
	lea	si,ss_save_area
	xor	di,di

	mov	cx,8
	cld
	rep	movsw

	mov	ds,dx

ifdef 0
;	This code shows whats really happening, but is larger and
;	takes longer to execute than the rep mosvw above.

	pop	ss_p_frame
 	pop	ss_colors
   	errnz	ss_num_chars-ss_clip_mask-1
   	pop	word ptr ss_clip_mask
   	pop	ss_phases
   	pop	ss_draw_clipped
   	pop	ss_draw
   	pop	ss_next_scan
   	pop	ss_height
   	assumes ss,nothing
endif

p_maybe_output_o_rect:
	mov	bl,excel
	test	bl,OPAQUE_RECT
	jz	p_strblt_clean_up
	call	p_output_o_rect

p_strblt_clean_up:
p_exit_strblt_null:
	xor	ax,ax			;Return DX:AX = 0000:0000
	cwd				;  to show success

p_exit_strblt:
	pop	ebp
	pop	edi
	pop	esi			;restore the extended register set
cEnd
	page
;--------------------------Private-Routine------------------------------;
; p_get_mode
;
;   Get Mode Parameters
;
;   The text/background colors and any justification information
;   is retrieved from the DrawMode structure and saved on the frame.
;
;
;   The following "accel" flags will be set accordingly:
;
;	HAVE_BRK_EXTRA	Set if extra pixels to insert each break character
;
;	DDA_NEEDED	Set if some number of pixels to distribute over
;			the break characters.  BrkRem, BrkErr, and
;			BrkCount are only valid if this bit is set.
;
;	HAVE_CHAR_EXTRA Set if there are extra pixels to be instered
;			between every character.  CharXtra is only
;			valid if this bit is set
;
;	IS_OPAQUE	Set if the background mode is opaque
;
;	HAVE_WIDTH_VECT Set if a non-null width vector was given
;
;
;
;   The following "excel" flags will be set if needed:
;
;	None
;
;
;   The following frame variables are initialized:
;
;	colors		Text/background colors
;	tot_brk_extra	Total break extra
;	char_xtra	# extra pixels to add each char
;
;
;   The following frame variables are initialized if needed:
;
;	brk_extra	# extra pixels to add each break char
;	brk_err 	Justification DDA error term
;	brk_count	DDA - # breaks into which brk_rem
;	brk_rem 	   extra pixels are distributed
;
; Entry:
;	None
; Returns:
;	BH = accel flags
;	BL = excel flags
; Error Returns:
;	None
; Registers Preserved:
;	DX,DI,ES,BP
; Registers Destroyed:
;	AX,CX,SI,DS,FLAGS
; Calls:
;	None
; History:
;	Thu 19-Feb-1987 14:06:41 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_get_mode proc	near

	mov	bx,off_lp_dx
	or	bx,seg_lp_dx
	neg	bx			;'C' set if width vector
	sbb	bx,bx			;BX = FFFF if width vector, else 0
	and	bx,HAVE_WIDTH_VECT shl (8-2) ;Also clears BL for "excel" flags

	lds	si,lp_draw_mode 	;Most information is in the DrawMode
	assumes ds,nothing

	mov	al,[si].TextColor.mono ;Get text color info
	mov	ah,[si].bkColor.mono   ;Get background color info
	mov	colors,ax
	errnz	FOREGROUND
	errnz	BACKGROUND-1

	mov	ax,[si].TBreakExtra	;Get any justification
	mov	tot_brk_extra,ax	;Needed for QuickClip code
	or	ax,ax
	jz	p_get_mode_char_extra	;No justification


;	Some text justification will be required for the string.
;	If BreakRem is zero, then the only action required is to
;	adjust by BreakExtra pixels for each break character
;	encountered.  If BreakRem is non-zero, then the DDA must
;	be run to distribute the remaining pixels.

	mov	cx,[si].BreakExtra
	mov	brk_extra,cx		;Save for justification code
	jcxz	p_get_mode_check_for_dda	;None to add
	or	bh,HAVE_BRK_EXTRA shr 2 ;Show some form of justification
	errnz	HAVE_BRK_EXTRA-00001000b;Don't want to shift it too far

p_get_mode_check_for_dda:
	mov	cx,[si].BreakRem	;Will the DDA be required?
	jcxz	p_get_mode_char_extra	;  No
	mov	brk_rem,cx		;  Yes, save count of odd pixels
	or	bh,DDA_NEEDED shr 2	;    show DDA is needed
	mov	cx,[si].BreakErr	;    save the DDA error term
	mov	brk_err,cx		;
	mov	cx,[si].BreakCount	;    save DDA addback value
	mov	brk_count,cx
	errnz	DDA_NEEDED-00000100b	;Don't want to shift it too far
p_get_mode_char_extra:
	mov	cx,[si].CharExtra	;Get possible CharExtra

; if the width array is being specified we shold not add the extra spacing for
; bold fonts

	cmp	seg_lp_dx,0		;was a width vector provided ?
	jnz	no_extra_bold_space	;yes, do not add extra space
	add	cx,fontweight		;add extra for bold fonts

no_extra_bold_space:
	mov	char_xtra,cx		;Must save regardless of being 0
	or	ax,cx			;Combine with TBreakExtra
	jl	p_get_mode_neg_spacing	;Could be stepping backwards

p_get_mode_finish_char_extra:
	neg	cx			;'C' if CX is non-zero (extra pixels)
	rcl	bh,1			;Show CHAR EXTRA if present
	errnz	HAVE_CHAR_EXTRA-00000010b

p_get_mode_get_background:
	cmp	[si].bkMode,OPAQUE	;Set 'C' if transparent (or 0)
	cmc				;'C' set if opaque
	rcl	bh,1			;Set/Clear opaque bit
	errnz	TRANSPARENT-1
	errnz	OPAQUE-2
	errnz	IS_OPAQUE-00000001b
	ret

p_get_mode_neg_spacing:
	or	bh,NEG_SPACING shr 2	;Show step backwards possible
	jmp	p_get_mode_finish_char_extra

p_get_mode endp
	page
;--------------------------Private-Routine------------------------------;
; p_get_font
;
;   Get Font Parameters
;
;   The following frame variables are initialized:
;
;	lfd			local font definition structure
;	null_char_offset	offset of the null character
;
; Entry:
;	BH = accel flags
; Returns:
;	BH = new accel flags
;	ES = SS
; Error Returns:
;	To p_exit_strblt_error if font isn't supported
; Registers Preserved:
;	BL,CX,DX
; Registers Destroyed:
;	AX,SI,DI,DS,FLAGS
; Calls:
;	None
; History:
;	Tue 17-Mar-1987 15:22:11 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_get_font proc	near

	lds	si,lp_font		;Prepare for copying data from font
	assumes ds,FontSeg

	lea	di,lfd			;Set ES:DI --> local font definition
	mov	ax,ss
	mov	es,ax
	assumes es,nothing

	cmp	si,offset fsType	;Is offset where we need it?
	jne	p_get_font_abort		;Cannot use fontseg, abort
	cmp	fsVersion,300h		;If not a version 3.00 font,
	jne	p_get_font_abort		;  cannot use it


;	Save the width of the characters.  If the font is fixed-pitch,
;	this field will be non-zero and the width of all characters.
;	If this field is zero, then the font is proportional, and
;	the width table will have to be used to compute the individual
;	character widths.
;
;	The accelerators for FIXED_PITCH and WIDTH_IS_8 will be
;	set based on this field.

	mov	si,offset fsPixWidth	;--> first field of interest
	lodsw
	stosw
	errnz	font_width		;Must be first field
;	xor	ax,ax			;force proportional width

	rol	bh,1			;Prepare the accel flag for rotating
	rol	bh,1			;  in the fixed-pitch and 8-wide flags
	neg	ax			;'C' set if fixed pitch font (AX <> 0)
	rcr	bh,1			;Rotate in fixed pitch flag
	errnz	FIXED_PITCH-01000000b

	add	ax,8			;Make AX = 0 if eight wide fixed pitch
	cmp	ax,1			;'C' if 8 wide fixed pitch
	rcr	bh,1			;Rotate in 8 wide flag
	errnz	WIDTH_IS_8-10000000b


;	Save the height of the font.  It will be required for both
;	clipping and output.

	movsw				;Move height of the font
	errnz	font_height-font_width-2
	errnz	fsPixHeight-fsPixWidth-2


;	Save the maximum width of a character.	This can be useful if
;	a quick clippping test were made to see if any clipping is
;	required for the string.

	add	si,fsMaxWidth-fsPixHeight-2
	movsw
	errnz	max_width-font_height-2


;	Save the value of the first and last characters.  Bias the
;	last character value by the first character before saving
;	it.  This will eliminate one comparison when performing the
;	range check on the character.
;
;	This
;		sub	al,lfd.first_char
;		cmp	al,lfd.last_char
;		ja	use_default
;
;	is better than
;
;		cmp	al,lfd.last_char
;		ja	use_default
;		sub	al,lfd.first_char
;		jb	use_default

	lodsw				;AL = fsFirstChar, AH = fsLastChar
	sub	ah,al			;Bias last char by first char
	stosw
	errnz	last_char-first_char-1
	errnz	first_char-max_width-2
	errnz	fsFirstChar-fsMaxWidth-2
	errnz	fsLastChar-fsFirstChar-1


;	Both the break character and the default character are already
;	biased by fsFirstChar.	Just copy them to the frame

	movsw
	errnz	default_char-last_char-1
	errnz	break_char-default_char-1
	errnz	fsDefaultChar-fsLastChar-1
	errnz	fsBreakChar-fsDefaultChar-1

	xchg	ah,al			;Compute and save the offset of
	xor	ah,ah			;  the null character
	inc	ax

;----------------------------------------------------------------------------;
; for normal code each entry is 4 bytes with last two bytes being offset to  ;
; bits, for protected mode code each entry is 6 bytes with last 4 bytes being;
; pointer to bits.							     ;
;----------------------------------------------------------------------------;

ifdef PROTECTEDMODE

     	mov	si,ax
	shl	si,1
	shiftl	ax,2
	add	si,ax

else

	shiftl	ax,2
	mov	si,ax

endif

;----------------------------------------------------------------------------;

	mov	eax,dword ptr fsCharOffset[si][PROP_OFFSET]
	mov	null_char_offset,eax


;	fsBitsPointer is the pointer to the top scan of the first
;	character in the font.	Copy it as is.	It will be altered
;	later.

	mov	si,offset fsBitsPointer
	movsw
	movsw
	errnz	lp_font_bits-break_char-1

	ret


;	This driver doesn't handle the given font.  Remove the
;	caller's return address and exit strblt with an error

p_get_font_abort:
	pop	ax			;Remove return address
	jmp	p_exit_strblt_error

p_get_font endp
	page
;--------------------------Private-Routine------------------------------;
; p_get_clip
;
; Entry:
;	BH = accel flags
; Returns:
;	BH = accel flags
;	BL = excel flags
;	clip   structure initialized
;	o_rect structure initialized if opaque rectangle given
; Error Returns:
;	None
; Registers Preserved:
;	None
; Registers Destroyed:
;	AX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;	None
; History:
;	Thu 09-Apr-1987 23:15:27 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_get_clip	proc   near

	cld
	lds	si,lp_clip_rect
	assumes ds,nothing

	mov	ax,eto_options
	test	ax,ETO_OPAQUE_CLIP+ETO_OPAQUE_FILL
	jnz	p_opaque_rect_needed

;	Only a clip rectangle is needed.  Simply copy it to the frame
;	and exit.

	mov	ax,ss
	mov	es,ax
	lea	di,clip
	movsw				;We assume that we will never
	movsw				;  get passed a null rectangle
	movsw
	movsw
	ret


;	The intersection of the clipping rectangle and the opaque
;	rectangle was null, and we were asked to use the result as
;	the real clipping rectangle.  Nothing to do now but exit.

p_null_clip_rect:
	pop	bx
	jmp	p_exit_strblt_null


p_opaque_rect_needed:
	mov	cx,seg_lp_opaque_rect	;If no rectangle, pretend it is
	jcxz	p_null_clip_rect		; clipped
	mov	di,off_lp_opaque_rect
	mov	es,cx
	assumes es,nothing

p_opaque_rect_given:
	test	ax,ETO_OPAQUE_CLIP
	jz	p_opaque_isnt_clip

;	The opaque rectangle is to be intersected with the clipping
;	region, and the result used as the actual clip region.

	lodsw				;Clip left
	errnz	left
	mov	cx,es:[di].left
	max_ax	cx
	mov	o_rect.left,ax
	mov	clip.left,ax

	lodsw				;Clip top
	errnz	top-left-2
	mov	cx,es:[di].top
	max_ax	cx
	mov	o_rect.top,ax
	mov	clip.top,ax

	lodsw				;Clip right
	errnz	right-top-2
	mov	cx,es:[di].right
	min_ax	cx
	cmp	ax,clip.left
	jle	p_null_clip_rect		;Right side <= left side, doesn't show
	mov	o_rect.right,ax
	mov	clip.right,ax

	lodsw				;Clip bottom
	errnz	bottom-right-2
	mov	cx,es:[di].bottom
	min_ax	cx
	cmp	ax,clip.top
	jle	p_null_clip_rect		;Bottom <= top, doesn't show
	mov	o_rect.bottom,ax
	mov	clip.bottom,ax
	jmp	short p_see_if_opaque_is_filled



;	The opaque rectangle is to be intersected with the clipping
;	region.  It will not be used as the actual clip rectangle.

p_opaque_isnt_clip:
	lodsw				;Clip left
	mov	clip.left,ax
	errnz	left
	mov	cx,es:[di].left
	max_ax	cx
	mov	o_rect.left,ax

	lodsw				;Clip top
	mov	clip.top,ax
	errnz	top-left-2
	mov	cx,es:[di].top
	max_ax	cx
	mov	o_rect.top,ax

	lodsw				;Clip right
	mov	clip.right,ax
	errnz	right-top-2
	mov	cx,es:[di].right
	min_ax	cx
	mov	o_rect.right,ax

;	We set the condition code for a null oapque rectangle now, but
;	we cannot make the jump until we've saved the bottom of the
;	clip region.

	cmp	ax,o_rect.left
;	jle	p_null_opaque_rect	;Hold off on the jump

	lodsw				;Clip bottom
	mov	clip.bottom,ax
	jle	p_null_opaque_rect	;Right side <= left side, doesn't show
	errnz	bottom-right-2
	mov	cx,es:[di].bottom
	min_ax	cx
	cmp	ax,o_rect.top
	jle	p_null_opaque_rect	;Bottom <= top, doesn't show
	mov	o_rect.bottom,ax


;	If the user has specified that the opaque rectangle is to
;	be filled,  then show that the rectangle is present.

p_see_if_opaque_is_filled:
	test	eto_options,ETO_OPAQUE_FILL
	jz	p_null_opaque_rect
	or	bl,OPAQUE_RECT		;Show it really exists

p_null_opaque_rect:
	ret


p_get_clip	endp
	page
;--------------------------Private-Routine------------------------------;
; p_quick_clip
;
;   Quick Clip Test
;
;   A quick test is made to see if the given string is entirely
;   clipped, possibly clipped, or completely visible.
;
;   An entirely clipped string can be discarded immediately.  A
;   completely visible string can skip the clipping code while
;   the stack data is being constructed.  A possibly clipped
;   string may or may not be clipped.  The clipping code must
;   be run while the stack data is being constructed in this
;   case.
;
;   At the same time, the string's bounding box minimum left and
;   maximum right edges are computed and saved.  This will be used
;   by the cursor exclusion code.
;
; Entry:
;	AX = character count
;	BH = accel flags
;	BL = excel flags
; Returns:
;	BH = new accel flags
;	BL = excel flags
;	text_bbox structure initialized if any text is visible
; Error Returns:
;	None
; Registers Preserved:
;	None
; Registers Destroyed:
;	AX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;	None
; History:
;	Tue 17-Mar-1987 15:22:11 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_quick_clip	proc   near


;-----------------------------------------------------------------------
;
;				   ----
;				       | compute how many scans are in
;	stuff here is clipped	       | this area.  These scans are
;				       | clipped out
;				       |
;	 -----------------------   ----
;	| top			|      | these scans fit
;	|			|  ----
;	|			|
;	|   clip rectangle	|
;	|			|
;	|			|
;	| bottom		|
;	 -----------------------
;
;
;	stuff here is clipped
;
;
;-----------------------------------------------------------------------

	xor	dx,dx			;Amount clipped on top is 0
	mov	cx,y			;Need starting Y coordinate
	mov	di,lfd.font_height	;  and height of the font
	mov	ax,cx			;Don't destroy Y coordinate
	sub	ax,clip.top		;Is Y clipped on top?
	jge	p_quick_clip_y_fits_on_top;  No, Y fits within top


;	The Y coordinate is less than the top of the clip rectangle.
;	Move the Y coordinate down the screen (+Y) by the amount
;	clipped.  If the amount clipped is >= the font height, then
;	the entire string is clipped and can be discarded.


	add	di,ax			;Subtract clip overhang from height
	jle	p_quick_clip_doesnt_show	;Nothing shows, exit now
	sub	cx,ax			;Offset starting Y coordinate
	neg	ax
	xchg	ax,dx

p_quick_clip_y_fits_on_top:
	mov	text_bbox.top,cx	;Save new Y coordinate
	mov	amt_clipped_on_top,dx	;Save for output routines


;-----------------------------------------------------------------------
;
;	The starting Y has been computed.  Compute how many scans must
;	be clipped off the bottom of the font.	The clipping can be done
;	by adjusting the height.
;
;	stuff here is clipped
;
;
;	 -----------------------
;	| top			|
;	|			|
;	|			|
;	|   clip rectangle	|
;	|			|
;	|			|  ----
;	| bottom		|      | these scans fit
;	 -----------------------   ----
;				       | compute how many scans are in
;				       | this area.  These scans are
;	stuff here is clipped	       | clipped out
;				       |
;				   ----
;
;-----------------------------------------------------------------------


	mov	ax,cx			;Leave CX = text_bbox.top
	add	ax,di			;Compute text_bbox.bottom
	mov	dx,ax
	sub	ax,clip.bottom		;Is Y clipped on the bottom?

;----------------------------------------------------------------------------;
; ax could go negative at this point, so we have to treat that as a big numbr;
; as we don't want a wrap over. If the end of the character was genuinenly   ;
; negative, we will not get here. So do a unsigned compare.		     ;
;----------------------------------------------------------------------------;

	jb	p_quick_clip_have_y_ext	;  No, height fits within clip rect

;----------------------------------------------------------------------------;
	sub	di,ax			;Clip height
	jle	p_quick_clip_doesnt_show	;Nothing shows, exit
	sub	dx,ax			;Clip bottom of text box


;	The string has been clipped in Y, and at least one scan
;	will be visible.

p_quick_clip_have_y_ext:
	mov	clipped_font_height,di	;Save height of what will be drawn
	mov	text_bbox.bottom,dx


;	If the vertical portion of the text fits within the opaque
;	rectangle, then set the BOUNDED_IN_Y flag for the code which
;	creates the stack data.  When this flag is set, that code
;	should attempt to fill to byte boundaries.

	test	bl,OPAQUE_RECT
	jz	p_quick_clip_do_x
	cmp	dx,o_rect.bottom
	jg	p_quick_clip_do_x 	;Text extends below opaque bottom
	cmp	cx,o_rect.top
	jl	p_quick_clip_do_x 	;Text extends above opaque top
	or	bl,BOUNDED_IN_Y


;-----------------------------------------------------------------------
;
;	Clip the string in X.  An attempt will be made to detect
;	the following five cases based on the starting X, the
;	character count, and the maximum width of a character.
;
;	Because of interactions with maximum character width,
;	intercharacter spacing, or break character spacing, it
;	is possible for a case1 or case3 string to incorrectly
;	show that clipping is required.
;
;	If a negative character extra, negative total break extra,
;	or the delta vector is present, then we cannot perform the
;	quick clipping since we could be stepping backwards.
;
;
;		 -----------------------
;		|			|
;		|			|
;    case1   case2	 case3	      case4   case5
;		|			|
;		|   clip rectangle	|
;		|			|
;		|			|
;		|			|
;		 -----------------------
;	       left		      right
;
;
;-----------------------------------------------------------------------


;	As long as we don't allow a pixel to be drawn at a location
;	less than the starting X, we can test for Case 5 before we
;	test for width_vector and negative spacing.

p_quick_clip_do_x:

	if 1
	mov	cx,clip.right		;Must be in cx for cant_quick_clip
	mov	di,x
	mov	text_bbox.left,di	;X is bounding box left hand side
	cmp	di,cx
	jge	p_quick_clip_doesnt_show	;Case 5
	test	bh,HAVE_WIDTH_VECT+NEG_SPACING
	jnz	p_peg_out_at_max_int

	else

	mov	cx,clip.right		;Must be in cx for cant_quick_clip
	test	bh,HAVE_WIDTH_VECT+NEG_SPACING
	jnz	cant_quick_clip
	mov	di,x
	mov	text_bbox.left,di	;X is bounding box left hand side
	cmp	di,cx
	jge	p_quick_clip_doesnt_show	;Case 5

	endif



;	Compute the maximum extent of the string.  This is:
;
;	    #chars * (maximum_width + char_extra) + total_break_extra


	mov	ax,lfd.max_width
	add	ax,char_xtra
	imul	count
	jo	p_peg_out_at_max_int
	add	ax,tot_brk_extra	;Add any extra pixels
	jo	p_peg_out_at_max_int
	add	ax,di			;AX = maximum right hand side
	jo	p_peg_out_at_max_int


;	If the starting X is less than the left side of the clip
;	rectangle, then the test will be made for cases 1,2, and 4.
;	If the starting X is within the clip rectangle, then the
;	test will be made for cases 3 or 4.


p_quick_clip_have_right_x:
	mov	dx,clip.left
	cmp	di,dx
	jge	p_quick_clip_case_3_or_4

p_quick_clip_case_1_or_2:
	cmp	ax,dx			;Crossing into the left side?
	jl	p_quick_clip_doesnt_show	;Case 1
	or	bl,CLIPPED_LEFT 	;Show clipped on left side
	mov	text_bbox.left,dx	;Set new lhs for text

p_quick_clip_case_3_or_4:
	cmp	ax,cx			;Overhanging the right side?
	jl	p_quick_clip_exit 	;  No
	or	bl,CLIPPED_RIGHT	;Right clipping needed

p_quick_clip_exit:
	or	bl,TEXT_VISIBLE 	;Some part of the text may show
	mov	text_bbox.right,ax	;Save bounding box right hand side

p_quick_clip_doesnt_show:
	ret


;	We handle the case where the string extent is unknown by
;	setting the left and right hand sides of the strings to
;	the most negative and most positive numbers and falling
;	through the normal case code.

	if	0
cant_quick_clip:
	mov	di,MOST_NEG_INT
	mov	text_bbox.left,di	;Save bounding box left hand side
	endif

p_peg_out_at_max_int:
	mov	ax,MOST_POS_INT 	;Make rhs as large as possible
	jmp	p_quick_clip_have_right_x

p_quick_clip	endp
	page
;--------------------------Private-Routine------------------------------;
; p_comp_extent
;
;   Compute Extents of the String
;
;   The extents of the string are computed and returned to the caller.
;   DrawMode data and font information has already been saved on the
;   frame.
;
; Entry:
;	AX = - character count
;	BH = accel flags
;	BL = excel flags
; Returns:
;	AX = extent of the string
;	DX = height of the string (font height)
; Error Returns:
;	None
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,ES,DS,FLAGS
; Calls:
;
; History:
;	Thu 09-Apr-1987 13:36:08 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; // The computation for fixed-pitch font extents is:
; //
; //	Y extent = height of character
; //	X extent = (width+CharExtra)*#chars + DDA interspersed pixels
; //
; //	since all characters are the same width, including the default
; //	character, and any character extra is applied to all characters.
; //
; //
; // The computation for proportional font extents is:
; //
; //	Y extent = height of character
; //					   count-1
; //					    ---
; //					    \
; //	X extent = DDA interspersed pixels +   >  width(char(n))+CharExtra
; //					    /
; //					    ---
; //					   n = 0
; //
; // If the width vector is present, or there is any negative character
; // justification, then the above doesn't hold true.  The extents must
; // be computed by checking each and every character, and tracking the
; // justification.
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing

p_comp_extent	proc near

	neg	ax			;Make character count positive
	test	bh,HAVE_WIDTH_VECT+NEG_SPACING
	jnz	p_comp_ext_hard_way	;Could step backwards
	test	bh,FIXED_PITCH
	jz	p_comp_ext_proportional	;Proportional font

	mov	dx,lfd.font_width	;Fixed pitch font
	add	dx,char_xtra		;Always add in character extra
	imul	dx			;AX = string width in pixels
	jo	p_comp_ext_peg_right	;Too large

p_comp_ext_check_for_just:
	test	bh,HAVE_BRK_EXTRA+DDA_NEEDED
	jnz	p_comp_ext_check_dda	;Justification is needed


;	The extent has been computed and is in ax.  Add the left side
;	of the string to the exclusive right hand side.

p_comp_ext_have_ext:
	mov	dx,lfd.font_height	;Set height
	ret


;	The extent of the string overflowed.  Return the most positive
;	number for the right hand side.

p_comp_ext_peg_right:
	mov	dx,x			;Set left hand side
	mov	ax,MOST_POS_INT 	;Set right hand side
	ret



;-----------------------------------------------------------------------;
;
;	Compute the extent for a string which has some form of
;	negative justification and/or a width vector.  The string
;	will be run one character at a time in the same manner as
;	when we build it up on the stack.
;
;	The actual extent code is a subroutine since it can be

;	called by the drawing code at output time, in which case
;	the dda locations must not be updated.
;
;	Currently:
;	    AX	   =  character count
;
;-----------------------------------------------------------------------;


p_comp_ext_hard_way:
	xchg	ax,cx
	call	p_worst_case_ext		;BX = X extent
	mov	ax,brk_err		;Need break remainder in AX
	jmp	p_comp_ext_save_err_term



;-----------------------------------------------------------------------;
;	The font is a proportional font.  Sum up the widths of all
;	the characters.  The summation loop will use the following
;	registers:
;
;	    DS:SI --> string
;	    ES	  --> font
;	    AX	   =  sum
;	    BX	   =  work register
;	    CX	   =  character count
;	    DL	   =  First Character
;	    DH	   =  Last Chararacter
;
;	Currently:
;	    AX	   =  character count
;	    BH	   =  accel flags
;-----------------------------------------------------------------------;


p_comp_ext_proportional:
	xchg	ax,cx			;CX = character count for scanning
	xor	ax,ax			;Initial string width = 0
	test	bh,HAVE_CHAR_EXTRA	;Any CharExtra?
	jz	p_comp_ext_no_char_extra	;  No CharExtra, so skip multiply
	mov	ax,char_xtra
	imul	cx			;AX = initial width
	add	ax,fontweight
	jo	p_comp_ext_peg_right	;Too large

p_comp_ext_no_char_extra:
	mov	dl,lfd.first_char
	mov	dh,lfd.last_char	;Last char was mapped by p_get_font
	lds	si,lp_string
	assumes ds,nothing

	mov	es,seg_lp_font
	assumes es,FontSeg

p_comp_ext_prop_loop:
	mov	bl,[si] 		;Get next character
	inc	si			;--> next character
	sub	bl,dl			;Make character zero based
	cmp	bl,dh			;Is this character out of range?
	ja	p_comp_ext_prop_default	;  Yes, use default character

p_comp_ext_prop_add_width:
	xor	bh,bh			;Compute pointer to the offset

;----------------------------------------------------------------------------;
; for normal code each entry in header table is 4 bytes with the last 2 bytes;
; being the pointer to the bits. For protected mode code each entry is of 6  ;
; bytes with the last 4 bytes being the pointer to the bits.		     ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

	push	ax
	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
	pop	ax

else

	shiftl	bx,2

endif

;----------------------------------------------------------------------------;

	add	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	jo	p_comp_ext_peg_right	;Too large
	loop	p_comp_ext_prop_loop
	mov	bh,accel		;DDA code expects this
	jmp	p_comp_ext_check_for_just ;Have the extent, maybe exit?

p_comp_ext_prop_default:
	mov	bl,lfd.default_char	;Load default character
	jmp	p_comp_ext_prop_add_width



;-----------------------------------------------------------------------;
;	Justification other than the width vector is involved for
;	the string.  Compute how many extra pixels would be added
;	to the string if it were drawn.  Also update the DDA error
;	term.
;
;	Currently:
;		AX = X extent so far
;		BH = accel flags
;-----------------------------------------------------------------------;

p_comp_ext_check_dda:
	cld				;Yech!
	xchg	ax,bx			;BX = extent so far
	xor	dx,dx			;Clear break char count
	mov	cx,count		;Set count of characters to search
	neg	cx			;Remember that it was negative
	mov	al,lfd.break_char	;Load break character for scanning
	mov	ah,al			;Keep copy for default=break case
	add	al,lfd.first_char	;Break char is relative to first char
	les	di,lp_string
	assumes es,nothing

p_comp_ext_scan_break:
	repne	scasb			;Search for a match
	jne	p_comp_ext_done_scanning	;Ran out of text
	inc	dx			;Show one more match
	inc	cx			;Must handle the condition where
	loop	p_comp_ext_scan_break	;  the match was on the last character

p_comp_ext_done_scanning:




;-----------------------------------------------------------------------;
;	If the default character is the same as the break character,
;	then the string must be scanned looking for all characters
;	which are out of range.
;
;	This is handled seperately from the loop above since all of
;	the raster fonts shipped with Windows have the default char
;	different from the break char, and I don't want to waste the
;	time if I don't have to.
;-----------------------------------------------------------------------;


	cmp	ah,lfd.break_char	;Is break and default the same?
	jne	p_comp_ext_have_breaks	;  No, have found all break chars
	mov	di,dx			;Set DI = breaks detected so far
	mov	dl,lfd.first_char
	mov	dh,lfd.last_char	;Last char was mapped by p_get_font
	mov	cx,count		;Set count of characters to search
	neg	cx
	lds	si,lp_string
	assumes ds,nothing

p_comp_ext_slow_scan:
	lodsb				;Get next character
	sub	al,dl			;Make character zero based
	cmp	al,dh			;Is character out of range?
	ja	p_comp_ext_invalid_char	;  Yes, go show one more break
	loop	p_comp_ext_slow_scan
	jmp	short p_comp_ext_done_slow_scan

p_comp_ext_invalid_char:
	inc	di
	loop	p_comp_ext_slow_scan

p_comp_ext_done_slow_scan:
	mov	dx,di			;Need break char count in DX


;-----------------------------------------------------------------------;
;	The number of break characters has been found.	If too many
;	were found, then we could limit them to the maximum specified
;	by BreakCount.	 There is a problem with this:
;
;	    The string to be output could be passed via multiple
;	    calls to STRBLT.  Since there would be no way of keeping
;	    track of the number of BreakChars encountered over these
;	    multiple calls, the output routine could actually recognize
;	    more than BreakCount BreakChars, making the string longer
;	    than the extent returned by this routine.
;
;	Therefore, BreakCount will not be recognized by this code.  It
;	is the caller's responsibility to make sure BreakCount was set
;	correctly.
;
;	Currently:
;		BX = extent so far
;		DX = number of break characters found
;-----------------------------------------------------------------------;

p_comp_ext_have_breaks:
	mov	cx,dx			;CX = # break characters found
	jcxz	p_comp_ext_done_dda	;No break chars, don't update BreakErr
	mov	ax,brk_extra		;Will always add BreakExtra
	mul	cx			;  for each break character
	add	bx,ax			;(TBreakExtra must have been <32k)
	jo	p_v_comp_ext_peg_right	;Too large


;-----------------------------------------------------------------------;
;	The extent for all the characters, CharExtra, and BreakExtra
;	has been computed.  All that needs to be done now is run the DDA.
;
;	Currently:	BX     =  X extent so far
;			CX     =  # break characters in string (or BreakCount)
;			DI     =  BreakCount
;			DS:SI --> DrawMode
;
;
;	The DDA will use the following registers:
;
;		AX = BreakErr
;		BX = X extent so far
;		CX = # break characters in string
;		DX = BreakRem
;		DI = BreakCount
 ;-----------------------------------------------------------------------;


	test	accel,DDA_NEEDED
	jz	p_comp_ext_done_dda
	mov	dx,brk_rem
	mov	ax,brk_err
	mov	di,brk_count		;Get max # of break characters

p_comp_ext_run_dda:
	sub	ax,dx			;BreakErr -= BreakRem
	jg	p_comp_ext_dont_distribute;Don't distribute pixel here
	add	ax,di			;BreakErr += BreakCount
	inc	bx			;Distribute one pixel here
	jo	p_comp_ext_peg_and_save	;Too large

p_comp_ext_dont_distribute:
	loop	p_comp_ext_run_dda

p_comp_ext_save_err_term:
	lds	si,lp_draw_mode
	assumes ds,nothing
	mov	[si].BreakErr,ax	;Set new remainder

p_comp_ext_done_dda:
	xchg	ax,bx			;Need X extent in AX
	jmp	p_comp_ext_have_ext

p_comp_ext_peg_and_save:
	mov	bx,MOST_POS_INT 	;Return maximum value
	jmp	p_comp_ext_save_err_term	;  and update BreakErr

p_v_comp_ext_peg_right:
	jmp	p_comp_ext_peg_right

p_comp_extent	endp


;--------------------------Private-Routine------------------------------;
; p_worst_case_ext
;
;   Compute Extents for Worst Case String
;
;   Compute the extent for a string which has some form of
;   negative justification and/or a width vector.  The string
;   will be run one character at a time in the same manner as
;   when we build it up on the stack.
;
;   Since it is possible for characters to overlap, the extent
;   of the string may be greater than where the next character
;   would start (think about a "w" and an "i" which started at
;   the same location).
;
;   The actual extent code is a subroutine since it can be
;   called by the drawing code at output time, in which case
;   the dda locations must not be updated.
;
; Entry:
;	CX = character count
; Returns:
;	BX = extent of the string
; Error Returns:
;	None
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,ES,DS,FLAGS
; Calls:
;
; History:
;	Thu 09-Apr-1987 13:36:08 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

		public p_worst_case_ext
p_worst_case_ext	proc   near

	lds	si,lp_string
	assumes ds,nothing

	mov	es,seg_lp_font
	assumes es,FontSeg

	xor	di,di			;DI will be char's starting X
	mov	num_null_pixels,di	;Keep rhs in here
	jmp	short p_wce_next_char

p_wce_bad_char:
	mov	al,lfd.default_char	;Character was out of range,
	jmp	short p_wce_good_character

p_wce_have_break_char:
	mov	bl,accel
	and	bl,DDA_NEEDED+HAVE_BRK_EXTRA
	jz	p_wce_have_tot_extra	;Must have only been char extra
	add	dx,brk_extra		;Extra every break (0 if none)
	test	bl,DDA_NEEDED
	jz	p_wce_have_tot_extra
	mov	bx,brk_rem		;If the dda is stepping left instead
	or	bx,bx			;  of stepping right, then brk_rem
	jl	p_wce_neg_dda		;  will be negative
	sub	brk_err,bx		;Run dda and add in an extra pixel
	jg	p_wce_have_tot_extra	;  if needed.
	mov	bx,brk_count
	add	brk_err,bx
	inc	dx			;Add one pixel for the dda
	jmp	short p_wce_have_tot_extra

p_wce_neg_dda:
	add	brk_err,bx		;Run dda and subtract an extra pixel
	jg	p_wce_have_tot_extra	;  if needed.
	mov	bx,brk_count
	add	brk_err,bx
	dec	dx			;Subtract one pixel for the dda
	jmp	short p_wce_have_tot_extra

p_wce_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	p_wce_bad_char

p_wce_good_character:
	mov	dx,char_xtra		;Base amount of extra pixels needed
	cmp	al,lfd.break_char
	je	p_wce_have_break_char	;Go compute dda added pixels

p_wce_have_tot_extra:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code each entry in header table is 4 bytes with the last 2 bytes;
; being the pointer to the bits. For protected mode code each entry is of 6  ;
; bytes with the last 4 bytes being the pointer to the bits.		     ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax

else

	shiftl	bx,2

endif

;----------------------------------------------------------------------------;

	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	test	accel,HAVE_WIDTH_VECT
	jz	p_wce_have_width

p_wce_get_users_width:
	lds	bx,lp_dx
	assumes ds,nothing
	add	dx,wptr [bx]		;DX is delta to next char's start
	inc	bx
	inc	bx
	mov	off_lp_dx,bx
	mov	ds,seg_lp_string
	assumes ds,nothing
	sub	dx,ax			;DX = number extra pixels


;	DX contains any adjustment to the next characters starting
;	position.  If this number is negative, then we'll be stepping
;	over the previous character.
;
;	We don't allow a character to start before the previous character.
;	We'll have to enforce this at this point by making sure that any
;	negative adjustment is less than or equal to the character width.
;
;	Whenever we step backwards, we set a flag for the output routine
;	that backwards stepping can occur, except on the very last char.

p_wce_have_width:
	or	dx,dx
	jge	p_wce_adjustment_ok
	dec	cx
	jz	p_wce_step_back_on_last
	or	wc_flags,STEPPED_BACK
	add	dx,ax			;Don't allow the backwards step
	sbb	bx,bx			;  character
	and	dx,bx
	sub	dx,ax

p_wce_step_back_on_last:
	inc	cx

p_wce_adjustment_ok:
	mov	bx,num_null_pixels	;Want to determine the maximum right
	add	di,ax			;  side of the string, which is kept
	jo	p_wce_peg_out		;  in num_null_pixels
	cmp	bx,di
	jg	p_wce_char_not_new_rhs
	mov	bx,di

p_wce_char_not_new_rhs:
	add	di,dx			;Add in any adjustment to next char's
	jo	p_wce_peg_out		;  starting position.
	cmp	bx,di
	jg	p_wce_extra_not_new_rhs
	mov	bx,di

p_wce_extra_not_new_rhs:
	mov	num_null_pixels,bx

p_wce_see_if_next:
	loop	p_wce_next_char		;Until all characters processed
	ret				;Return X extent in BX

p_wce_peg_out:
	mov	bx,MOST_POS_INT
	ret

p_worst_case_ext	endp

	page
;--------------------------Private-Routine------------------------------;
; p_comp_byte_interval
;
;   A interval will be computed for byte boundaries.
;
;   A first mask and a last mask will be calculated, and possibly
;   combined into the inner loop count.  If no first byte exists,
;   the start address will be incremented to adjust for it.
;
; Entry:
;	BX    = right coordinate (exclusive)
;	DX    = left coordinate  (inclusive)
; Returns:
;	'C' clear
;	DI    = offset to first byte to be altered in the scan
;	SI    = inner loop count
;	AL    = first byte mask (possibly 0)
;	AH    = last  byte mask (possibly 0)
; Error Returns:
;	'C' if interval is null
; Registers Preserved:
;	ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,FLAGS
; Calls:
;	None
; History:
;	Sat 11-Apr-1987 20:39:10 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing

		   public p_comp_byte_interval
p_comp_byte_interval proc near

	sub	bx,dx			;Compute extent of interval
	jle	p_return_null_interval	;Null interval, skip it
	dec	bx			;Make interval inclusive
	mov	di,dx			;Don't destroy starting X
	shiftr	di,3			;/8 for byte address


;	We now have to determine how many bits will be affected,
;	and how they are aligned within the bytes.
;
;	(left_x MOD word_size) will give us the starting pixel
;	within the left byte/word.  Adding the inclusive extent
;	of the interval to left_x MOD word_size) and taking the
;	result MOD word_size will give us the last pixel affected
;	in the last byte/word.	These pixel indexes (0:7 for bytes,
;	0:15 for words), can be used to create the first and last
;	altered bits mask.
;
;
;	To compute the number of bytes/words in the inner loop,
;	use the second calculation above
;
;		(left_x MOD word_size) + inclusive_extent
;
;	and divide it by the word size (8/16).	This gives you
;	the following:
;
;
;	    1)	If the result is 0, then only one destination
;		byte/word is being altered.  In this case, the
;		start & ending masks should be ANDed together,
;		the innerloop count set to zero, and last_mask
;		set to to all 0's (don't alter any bits).
;
;			|      x x x x x|		|
;			|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;			 0 1 2 3 4 5 6 7
;
;			start MOD 8 = 3,  extent-1 = 4
;			3+7 DIV 8 = 0, only altering one byte
;
;
;
;	    2)	If the result is 1, then only two bytes/words
;		will be altered.  In this case, the start and
;		ending masks are valid, and all that needs to
;		be done is set the innerloop count to 0.
;
;			|  x x x x x x x|x x x x x x x|
;			|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;			 0 1 2 3 4 5 6 7
;
;			start MOD 8 = 1,  extent-1 = 14
;			3+14 DIV 8 = 1.  There is a first and last
;			byte but no innerloop count
;
;
;
;	    3)	If the result is > 1, then some number of entire
;		bytes/words will be altered by the innerloop.  In
;		this case the number of innerloop bytes/words will
;		be the result - 1.
;
;			|	       x|x x x x x x x x|x
;			|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
;			 0 1 2 3 4 5 6 7
;
;			start MOD 8 = 7,  extent-1 = 9
;			7+9  DIV 8 = 2.  There is a first and last
;			byte and an innerloop count of 1 (result - 1)



;	Compute the starting bit position on the left and the ending
;	bit position on the right


	and	dl,00000111b		;Compute bit index for left side
	xor	dh,dh
	add	bx,dx			;Compute bit index for right side
	mov	si,bx			;(save for inner loop count)
	and	bl,00000111b
	mov	cl,dl			;Compute left side altered bits mask
	mov	ax,0FFFFh
	mov	dx,ax			;Need this here later
	shr	al,cl			;AL = left side altered bytes mask
	mov	cl,bl			;Compute right side altered bits mask
	mov	ah,80h
	sar	ah,cl			;AH = right side altered bits mask
	shiftr	si,3			;Compute inner byte count
	jnz	p_comp_byte_dont_combine	;loop count + 1 > 0, check it out


;	Only one byte will be affected.  Combine the first and
;	last byte masks, and set the loop count to 0.

	and	al,ah			;Will use first byte mask only
	xor	ah,ah			;Want last byte mask to be 0
	inc	si			;Fall through to set 0

p_comp_byte_dont_combine:
	dec	si			;Dec inner loop count (might become 0)


;	If all pixels in the first byte are altered, combine the
;	first byte into the inner loop and clear the first byte
;	mask.  Ditto for the last byte

	cmp	al,dl			;Set 'C' if not all pixels 1
	sbb	si,dx			;If no 'C', sub -1 (add 1), else sub 0
	cmp	al,dl			;Set 'C' if not all pixels 1
	sbb	al,dl			;If no 'C', sub -1 (add 1), else sub 0

	cmp	ah,dl			;Set 'C' if not all pixels 1
	sbb	si,dx			;If no 'C', sub -1 (add 1), else sub 0
	cmp	ah,dl			;Set 'C' if not all pixels 1
	sbb	ah,dl			;If no 'C', sub -1 (add 1), else sub 0
	clc				;Carry clear to show there is something
	ret

p_return_null_interval:			;Null interval, skip it
	stc
	ret

p_comp_byte_interval endp
;--------------------------Private-Routine------------------------------;
; p_color_bitmap_opaque
;
;   Fill the given bitmap with all the background color
;
; Entry:
;	AL = first byte mask
;	AH = last byte mask
;	BX = height
;	DX = next scan increment
;	SI = inner loop count
;	ES:DI --> destination
;	DS:DI --> destination
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	DX,SI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,DI,FLAGS
; Calls:
;	None
; History:
;	Sat 11-Apr-1987 22:43:48 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_color_bitmap_opaque	proc near

	mov	cl,num_planes		;Loop count
	mov	ch,bptr colors[BACKGROUND]

p_color_bitmap_opaque_loop:
	shr	ch,1			;Set color
	dec	cl			;DEC preserves 'C'
	jz	p_bitmap_opaque		;If last plane, just jump to it
	push	cx
	push	ax
	push	bx
	push	dx
	push	si
	push	di
	call	p_bitmap_opaque
	pop	di
	pop	si
	pop	dx
	pop	bx
	pop	ax
	pop	cx
	add	di,next_plane
	jmp	p_color_bitmap_opaque_loop

p_color_bitmap_opaque	endp
	page
;--------------------------Private-Routine------------------------------;
; p_mono_bitmap_opaque
;
;   Fill the given bitmap with all 1's or all 0's
;
; Entry:
;	AL = first byte mask
;	AH = last byte mask
;	BX = height
;	DX = next scan increment
;	SI = inner loop count
;	ES:DI --> destination
;	DS:DI --> destination
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	DX,SI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,DI,FLAGS
; Calls:
;	None
; History:
;	Sat 11-Apr-1987 22:43:48 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_mono_bitmap_opaque   proc near

	mov	cl,bptr colors[BACKGROUND] ;Set 'C' to fill color
	shr	cl,1
;	jmp	p_bitmap_opaque
	errn$	p_bitmap_opaque

p_mono_bitmap_opaque   endp
	page
;--------------------------Private-Routine------------------------------;
; p_bitmap_opaque
;
;   Fill the given bitmap with all 1's or all 0's
;
; Entry:
;	AL = first byte mask
;	AH = last byte mask
;	BX = height
;	DX = next scan increment
;	SI = inner loop count
;	ES:DI --> destination
;	DS:DI --> destination
;	'C' set   if to fill with 1's
;	'C' clear if to fill with 0's
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	DX,SI,DS,ES,BP
; Registers Destroyed:
;	AX,BX,CX,DX,DI,FLAGS
; Calls:
;	None
; History:
;	Sat 11-Apr-1987 22:43:48 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_bitmap_opaque	proc near

	cld
	jnc	p_bitmap_opaque_0s	;Filling with 0's


;	Filling the bitmap with all 1's.  Simply OR in the first
;	and/or last byte masks, and STOSW 0FFFF for the inner loop.

	or	ax,ax			;If no first or last byte
	jz	p_bo1_inner_start 	;  then only inner loop exists
	mov	cx,di			;If there is a first byte, then
	cmp	al,1			;  increment destination pointer
	sbb	cx,-1			;  for the inner loop.
	push	cx			;Save inner loop start addr
	mov	cx,bx			;Set CX = height
	or	ah,ah
	jz	p_bo1_first_only		;No last byte
	or	al,al
	jz	p_bo1_last_only		;No first byte


p_bo1_first_and_last:
	xchg	bx,si			;Set BX = delta from first to last
	inc	bx
p_bo1_both_loop:
	or	[di],al 		;Do first
	or	[di][bx],ah		;Do last
	add	di,dx			;--> next scan line
	loop	p_bo1_both_loop
	dec	bx
	xchg	bx,si
	jmp	short p_bo1_restore_start_addr


p_bo1_last_only:
	add	di,si			;--> last byte of first scan
	xchg	al,ah
p_bo1_first_only:
	or	[di],al 		;Do first
	add	di,dx			;--> next scan line
	loop	p_bo1_first_only

p_bo1_restore_start_addr:
	pop	di			;--> to first byte for inner loop

p_bo1_inner_start:
	mov	ax,0FFFFh		;Filling with 1's


;	Inner loop for filling with both 0's and 1's, or whatever
;	is in AX.

p_bo_start_inner_loop:
	or	si,si			;SI = inner loop count
	jz	p_bo_done 		;No inner loop
	sub	dx,si			;BX = delta to next scan line

p_bo_inner_loop:
	mov	cx,si
	shr	cx,1
	rep	stosw
	rcl	cx,1
	rep	stosb
	add	di,dx
	dec	bx
	jnz	p_bo_inner_loop
	add	dx,si			;Restore next scan increment

p_bo_done:
	ret



;	Filling the bitmap with all 0's.  Simply AND in the inverse
;	of the first and/or last byte masks, and STOSW 0000 for the
;	inner loop.

p_bitmap_opaque_0s:
	or	ax,ax			;If no first or last byte
	jz	p_bo_start_inner_loop	;  then only inner loop exists
	mov	cx,di			;If there is a first byte, then
	cmp	al,1			;  increment destination pointer
	sbb	cx,-1			;  for the inner loop.
	push	cx			;Save inner loop start addr
	mov	cx,bx			;Set CX = height
	or	ah,ah
	jz	p_bo0_first_only		;No last byte
	or	al,al			;'C' if AL was non-zero
	jz	p_bo0_last_only		;No first byte


p_bo0_first_and_last:
	xchg	bx,si			;Set BX = delta from first to last
	inc	bx
	not	ax
p_bo0_both_loop:
	and	[di],al 		;Do first
	and	[di][bx],ah		;Do last
	add	di,dx			;--> next scan line
	loop	p_bo0_both_loop
	dec	bx
	xchg	bx,si
	pop	di			;--> to first byte for inner loop
	xchg	ax,cx			;Set AX = 0
	jmp	short p_bo_start_inner_loop


p_bo0_last_only:
	add	di,si			;--> last byte of first scan
	xchg	al,ah
p_bo0_first_only:
	not	al
p_bo0_one_only_loop:
	and	[di],al
	add	di,dx			;--> next scan line
	loop	p_bo0_one_only_loop

p_bo0_restore_start_addr:
	pop	di			;--> to first byte for inner loop
	xchg	ax,cx			;Set AX = 0
	jmp	p_bo_start_inner_loop

p_bitmap_opaque	endp
	page
	page
;--------------------------Private-Routine------------------------------;
; p_output_o_rect
;
;   Fill the opaque rectangle with the background color.
;
; Entry:
;	BL = excel flags
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;	???
; History:
;	Tue 14-Apr-1987 12:22:31 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing

		public	p_output_o_rect
p_output_o_rect	proc	near

	mov	ax,seg_lp_surface	;DS and ES will always point to the
	mov	ds,ax			;  segment of the destination
	assumes ds,nothing		;  surface while in this routine
	mov	es,ax
	assumes es,nothing

	shiftr	bl,2
	jnc	p_oor_exclusive_rects	;No text, just output opaque rect
	errnz	TEXT_VISIBLE-00000010b

	test	accel,IS_OPAQUE 	;If in transparent mode, must always
	jz	p_oor_exclusive_rects	;  opaque the rectangle

	shr	bl,1
	jc	p_oor_check_x_bounds	;Skip Y range check
	errnz	BOUNDED_IN_Y-00000100b


;	We have no idea if any of the text bounding box intersects
;	with the opaque rectangle.  If the intersection is empty,
;	then we can output the opaque rectangle using the same code
;	for when there isn't any text.


p_oor_check_y_bounds:
	mov	ax,text_bbox.top
	cmp	ax,o_rect.bottom
	jge	p_oor_exclusive_rects
	mov	ax,text_bbox.bottom
	cmp	ax,o_rect.top
	jle	p_oor_exclusive_rects


;	The rectangles overlap in Y.  Check to see if they overlap in X.

p_oor_check_x_bounds:
	mov	ax,text_bbox.left
	cmp	ax,o_rect.right
	jge	p_oor_exclusive_rects
	mov	ax,text_bbox.right
	cmp	ax,o_rect.left
	jnle	p_oor_inclusive_rects	;Rectangles intersect somehow



;	The rectangles are exclusive.  Just fill the opaque rectangle
;	and exit.

p_oor_exclusive_rects:
	mov	bx,o_rect.right
	mov	dx,o_rect.left
	call	p_comp_byte_interval
	xchg	ax,cx			;Save first/last mask
	mov	ax,o_rect.top		;Compute height of box and leave
	mov	bx,o_rect.bottom	;  it in BX for the opaquing
	sub	bx,ax			;  subroutines
	mul	next_scan		;Add in Y component
	add	di,ax
	add	di,off_lp_surface	;Add any original offset into surface
	xchg	ax,cx			;First/last masks need to be in AX
	mov	dx,next_scan		;Need increment to next scan line
	jmp	opaque_routine

p_oor_an_exit:
	ret


;-----------------------------------------------------------------------;
;
;	The rectangles overlap in some unknown manner.	Compute
;	all the areas which need to be opaqued, and opaque them
;	(easier said than done).
;
;
;	First the area completely above and below the text bounding
;	box will be opaqued.  The scanline information will be the
;	same for both areas (same first/last masks and inner loop
;	counts).  Remember that any of the areas could be null.
;
;
;	heights
;
;		------------------------------------  o_rect.top
;	       |				    |
;	 top   |	  opaque rectangle	    |
;	       |				    |
;	       |     --------------------------     | text_bbox.top
;	       |    |			       |    |
;	middle |    |	 text bounding box     |    |
;	       |    |			       |    |
;	       |     --------------------------     | text_bbox.bottom
;	bottom |				    |
;	       |				    |
;		------------------------------------  o_rect.bottom
;
;
;	For the top rectangle:
;
;		starting Y is o_rect.top
;		height is MAX(text_bbox.top-o_rect.top,0)
;
;	For the bottom rectangle:
;
;		starting Y is text_bbox.top
;		height is MAX(o_rect.bottom-text_bbox.bottom,0)
;
;	For the middle rectangle:
;
;		starting Y is o_rect.top + height of top
;		height is MAX(text_bbox.bottom-starting Y,0)
;
;-----------------------------------------------------------------------;


p_oor_inclusive_rects:
	mov	dx,text_bbox.top	;Compute opaque rect area above the
	mov	ax,o_rect.top		;  text bounding box
	sub	dx,ax
	sbb	cx,cx
	not	cx
	and	cx,dx			;CX = height of top, or 0
	jz	p_oor_comp_bottom
	mul	next_scan
	xchg	bx,ax			;Set BX = offset to start of top scan

p_oor_comp_bottom:
	mov	ax,text_bbox.bottom	;Compute opaque rect area below the
	mov	dx,o_rect.bottom	;  text bounding box
	sub	dx,ax
	sbb	si,si
	not	si
	and	si,dx			;SI = height of bottom, or 0
	jz	p_oor_comp_middle_y
	mul	next_scan		;AX = offset to start of top scan

p_oor_comp_middle_y:
	mov	di,o_rect.top		;Add o_rect.top and height of top
	add	di,cx			;  area to get middle Y

p_oor_see_if_top_or_bottom:
	mov	dx,si			;If both heights are 0, then
	or	dx,cx			;  no top and no bottom
	jz	p_oor_see_about_middle


;	There is at least a top or a bottom.  Compute the scan interval
;	and fill the areas.

	push	di			;Save middle area Y
	push	ax			;Save bottom area Y offset
	push	si			;Save bottom area height
	push	bx			;Save top    area Y offset
	push	cx			;Save top    area height
	mov	bx,o_rect.right
	mov	dx,o_rect.left
	call	p_comp_byte_interval
	mov	dx,next_scan		;Set next scan increment incase bitmap
	add	di,off_lp_surface	;Add any initial offset to bits/device
	pop	bx			;Get top area height
	pop	cx			;Get top area Y offset
	or	bx,bx
	jz	p_oor_see_about_bottom	;There is no top area
	push	di			;Save X component of start address
	push	ax			;Save first/last mask
	add	di,cx			;--> first byte
	call	opaque_routine
	pop	ax			;Restore first/last mask
	pop	di			;Restore X component of start address

p_oor_see_about_bottom:
	pop	bx			;Get bottom area height
	pop	cx			;Get bottom area Y offset
	or	bx,bx
	jz	p_oor_no_bottom_area	;There is no bottom area
	add	di,cx			;--> first byte
	call	opaque_routine

p_oor_no_bottom_area:
	pop	di			;Restore middle Y coordinate


p_oor_see_about_middle:
	mov	ax,o_rect.bottom	;Bottom of middle bytes is
	mov	bx,text_bbox.bottom	;  minimum of these two
	min_ax	bx
	sub	ax,di			;Compute height of middle area
	jle	p_oor_a_return		;This should never occur, but....
	mov	opaque_height,ax
	mov	ax,next_scan		;Compute Y offset of middle start
	mul	di
	add	ax,off_lp_surface
	mov	temp_off_lp_bits,ax

	mov	dx,text_bbox.right	;Compute rhs interval
	mov	bx,o_rect.right
	call	p_comp_byte_interval
	mov	bx,text_bbox.left	;For lhs interval computation
	mov	dx,o_rect.left
	jc	p_oor_check_lhs_only	;No rhs
	push	si			;Save inner loop count
	push	di			;Save X offset into scan
	push	ax			;Save first/last mask
	call	p_comp_byte_interval
	jnc	p_oor_output_both_sides	;There was a left and a right


;	There was only a rhs.  Restore the saved interval parameters
;	and output the rectangle.

p_oor_process_rhs:
	pop	ax			;Restore first/last mask
	pop	di			;Restore X offset
	pop	si			;Restore inner loop count

p_oor_output_one_rect:
	add	di,temp_off_lp_bits	;--> first byte
	mov	bx,opaque_height	;Set height
	mov	dx,next_scan		;Set index to next scanline
	jmp	opaque_routine		;Invoke opaquing routine and exit



;	There was only a lhs.  Compute the interval parameters
;	and see if the text string was bounded in Y.  If it was,
;	then any portion of the last byte of the lhs interval
;	will have been handled by the string output code.

p_oor_check_lhs_only:
	call	p_comp_byte_interval	;Compute lhs interval
	jc	p_oor_a_return		;No interval exists, exit
	jmp	p_oor_output_one_rect

p_oor_a_return:
	ret


;	The parameters for the lhs and rhs have been computed.
;	If the lhs stops in the same byte that the rhs starts,
;	then we'll want to combine the lhs last byte mask with
;	the rhs first byte mask.

p_oor_output_both_sides:
	mov	cx,text_bbox.left	;Compute the byte address of the
	and	cl,11111000b		;  start and end of the text string
	mov	dx,text_bbox.right
	and	dl,11111000b
	cmp	dx,cx
	jne	p_oor_output_both_anyway	;Ends/starts in different bytes


;	We have to combine the last byte mask of the lhs with the
;	first byte mask of the rhs.  If one of the intervals consists
;	only of the mask, we want to make it disappear.  Both the last
;	byte mask for the lhs and the first byte mask for the rhs must
;	exist.

p_oor_overlapping_middle:
	pop	bx			;Restore rhs first/last mask
	pop	dx			;Restore rhs X offset
	pop	cx			;Restore rhs inner loop count

	or	si,si			;If an inner loop for lhs, combine
	jnz	p_oor_combine_with_lhs	;  rhs first mask with lhs last mask
	or	ah,ah			;If a last byte mask for lhs, combine
	jnz	p_oor_combine_with_lhs	;  rhs first byte mask with it


;	There is only a first byte on the lhs.	Combine it with the
;	rhs byte.  This will eliminate the entire lhs.	We don't
;	have to worry about there not being a rhs first byte mask
;	(and therefore decrementing the destination pointer) since
;	there must be one for the bytes to overlap.

	or	ax,bx			;Combine lhs mask with rhs masks
	mov	si,cx			;Set inner loop count
	mov	di,dx
	jmp	p_oor_output_one_rect

p_oor_combine_with_lhs:
	or	ah,bl			;Combine the masks
	xor	bl,bl			;Clear old mask
	inc	dx			;Increment rhs start offset
	push	cx			;Save inner loop count
	push	dx			;Save X offset into scan
	push	bx			;Save first/last mask

p_oor_output_both_anyway:
	call	p_oor_output_one_rect	;Do whatever for lhs
	jmp	p_oor_process_rhs 	;Do whatever for rhs

p_output_o_rect	endp
	page
;--------------------------Private-Routine------------------------------;
; p_get_device_data
;
;   All device specific local variables are intiialized.
;
;   This function must be called after both p_get_clip and p_quick_clip
;   (when the character count is positive) have been called.  This
;   requirement is because of the manner in which huge bitmaps are
;   handled), and for cursor exclusion.
;
; Entry:
;	None
; Returns:
;	None
; Error Returns:
;	to p_exit_strblt_error if device isn't enabled
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;	???
; History:
;	Tue 18-Aug-1987 16:30:43 -by-  Walt Moore [waltm]
;	Added test of the disabled flag.
;
;	Tue 14-Apr-1987 12:22:31 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing

p_v_gdd_dev_is_bitmap:
	jmp	p_gdd_dev_is_bitmap

p_gdd_device_isnt_enabled:
	pop	ax
	jmp	p_exit_strblt_error


p_get_device_data proc near

	lds	si,lp_device
;-----------------------------------------------------------------------;
;
;	This is a memory bitmap.  Initialize the frame variables for
;	either a color or a monochrome bitmap.
;
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing


p_gdd_dev_is_bitmap:
	mov	cx,[si].bmSegmentIndex	;If bmSegmentIndex is non-zero,
	or	cx,cx
	jne	p_gdd_huge_bitmap 	;  then this is a huge bitmap

p_gdd_small_bitmap:

;----------------------------------------------------------------------------;
; small bitmaps will now have interleaved scan line format                   ;
;----------------------------------------------------------------------------;

	mov	ax,[si].bmWidthBytes	;Compute index to next plane and
	mov	next_scan,ax		;  to the next scan
	mov	ax,[si].wptr bmWidthPlanes
	mov	next_plane,ax
;	mov	bx,COLOR_FORMAT
;	cmp	bx,wptr [si].bmPlanes
;	je	p_gdd_small_have_our_color
;	mov	bl,1			;Force it to be mono
;
;p_gdd_small_have_our_color:
;	xor	bh,bh
;	mul	bx
;	mov	next_plane,ax

;	mov	ax,[si].bmWidthBytes	;Save index to next plane and
;	mov	next_scan,ax		;  to next segment
;	mov	ax,[si].wptr bmWidthPlanes
;	mov	next_plane,ax
	mov	ax,wptr [si].bmBits[0]	;Save pointer to the bits
	mov	off_lp_surface,ax
	mov	ax,wptr [si].bmBits[2]
	mov	seg_lp_surface,ax

p_gdd_finish_bitmap:
	mov	ax,next_scan		;Compute adjustment from bottom
	mul	clipped_font_height	;  of character to top of
	dec	ax			;  next character
	dec	ax
	mov	cell_adjust,ax

	mov	ax,wptr [si].bmPlanes
	errnz	bmBitsPixel-bmPlanes-1
	cmp	ax,COLOR_FORMAT
	je	p_gdd_is_color_bitmap

	mov	num_planes,1		;Use constant incase bad color format
	mov	opaque_routine,CodeOFFSET p_mono_bitmap_opaque
	mov	ax,colors		;Bitmap routines expect the mono bit
	mov	cl,6			;  in D0
	shr	ax,cl
	errnz	MonoBit-01000000b
	mov	colors,ax

;-----------------------------------------------------------------------;
;
;	Create a more useful form of the background/foreground color:
;
;	XOR the two colors together.  This will give a 1 wherever
;	the colors are different.  This 1 (sign extended) will be
;	used as an AND mask with the character pattern.  The result
;	of this will be XORed with the background color to give the
;	destination byte.
;
;-----------------------------------------------------------------------;

	xor	al,ah			;Set special opaque mode color
	shl	ah,1			; D1 = background color
	or	al,ah			; D0 = invert mask
	mov	special_bm_opaque_color,al
	errnz	FOREGROUND-0
	errnz	BACKGROUND-1
	ret

p_gdd_is_color_bitmap:
	mov	num_planes,al
	errnz	bmBitsPixel-bmPlanes-1
	mov	opaque_routine,CodeOFFSET p_color_bitmap_opaque


;-----------------------------------------------------------------------;
;
;	Create a more useful form of the background/foreground color:
;
;	XOR the two colors togeteher.  This will give a 1 wherever
;	the colors are different.  This 1 (sign extended) will be
;	used as an AND mask with the character pattern.  The result
;	of this will be XORed with the background color to give the
;	destination byte.
;
;-----------------------------------------------------------------------;

	mov	ax,colors		;Bitmap routine expect the mono bit
	xor	al,ah
	mov	cx,4

p_gdd_cbm_get_color:
	shr	al,1
	rcr	bl,1
	shr	ah,1
	rcr	bl,1
	loop	p_gdd_cbm_get_color
	mov	special_bm_opaque_color,bl
	ret


;-----------------------------------------------------------------------;
;
;	This is a huge bitmap.	If the text or opaque rectangle
;	spans a segment boundary, then we'll recurse (curse?) to
;	handle the string, setting a clipping rectangle which is
;	appropriate for each 64K segment of the string.
;
;	Both the opaque rectangle and the text bounding box have
;	already been clipped in Y to the actual clipping rectangle,
;	so we are guaranteed not to overflow the real clipping
;	rectangle's Y range.
;
;-----------------------------------------------------------------------;

p_gdd_huge_bitmap:
	mov	al,excel		;If we have both, then the union
	not	al			;  of the two will be needed
	and	al,OPAQUE_RECT+TEXT_VISIBLE
	mov	ax,text_bbox.top
	mov	bx,text_bbox.bottom
	mov	cx,o_rect.top
	mov	di,o_rect.bottom
	jnz	p_gdd_dont_do_union	;Only have one rectangle
	min_ax	cx
	xchg	ax,bx
	max_ax	di
	xchg	ax,bx			;AX = top, BX = bottom
	jmp	short p_gdd_have_y_ext

p_gdd_dont_do_union:
	test	excel,TEXT_VISIBLE	;Y top and bottom for text only
	jnz	p_gdd_have_y_ext		;  is already set in AX, BX
	xchg	ax,cx			;Need Y top and bottom for the
	xchg	bx,di			;  opaque rectangle

p_gdd_have_y_ext:
	mov	clip.bottom,bx		;Save bottom and top incase
	mov	clip.top,ax		;  we cross a segment
	mov	di,[si].bmScanSegment	;#  of scanlines in each segment
	xor	dx,dx			;Compute segment and starting
	div	di			;  scan of top line
	xchg	ax,bx
	mov	cx,dx
	xor	dx,dx			;Compute segment and scan of the
	dec	ax			;  last line (want the calc to be
	div	di			;  inclusive of last line).


;-----------------------------------------------------------------------;
;
;	We now have the following information:
;
;	    BX = segment number of the starting scan line
;	    CX = starting scan line within the segment
;	    AX = segment number of the last scan line
;	    DX = ending scan line within the segment
;	    DI = # scans per segment
;
;	If everything is contained within one segment, then we blow
;	off the recursion and just output the stuff.  If seperate
;	segments are involved, we dummy up a clip rectangle and
;	call ourselves again.  If only one segment is involved,
;	we just alter the Y values a little to adjust for the actual
;	segment, and pretend we're a small bitmap.
;
;
;-----------------------------------------------------------------------;

	cmp	ax,bx
	jne	p_gdd_different_segments
	or	ax,ax			;If in first segment, don't need
	jz	p_gdd_have_segment	;  to adjust Y values
	mul	di			;Compute the number of scans to
	sub	text_bbox.top,ax	;  subtract off of the original
	sub	text_bbox.bottom,ax	;  Y's to bring them within the
	sub	o_rect.top,ax		;  allowable range for this segment
	sub	o_rect.bottom,ax
	mov	ax,[si].bmSegmentIndex	;Compute the adjustment to the
	mul	bx			;  selector

p_gdd_have_segment:
	add	ax,wptr [si].bmBits[2]	;Save pointer to the bits
	mov	seg_lp_surface,ax
	mov	ax,wptr [si].bmBits[0]
	mov	off_lp_surface,ax
	mov	ax,[si].bmWidthBytes	;Compute index to next plane and
	mov	next_plane,ax		;  to the next scan
	mov	bx,COLOR_FORMAT
	cmp	bx,wptr [si].bmPlanes
	je	p_gdd_have_our_color
	mov	bl,1			;Force it to be mono

p_gdd_have_our_color:
	xor	bh,bh
	mul	bx
	mov	next_scan,ax
	jmp	p_gdd_finish_bitmap


p_gdd_different_segments:
	push	clip.bottom		;Save until we need it


;	We now have the following information:
;
;	BX = segment number of the starting scan line
;	CX = starting scan line within the segment
;	AX = segment number of the last scan line
;	DX = ending scan line within the segment
;	DI = # scans per segment
;

	sub	ax,bx			;Number of times we'll make the call
	xchg	ax,si			;Keep count in SI
	xchg	ax,bx			;Get segment index of first scan
	inc	ax
	mul	di			;Compute end of first segment

p_gdd_do_next_whole_segment:
	mov	clip.bottom,ax

p_gdd_do_last_segment:
	lea	ax,clip
	farPtr	lp_new_clip_rect,ss,ax

	arg	lp_device		;Destination device
	arg	x			;Left origin of string
	arg	y			;Top  origin of string
	arg	lp_new_clip_rect	;Clipping rectangle
	arg	lp_string		;The string itself
	arg	count			;Number of characters in the string
	arg	lp_font 		;Font to use
	arg	lp_draw_mode		;Drawmode structure to use
	arg	lp_xform		;Current text transform
	arg	lp_dx			;Widths for the characters
	arg	lp_opaque_rect		;Opaquing rectangle
	arg	eto_options		;ExtTextOut options
	cCall	PdmEXTTextOut


;	If all went well, the first portion of the text has been
;	displayed.  Let's just keep doung it until we've done all
;	remaining segments.

	mov	ax,clip.bottom		;Set start of next segment
	mov	clip.top,ax
	add	ax,di
	dec	si
	jg	p_gdd_do_next_whole_segment


;	A little slime here folks.  The first time we'll pop the real
;	bottom scan line.  The second time, we'll pop the return address.

	pop	clip.bottom		;Set last scan
	jz	p_gdd_do_last_segment
	jmp	p_exit_strblt


p_get_device_data endp

ifdef	PUBDEFS
	include STRBLT.PUB
endif

sEnd	Code
	end

