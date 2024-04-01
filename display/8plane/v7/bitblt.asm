page	,132
;----------------------------Module-Header------------------------------;
; Module Name: BITBLT.ASM
;
; BitBLT at level of device driver.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
;
; This is the main module of those comprising the source to BitBLT
; (Bit BLock Transfer) for Microsoft Windows display drivers. It
; defines the procedure, and performs general preprocessing for all BLT
; requests.
;
; BitBLT  transfers a rectangle of bits from source to destination,
; doing some useful operations on the way, namely:
;
; o	clipping the source rectangle to fit within the
; 	source device dimensions;
;
; o	excluding the cursor;
;
; o	performing a user-specified raster operation, out of
; 	a vast array of choices, which takes the form
;
; 	D = f(S,D,P)
;
; 	where S = source bit, D = destination bit, P = pattern
; 	bit, and  f  is a sequence of logical operations (AND, OR,
;	XOR, NOT) on S, D, and P;
;		
; o	recognizing common special cases for accelerated processing.
;
;
; For a detailed explanation of the contortions BitBLT goes through
; to put your bits in place, see the file COMMENT.BLT.
;
;
; BitBLT consists of the following files:
;
;	BITBLT.ASM		procedure definition
;	CBLT.BLT		procedure to compile arbitrary BLT on stack
;
;	GENLOCAL.BLT		function parameters and generic locals
;	CLRLOCAL.BLT		color/monochrome-related locals
;	DEVLOCAL.BLT		device-related locals
;
;	GENCONST.BLT		generic constants
;	CLRCONST.BLT		color/monochrome constants
;	DEVCONST.BLT		constants used by device-dependent code
;
;	GENDATA.BLT		generic compiled code templates and data
;	CLRDATA.BLT		color/monochrome-dependent templates and data
;	DEVDATA.BLT		device-dependent code templates and data
;
;	ROPDEFS.BLT		constants relating to ROP definitions
;	ROPTABLE.BLT		table of ROP templates
;
;	PDEVICE.BLT		PDevice processing
;	PATTERN.BLT		pattern preprocessing
;	COPYDEV.BLT		copy device data into local frame
;	COMPUTEY.BLT		compute y-related values
;
;	EXIT.BLT		device-specific cleanup before exiting
;	SPECIAL.BLT		special case code
;
;	COMMENT.BLT		overview of history and design
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.

THIS_IS_DOS_3_STUFF = 1		; remove this line for WinThorn

	title	BitBLT
	%out	BitBlt


;	This function will perform private stack checking.  In order for
;	private stack checking to occur, two symbols must be defined
;	prior to the inclusion of CMACROS.INC.	?CHKSTK must be defined
;	if the cmacros are to perform stack checking on procedures with
;	local parameters.  ?CHKSTKPROC must be defined if private stack
;	checking will be used.
;
;	The actual macro body for ?CHKSTKPROC will be defined later.
;	(See MACROS.MAC).

?CHKSTK		 = 1
?CHKSTKPROC	macro
		endm
ifdef	THIS_IS_DOS_3_STUFF
else
	.286p
endif


;	Define the portions of GDIDEFS.INC that will be needed by bitblt.

incLogical	= 1		;Include GDI logical object definitions
incDrawMode	= 1		;Include GDI DrawMode definitions

	.xlist
	include	CMACROS.INC
	include	GDIDEFS.INC
	include	MACROS.MAC
	include	DISPLAY.INC
	include	CURSOR.INC
ifdef	THIS_IS_DOS_3_STUFF
else
	include	FIREWALL.INC
	include	DDC.INC
	include	ERROR.INC
	include	INSTANCE.INC
endif
	include	NJUMPS.MAC
	.list


	externA  SCREEN_W_BYTES		;Screen width in bytes
	externNP set_bank_select
	externNP CBLT

ifdef	PALETTES
	externB  PaletteModified	;Set when palette is modified
	externFP TranslateBrush		;translates the brush
	externFP TranslateTextColor     ;translates text colors
endif

ifdef	THIS_IS_DOS_3_STUFF
	externA  ScreenSelector		;Segment of Regen RAM
endif

	externFP AllocSelector		; allocate a new selector
	externFP PrestoChangeoSelector	; CS <--> DS conversion
	externFP FreeSelector		; free an allocated selector

	externFP AllocDSToCSAlias	; alocates a CS alias for a Data seg
ifdef	EXCLUSION			;If cursor exclusion
	externNP exclude		;Exclude area from screen
	externNP unexclude		;Restore excluded area to screen
endif

ifdef	THIS_IS_DOS_3_STUFF
sBegin	Data

	externB enabled_flag		;Non-zero if output allowed
	externW	ScratchSel		; the free selector
	EXTRN	abPaletteAccl:BYTE

ifdef   PALETTES
	externB	PaletteModified		;0ffh if palette modified
endif

work_buf  db	SCREEN_WIDTH dup(?)	;device-to-device scratch buffer

sEnd	Data
endif


%OUT2	macro	text
	if2
	%out	text
	endif
	endm


lp_	struc
  off	dw	?
  sel	dw	?
lp_	ends


sBegin	Code
assumes cs,Code
assumes ds,Data
assumes es,nothing


;	Following are the BitBLT include-files.  Some are commented out
;	because they contain address definitions are are included in
;	CBLT.ASM, but are listed here for completeness.  The remaining
;	files include those that make up the local variable frame, and 
;	those containing subroutines.  The frame-variable files are
;	included immediately after the cProc BITBLT declaration.  The
;	subroutines files are included near the end of this file.

	.xlist
	include	GENCONST.BLT	;EQUs
	include	CLRCONST.BLT	;EQUs
	include	DEVCONST.BLT	;EQUs
	include	GENDATA.BLT	;bitmask and phase tables
	include	CLRDATA.BLT	;Color/mono specific templates,data
	include DEVDATA.BLT	;Driver specific templates,data
	include ROPDEFS.BLT	;Raster operation definitions
	include	ROPTABLE.BLT	;Raster operation code templates
	.list



	page

;	gl_flag0
;
;	F0_GAG_CHOKE	Set if the source and destination are of different
;			color formats.	When set, some form of color
;			conversion will be required.
;
;			Once you see what all is involved with color
;			conversion, you'll understand why this flag is
;			called this.
;
;	F0_COLOR_PAT	Set if color pattern fetch code will be used.  If
;			clear, then mono pattern fetch code will be used.
;			Mono/color pattern fetch is always based on the
;			destination being mono/color (it is the same).
;
;	F0_PAT_PRESENT	Set if a pattern is involved in the BLT.
;
;	F0_SRC_PRESENT	Set if a source  is involved in the BLT.
;
;	F0_SRC_IS_DEV	Set if the source is the physical device.  Clear if
;			the source is a memory bitmap.
;
;	F0_SRC_IS_COLOR	Set if the source is color, clear if monochrome.
;
;	F0_DEST_IS_DEV	Set if the destination is the physical device.
;			Clear if the destination is a memory bitmap.
;
;	F0_DEST_IS_COLOR
;			Set if the destination is color, clear if
;			monochrome.

	subttl	BITBLT entry
	page

cProc	BITBLT,<FAR,PUBLIC,WIN,PASCAL>,<si,di>
	include	bitblt.var
cBegin
WriteAux <'bitblt'>
	page
ife	???				;If no locals
	?CHKSTKPROC 0			;See if room
endif
	jnc	bitblt_stack_ok		;There was room for the frame
	jmp	bitblt_stack_ov 	;There was no room

	db	"COPYRIGHT FEBRUARY 1990 HEADLAND TECHNOLOGY, INC."
bitblt_stack_ok:

	cld	; INSURANCE **************************

	mov	my_data_seg, ds
	mov	al,enabled_flag 	;Save enabled_flag while we still
	mov	local_enable_flag,al	;  have DS pointing to Data
	mov	ax,ScratchSel		; get the free selector
	mov	WorkSelector,ax		; save it

;----------------------------------------------------------------------------;
; if the palette manager is supported, do the on-the-fly translation now     ;
;----------------------------------------------------------------------------;

ifdef	PALETTES
	cmp	PaletteModified,0ffh	; was the palette modified
        jnz     no_translation
        les     si,lpDestDev            ; the destination device
        mov     ax,es:[si].bmType       ; get the type of the device
	or	ax,ax			; test for physical display
	jz	no_translation		; no translation for a mem output
	arg	lpPBrush
	cCall	TranslateBrush
	mov	seg_lpPBrush,dx
	mov	off_lpPBrush,ax
        les     si,lpSrcDev             ; get the pointer to source desc
        mov     ax,es
	or	ax,si
	jz	no_translation		; if no source device
        mov     al,byte ptr es:[si].bmPlanes ; get the number of planes
	cmp	al,1			; monochrome source ?
	jz	no_translation		; donot translate the colors

	arg	lpDrawMode
	cCall	TranslateTextColor
	mov	seg_lpDrawMode,dx
	mov	off_lpDrawMode,ax
no_translation:

endif


;----------------------------------------------------------------------------;



	subttl	ROP Preprocessing
	page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	Get the encoded raster operation, and map the raster op if needed.
;
;	To map the ROPS 80h through FFh to 00h through 7Fh, take the
;	1's complement of the ROP, and invert the "negate needed" flag.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	cld				;Let's make no assumptions about this!

;----------------------------------------------------------------------------;

	xor	ax,ax			;Assume not 80h : FFh
	mov	bx,seg_Rop
	or	bh,bh			;Outside the legal range?
	jnz	complain		;  Yes, ignore it
	or	bl,bl			;Is this in the second half (80-FF)?
	jns	parse_10 		;  No, rop index is correct
	not	bl			;  Yes, want the inverse
	mov	ah,HIGH NEGATE_NEEDED	;Want to invert the not flag
	errnz	<LOW NEGATE_NEEDED>

parse_10:
	add	bx,bx			;Turn into a word index
	xor	ax,roptable[bx] 	;Get ROP, maybe toggle negate flag
	mov	gl_operands,ax		;Save the encoded raster operation

	mov	bl,ah			;Set gl_flag0 for source and pattern
	and	bl,HIGH (SOURCE_PRESENT+PATTERN_PRESENT)
	ror	bl,1

	errnz	 <SOURCE_PRESENT - 0010000000000000b>
	errnz	<PATTERN_PRESENT - 0100000000000000b>
	errnz	 <F0_SRC_PRESENT - 00010000b>
	errnz	 <F0_PAT_PRESENT - 00100000b>

	jmp	short parse_end

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	complain - complain that something is wrong
;
;	An error is returned to the caller without BLTing anything.
;
;	Entry:	None
;
;	Exit:	AX = 0 (error flag)
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

complain:
	xor	ax,ax			;Set the error code
	jmp	bitblt_exit_fail

;	v_exit - Just a vector to Exit

v_exit:
	jmp	bitblt_exit

parse_end:


	call	pdevice_processing
	jc	complain

;  if the device is involved in the blt,
;  then it must be enabled

	test	BH,F0_SRC_IS_DEV+F0_DEST_IS_DEV
	jz	dev_isnt_involved

	test	local_enable_flag,0FFh
	jz	complain

dev_isnt_involved:
	mov	gl_flag0,bh		;Save flag values


	test	bh,F0_PAT_PRESENT	;Pattern required?
	jz	pattern_preproc_end	;  No, skip pattern check

	lds	si,lpPBrush		;--> physical brush
	mov	ax,ds
	or	ax,si
	jz	complain    ;Null pointer, error

	les	di	,lpDrawMode
	mov	cx	,ss
	lea	ax	,cl_a_brush

	push	bp
	mov	bp	,my_data_seg
	call	pattern_preprocessing
	pop	bp
	jc	complain
	mov	off_gl_lp_pattern ,di
	mov	seg_gl_lp_pattern ,es


	mov	cl_brush_accel,al

pattern_preproc_end:

	subttl	Input Clipping
	page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	GDI doesn't do input clipping.  The source device must be clipped
;	to the device limits, otherwise an exception could occur while in
;	protected mode.
;
;	The destination X and Y, and the extents have been clipped by GDI
;	and are positive numbers (0-7FFFh).  The source X and Y could be
;	negative.  The clipping code will have to check constantly for
;	negative values.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	?_pub	input_clipping
input_clipping:
input_clip_x:
	mov	si,xExt 		;X extent will be used a lot
	mov	di,yExt 		;Y extent will be used a lot
	test	gl_flag0,F0_SRC_PRESENT	;Is there a source?
	jz	input_clip_chk_null_blt	;No source, no input clipping needed

	mov	ax,SrcxOrg		;Will need source X org
	mov	bx,gl_src.width_bits	;Maximum allowable is width_bits-1
	or	ax,ax			;Any left edge overhang?
	jns	input_clip_rgt_edge	;  No, left edge is on the surface


;	The source origin is off the left hand edge of the device surface.
;	Move both the source and destination origins right by the amount
;	of the overhang and also remove the overhang from the extent.
;
;	There is no need to check for the destination being moved off the
;	right hand edge of the device's surface since the extent would go
;	zero or negative were that to happen.


	add	si,ax			;Subtract overhang from X extent
	js	v_exit			;Wasn't enough, nothing to BLT
	sub	DestxOrg,ax		;Move destination left
	xor	ax,ax			;Set new source X origin
	mov	SrcxOrg,ax


;	The left hand edge has been clipped.  Now clip the right hand
;	edge.  Since both the extent and the source origin must be
;	positive numbers now, any sign change from adding them together
;	can be ignored if the comparison to bmWidth is made as an
;	unsigned compare (maximum result of the add would be 7FFFh+7FFFh,
;	which doesn't wrap past zero).


input_clip_rgt_edge:
	add	ax,si			;Compute right edge + 1
	sub	ax,bx			;Compute right edge overhang
	jbe	input_clip_save_xext	;No overhang
	sub	si,ax			;Subtract overhang from X extent
	js	v_exit			;Wasn't enough, nothing to BLT

input_clip_save_xext:
	mov	xExt,si 		;Save new X extent


;	Now clip the Y coordinates.  The procedure is the same and all
;	the above about positive and negative numbers still holds true.


input_clip_y:
	mov	ax,SrcyOrg		;Will need source Y org
	mov	bx,gl_src.height	;Maximum allowable is height-1
	or	ax,ax			;Any top edge overhang?
	jns	input_clip_btm_edge	;  No, top is on the surface


;	The source origin is off the top edge of the device surface.
;	Move both the source and destination origins down by the amount
;	of the overhang, and also remove the overhang from the extent.
;
;	There is no need to check for the destination being moved off
;	the bottom of the device's surface since the extent would go
;	zero or negative were that to happen.


	add	di,ax			;Subtract overhang from Y extent
	js	v_exit			;Wasn't enough, nothing to BLT
	sub	DestyOrg,ax		;Move destination down
	xor	ax,ax			;Set new source Y origin
	mov	SrcyOrg,ax


;	The top edge has been clipped.	Now clip the bottom edge. Since
;	both the extent and the source origin must be positive numbers
;	now, any sign change from adding them together can be ignored if
;	the comparison to bmWidth is made as an unsigned compare (maximum
;	result of the add would be 7FFFh+7FFFh, which doesn't wrap thru 0).


input_clip_btm_edge:
	add	ax,di			;Compute bottom edge + 1
	sub	ax,bx			;Compute bottom edge overhang
	jbe	input_clip_save_yext		;No overhang
	sub	di,ax			;Subtract overhang from Y extent
	jns	input_clip_save_yext

qq_exit:
	jmp	bitblt_exit		;Wasn't enough, nothing to BLT


input_clip_save_yext:
	mov	yExt,di 		;Save new Y extent

input_clip_chk_null_blt:
	or	si,si
	jz	qq_exit 		;X extent is 0
	or	di,di
	jz	qq_exit 		;Y extent is 0

input_clip_end:


	subttl	Cursor Exclusion
	page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	Cursor Exclusion
;
;	If either device or both devices are for the display, then
;	the cursor must be excluded.  If both devices are the display,
;	then a union of both rectangles must be performed to determine
;	the exclusion area.
;
;	Currently:
;		SI = X extent
;		DI = Y extent
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	?_pub	cursor_exclusion
cursor_exclusion:

ifdef	EXCLUSION
	mov	al,gl_flag0
	and	al,F0_SRC_IS_DEV+F0_DEST_IS_DEV	;Are both memory bitmaps?
	jz	cursor_exclusion_end	;  Yes, no exclusion needed

	dec	si			;Make the extents inclusive of the
	dec	di			;  last point

	mov	cx,DestxOrg		;Assume only a destination on the
	mov	dx,DestyOrg		;  display
	test	al,F0_SRC_IS_DEV	;Is the source a memory bitmap?
	jz	cursor_exclusion_no_union;  Yes, go set right and bottom
	test	al,F0_DEST_IS_DEV	;  (set 'Z' if dest is memory)
	xchg	ax,cx			;  No, prepare for the union
	mov	bx,dx

	mov	cx,SrcxOrg		;Set source org
	mov	dx,SrcyOrg
	jz	cursor_exclusion_no_union;Dest is memory. Set right and bottom

;	If the source/destination starting Y is greater than SCREEN_HEIGHT,
;	then the blt is supporting a save_screen_bitmap call.  In this case,
;	we only want to exclude whichever rectangle is visible.

	cmp	bx,SCREEN_HEIGHT	  ;If destination is off the screen
	jge	cursor_exclusion_no_union ;  then only use source rectangle
	cmp	dx,SCREEN_HEIGHT	  ;If source is off the screen
	jl	cursor_exclusion_not_ssb  ;  then only use dest rectangle
	xchg	ax,cx
	mov	dx,bx
	jmp	short cursor_exclusion_no_union


;	The union of the two rectangles must be performed.  The top left
;	corner will be the smallest x and smallest y.  The bottom right
;	corner will be the largest x and the largest y added into the
;	extents

cursor_exclusion_not_ssb:
	cmp	cx,ax			;Get smallest x
	jle	cursor_exclusion_y	;CX is smallest
	xchg	ax,cx			;AX is smallest

cursor_exclusion_y:
	cmp	dx,bx			;Get smallest y
	jle	cursor_exclusion_union	;DX is smallest
	xchg	dx,bx			;BX is smallest

cursor_exclusion_union:
	add	si,ax			;Set right
	add	di,bx			;Set bottom
	jmp	short cursor_exclusion_do_it	;Go do exclusion

cursor_exclusion_no_union:
	add	si,cx			;Set right
	add	di,dx			;Set bottom

cursor_exclusion_do_it:
	call	exclude 		;Exclude the area from the screen

endif	;EXCLUSION

cursor_exclusion_end:


	subttl	Step Direction
	page

;-----------------------------------------------------------------------;
;									;
;	Some would have you believe there are TEN distinct cases	;
;	that must be considered to determine the order in which		;
;	source and destination bytes must be processed so that a	;
;	byte isn't overwritten before it is read.			;
;									;
;	Tain't so - if (and only if) the source and destination		;
;	are the SAME, then if (and only if) the source area and		;
;	destination area overlap, then bytes must either be moved	;
;	from higher locations to lower OR vice versa.  FOUR cases,	;
;	maximum:							;
;									;
;		1) Source and destination are different devices		;
;		   (including NO source device)				;
;									;
;		2) Source and destination areas don't overlap		;
;									;
;		3) Source starts lower in memory than destination	;
;		   (start at HIGH end, DECREASING)			;
;									;
;		4) Source starts higher in memory than destination	;
;		   (start at LOW end, INCREASING)			;
;		   (include exact overlap in this case)			;
;									;
;						Gary Maltzen		;
;						January, 1989		;
;									;
;-----------------------------------------------------------------------;

	?_pub	step_direction
step_direction:

;  assume most favorable case

	mov	ah,INCREASING

;  check if there is a source

	mov	al,gl_flag0
	test	al,F0_SRC_PRESENT
	jz	step_dir_found

;  source present
;  check if source and destination are same bitmaps

	mov	dx,off_lpSrcDev
	cmp	dx,off_lpDestDev
	jne	step_dir_found

	mov	dx,seg_lpSrcDev
	cmp	dx,seg_lpDestDev
	jne	step_dir_found

;  source and destination are the same
;  check if rectangles overlap

	mov	dx,xExt
	add	dx,SrcxOrg
	cmp	dx,DestxOrg		; src.x + n.x <= dest.x ?
	jbe	step_dir_found		; --yes

	mov	dx,xExt
	add	dx,DestxOrg
	cmp	dx,SrcxOrg		; dest.x + n.x <= src.x ?
	jbe	step_dir_found		; --yes

	mov	dx,yExt
	add	dx,SrcyOrg
	cmp	dx,DestyOrg		; src.y + n.y <= dest.y ?
	jbe	step_dir_found		; --yes

	mov	dx,yExt
	add	dx,DestyOrg
	cmp	dx,SrcyOrg		; dest.y + n.y <= src.y ?
	jbe	step_dir_found		; --yes

;  rectangles overlap
;  determine which direction to process

	mov	dx,DestyOrg
	cmp	dx,SrcyOrg		; dest.y :: src.y ?
	jne	step_check		; --not equal

	mov	dx,DestxOrg
	cmp	dx,SrcxOrg		; dest.x :: src.x ?
step_check:
	jbe	step_dir_found		; -- if dest.yx <= src.yx

;  overlap requires move start at high end

	mov	ah,DECREASING

;  save the results

step_dir_found:
	mov	gl_direction,ah

;-----------------------------------------------------------------------;
;	Now, that wasn't so difficult, was it?
;-----------------------------------------------------------------------;

	
	subttl	Phase Processing (X)
	page


	?_pub	phase_processing
phase_processing:

;  set up the following...
;
;    gl_extra_fetch
;
;		     v
;	0 ...	s: abcdefgh ijklmnop qrstuvwx
;		d: ABCDEFGH IJKLMNOP QRSTUVWX
;		       ^
;
;		lodsb			al=abcdefgh
;		ror   al,02		al=ghabcdef
;		mov   ah,al		ah=ghabcdef
;		and   ax,F00F		ah=ghab.... al=....cdef
;		or    al,bh		al=----cdef
;		mov   bh,ah		bh=ghab....
;		-RasterOp		al=12345678
;		mov   ah,es:[di]	ah=ABCDEFGH
;		and   ax,F00F		ah=ABCD.... al=....5678
;		or    al,ah		al=ABCD5678
;		stosb
;
;		         v
;	1 ...	s: abcdefgh ijklmnop qrstuvwx
;		d: ABCDEFGH IJKLMNOP QRSTUVWX
;		       ^
;
;		lodsb			al=abcdefgh
;		rol	al,02		al=cdefghab
;		mov	ah,al		ah=cdefghab
;		and	ax,FC03		ah=cdefgh.. al=......ab
;		or	al,bh		al=------ab
;		mov	bh,ah		bh=cdefgh..
;		lodsb			al=ijklmnop
;		rol	al,02		al=klmnopij
;		mov	ah,al		ah=klmnopij
;		and	ax,FC03		ah=klmnop.. al=......ij
;		or	al,bh		al=cdefghij
;		mov	bh,ah		bh=klmnop..
;		-RasterOp		al=12345678
;		mov	ah,es:[di]	ah=ABCDEFGH
;		and	ax,F00F		ah=ABCD.... al=....5678
;		or	al,ah		al=ABCD5678
;		stosb
;
;		
;    gl_step_direction
;
;	(SEE gl_direction?)
;
;    gl_phase_h
;
;    gl_first_mask
;
;    gl_inner_count
;
;	number of bytes to update (per scanline)
;
;    gl_final_mask
;
;	used in last (right most?) masked store
;	hi byte = not lo byte
;
;    gl_align_mask
;
;	used in first (leftmost?) masked store
;	hi byte = not lo byte
;
;    gl_dest.lp_bits.off
;
;    gl_src.lp_bits.off
;
;    gl_pat_col
;
;    gl_pat_row
;
;    gl_direction
;
;	(see step_direction, above)
;

	mov	ax,DestyOrg
	mov	bx,DestxOrg
	cmp	gl_direction,INCREASING
	je	@f
	add	ax,yExt
	add	bx,xExt
	dec	ax
	dec	bx
@@:	and	al,SIZE_PATTERN-1
	and	bl,SIZE_PATTERN-1
	mov	gl_pat_row,al
	mov	gl_pat_col,bl

	mov	gl_extra_fetch,0
	mov	gl_phase_h,0

;  check if we need to do destination phase processing

	test	gl_flag0,F0_DEST_IS_COLOR
	njnz	phase_proc__8

; - - - - - - - - - - - - - - - - - - - 
; DESTINATION IS MONO
; - - - - - - - - - - - - - - - - - - -

public phase_proc__1
phase_proc__1:
	mov	bx,DestxOrg
	mov	cx,bx
	and	cx,not 7
	add	bx,xExt
	dec	bx
	and	bx,not 7
	sub	bx,cx
;
        shr     bx,1
	shr	bx,1
        shr     bx,1
;
	mov	cx,DestxOrg
	and	cx,7
	mov	dl,0ffh
	shr	dl,cl
	mov	gl_first_bit,cl
;
	add	cx,xExt
	mov	ax,0ff00h
	and	cx,7
	jne	@F
;
	mov	al,ah
;
@@:
	shr	ax,cl
	dec	cl
	and	cl,7
	mov	gl_final_bit,cl
;
	dec	bx
	jg	combine
	je	done
	and	al,dl
	sub	bx,bx
	sub	dl,dl
	xchg	ax,dx
	jmp	done
;
combine:
	cmp	al,0ffh
	jne	done
	mov	al,0
	inc	bx
;
done:
	mov	dh,dl
	not	dh
        mov     gl_first_mask,dx
	mov	ah,al
	not	ah
;	 xchg	 ah,al			;JAK
	mov	gl_final_mask,ax
;;;;;;	      or      ah,dh
;;;;;;	      jnz     @F
;;;;;;	      inc     bx
;;;;;;
;;;;;;@@:

;  save inner loop full byte count

	mov	gl_inner_count,bx

;  check for special source processing

	test	gl_flag0,F0_SRC_PRESENT
	jz	phase_proc_end


; - - - - - - - - - - - - - - - - - - - 
; MONO -> MONO
; - - - - - - - - - - - - - - - - - - - 

phase_proc_11:

;  compute the phase adjustment and masks

	mov	bx,DestxOrg
	and	bl,0111b		;bit within byte
	mov	cx,SrcxOrg
	and	cl,0111b		;bit within byte
	sub	cl,bl			;ROL to 
	jz	phase_proc_11b		;..if no phase adjustment
	jb	phase_proc_11a		;..if no extra fetch need

;  requires extra initial fetch

	inc	gl_extra_fetch

phase_proc_11a:

	and	cl,0111b

phase_proc_11b:
	mov	gl_phase_h,cl		;ROL to align bits
	mov	ax,0FF00h
	rol	ax,cl
	mov	gl_align_mask,ax
	jmp	short phase_proc_end


; - - - - - - - - - - - - - - - - - - - 
; DESTINATION IS COLOR
; - - - - - - - - - - - - - - - - - - - 

phase_proc__8:

	mov	ax,xExt
	mov	gl_inner_count,ax
	
; - - - - - - - - - - - - - - - - - - - 

phase_proc_end:

	page


	call	check_device_special_cases
	jc	bitblt_exit		;C ==> BLT done w/special case


	subttl	Memory allocation for BLT compilation
	page

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	Allow room for the BLT code.  The maximum that can be generated
;	is defined by the variable MAX_BLT_SIZE.  This variable must be
;	an even number.
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	assumes cs,Code
	assumes ds,nothing
	assumes es,nothing


	?_pub	cblt_allocate
cblt_allocate:

	?CHKSTKPROC MAX_BLT_SIZE+20h	;See if room on stack			 ;See if room
	jnc	cblt_alloc_stack_ok	;There was room
	jmp	bitblt_exit_fail	;There was no room

cblt_alloc_stack_ok:
	add	sp,20h			;Take off the slop

	mov	di,sp
	mov	off_gl_blt_addr,di	;Save the address for later
	mov	ax,ss			;Set the segment for the BLT
	mov	es,ax

ifdef	THIS_IS_DOS_3_STUFF
else
	assumes ss,InstanceData
	mov	ax,proc_cs_alias
	assumes ss,nothing
endif

;----------------------------------------------------------------------------;
; we will now get a free selector and convert it into a code segment alias of;
; the stack selector and later free it.				             ;
;----------------------------------------------------------------------------;

	mov	ax,WorkSelector		; get the free selector

	push	es			; save stack selector

; convert it to a code segment copy of SS

	cCall	PrestoChangeoSelector,<ss,ax>
	mov	seg_gl_blt_addr,ax	;Save the address for later
	pop	es			; get back stack selector

; we will now execute off this new code selector from the stack

	mov	ax,cs			;Set data seg to CS so we can access
	mov	ds,ax			;  code without overrides
	xor	cx,cx			;Clear out count register


	assumes ds,Code

	call	CBLT			;compile the BLT onto the stack


	subttl	Stack BLT Invocation and Exit
	page


;	The BLT has been created on the stack.	Set up the initial registers,
;	set the direction flag as needed, and execute the BLT.


	test	gl_flag0,F0_SRC_PRESENT ;Is there a source?
	jz	call_blt_get_dest_bits	;  No, don't load its pointer
	lds	si,gl_src.lp_init	;--> source device's first byte

call_blt_get_dest_bits:
	les	di,gl_dest.lp_init	;--> destination device's first byte
	mov	cx,yExt 		;Get count of lines to BLT
	cld				;Assume this is the direction
	cmp	gl_direction,INCREASING	;Stepping to the right?
	jz	call_blt_do_it		;  Yes
	std

call_blt_do_it:
	push	bp			;MUST SAVE THIS

	?_pub	execute_stack_blt
execute_stack_blt:
	call	gl_blt_addr 		;Call the FAR process

	pop	bp

	add	sp,MAX_BLT_SIZE		;Return BLT space

;	jmp	bitblt_exit		;Hey, we're done!
	errn$	bitblt_exit


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	exit - leave BitBLT
;
;	Well, the BLT has been processed.  Restore the stack to its
;	original status, restore the saved user registers, show no
;	error, and return to the caller.
;
;	Entry:	None
;
;	Exit:	AX = 1
;
;	Uses:	All
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	?_pub	bitblt_exit
bitblt_exit:


	mov	ax,1			;Clear out error register (good exit)
;	jmp	bitblt_exit_fail
	errn$	bitblt_exit_fail


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	bitblt_exit_fail - exit because of failure
;
;	The BLT is exited.
;
;	Entry:	AX = error code (0 if error)
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

bitblt_exit_fail:
	cld				;Leave direction cleared
ifdef	EXCLUSION			     
	call	unexclude		;Remove any exclusion area
endif

bitblt_stack_ov:
cEnd


;-----------------------------------------------------------------------;
;	Subroutines.  These have been included with the aim of
;	segregating device dependent code from independent code,
;	while cleanly preserving the local variable frame.
;-----------------------------------------------------------------------;

	include	PDEVICE.BLT	;PDevice processing
	include	PATTERN.BLT	;pattern preprocessing
	include	SPECIAL.BLT	;non-compiled BLT subroutines

sEnd	Code


end
