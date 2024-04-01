	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	BUILDSTR.ASM
;
;   This module contains the strblt functions which build
;   up data on the stack for the actual output routines
;
; Create   17-Apr-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1987 Microsoft Corporation
;
; Exported Functions:	!!!
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


	.xlist
	include cmacros.inc
	include gdidefs.inc
	include display.inc
	include ega.inc
	include egamem.inc
	include macros.mac
	include strblt.inc
	include fontseg.inc
	include buildstr.pub
	.list
	

	externNP	p_comp_byte_interval
	externNP	p_worst_case_ext
	externNP	p_output_o_rect
 	externNP	p_preset_pro_text
	externA 	stack_top	;Stack probe location
	
	public	p_process_stack_data

;----------------------------------------------------------------------------;
; the following macro is a used as a part of the code to add a word to ebx   ;
; the word is added to bx and this macro adds 10000h to ebx if a carry is    ;
; generated.							             ;
;----------------------------------------------------------------------------;

updc_ebx  macro
	local	no_carry_from_bx

	jnc	no_carry_from_bx
	add	ebx,10000h
no_carry_from_bx:
	endm

;----------------------------------------------------------------------------;

createSeg _PROTECT,pCode,word,public,CODE
sBegin	pCode

	.386p

	assumes cs,pCode
	assumes ss,StrStuff
	page

;---------------------------Public-Routine------------------------------;
; p_build_string
;
;   p_build_string builds up data on the stack consisting of character
;   offsets and widths, then invokes the routine which processes
;   this data.
;
; Entry:
;	stack frame as per strblt
; Returns:
;	none
; Error Returns:
;	none
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;
; History:
;
;       Thu 06-Apr-1989 09:55:20 -by-  Amit Chatterjee [amitc]
;	Modified.
;          This TextOut function has 80386 soecific code in it, so
;          will non run on 8086 or 80286 protected mode. So now all
;          the text function is being put in a separate fixed segment
;          with the 8086 TextOut appearing in another fixed segment and
;          one of these two segments chosen at enable time.	
;		. Moved code to _PROTECT segment
;		. prefixed a 'p_' to all public labels
;
;	Fri 27-Jan-1989 14:18:04 -by-  Amit Chatterjee [amitc]
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
;       Wed 18-Jan-1989 15:34:10 -by-  Amit Chatterjee [amitc]
;       included 16 bit gathering code from the stack and included
;       code for emboldening
;
;	Wed 06-May-1987 16:51:42 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
define_frame p_build_string		;Define strblt's frame

cBegin	<nogen>
cEnd	<nogen>


p_real_build_string proc near

	mov	ax,StackTop	 	;Compute minimum allowable SP
	add	ax,STACK_SLOP
	mov	min_stack,ax

	mov	ax,text_bbox.left	;Set up text bbox to be a null interval
	mov	text_bbox.right,ax	;  We'll grow it as we progress
	mov	current_lhs,ax		;Need to init this for interval calc

	mov	al,accel		;Save the actual state of the
	mov	bl,al			;  IS_OPAQUE flag since the
	and	al,IS_OPAQUE		;  worst case code may alter
	mov	wc_flags,al		;  it
	mov	ah,excel		;Only set text visible once a string
	and	ah,not TEXT_VISIBLE	; is actually displayed
	mov	excel,ah

	mov	cx,pCodeOFFSET p_non_justified_text
	test	bl,WEIRD_SPACING
	jz	short p_build_have_proc
	mov	cx,pCodeOFFSET p_justified_text
	test	bl,NEG_SPACING+HAVE_WIDTH_VECT
	jz	short p_build_have_proc
	test	bl,IS_OPAQUE		;If transparent mode, then no
	jz	short p_build_worst_ok	;  special work is required
	call	p_prepare_for_overlap	;Massive preprocessing required

p_build_worst_ok:
	mov	cx,pCodeOFFSET p_worst_case
	mov	bl,accel
	mov	ah,excel

p_build_have_proc:
	mov	build_proc,cx		;Save build routine address
	call	p_preset_pro_text 	;Set frame/ss: locations as needed

p_build_restart:
	mov	clear_stack,sp		;Used to clean the stack on exit
	mov	ax,sp			;Save buffer start pointer
	dec	ax
	dec	ax
	mov	buffer,ax

;-----------------------------------------------------------------------;
;
;	The routines which push text onto the stack expect the
;	following registers to be set on entry:
;
;		DS:SI --> current character in the string
;		ES:    =  font segment
;		AL     =  excel flags
;		AH     =  accel flags
;		CX     =  number of characters in the string
;		DI     =  current X position (and therefore starting x)
;
;-----------------------------------------------------------------------;

	lds	si,lp_string
	assumes ds,nothing

	mov	es,seg_lp_font
	assumes es,FontSeg

	mov	al,excel
	mov	ah,accel
	mov	cx,count
	mov	di,x
	jmp	build_proc

p_build_ret_addr: 			;build routines return here
	mov	count,cx		;Save count
	mov	x,ax			;Save next char's start
	mov	off_lp_string,si	;Save next character
	call	p_pad_right_hand_side	;Fill to byte boundary if possible
	mov	current_rhs,di		;Save rhs
	mov	bx,di			;Compute the interval data
	mov	dx,current_lhs
	call	p_comp_byte_interval
	jc	p_build_all_done		;If nothing shows, we're done
	mov	left_clip_mask,al
	mov	right_clip_mask,ah
	mov	inner_byte_count,si
	mov	scan_start,di
	call	p_process_stack_data	;Display the string
	mov	sp,clear_stack

	or	excel,TEXT_VISIBLE	;Show something was displayed

;-----------------------------------------------------------------------;
;
;	If there is an opaquing rectangle, we must update the text
;	bounding box so that it won't overwrite the text we just
;	displayed when the rectangle is output.  IN transparent mode,
;	OPAQUE_RECT would have been cleared after the rectangle was
;	drawn and before we were called.
;
;-----------------------------------------------------------------------;

	test	excel,OPAQUE_RECT
	jz	short p_build_no_o_rect
	mov	ax,current_lhs
	mov	bx,text_bbox.left
	min_ax	bx
	mov	text_bbox.left,ax
	mov	ax,current_rhs
	mov	bx,text_bbox.right
	max_ax	bx
	mov	text_bbox.right,ax

p_build_no_o_rect:
	mov	cx,count		;If no more characters
	jcxz	p_build_restore_opaque	;  go home, have dinner, sleep


;-----------------------------------------------------------------------;
;
;	Prepare for the next string.  If in opaque mode, the
;	CLIPPED_LEFT flag will have to be set so that we don't
;	try padding the lhs.  If in transparent mode, because
;	of stepping backwards, we might actually be clipped
;	anyway, so we'll have to test for thsi also.
;
;	FIRST_IN_PREV must be cleared.	It will be set if the
;	clipping code determines it needs to be.
;
;-----------------------------------------------------------------------;

	mov	bl,excel		;Will be changing flags in here
	and	bl,not (CLIPPED_LEFT+FIRST_IN_PREV)
	mov	di,x
	mov	ax,di			;Assume start will be current_lhs
	test	accel,IS_OPAQUE 	;When opaque, must clip on
	jnz	short p_build_clip_next_time  ;  a restart
	cmp	di,clip.left
	jge	short p_build_no_clip_next_time

p_build_clip_next_time:
	or	bl,CLIPPED_LEFT 	;Clipping is required
	mov	ax,clip.left		;  and clip.left for clipping check
	max_ax	di			;  (but only if start X > old clip
	mov	clip.left,ax		;   lhs)

p_build_no_clip_next_time:
	mov	excel,bl
	mov	current_lhs,ax		;Need to update lhs for interval calc
	jmp	p_build_restart		;Try next part of the string

p_build_all_done:
	mov	sp,clear_stack

p_build_restore_opaque:
	mov	al,wc_flags
	test	al,WC_SET_LR		;If we stepped backwards, we'll
	jz	short p_build_really_done     ;  have to restore the real lhs
	mov	bx,wc_opaque_lhs	;  and rhs incase we have an
	mov	text_bbox.left,bx	;  opaque rectangle
	mov	bx,wc_opaque_rhs
	mov	text_bbox.right,bx

p_build_really_done:
	and	al,IS_OPAQUE		;Restore IS_OPAQUE incase p_worst_case
	or	accel,al		;  code cleared it, else the opaque

	ret				;  rectangle may overwrite our text

p_real_build_string endp

	page
;---------------------------Public-Routine------------------------------;
;
; p_non_justified_text
;
;   This is the simple case for proportional text.  No justification,
;   no width vector.  Just run the string.  If we run out of stack
;   space, then that portion of the string which fits will be displayed,
;   and we'll restart again after that.
;
;   spcl - simple, proportional, clip lhs
;   sfcl - simple, fixed pitch,  clip lhs
;   sc	 - simple, clip rhs
;
; Entry:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	AL     =  excel flags
;	AH     =  accel flags
;	CX     =  number of characters in the string
;	DI     =  current X position (and therefore starting x)
;	stack frame as per strblt
; Returns:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	CX     =  number of characters left in string
;	DI     =  string rhs
;	AX     =  next character's X
; Error Returns:
;	none
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;	None
; History:
;	Tue 05-May-1987 18:27:29 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,FontSeg
	assumes ss,StrStuff

p_non_justified_text proc near

	test	al,CLIPPED_LEFT
	jz	p_sc_no_left_clipping	;No clipping needed on lhs
	mov	dx,clip.left		;Characters become visible here
	test	ah,FIXED_PITCH
	jz	short p_spcl_next_char	;Proportional font


;-----------------------------------------------------------------------;
;
;	Fixed pitch, no justification, left hand clipping
;
;-----------------------------------------------------------------------;
	mov	bx,lfd.font_width	;Fixed pitch font.

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

p_sfcl_next_char:
	add	di,bx			;Does this character become visible?
	cmp	dx,di			;DX is clip.left
	jl	short p_sfcl_current_is_visible ;This char is visible
	inc	si
	loop	p_sfcl_next_char	;See if next character

p_sfcl_no_chars_visible:
	jmp	p_build_ret_addr	;Return to caller

p_sfcl_current_is_visible:
	sub	di,bx			;Restore staring address of character
					;  and just fall into the proportional
					;  code which will handle everything
;-----------------------------------------------------------------------;
;
;	Proportional, no justification, left hand clipping
;
;-----------------------------------------------------------------------;

p_spcl_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	jbe	short p_spcl_good_character
	mov	al,lfd.default_char	;Character was out of range

p_spcl_good_character:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;
	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	add	di,ax			;0 width chars won't change x position
	cmp	dx,di			;DX is clip.left
	jl	short p_spcl_current_is_visible ;This char is visible

p_spcl_see_if_next:
	loop	p_spcl_next_char		;See if next character
	jmp	p_build_ret_addr		;Return to caller

p_spcl_current_is_visible:
	sub	di,ax			;Restore starting x of character
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx
	



;-----------------------------------------------------------------------;
;
;	Instead of incrementing the current position by 8 and
;	having to recover the real current position, we just
;	slide the clip region left.  It has the same effect.
;
;-----------------------------------------------------------------------;

	sub	dx,di			;Compute bits until we're visible
	je	short p_spcl_save_first ;Starts on clip edge
	sub	dx,8			;Is current byte visible?
	jl	short p_spcl_have_vis_start   ;  Yes

p_spcl_step_clipped_char:
	sub	ax,8			;Shorten the width of the character
	add	di,8			;Move current position right
	add	bx,lfd.font_height	;Move to next column of character
	
; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	sub	dx,8			;Is current byte visible?
	jge	short p_spcl_step_clipped_char	;  No


;-----------------------------------------------------------------------;
;
;	If the lhs of the clip region and the starting X of the
;	character are in different bytes, then the FIRST_IN_PREV
;	flag must be set.  Only a clipped character can set this
;	flag.
;
;-----------------------------------------------------------------------;

p_spcl_have_vis_start:
	mov	dx,clip.left
	xor	dx,di
	and	dx,not 7
	jz	short p_spcl_save_first ;In same byte
	or	excel,FIRST_IN_PREV


;-----------------------------------------------------------------------;
;
;	We have the start of the first character which is visible
;	Determine which loop (clipped/non-clipped) will process it.
;	We let the routine we're about to call push the character
;	since it will handle both right clipping (if needed) and
;	fat characters.
;
;-----------------------------------------------------------------------;

p_spcl_save_first:
	jmp	short p_scc_clip_enters_here


;-----------------------------------------------------------------------;
;
;	There was no left hand clipping.  Whenever this is the case,
;	we want to try and pad the lhs out to a byte boundary so that
;	full byte code can be used.
;
;-----------------------------------------------------------------------;

p_sc_no_left_clipping:
	call	p_pad_left_hand_side	;Might be able to pad lhs
	jmp	short p_scc_next_char


;-----------------------------------------------------------------------;
;
;	scc - simple case, rhs clipping.
;
;	This loop is used when it is possible for the character
;	to be clipped on the rhs.  lhs clipping has already
;	been performed.  There is no justification.
;
;	Currently:
;		DS:SI --> current character in the string
;		ES:    =  font segment
;		DI     =  current X position
;		CX     =  number of bytes left in the string
;
;-----------------------------------------------------------------------;


p_scc_bad_char:
	mov	al,lfd.default_char	;Character was out of range,
	jmp	short p_scc_good_char

p_scc_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	short p_scc_bad_char

p_scc_good_char:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;

	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	or	ax,ax			;If width is 0, ignore character
	jz	short p_scc_see_if_next
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx


p_scc_clip_enters_here:
	mov	dx,di			;Compute phase
	and	dl,7
	add	di,ax			;DI = next char's X position
	cmp	di,clip.right
	jge	short p_scc_char_is_clipped	;Clipped (or first pixel of
						;next is)
p_scc_been_clipped:
	mov	dh,dl			;Need phase in DH
	cmp	ax,8			;If character is less than 8 bits
	jbe	short p_scc_width_ok	;  wide, push it's data
	mov	dl,8			;Need 8 for size in DL

p_scc_still_wide:
	push	dx			;Push data showing phase,
	push	ebx			;  character is 8 wide, then
	sub	ax,8			;  create another character
	add	bx,lfd.font_height	;  of the remaining width

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	cmp	ax,8
	ja	short p_scc_still_wide

p_scc_width_ok:
	mov	ah,dh
	push	ax			;Push data showing phase,

	push	ebx			;Save offset to bits
	cmp	sp,min_stack		;Stack compare must be unsigned
	jb	short p_scc_restart	;Not enough stack for another character

p_scc_see_if_next:
	loop	p_scc_next_char		;Until all characters pushed
	mov	ax,di			;Next character starts here
	jmp	p_build_ret_addr


;-----------------------------------------------------------------------;
;
;	This character is either clipped, or it's last pixel is
;	the last pixel which will fit within the clipping rhs.
;	Adjust it's width so it fits, set the remaining character
;	count to 1 so the loop will terminate, and reenter the
;	code where we came from.
;
;-----------------------------------------------------------------------;

p_scc_char_is_clipped:
	mov	cx,clip.right		;Compute number of pixels
	sub	di,cx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,cx			;Set new rhs
	mov	cx,1			;Show this as last character
	jmp	p_scc_been_clipped	;Finish here


;-----------------------------------------------------------------------;
;
;	These is no more space on the stack to build characters.
;	If this was the last character, then don't bother with the
;	restart.
;
;-----------------------------------------------------------------------;

p_scc_restart:
	dec	cx			;Adjust count for char just pushed
	mov	ax,di			;Next character starts here
	jmp	p_build_ret_addr

p_non_justified_text endp

	page
;---------------------------Public-Routine------------------------------;
;
; p_justified_text
;
;   This is the justification case for text, when positive character
;   extra and/or a positive DDA are present.  If we run out of stack
;   space, then that portion of the string which fits will be displayed,
;   and we'll restart again after that.
;
;   jc	- justify clipped
;   jcl - justify clip left
;
; Entry:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	AL     =  excel flags
;	AH     =  accel flags
;	CX     =  number of characters in the string
;	DI     =  current X position (and therefore starting x)
;	stack frame as per strblt
; Returns:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	CX     =  number of characters left in string
;	DI     =  string rhs
;	AX     =  next character's X
; Error Returns:
;	none
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;
; History:
;	Tue 05-May-1987 18:27:29 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,FontSeg
	assumes ss,StrStuff

p_justified_text	proc	near

	test	excel,CLIPPED_LEFT
	jnz	short p_jcl_next_char	;Clipping needed
	call	p_pad_left_hand_side	;Might be able to pad lhs
	jmp	p_jc_next_char


p_jcl_bad_character:
	mov	al,lfd.default_char	;Character was out of range
	jmp	short p_jcl_good_character


;-----------------------------------------------------------------------;
;
;	This is the code which runs the DDA to intersperse pixels
;	into the string
;
;	Compute the amount of white space that will be introduced by
;	this character.  This will be the sum of any character extra,
;	any break extra (if a break character), and dda interspersed
;	pixels (if a break character)
;
;-----------------------------------------------------------------------;

p_jcl_have_break_char:
	mov	bl,accel
	and	bl,DDA_NEEDED+HAVE_BRK_EXTRA
	jz	short p_jcl_have_tot_extra  ;Must have only been char extra
	add	dx,brk_extra		;Extra every break (0 if none)
	test	bl,DDA_NEEDED
	jz	short p_jcl_have_tot_extra
	mov	bx,brk_err		;The dda is required for this char
	sub	bx,brk_rem		;  Run it and add in an extra pixel
	jg	short p_jcl_dont_distribute ;  if needed.
	add	bx,brk_count		;Add one pixel for the dda
	inc	dx

p_jcl_dont_distribute:
	mov	brk_err,bx		;Save rem for next time
	jmp	short p_jcl_have_tot_extra


;-----------------------------------------------------------------------;
;
;	This is the code which computes the number of DDA interspersed
;	pixels to be added to the string
;
;	If all the extra pixels will fit on the end of this character,
;	just adjust it's width, otherwise a null character should be
;	created for the extra.
;
;-----------------------------------------------------------------------;

p_jcl_extra_pixels:
	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	neg	ax
	and	ax,7			;AX = # extra pixels which will fit
	jz	short p_jcl_have_entire_width ;None will fit
	cmp	ax,dx
	jl	short p_jcl_have_what_fits    ;Some extra pixels will not fit
	mov	ax,dx			;All pixels will fit, make DX = 0

p_jcl_have_what_fits:
	sub	dx,ax			;DX = extra for the dummy character

p_jcl_have_entire_width:
	add	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	jmp	short p_jcl_have_width


;-----------------------------------------------------------------------;
;
;	This is the start of the real loop for left hand clipping.
;
;-----------------------------------------------------------------------;

p_jcl_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	short p_jcl_bad_character

p_jcl_good_character:
	mov	dx,char_xtra		;Base amount of extra pixels needed
	cmp	al,lfd.break_char
	je	short p_jcl_have_break_char ;Go compute dda added pixels

p_jcl_have_tot_extra:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;

	or	dx,dx
	jnz	short p_jcl_extra_pixels    ;Extra pixels required
	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]

p_jcl_have_width:
	add	di,ax			;DI = next chars starting X
	cmp	clip.left,di
	jl	short p_jcl_current_is_visible	;This char is visible
	add	di,dx
	cmp	clip.left,di
	jl	short p_jcl_dummy_is_visible  ;Dummy is first visible character

p_jcl_see_if_next:
	loop	p_jcl_next_char		;See if next character
	jmp	p_build_ret_addr		;Return to caller


;-----------------------------------------------------------------------;
;
;	The dummy character is the first character which became
;	visible.  Just set the starting X to clip.left, and shorten
;	the width of the dummy character appropriately.
;
;-----------------------------------------------------------------------;

p_jcl_dummy_is_visible:
	mov	dx,di
	mov	di,clip.left		;Starting X is clip.left
	sub	dx,di			;DX is # pixels in dummy
	xor	ax,ax			;Show no real character
	mov	ebx, null_char_offset	;don't get fucked with invalid ebx
	jmp	short p_jcl_all_done


;-----------------------------------------------------------------------;
;
;	We just encountered the first character which will be visible
;	Clip it on the lhs as needed.
;
;-----------------------------------------------------------------------;

p_jcl_current_is_visible:
	sub	di,ax			;Restore starting x of character
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping
; the following macro will add 10000h to EBX if carry is set above
	updc_ebx


;-----------------------------------------------------------------------;
;
;	Instead of incrementing the current position by 8 and
;	having to recover the real current position, we just
;	slide the clip region left.  It has the same effect.
;
;-----------------------------------------------------------------------;

	push	dx			;Save extra pixels
	mov	dx,clip.left
	sub	dx,di			;Compute bits until we're visible
	je	short p_jcl_save_first	;Starts on clip edge
	sub	dx,8			;Is current byte visible?
	jl	short p_jcl_have_vis_start    ;  Yes

p_jcl_step_clipped_char:
	sub	ax,8			;Shorten the width of the character
	add	di,8			;Move current position right
	add	bx,lfd.font_height	;Move to next column of character

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	sub	dx,8			;Is current byte visible?
	jge	short p_jcl_step_clipped_char ;  No

;-----------------------------------------------------------------------;
;
;	If the lhs of the clip region and the starting X of the
;	character are in different bytes, then the FIRST_IN_PREV
;	flag must be set.  Only a clipped character can set this
;	flag.
;
;-----------------------------------------------------------------------;

p_jcl_have_vis_start:
	mov	dx,clip.left
	xor	dx,di
	and	dx,not 7
	jz	short p_jcl_save_first	;In same byte
	or	excel,FIRST_IN_PREV

;-----------------------------------------------------------------------;
;
;	We have the start of the first character which is visible
;	We let the routine we're about to call push the character
;	since it will handle both right clipping (if needed) and
;	fat characters.
;
;-----------------------------------------------------------------------;


p_jcl_save_first:
	pop	dx			;Restore extra pixels

p_jcl_all_done:
	jmp	p_jc_clip_enters_here



;-----------------------------------------------------------------------;
;
;	jc - justified with clipping
;
;	This loop is used for justified text.  It will perform
;	rhs clipping.  lhs clipping has already been performed.
;
;	Currently:
;		DS:SI --> current character in the string
;		ES:    =  font segment
;		DI     =  current X position
;		CX     =  number of bytes left in the string
;
;-----------------------------------------------------------------------;


p_jc_bad_char:
	mov	al,lfd.default_char	;Character was out of range,
	jmp	short p_jc_good_character


;-----------------------------------------------------------------------;
;
;	This is the code which runs the DDA to intersperse pixels
;	into the string
;
;	Compute the amount of white space that will be introduced by
;	this character.  This will be the sum of any character extra,
;	any break extra (if a break character), and dda interspersed
;	pixels (if a break character)
;
;-----------------------------------------------------------------------;

p_jc_have_break_char:
	mov	bl,accel
	and	bl,DDA_NEEDED+HAVE_BRK_EXTRA
	jz	short p_jc_have_tot_extra   ;Must have only been char extra
	add	dx,brk_extra		;Extra every break (0 if none)
	test	bl,DDA_NEEDED
	jz	short p_jc_have_tot_extra
	mov	bx,brk_err		;The dda is required for this char
	sub	bx,brk_rem		;  Run it and add in an extra pixel
	jg	short p_jc_dont_distribute  ;  if needed.
	add	bx,brk_count		;Add one pixel for the dda
	inc	dx

p_jc_dont_distribute:
	mov	brk_err,bx		;Save rem for next time
	jmp	short p_jc_have_tot_extra


;-----------------------------------------------------------------------;
;
;	If all the extra pixels will fit on the end of this character,
;	just adjust it's width, otherwise a null character should be
;	created for the extra.
;
;-----------------------------------------------------------------------;

p_jc_extra_pixels:
	neg	ax
	and	ax,7			;AX = # extra pixels which will fit
	jz	short p_jc_have_entire_width  ;None will fit
	cmp	ax,dx
	jl	short p_jc_have_what_fits     ;Some extra pixels will not fit
	mov	ax,dx			;All pixels will fit, make DX = 0

p_jc_have_what_fits:
	sub	dx,ax			;DX = extra for the dummy character

p_jc_have_entire_width:
	add	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	jmp	short p_jc_have_width


;-----------------------------------------------------------------------;
;
;	This is the start of the real loop
;
;-----------------------------------------------------------------------;

p_jc_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	short p_jc_bad_char

p_jc_good_character:
	mov	dx,char_xtra		;Base amount of extra pixels needed
	cmp	al,lfd.break_char
	je	short p_jc_have_break_char    ;Go compute dda added pixels

p_jc_have_tot_extra:
	xor	ah,ah

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	shl	ax,1
	mov	bx,ax
	shl	ax,1
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;

	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	or	dx,dx
	jnz	short p_jc_extra_pixels ;Extra pixels required

p_jc_have_width:
	or	ax,ax
	jz	short p_jc_check_dummy	;If width is 0, might still have dummy
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

p_jc_clip_enters_here:
	mov	num_null_pixels,dx	;Save # null pixels
	mov	dx,di			;Compute phase
	and	dl,7
	add	di,ax			;DI = next char's X position
	cmp	di,clip.right
	jge	short p_jc_char_is_clipped  ;Clipped (or first pixel of next is)

p_jc_been_clipped:
	mov	dh,dl			;Need phase in DH
	mov	dl,8			;Need 8 for size in DL if fat
	cmp	ax,8			;If character is less than 8 bits
	jbe	short p_jc_width_ok	;  wide, push it's data

p_jc_still_wide:
	push	dx			;Push data showing phase,

	push	ebx			;  character is 8 wide, then
	sub	ax,8			;  create another character
	add	bx,lfd.font_height	;  of the remaining width

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	cmp	ax,8
	ja	short p_jc_still_wide

p_jc_width_ok:
	mov	ah,dh
	push	ax			;Push data showing phase,

	push	ebx			;Save offset to bits
	mov	dx,num_null_pixels

p_jc_check_dummy:
	or	dx,dx
	jz	short p_jc_see_if_next	;No pixels for justification
	xchg	ax,dx			;Set ax = number of pixels to fill
	mov	ebx,null_char_offset
	mov	dx,di			;Compute phase
	and	dl,7
	add	di,ax			;DI = next char's X position
	cmp	di,clip.right
	jge	short p_jc_dummy_is_clipped ;Clipped (or first pixel of next is)

p_jc_dummys_been_clipped:
	mov	dh,dl			;Need phase in DH
	mov	dl,8			;Need 8 for size in DL if fat
	cmp	ax,8			;If dummy is less than 8 bits
	jbe	short p_jc_dummy_width_ok   ;  wide, push it's data

p_jc_dummy_still_wide:
	push	dx			;Push data showing phase,

	push	ebx			;  character is 8 wide, then
	sub	ax,8			;  create another character
	cmp	ax,8
	ja	short p_jc_dummy_still_wide

p_jc_dummy_width_ok:
	mov	ah,dh
	push	ax			;Push data showing phase,

	push	ebx			;Save offset to bits

p_jc_see_if_next:
	cmp	sp,min_stack		;Stack compare must be unsigned
	jb	short p_jc_restart	;Not enough stack for another character
	dec	cx
	jle	short p_jc_all_done
	jmp	p_jc_next_char		;Until all characters pushed

p_jc_all_done:
	mov	ax,di			;Next character starts here
	jmp	p_build_ret_addr


;-----------------------------------------------------------------------;
;
;	This character is either clipped, or it's last pixel is
;	the last pixel which will fit within the clipping rhs.
;	Adjust it's width so it fits, set the remaining character
;	count to 1 so the loop will terminate, and reenter the
;	code where we came from.
;
;	Might as well set num_null_pixels to zero to skip that code.
;
;-----------------------------------------------------------------------;

p_jc_char_is_clipped:
	mov	cx,clip.right		;Compute number of pixels
	sub	di,cx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,cx			;Set new rhs
	xor	cx,cx			;Dont't want any extra pixels
	mov	num_null_pixels,cx
	inc	cx			;Show this as last character
	jmp	p_jc_been_clipped 	;Finish here


;-----------------------------------------------------------------------;
;
;	The dummy is either clipped, or it's last pixel is
;	the last pixel which will fit within the clipping rhs.
;	Adjust it's width so it fits, set the remaining character
;	count to 1 so the loop will terminate, and reenter the
;	code where we came from.
;
;-----------------------------------------------------------------------;

p_jc_dummy_is_clipped:
	mov	cx,clip.right		;Compute number of pixels
	sub	di,cx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,cx			;Set new rhs
	mov	cx,1			;Show this as last character
	jmp	p_jc_dummys_been_clipped	;Finish here


;-----------------------------------------------------------------------;
;
;	These is no more space on the stack to build characters.
;	If this was the last character, then don't bother with the
;	restart.
;
;-----------------------------------------------------------------------;

p_jc_restart:
	dec	cx			;Adjust count for char just pushed
	mov	ax,di			;Next character starts here
	jmp	p_build_ret_addr

p_justified_text	endp

	page
;---------------------------Public-Routine------------------------------;
;
; p_worst_case
;
;   This is the worst case text code, when there is some combination
;   of the width vector, negative character extra, and negative dda.
;   If we step backwards or we run out of stack space, then that
;   whatever has been built up on the stack will be displayed, and
;   we'll restart again after that.
;
;   wcc - worse case clipped
;   wccl - worse case clip left
;
; Entry:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	AL     =  excel flags
;	AH     =  accel flags
;	CX     =  number of characters in the string
;	DI     =  current X position (and therefore starting x)
;	stack frame as per strblt
; Returns:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	CX     =  number of characters left in string
;	DI     =  string rhs
;	AX     =  next character's X
; Error Returns:
;	none
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Calls:
;
; History:
;	Tue 05-May-1987 18:27:29 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,FontSeg
	assumes ss,StrStuff

p_worst_case	proc near

	test	excel,CLIPPED_LEFT
	jnz	short p_wccl_next_char	;Clipping needed
	call	p_pad_left_hand_side	;Might be able to pad lhs
	jmp	p_wcc_next_char


;-----------------------------------------------------------------------;
;
;	Set current character to the default character
;
;-----------------------------------------------------------------------;

p_wccl_bad_character:
	mov	al,lfd.default_char
	jmp	short p_wccl_good_character


;-----------------------------------------------------------------------;
;
;	This runs the DDA to intersperse pixels into the string
;
;	Compute the adjustment to the character's width.  This will
;	be the sum of any character extra, any break extra (if a
;	break character), and dda interspersed pixels (if a break
;	character)
;
;	The dda must be capable of handling positive and negative
;	justification.	Character extra may be negative.
;
;-----------------------------------------------------------------------;

p_wccl_have_break_char:
	mov	bl,accel
	and	bl,DDA_NEEDED+HAVE_BRK_EXTRA
	jz	short p_wccl_have_tot_extra ;Must have only been char extra
	add	dx,brk_extra		;Extra every break (0 if none)
	test	bl,DDA_NEEDED
	jz	short p_wccl_have_tot_extra

	mov	bx,brk_rem		;If the dda is stepping left instead
	or	bx,bx			;  of stepping right, then brk_rem
	jl	short p_wccl_neg_dda	;  will be negative
	sub	brk_err,bx		;Run dda and add in an extra pixel
	jg	short p_wccl_have_tot_extra ;  if needed.
	mov	bx,brk_count
	add	brk_err,bx
	inc	dx			;Add one pixel for the dda
	jmp	short p_wccl_have_tot_extra

p_wccl_neg_dda:
	add	brk_err,bx		;Run dda and subtract an extra pixel
	jl	short p_wccl_have_tot_extra ;  if needed.
	mov	bx,brk_count
	sub	brk_err,bx		;Subtract one pixel for the dda
	dec	dx
	jmp	short p_wccl_have_tot_extra



;-----------------------------------------------------------------------;
;
;	This is the start of the real loop for left hand clipping.
;
;-----------------------------------------------------------------------;

p_wccl_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	short p_wccl_bad_character

p_wccl_good_character:
	mov	dx,char_xtra		;Base amount of extra pixels needed
	cmp	al,lfd.break_char
	je	short p_wccl_have_break_char  ;Go compute dda added pixels

p_wccl_have_tot_extra:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;

	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	push	bx			;Must save character index
	test	accel,HAVE_WIDTH_VECT
	jz	short p_wccl_have_width

p_wccl_get_user_width:
	les	bx,lp_dx
	assumes es,nothing
	add	dx,wptr es:[bx] 	;DX is delta to next char's start
	inc	bx
	inc	bx
	mov	off_lp_dx,bx
	mov	es,seg_lp_font
	assumes es,FontSeg
	sub	dx,ax			;Compute adjustment to width


;-----------------------------------------------------------------------;
;
;	DX contains any adjustment to the next characters starting
;	position.  If this number is negative, then we'll be stepping
;	over the previous character.
;
;	We don't allow a character to start before the previous
;	character.  We'll have to enforce this at this point by
;	making sure that any negative adjustment is less than or
;	equal to the character width.
;
;-----------------------------------------------------------------------;

p_wccl_have_width:
	or	dx,dx
	jge	short p_wccl_adjustment_ok
	add	dx,ax			;Don't allow the backwards step
	sbb	bx,bx			;  to go beyond the start of this
	and	dx,bx			;  character
	sub	dx,ax

p_wccl_adjustment_ok:
	pop	bx			;Restore char index
	add	ax,di			;AX = rhs of character
	cmp	clip.left,ax
	jl	short p_wccl_current_is_visible ;Some part of char is visible


;-----------------------------------------------------------------------;
;
;	If the adjustment for the character width is greater than the
;	actual width of the character, then it is possible that the
;	dummy pixels could become visible.  If the adjustment for the
;	character width is less than the actual character width, then
;	the dummy pixel (negative dummy pixels?) cannot become visible.
;
;-----------------------------------------------------------------------;

	add	ax,dx			;Next char starts at AX
	mov	di,clip.left
	cmp	di,ax
	jl	short p_wccl_dummy_is_visible ;part of dummy char became visible
	xchg	di,ax			;Set start of next character

p_wccl_see_if_next:
	loop	p_wccl_next_char		;See if next character
	jmp	p_build_ret_addr		;Return to caller



;-----------------------------------------------------------------------;
;
;	The dummy character is the first character which became
;	visible.  Just set the starting X to clip.left, and shorten
;	the width of the dummy character appropriately.
;
;-----------------------------------------------------------------------;

p_wccl_dummy_is_visible:
	xchg	ax,dx			;Set DX = # pixels in dummy
	sub	dx,di
	xor	ax,ax			;Show no real character
	jmp	short p_wccl_all_done


;-----------------------------------------------------------------------;
;
;	So here we are, we have a character which will become visible,
;	and possibly have some adjustment to the character.
;
;	Our registers currently contain:
;
;		AX = rhs of character
;		BX = index into offset/width table
;		CX = # of characters left in the string
;		DX = # of extra pixels (zero or positive)
;		DI = starting X offset
;		ES = FontSeg
;		DS:SI --> string
;
;-----------------------------------------------------------------------;


p_wccl_current_is_visible:
	sub	ax,di			;Restore width of the character
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

;-----------------------------------------------------------------------;
;
;	Instead of incrementing the current position by 8 and
;	having to recover the real current position, we just
;	slide the clip region left.  It has the same effect.
;
;-----------------------------------------------------------------------;

	push	dx			;Save extra pixels to be added
	mov	dx,clip.left
	sub	dx,di			;Compute bits until we're visible
	je	short p_wccl_save_first ;Starts on clip edge
	sub	dx,8			;Is current byte visible?
	jl	short p_wccl_have_vis_start ;  Yes

p_wccl_step_clipped_char:
	sub	ax,8			;Shorten the width of the character
	add	di,8			;Move current position right
	add	bx,lfd.font_height	;Move to next column of character

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	sub	dx,8			;Is current byte visible?
	jge	short p_wccl_step_clipped_char	; No


;-----------------------------------------------------------------------;
;
;	If the lhs of the clip region and the starting X of the
;	character are in different bytes, then the FIRST_IN_PREV
;	flag must be set.  Only a clipped character can set this
;	flag.
;
;-----------------------------------------------------------------------;

p_wccl_have_vis_start:
	mov	dx,clip.left
	xor	dx,di
	and	dx,not 7
	jz	short p_wccl_save_first ;In same byte
	or	excel,FIRST_IN_PREV


;-----------------------------------------------------------------------;
;
;	We have the start of the first character which is visible.
;	We let the routine we're about to call push the character
;	since it will handle both right clipping (if needed) and
;	fat characters.
;
;-----------------------------------------------------------------------;

p_wccl_save_first:
	pop	dx			;Restore extra pixels

p_wccl_all_done:
	jmp	p_wcc_clip_enters_here


;-----------------------------------------------------------------------;
;
;	wcc - worse case, with rhs clipping
;
;	Currently:
;		DS:SI --> current character in the string
;		ES:    =  font segment
;		DI     =  current X position
;		CX     =  number of bytes left in the string
;
;-----------------------------------------------------------------------;


p_wcc_bad_char:
	mov	al,lfd.default_char	;Character was out of range,
	jmp	short p_wcc_good_character


;-----------------------------------------------------------------------;
;
;	This is the code which runs the DDA to intersperse pixels
;	into the string
;
;	Compute the adjustment to the character's width.  This will
;	be the sum of any character extra, any break extra (if a
;	break character), and dda interspersed pixels (if a break
;	character)
;
;	The dda must be capable of handling positive and negative
;	justification.	Character extra may be negative.
;
;-----------------------------------------------------------------------;

p_wcc_have_break_char:
	mov	bl,accel
	and	bl,DDA_NEEDED+HAVE_BRK_EXTRA
	jz	short p_wcc_have_tot_extra  ;Must have only been char extra
	add	dx,brk_extra		;Extra every break (0 if none)
	test	bl,DDA_NEEDED
	jz	short p_wcc_have_tot_extra

	mov	bx,brk_rem		;If the dda is stepping left instead
	or	bx,bx			;  of stepping right, then brk_rem
	jl	short p_wcc_neg_dda	;  will be negative
	sub	brk_err,bx		;Run dda and add in an extra pixel
	jg	short p_wcc_have_tot_extra  ;  if needed.
	mov	bx,brk_count
	add	brk_err,bx
	inc	dx			;Add one pixel for the dda
	jmp	short p_wcc_have_tot_extra

p_wcc_neg_dda:
	add	brk_err,bx		;Run dda and subtract an extra pixel
	jl	short p_wcc_have_tot_extra  ;  if needed.
	mov	bx,brk_count
	sub	brk_err,bx
	dec	dx			;Subtract one pixel for the dda
	jmp	short p_wcc_have_tot_extra


;-----------------------------------------------------------------------;
;
;	This is the start of the real loop for right hand clipping.
;
;-----------------------------------------------------------------------;

p_wcc_next_char:
	lodsb
	sub	al,lfd.first_char
	cmp	al,lfd.last_char
	ja	short p_wcc_bad_char

p_wcc_good_character:
	mov	dx,char_xtra		;Base amount of extra pixels needed
	cmp	al,lfd.break_char
	je	short p_wcc_have_break_char ;Go compute dda added pixels

p_wcc_have_tot_extra:
	xor	ah,ah
	xchg	ax,bx

;----------------------------------------------------------------------------;
; for normal code the header has 2 byte pointer and entry size is 4 per char ;
; for protected mode code, pointers are 4 byte and size of entry is 6 bytes  ;
;----------------------------------------------------------------------------;

ifdef	PROTECTEDMODE

; multiply by 6

	mov	ax,bx
	shl	ax,1
	shiftl	bx,2
	add	bx,ax
else

; multiply by 4

	shiftl	bx,2

endif
;----------------------------------------------------------------------------;

	mov	ax,wptr fsCharOffset[bx][PROP_WIDTH]
	mov	ebx,dword ptr fsCharOffset[bx][PROP_OFFSET]
	add	bx,amt_clipped_on_top	;Adjust pointer for any clipping

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	push	ebx
	test	accel,HAVE_WIDTH_VECT
	jz	short p_wcc_have_width

p_wcc_get_users_width:
	les	bx,lp_dx
	assumes es,nothing
	add	dx,wptr es:[bx] 	;DX is delta to next char's start
	inc	bx
	inc	bx
	mov	off_lp_dx,bx
	mov	es,seg_lp_font
	assumes es,FontSeg
	sub	dx,ax			;Compute adjustment to width


;-----------------------------------------------------------------------;
;
;	DX contains any adjustment to the next characters starting
;	position.  If this number is negative, then we'll be stepping
;	over the previous character.
;
;	We don't allow a character to start before the previous
;	character.  We'll have to enforce this at this point by
;	making sure that any negative adjustment is less than or
;	equal to the character width.
;
;-----------------------------------------------------------------------;

p_wcc_have_width:
	or	dx,dx
	jge	short p_wcc_adj_is_ok
	add	dx,ax			;Don't allow the backwards step
	sbb	bx,bx			;  to go beyond the start of this
	and	dx,bx			;  character
	sub	dx,ax

p_wcc_adj_is_ok:
	pop	ebx			;Restore bits offset

p_wcc_clip_enters_here:
	or	ax,ax			;If character width is 0,
	jz	short p_wcc_check_dummy ;  might still have dummy char
	or	dx,dx			;Any adjustment to the width?
	jle	short p_wcc_have_adj_width  ;No extra pixels to add
	push	ebx
	mov	bx,ax			;  into the empty space of the
	neg	ax			;  character
	and	ax,7			;AX = # extra pixels which will fit
	jz	short p_wcc_have_entire_width ;None will fit
	cmp	ax,dx
	jl	short p_wcc_have_what_fits  ;Some extra pixels will not fit
	mov	ax,dx			;All pixels will fit, make DX = 0

p_wcc_have_what_fits:
	sub	dx,ax			;DX = extra for the dummy character

p_wcc_have_entire_width:
	add	ax,bx			;Set number of pixels to use in char
	pop	ebx

p_wcc_have_adj_width:
	mov	num_null_pixels,dx	;Save number of dummy pixels
	mov	dx,di			;Compute phase
	and	dl,7
	add	di,ax			;DI = next char's X position
	cmp	di,clip.right
	jge	short p_wcc_char_is_clipped ;Clipped (or first pixel of next is)

p_wcc_been_clipped:
	mov	dh,dl			;Need phase in DH
	mov	dl,8			;Need 8 for size in DL if fat
	cmp	ax,8			;If character is less than 8 bits
	jbe	short p_wcc_width_ok	;  wide, push it's data

p_wcc_still_wide:
	push	dx			;Push data showing phase,

	push	ebx			;  character is 8 wide, then
	sub	ax,8			;  create another character
	add	bx,lfd.font_height	;  of the remaining width

; the following macro will add 10000h to EBX if carry is set above
	updc_ebx

	cmp	ax,8
	ja	short p_wcc_still_wide

p_wcc_width_ok:
	mov	ah,dh
	push	ax			;Push data showing phase,

	push	ebx			;Save offset to bits
	mov	dx,num_null_pixels

p_wcc_check_dummy:
	xchg	ax,dx			;Just incase we go backwards
	or	ax,ax
	jz	short p_wcc_see_if_next ;No pixels for justification
	jl	short p_wcc_going_back
	mov	ebx,null_char_offset
	mov	dx,di			;Compute phase
	and	dl,7
	add	di,ax			;DI = next char's X position
	cmp	di,clip.right
	jge	short p_wcc_dummy_is_clipped  ;Clipped (or first pixel of next is)

p_wcc_dummys_been_clipped:
	mov	dh,dl			;Need phase in DH
	mov	dl,8			;Need 8 for size in DL if fat
	cmp	ax,8			;If dummy is less than 8 bits
	jbe	short p_wcc_dummy_width_ok  ;  wide, push it's data

p_wcc_dummy_still_wide:
	push	dx			;Push data showing phase,
	push	ebx			;  character is 8 wide, then
	sub	ax,8			;  create another character
	cmp	ax,8
	ja	short p_wcc_dummy_still_wide

p_wcc_dummy_width_ok:
	mov	ah,dh
	push	ax			;Save width and phase
	push	ebx			;Save offset to bits

p_wcc_see_if_next:
	cmp	sp,min_stack		;Stack compare must be unsigned
	jb	short p_wcc_restart	;Not enough stack for another character
	dec	cx
	jle	short p_wcc_all_done
	jmp	p_wcc_next_char		;Until all characters pushed

p_wcc_all_done:
	mov	ax,di			;Next character starts here
	jmp	p_build_ret_addr



;-----------------------------------------------------------------------;
;
;	The dummy is either clipped, or it's last pixel is
;	the last pixel which will fit within the clipping rhs.
;	Adjust it's width so it fits, set the remaining character
;	count to 1 so the loop will terminate, and reenter the
;	code where we came from.
;
;	If the width adjustment was negative, we would never have
;	reached this code, so ignore any restart.
;
;-----------------------------------------------------------------------;

p_wcc_dummy_is_clipped:
	mov	cx,clip.right		;Compute number of pixels
	sub	di,cx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,cx			;Set new rhs
	mov	cx,1			;Show this as last character
	jmp	p_wcc_dummys_been_clipped ;Finish here



;-----------------------------------------------------------------------;
;
;	This character is either clipped, or it's last pixel is
;	the last pixel which will fit within the clipping rhs.
;
;	If there is a  negative adjustment to the width of the
;	character, it is possible that the next character could
;	be partially visible.  We'll have set up for a restart
;	if this is the case.
;
;	If this is the last character of the string, then there
;	is no problem
;
;	If no negative adjustment, adjust it's width so it fits,
;	set the remaining character count to 1 so the loop will
;	terminate, and reenter the code where we came from.
;
;-----------------------------------------------------------------------;

p_wcc_char_is_clipped:

	push	dx			;If num_null_pixels < 0, then
	mov	dx,num_null_pixels	;  a restart might be possible
	or	dx,dx
	jl	short p_wcc_might_need_restart	; Might need a restart

p_wcc_clipped_no_restart:
	mov	cx,clip.right		;Compute number of pixels
	sub	di,cx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,cx			;Set new rhs
	xor	cx,cx			;Don't want any extra pixels
	mov	num_null_pixels,cx
	inc	cx			;Show this as last character
	pop	dx
	jmp	p_wcc_been_clipped	;Finish here


;-----------------------------------------------------------------------;
;
;	Might be looking a restart in the face.  Compute where the
;	next character would start, and if it is to the left of
;	clip.right, then a restart is needed.
;
;-----------------------------------------------------------------------;

p_wcc_might_need_restart:
	cmp	cx,1			;If last character
	jle	short p_wcc_clipped_no_restart	;  then no restart needed
	add	dx,di			;Compute next starting x
	sub	dx,clip.right
	jge	short p_wcc_clipped_no_restart	;Rest of string is clipped


;-----------------------------------------------------------------------;
;
;	Will have to force a restart for the next character.  We
;	can do this by computing the number of pixels between where
;	where the next character starts and clip.right.  This negative
;	number will be stuffed into num_null_pixels, so when we reenter
;	the main loop, we'll force a restart after pushing the character
;	data
;
;-----------------------------------------------------------------------;

	mov	num_null_pixels,dx
	mov	dx,clip.right		;Compute number of pixels
	sub	di,dx			;  which have to be clipped
	sub	ax,di			;Set new character width
	mov	di,dx			;Set new rhs
	pop	dx
	jmp	p_wcc_been_clipped	;Finish here


;-----------------------------------------------------------------------;
;
;	I have the unfortunate task of informing you that the next
;	character will start somewhere in the middle of the current
;	character.  If this is the last character of the string,
;	then nothing need be done.  If this isn't the last character,
;	then a restart will be needed.
;
;-----------------------------------------------------------------------;

p_wcc_going_back:
	add	ax,di			;Set position of next character
	dec	cx			;Adjust count for char just pushed
	jmp	p_build_ret_addr



;-----------------------------------------------------------------------;
;
;	Out of stack space.  Set up for a restart.
;
;-----------------------------------------------------------------------;

p_wcc_restart:
	mov	ax,di
	dec	cx			;Adjust count for char just pushed
	jmp	p_build_ret_addr

p_worst_case	endp

	page
;--------------------------Private-Routine------------------------------;
; p_pad_left_hand_side
;
;   This routine is called when the text string isn't clipped on the
;   left side.	It attempts to pad the character with 0's if at all
;   possible.
;
;   If we can pad to the left of the first character with 0's, then
;   we can use the full byte code, which is many times faster than
;   the partial byte code.
;
;   If we do pad, we must update both current_lhs and the starting
;   X coordinate which will be used by the main loop code.
;
; Entry:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	CX     =  number of characters in the string
;	DI     =  current X position (and therefore starting x)
; Returns:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	CX     =  number of characters in the string
;	DI     =  current X position
; Error Returns:
;	None
; Registers Preserved:
;	CX,SI,DI,DS,ES,FLAGS
; Registers Destroyed:
;	AX,BX,DX
; Calls:
;	None
; History:
;	Thu 16-Apr-1987 23:37:27 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,FontSeg
	assumes ss,StrStuff

p_pad_left_hand_side	proc near

	mov	ax,di			;Get starting x coordinate
	and	ax,7			;Address MOD 8 is amount to pad
	jz	short p_plhs_all_done	;At byte boundary, don't need padding


;	If we're in transparent mode, we can always create a dummy
;	character of 0's for the left side to avoid any left clipping

	test	accel,IS_OPAQUE 	;If in transparent mode, we can
	jz	short p_plhs_add_padding    ;  always add the padding


;	In opaque mode.  If there is an opaquing rectangle, try and pad
;	the left side up to the byte boundary. If we cannot make it to
;	the byte boundary, go as far as we can so that the opaquing code
;	can skip the byte.
;
;	If there isn't an opaque rectangle, we cannot do any padding.

	mov	bl,excel
	not	bl
	test	bl,OPAQUE_RECT+BOUNDED_IN_Y
	jnz	short p_plhs_all_done	;Cannot add any padding
	mov	bx,di
	sub	bx,o_rect.left
	jle	short p_plhs_all_done	;Cannot add any.  Darn
	min_ax	bx			;Set AX = number of bits to pad


;	Some number of 0's can be added to the left of the character.
;	Add them, then move the lhs left by that amount.

p_plhs_add_padding:
	mov	dx,di			;DI = start x = text_bbox.left
	sub	dx,ax			;Set new lhs of text bounding box
	mov	current_lhs,dx
	mov	ah,dl			;Set phase (x coordinate) of char
	and	ah,7
	pop	dx
	push	ax			;Width and phase of null character
	push	null_char_offset	;Offset in font of null character
	jmp	dx

p_plhs_all_done:
	ret

p_pad_left_hand_side endp

	page
;--------------------------Private-Routine------------------------------;
; p_pad_right_hand_side
;
;   This routine is called once the text string has been pushed onto
;   the stack.	It pads the character on the right up to a byte boundary
;   if possible.
;
;   We always pad out to a byte boundary with 0 since it makes the
;   stopping condition in the driving loop simpler since it never
;   has to check to see if it has any more bytes of data left, it
;   knows it does.  It just checks after each destination column
;   has been output to see if anything else exists.
;
;   The clipping mask will be computed to the last pixel we can
;   alter.  In transparent mode, this will always be to the byte
;   boundary.  In opaque mode, it depends on the opaquing rectangle.
;
; Entry:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	DI     =  X position where next char would have gone
; Returns:
;	DS:SI --> current character in the string
;	ES:    =  font segment
;	DI     =  rhs of the string, padded to boundary if possible
; Error Returns:
;	'C' set if no interval to output
; Registers Preserved:
;	AX,BX,CX,DX,SI,DI,DS,ES,FLAGS
; Registers Destroyed:
;	AX,BX,DX
; Calls:
;	p_comp_byte_interval
; History:
;	Thu 16-Apr-1987 23:37:27 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,FontSeg
	assumes ss,StrStuff

p_pad_right_hand_side	proc near
	mov	ax,di			;If next char would start at bit 0,
	and	al,7			;  then no padding is required.
	jz	short p_prhs_have_rhs	;No padding needed
	mov	ah,al			;Save phase
	neg	al			;Set width needed
	and	al,7
	pop	bx			;save return address in BX
	push	ax			;Width and phase of null character
	push	null_char_offset	;Offset in font of null character
	push	bx			;put return address back on stack
	xor	ah,ah			;Only want width of the nulls in AX
	test	accel,IS_OPAQUE 	;If in transparent mode and
	jnz	short p_prhs_in_opaque_mode ;num_null_pixels>0 or no embolding
	cmp	fontweight, 0		;  move text_bbox to byte boundary
	je	short p_prhs_new_rhs	; else leave rhs as is and exit
	cmp	num_null_pixels, 0
	jle	p_prhs_have_rhs
	jmp	short p_prhs_new_rhs

p_prhs_in_opaque_mode:
	mov	bl,excel
	not	bl
	test	bl,OPAQUE_RECT+BOUNDED_IN_Y
	jnz	short p_prhs_have_rhs	;Cannot alter actual rhs
	mov	bx,o_rect.right 	;Compute distance from where I am to
	sub	bx,di			;  where opaque rectangle ends
	jle	short p_prhs_have_rhs	;Opaque rect is left of text end
	min_ax	bx			;Compute amount to move rhs

p_prhs_new_rhs:
	add	di,ax			;Slide rhs right

p_prhs_have_rhs:
	ret

p_pad_right_hand_side	endp

	page
;--------------------------Private-Routine------------------------------;
;
; p_process_stack_data
;
; The non-overlapped data which has been accumulated on the stack is
; output using the supplied dispatch tables.
;
; Entry:
;	GRAF_ENAB_SR set for all planes enabled
;	Tonys_bar_n_grill initialized to background color
; Returns:
;	None
; Error Returns:
;	none
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI
; Registers Preserved:
;	DS,ES,BP
; Calls:
;	Lots
; History:
;	Tue 05-May-1987 18:27:29 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

p_process_stack_data proc near


;-----------------------------------------------------------------------;
;
;	The EGA needs a little special handling at this point.
;
;	The full opaque byte code expects the EGA to be correctly
;	programmed for it's needs.  The EGA opaque partial byte
;	code will correctly restore the adapter to the needed mode
;	if it is called and RES_EGA_INNER is set.
;
;	So, if there is no first byte and we're in opaque mode,
;	we must program the ega correctly for the inner loop.
;	If there is a first byte and an inner loop, then we want
;	to set the RES_EGA_INNER flag.
;
;	If in transparent mode, we must set the set/reset register
;	to the foreground (text) color.
;
;-----------------------------------------------------------------------;


p_psd_pre_proc:
	les	di,lp_surface		;--> destination surface
	assumes es,nothing		;  (need EGAMem if this is the device)

	mov	bl,excel
	test	bl,IS_DEVICE		;Only preprocess if this is
	jz	short p_psd_pp_done	;  the EGA
	test	accel,IS_OPAQUE
	jz	short p_psd_pp_ega_trans
	mov	cx,inner_byte_count
	jcxz	p_psd_pp_done		;No inner loop, don't set the bit
	or	bl,RES_EGA_INNER	;Assume first byte
	cmp	left_clip_mask,0
	jne	short p_psd_pp_have_first   ;First byte
	call	p_set_ega_opaque_mode	;No first, must set for inner loop
	jmp	short p_psd_pp_done

p_psd_pp_have_first:
	mov	excel,bl		;Show reset needed
	jmp	short p_psd_pp_done

p_psd_pp_ega_trans:
	mov	dx,EGA_BASE + GRAF_ADDR
	mov	ah,bptr colors[FOREGROUND]
	mov	al,GRAF_SET_RESET
	out16	dx,ax
	mov	ax,MM_ALL shl 8 + GRAF_ENAB_SR
	out16	dx,ax
	mov	al,GRAF_BIT_MASK	;Transparent mode expects GRAF_ADDR
	out	dx,al			;  to be set to the bitmask register

p_psd_pp_done:	
	mov	ds,wptr lfd.lp_font_bits[2]
	assumes ds,nothing

	les	di,lp_surface		;--> destination surface
	assumes es,nothing		;  (need EGAMem if this is the device)

	mov	ax,text_bbox.top	;Compute Y component of the string
	mul	next_scan
	add	di,ax
	add	di,scan_start		;Add delta into scan

;	It is possible that the first character requires clipping.
;	If the clip mask is non-zero, then this is the case.


	mov	dl,0FFh			;initialize OR-mask for clipped bits
 	mov	al,left_clip_mask
 	or	al,al
	jz	short p_psd_not_clipped ;No partial first byte
	inc	inner_byte_count	;One extra time through the loop
	inc	dl			;set OR-mask to 00h

p_psd_not_clipped:
	cmp	inner_byte_count,0	;is there an inner byte ?
	jnz	short p_psd_nc_ok
	jmp	p_psd_exit
p_psd_nc_ok:
	or	al,dl			;if no clipping, set AL to 0FFh
	mov	ss_clip_mask,al


;----------------------------------------------------------------------------;
;*16bit									     ;
;----------------------------------------------------------------------------;
	mov	bx,clipped_table	;assume clipped case
	mov	ah,8			;accumulate 8 bits for cliiped case
	dec	inner_byte_count	;one byte will be converted
	jz	short collect_8_ok	;ok for 1 byte
	cmp	al,0ffh			;no clipping ?
	jnz	short collect_8_ok	;clipping.so collect 8 bits only
	mov	ah,16			;do 16
	mov	bx,non_clipped_table	;use non-clipped output code
	dec	inner_byte_count	;2 bytes will be converted
collect_8_ok:
	mov	al,ah
;----------------------------------------------------------------------------;
	test	excel,FIRST_IN_PREV	;Does first char span a boundary?
	jz	short p_psd_collect_chars   ;Need to cross two
	add	al,8			;have to collect 8 more 

;	This is the start of the real loop where we zip through
;	all the data we pushed onto the stack.


p_psd_collect_chars:
	xor	si,si			;Dispatch table index
	mov	dx,bp			;Save frame pointer
	mov	bp,buffer		;--> next character's data
	mov	cx,wptr [bp].fd_width
	errnz	fd_phase-fd_width-1

	add	ch,cl			;CH = next char's start bit position
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit

	inc	si
	add	ch,[bp].fd_width[-1 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit

	inc	si
	add	ch,[bp].fd_width[-2 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit

	inc	si
	add	ch,[bp].fd_width[-3 * size frame_data]
	cmp	ch,al
	jb	short p_psd_unlikely_cases
	ja	short p_psd_have_more_than_enough
;	je	p_psd_have_exact_fit

p_psd_have_exact_fit:
	mov	cx,si			;# of source character involved
	shl	si,1			;Compute index into dispatch table
	mov	bx,wptr cs:[bx][si]	;Get address of drawing function
	mov	ax,si
	shl	si,1
	add	si,ax
	errnz	<(size frame_data) - 6>
	neg	si			;Going backwards
	lea	ax,[bp].[si]
	test	ss_boldflag,01h		;bolding required ?
	jnz	short p_psd_exact_fit_from_bold ;yes
	lea	ax,[bp].[-(size frame_data)][si]
p_psd_exact_fit_from_bold:
	xchg	bp,dx			;Restore frame pointer
	mov	buffer,ax		;Save new buffer pointer


;	Call the procedure with the following register variables
;
;	    DS:     =  Font bits segment
;	    ES:DI  --> destination
;	    DX	    =  pointer to frame data
;	    CX	    =  # of source characters - 1
;	    AX	    =  Visible height

	mov	ax,clipped_font_height
	call	bx			;Invoke drawing routine
;----------------------------------------------------------------------------;
;*16bit								             ;
;----------------------------------------------------------------------------;
	mov	al,24
	test	ss_boldflag,01h
	jnz	short p_psd_exact_count_from_bold
	mov	al,16
p_psd_exact_count_from_bold:
;----------------------------------------------------------------------------;
process_counts:
	mov	bx,non_clipped_table	;No clipping required
	mov	ss_clip_mask,0FFh
	cmp	inner_byte_count,0	; all bytes converted ?
	jz	short p_psd_see_about_last  ; yes. process any last byte
	dec	inner_byte_count	; at least one more to convert
	jz	short odd_byte_at_end_case  ; last byte at end
	dec	inner_byte_count	; we will convert two
	jmp	p_psd_collect_chars
odd_byte_at_end_case:
	sub	al,8			; collect 8 bits less
	mov	ss_clip_mask,0ffh
	mov	bx,clipped_table	; force clipping code
	jmp	p_psd_collect_chars


p_psd_have_more_than_enough:
	mov	cx,si			;# of source character involved
	shl	si,1			;Compute index into dispatch table
	mov	bx,wptr cs:[bx][si]	;Get address of drawing function

	mov	ax,si
	shl	si,1
	add	si,ax

	errnz	<(size frame_data) - 6>
	neg	si			;Going backwards
	lea	ax,[bp][si]
	xchg	bp,dx			;Restore frame pointer
	mov	buffer,ax		;Save new buffer pointer


;	Call the procedure with the following register variables
;
;	    DS:     =  Font bits segment
;	    ES:DI  --> destination
;	    DX	    =  pointer to frame data
;	    CX	    =  # of source characters - 1
;	    AX	    =  Visible height

 	mov	ax,clipped_font_height
	call	bx			;Invoke drawing routine
;----------------------------------------------------------------------------;
;*16bit									     ;
;----------------------------------------------------------------------------;
;	mov	al,16			;First crosses byte boundary
	mov	al,24
;----------------------------------------------------------------------------;
	jmp	short process_counts


p_psd_unlikely_cases:
	inc	si
	add	ch,[bp].fd_width[-4 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	p_psd_have_exact_fit

	inc	si
	add	ch,[bp].fd_width[-5 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-6 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-7 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit_relay
	jmp	p_psd_very_unlikely

p_psd_have_exact_fit_relay:
	jmp	p_psd_have_exact_fit	;It had to have fit!
p_psd_have_more_than_enough_relay:
	jmp	p_psd_have_more_than_enough


p_psd_see_about_last:
	sub	al,8			;only 1 byte to do
;	mov	al,either 8 or 16	;Was done on the way here
	and	excel,not RES_EGA_INNER ;Don't restore ega after partial byte
	xor	cx,cx			;Zero last byte clipped mask
	xchg	cl,right_clip_mask	;  and get it
	jcxz	p_psd_exit		;No last byte to deal with
 	mov	ss_clip_mask,cl 	;have last byte, set clip mask
	mov	bx,clipped_table	;Must use the clipped table
	jmp	p_psd_collect_chars

p_psd_exit:
	ret


p_psd_very_unlikely:

	inc	si
	add	ch,[bp].fd_width[-8 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-9 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-10 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-11 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-12 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-13 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-14 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	je	short p_psd_have_exact_fit_relay

	inc	si
	add	ch,[bp].fd_width[-15 * size frame_data]
	cmp	ch,al
	ja	short p_psd_have_more_than_enough_relay
	jmp	p_psd_have_exact_fit	;It had to have fit!

p_process_stack_data endp

	page
;---------------------------Public-Routine------------------------------;
;
; p_set_ega_opaque_mode
;
;   The ega is programmed for entire byte opaquing
;
; Entry:
;	None
; Returns:
;	none
; Error Returns:
;	none
; Registers Destroyed:
;	AX, DX
; Registers Preserved:
;	BX,CX,SI,DI,DS,ES
; Calls:
;
; History:
;  Wed Mar 04, 1987 04:32:46a	-by-  Tony Pisculli	[tonyp]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,EGAMem


	public	p_set_ega_opaque_mode
p_set_ega_opaque_mode	proc near

	mov	dx,EGA_BASE + SEQ_DATA
	mov	al,MM_ALL
	out	dx,al
	mov	dl,GRAF_ADDR
	mov	ax,0FFh shl 8 + GRAF_BIT_MASK
	out16	dx,ax
	mov	ax,colors
	xor	ah,al
	mov	al,GRAF_SET_RESET
	out16	dx,ax
	not	ah
	mov	al,GRAF_ENAB_SR
	out16	dx,ax
	mov	ax,DR_XOR shl 8 + GRAF_DATA_ROT
	out16	dx,ax
	mov	al,tonys_bar_n_grill
	ret

p_set_ega_opaque_mode	endp

	page
;---------------------------Public-Routine------------------------------;
;
; p_prepare_for_overlap
;
;   Possible negative justification and/or width vector.  If
;   opaque mode, then compute the extents of the string so that
;   the bounding box can be output if we step backwards.  If we
;   will step backwards, opaque the area where the string will
;   go and set transparent mode for the actual text output routine.
;
; Entry:
;	bl = accel
; Returns:
;	None
; Error Returns:
;	to p_build_all_done if nothing will show
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,DS,ES
; Registers Preserved:
;	None
; Calls:
;	p_worst_case_ext
;	p_output_o_rect
; History:
;	Wed 06-May-1987 21:33:15 -by-  Walt Moore [waltm]
; wrote it
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,nothing
	assumes es,nothing
	assumes	ss,StrStuff

p_prepare_for_overlap proc near


	push	brk_err 		;Must not destroy the justification
	push	brk_count		;  DDA parameters while we find out
	push	brk_rem 		;  how long the string is and if
	push	off_lp_dx		;  we stepped backwards
	mov	cx,count		;  we stepped backwards
	call	p_worst_case_ext
	pop	off_lp_dx
	pop	brk_rem
	pop	brk_count
	pop	brk_err
	test	wc_flags,STEPPED_BACK
	jz	short p_pfo_exit	;Did not step backwards


;-----------------------------------------------------------------------;
;
;	Will be stepping backwards.  Opaque the area where the string
;	will go.  This area will have to be clipped.
;
;-----------------------------------------------------------------------;

	mov	ax,clip.right
	mov	cx,x			;CX = lhs
	add	bx,cx			;BX = rhs
	jo	short p_pfo_have_rhs	;Use clip.right for right side
	min_ax	bx

p_pfo_have_rhs:
	xchg	ax,bx			;Need rhs in BX
	mov	ax,clip.left
	max_ax	cx
	cmp	bx,ax
	stc				;JGE is SF = OF, doesn't use carry!
	jle	short p_pfo_exit_nothing_shows	;Null or negative interval


;-----------------------------------------------------------------------;
;
;	The interval will show.  Dummy this up as a call to the
;	opaque rectangle code to output the bounding box.  Since
;	TEXT_VISIBLE was clear bu p_build_string, the opaque code
;	will not perform an intersection of text_bbox and o_rect.
;
;-----------------------------------------------------------------------;

	mov	wc_opaque_lhs,ax	;Save lhs/rhs incase we have an
	mov	wc_opaque_rhs,bx	;  opaquing rectangle
	or	wc_flags,WC_SET_LR	;Set left/right into text bbox
	push	o_rect.left		;Save real o_rect bbox
	push	o_rect.right
	push	o_rect.top
	push	o_rect.bottom
	mov	cx,text_bbox.top
	mov	dx,text_bbox.bottom
	mov	o_rect.left,ax		;Set text bbox as area to opaque
	mov	o_rect.right,bx
	mov	o_rect.top,cx
	mov	o_rect.bottom,dx
	mov	bl,excel
	call	p_output_o_rect
	pop	o_rect.bottom
	pop	o_rect.top
	pop	o_rect.right
	pop	o_rect.left

	and	accel,not IS_OPAQUE	;Will output text in transparent mode

p_pfo_exit:
	ret

p_pfo_exit_nothing_shows:
	pop	ax
	jmp	p_build_restore_opaque

p_prepare_for_overlap endp


ifdef	PUBDEFS
	include BUILDSTR.PUB
endif


sEnd	pCode
	end
