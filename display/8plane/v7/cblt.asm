 	page	,132
;----------------------------Module-Header------------------------------;
; Module Name: CBLT.ASM
;
; Subroutine to compile a BLT subroutine onto the stack.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
; This file contains two subroutines which build a small program on the
; stack to accomplish the requested BLT.
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.


THIS_IS_DOS_3_STUFF = 1		;remove this line for WinThorn
	.xlist
ifdef	TEFTI
	include TEFTI.MAC
endif
	include CMACROS.INC
	include MACROS.MAC
	include	NJUMPS.MAC
incLogical	= 1		;Include GDI logical object definitions
incDrawMode	= 1		;Include GDI DrawMode definitions
	include GDIDEFS.INC
	include DISPLAY.INC
	.list


ifdef	THIS_IS_DOS_3_STUFF
	externA ScreenSelector		; Segment of Regen RAM
endif
	externA SCREEN_W_BYTES		; Screen width in bytes
ifdef	GEN_COLOR_BLT
;;;	externA COLOR_DONT_CARE 	; for use with GRAF_CDC register
endif

sBegin	Data

ifdef PALETTES ;========================>

	externB	PaletteModified		; 0ffh IFF palette modified
	externB	PaletteTranslationTable	; mem8 -> dev color translation
	externB	PaletteIndexTable	; dev -> mem8 color translation

endif ;=================================>

sEnd	Data

GEN	macro	opcode
	if	high opcode
		mov	ax,opcode
		stosw
	else
		mov	al,opcode
		stosb
	endif
	endm


sBegin	Code
	assumes cs,Code
	assumes ds,Code
	assumes es,nothing


	.xlist

;	Following are the BitBLT include-files.  Some are commented out
;	because they contain address definitions are are included in
;	BITBLT.ASM, but are listed here for completeness.  The remaining
;	files include those that make up the local variable frame, and 
;	those containing subroutines.  The frame-variable files are
;	included immediately after the cProc CBLT declaration.  The
;	subroutines files are not included in CBLT.ASM.

	include	GENCONST.BLT		;EQUs
	include	CLRCONST.BLT		;EQUs
	include	DEVCONST.BLT		;EQUs
;	include	GENDATA.BLT		;bitmask and phase tables
;	include	CLRDATA.BLT		;color/mono templates,data
;	include DEVDATA.BLT		;driver-specific templates,data
	include	ROPDEFS.BLT		;ROP definitions
;	include	ROPTABLE.BLT		;Raster operation code templates
ifdef	THIS_IS_DOS_3_STUFF
else
;	include	FIREWALL.INC
;	include	DDC.INC
endif

	externW	roptable
	.list

	page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	Pattern Fetch Code
;
;	The pattern fetch code will be created on the fly since
;	most of the instructions need fixups.
;
;	This template is really just a comment to indicate what
;	the generated code should look like.
;
;	Entry:	None
;
;	Exit:	DH = pattern
;
;	Uses:	AX,BX,CX,DH,flags
;
;
;	The following registers are available to the pattern fetch
;	logic (as implemented herein):
;
;		AX,BX,CX,DX,flags
;
;
;	For monochrome brushes:
;
;	    mov     ax,1234h		;Load segment of the brush
;	    mov     bx,1234h		;Load offset of the brush
;	    mov     cx,ds		;Save DS
;	    mov     ds,ax		;DS:BX --> brush
;	    mov     dh,7[bx]		;Get next brush byte
;	    mov     al,ss:[1234h]	;Get brush index
;	    add     al,gl_direction	;Add displacement to next byte (+1/-1)
;	    and     al,00000111b	;Keep it in range
;	    mov     ss:[1234h],al	;Store displacement to next byte
;	    mov     ds,cx		;Restore DS
;
;
;	For color brushes:
;
;	    mov     ax,1234h		;Load segment of the brush
;	    mov     bx,1234h		;Load offset of the brush
;	    mov     cx,ds		;Save DS
;	    mov     ds,ax		;DS:BX --> brush
;	    mov     dh,7[bx]		;Get next brush byte
;	    mov     al,ss:[1234h]	;Get brush index
;	    add     al,SIZE Pattern	;Add disp. to next plane's bits
;	    and     al,00011111b	;Keep it within the brush
;	    mov     ss:[1234h],al	;Store disp. to next plane's bits
;	    mov     ds,cx		;Restore DS
;
;
;	For both templates, SS:[1234] is the address of the 7 in the
;	"mov dh,7[bx]" instruction.  This is the index to this scan's
;	bit pattern in the brush.  This value will range from 0 to
;	(SIZE pattern)-1 for monochrome devices, and from 0 to
;	((NumberPlanes)*(SIZE pattern))-1 for color devices.
;
;	For color brushes, SS:[1234] must also be fixed up when the next
;	scan line is selected, else it would index into the monochrome
;	portion of the brush (e.g. 1,9,17,25, where 25 is not part of the
;	color brush).
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

	page
;-----------------------------Public-Routine----------------------------;
; CBLT
;
; Compile a BLT onto the stack.
;
; Entry:
;	ES:DI --> memory on stack to receive BLT program
; Returns:
;	Nothing
; Registers Preserved:
; Registers Destroyed:
; Calls:
;	y_update
; History:
;  Sun 16-Aug-1987 16:45:47 -by-  Wesley O. Rupel [wesleyr]
; Bitmap Color Conversion uses image color
;  Mon 20-Jul-1987 17:30:14 -by-  Wesley O. Rupel [wesleyr]
; Added 4-plane support.
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Wrote it for Windows in distant past.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


cProc	CBLT_frame,<FAR>,<>
	include	bitblt.var
cBegin	nogen
cEnd	nogen
	page
	public	CBLT
CBLT	proc	near
WriteAux <'cblt'>
	mov	al,gl_flag0
	test	al,F0_SRC_PRESENT
	jz	@f

;  compute initial source pointer (if present)

	mov	ax,SrcyOrg
	mov	bx,SrcxOrg
	lea	si,gl_src
	call	map_address

;  compute initial destination pointer

@@:	mov	ax,DestyOrg
	mov	bx,DestxOrg
	lea	si,gl_dest
	call	map_address

	mov	al,gl_flag0
	test	al,F0_DEST_IS_COLOR
	jnz	cblt_x8

cblt_x1:
	test	al,F0_SRC_PRESENT
	jz	use_11
	test	al,F0_SRC_IS_COLOR
	jnz	use_81
use_11:	jmp	cblt11
use_81:	jmp	cblt81

cblt_x8:
	test	al,F0_SRC_PRESENT
	jz	use_88
	test	al,F0_SRC_IS_COLOR
	jz	use_18
use_88:	jmp	cblt88
use_18:	jmp	cblt88

CBLT	endp

	include	cblt11.blt
;	include	cblt18.blt
	include	cblt81.blt
	include	cblt88.blt


	subttl	Stack Loop Generation
	page

;-----------------------------------------------------------------------
;
;  gen_loop - generate LOOP on CX in stack
;
; ENTRY:
;	SS:BX = loop destination
;	SS:DI = current instruction
; EXIT:
;	SS:DI updated
;	one of two loop sequences generated
;	(1)	LOOP	back
;	(2)	DEC	CX
;		JZ	$+5
;		JMP	back

gen_loop	proc	near
IFDEF DEBUG
INT 1
ENDIF

	sub	bx,di			;  start of loop
	cmp	bx,2-127 		;Can this be a short label?
	jc	@f			;  No, must make it a near jmp

	mov	al,I_LOOP
	mov	ah,bl
	sub	ah,2			;Bias offset by length of LOOP inst.
	stosw				;Set the loop instruction
	ret

@@:	mov	al,I_DEC_CX
	stosb
	mov	al,I_JZ
	mov	ah,3	;(jump over JUMP_NEAR)
	stosw
	mov	al,I_JMP_NEAR
	stosb
	mov	ax,bx
	sub	ax,6			;Adjust jump bias
	stosw				;  and store it into jump
	ret

gen_loop	endp


	subttl	Base Address Computation
	page

;-----------------------------------------------------------------------
;
;  map_address - convert X,Y coordinate to initial pointer
;
; ENTRY:
;	AX = Y
;	BX = X
;	SS:SI -> local DEV structure
; EXIT:
;	DEV.lp_init set
;

map_address	proc	near

	cmp	gl_direction,INCREASING
	je	@f

;  decreasing blt, start at far end

	add	ax,yExt
	dec	ax
	add	bx,xExt
	dec	bx

;  select appropriate routine

@@:	test	ss:[si].dev_flags,IS_DEVICE
	jz	map_bitmap

;----------; 
;  DEVICE  ;
;----------; 

map_device:
	mov	dx,ss:[si].width_b
	mul	dx
	add	ax,bx
	adc	dl,0
	mov	ss:[si].init_page,dl

	mov	dx,ss:[si].SEG_lp_bits
	mov	ss:[si].SEG_lp_init,dx

	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
	ret

;----------; 
;  BITMAP  ;
;----------; 

map_bitmap:
	mov	dx,ss:[si].SEG_lp_bits
	mov	cx,ss:[si].seg_index
	jcxz	map_bitmapx

@@:	add	dx,cx
	sub	ax,ss:[si].scans_seg
	jnc	@b
	sub	dx,cx
	add	ax,ss:[si].scans_seg

map_bitmapx:
	mov	ss:[si].SEG_lp_init,dx

	test	ss:[si].dev_flags,IS_COLOR
	jnz	map_bitmap8

;---------------------;
;  MONOCHROME BITMAP  ;
;---------------------;

map_bitmap1:
	mov	dx,ss:[si].width_b
	mul	dx
	shiftr	bx,3
	add	ax,bx
	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
	ret


;----------------;
;  COLOR BITMAP  ;
;----------------;

map_bitmap8:
	mov	dx,ss:[si].width_b
	mul	dx
	add	ax,bx
	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
	ret

map_address	endp

	subttl	Scan Line Update Generation
	page

;----------------------------Private-Routine----------------------------;
; s_update
;
; Generate Y source update code.
;
;
; The Y update code is generated as follows:
;
; For small bitmaps and huge bitmaps where the BLT
; doesn't span a segment bounday, all that need be done is add
; next_scan to the offset portion of the bits pointer.
;
; For huge bitmaps where the BLT spans a segment boundary, the
; above update must be performed, and the overflow/undeflow
; detected.  This isn't too hard to detect.
;
; For any huge bitmap, there can be a maximum of bmWidthBytes-1
; unused bytes in a 64K segment.  The minimum is 0.  The scan line
; update always updates to the first plane of the next (previous) scan.
;
;-----------------------------------------------------------------------
;
; When the BLT is Y+, if the new offset is anywhere within the
; unused bytes of a segment, or in the first scan of a segment,
; then overflow must have occured:
;
;       -bmFillBytes <= offset < bmWidthBytes
;
; IF bmFillBytes is added to both sides of the equation:
;
;	0 <= offset+bmFillBytes < bmWidthBytes+bmFillBytes  (unsigned compare)
;
; will be true if overflow occurs.  The Y+ overflow check will
; look like:
;
;
;     add si,next_scan
;     lea ax,bmFillBytes[si]		;Adjust for fill bytes now
;     cmp ax,bmWidthBytes+bmFillBytes	;Overflow occur?
;     jnc @f				;  No
;     add si,bmFillBytes		;Step over fill bytes
;     mov ax,ds				;Compute new selector
;     add ax,bmSegmentIndex
;     mov ds,ax
;   @@:
;
;-----------------------------------------------------------------------
;
; For Y- BLTs, the test is almost the same.  The equation becomes
;
;    -bmWidthBytes > offset	(unsigned compare)
;
; The Y- update and underflow check will look like:
;
;
;     sub si,next_scan
;     mov ax,si
;     cmp ax,-bmWidthBytes		;Overflow occur?
;     jc  @f				;  No
;     sub si,bmFillBytes		;Step over fill bytes
;     mov ax,ds				;Compute new selector
;     sub ax,bmSegmentIndex
;     mov ds,ax
;   @@:
;
;-----------------------------------------------------------------------
;
; Entry:
;	SS:DI --> where to generate the code
; Returns:
;	SS:DI --> where to generate the code
; Registers Preserved:
;	DX,SI
; Registers Destroyed:
;	AX,DI,flags
; Calls:
;	None
; History:
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

s_update	proc	near

;  If there is NO increment, exit.

	mov	ax,gl_src.width_b
	or	ax,ax
	jnz	@f
	ret

;  Code generated depends on Y+ or Y- BLT

@@:	cmp	gl_direction,INCREASING
	jne	s_dec

;====================;
;  This is a Y+ BLT  ;
;====================;

s_inc:

	test	gl_src.dev_flags,IS_DEVICE
	jnz	s_inc_dev

;-------------;
;  Y+ BITMAP  ;
;-------------;

s_inc_bit:

	mov	ax,I_ADD_SI_WORD_I	;update to next scanline
	stosw
	mov	ax,gl_src.width_b
	stosw				;generate: add si,width_b

;  If the BLT doesn't span a segment just generate increment

	mov	cx,gl_src.seg_index
	jcxz	s_inc_bit2

;  The BLT spans a segment, so generate the segment update

	mov	ax,I_MOV_AX_SI
	stosw				; mov ax,si
	mov	cx, gl_src.fill_bytes	;CX: fill_bytes
	jcxz	s_inc_bit0
	GEN	I_ADD_AX_WORD_I
	mov	ax, cx
	stosw				;generate add ax,fill_bytes

s_inc_bit0:
	GEN	I_CMP_AX_WORD_I
	mov	ax, gl_src.width_b
	add	ax, cx
	stosw				;cmp ax, (fill_bytes+width_bytes)

	mov	al,I_JNC
	xor	ah,ah
	stosw				; jb @f
	mov	bx,di

	jcxz	s_inc_bit1		;if no fill bytes, don't add them to SI
	GEN	I_ADD_SI_WORD_I
	mov	ax, cx
	stosw				;generate: add si, fill_bytes

s_inc_bit1:
	mov	ax,I_MOV_AX_DS
	stosw				; mov ax,ds

	mov	al,I_ADD_AX_WORD_I
	stosb
	mov	ax,gl_src.seg_index
	stosw				; add ax,seg_index

	mov	ax ,I_CMP_CX		;To avoid last segment load
	stosw				;test for end of outer loop
	mov	al ,1
	stosb
	mov	al ,I_JE
	stosb
	mov	al ,2
        stosb

	mov	ax,I_MOV_DS_AX
	stosw				; mov ds,ax

	mov	ax,di
	sub	ax,bx
	mov	es:[bx-1],al		; @@:

s_inc_bit2:
	ret


;-------------;
;  Y+ DEVICE  ;
;-------------;

s_inc_dev:

	mov	ax,I_ADD_SI_WORD_I
	stosw
	mov	ax,gl_src.width_b
	stosw				; add si,width_b

	mov	al,I_SS_OVERRIDE
	stosb
	mov	ax,I_ADC_MEM_BYTE_I
	stosw
	mov	ax,gl_s_fixup
	stosw
	xor	al,al
	stosb

	ret


;======;
;  Y-  ;
;======;

s_dec:

	test	gl_src.dev_flags,IS_DEVICE
	jnz	s_dec_dev

;-------------;
;  Y- BITMAP  ;
;-------------;

s_dec_bit:

;  If the BLT doesn't span a segment just generate decrement

	mov	cx,gl_src.seg_index
	jcxz	s_dec_bit2

;  The BLT spans a segment, so generate the segment code.

	mov	ax,I_MOV_AX_SI
	stosw				; mov ax,si

	GEN	I_CMP_AX_WORD_I
	mov	ax,gl_src.width_b
	stosw				; cmp ax,width_b

	mov	al,I_JNC
	xor	ah,ah
	stosw				; jnb @f
	mov	bx,di

	mov	ax,gl_src.fill_bytes
	or	ax,ax
	jz	s_dec_bit1

	mov	ax,I_SUB_SI_WORD_I
	stosw
	mov	ax,gl_src.fill_bytes
	stosw				; sub si,fill_bytes

s_dec_bit1:
	mov	ax,I_MOV_AX_DS
	stosw				; mov ax,ds

	mov	al,I_SUB_AX_WORD_I
	stosb
	mov	ax,gl_src.seg_index
	stosw				; sub ax,seg_index

	mov	ax	,I_CMP_CX	     ;To avoid last segment load
	stosw				     ;test for end of outer loop
	mov	al	,1
	stosb
	mov	al	,I_JE
	stosb
	mov	al	,2
        stosb

	mov	ax,I_MOV_DS_AX
	stosw				; mov ds,ax

	mov	ax,di
	sub	ax,bx
	mov	es:[bx-1],al		; @@:

s_dec_bit2:
	mov	ax,I_SUB_SI_WORD_I
	stosw
	mov	ax,gl_src.width_b
	stosw				; sub si,width_b

	ret


;-------------;
;  Y- DEVICE  ;
;-------------;

s_dec_dev:

	mov	ax,I_SUB_SI_WORD_I
	stosw
	mov	ax,gl_src.width_b
	stosw				; sub si,width_b

	mov	al,I_SS_OVERRIDE
	stosb
	mov	ax,I_SBB_MEM_BYTE_I
	stosw
	mov	ax,gl_s_fixup
	stosw
	xor	al,al
	stosb

	ret


s_update	endp


;----------------------------Private-Routine----------------------------;
; d_update
;
; Generate Y destination update code.
;
;
; The Y update code is generated as follows:
;
; For small bitmaps and huge bitmaps where the BLT
; doesn't span a segment bounday, all that need be done is add
; next_scan to the offset portion of the bits pointer.
;
; For huge bitmaps where the BLT spans a segment boundary, the
; above update must be performed, and the overflow/undeflow
; detected.  This isn't too hard to detect.
;
; For any huge bitmap, there can be a maximum of bmWidthBytes-1
; unused bytes in a 64K segment.  The minimum is 0.  The scan line
; update always updates to the first plane of the next (previous) scan.
;
;-----------------------------------------------------------------------
;
; When the BLT is Y+, if the new offset is anywhere within the
; unused bytes of a segment, or in the first scan of a segment,
; then overflow must have occured:
;
;       -bmFillBytes <= offset < bmWidthBytes
;
; IF bmFillBytes is added to both sides of the equation:
;
; 	0 <= offset < bmWidthBytes+bmFillBytes	(unsigned compare)
;
; will be true if overflow occurs.  The Y+ overflow check will
; look like:
;
;
;     add di,next_scan
;     lea ax,bmFillBytes[di]		;Adjust for fill bytes now
;     cmp ax,bmWidthBytes+bmFillBytes	;Overflow occur?
;     jnc @f				;  No
;     add di,bmFillBytes		;Step over fill bytes
;     mov ax,es				;Compute new selector
;     add ax,bmSegmentIndex
;     mov es,ax
;   @@:
;
;-----------------------------------------------------------------------
;
; For Y- BLTs, the test is almost the same.  The equation becomes
;
;    -bmWidthBytes > offset	(unsigned compare)
;
; The Y- update and underflow check will look like:
;
;
;     sub di,next_scan
;     mov ax,di
;     cmp ax,-bmWidthBytes		;Overflow occur?
;     jc  @f				;  No
;     sub di,bmFillBytes		;Step over fill bytes
;     mov ax,es				;Compute new selector
;     sub ax,bmSegmentIndex
;     mov es,ax
;   @@:
;
;-----------------------------------------------------------------------
;
; Entry:
;	SS:DI --> where to generate the code
; Returns:
;	SS:DI --> where to generate the code
; Registers Preserved:
;	DX,SI
; Registers Destroyed:
;	AX,DI,flags
; Calls:
;	None
; History:
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

d_update	proc	near

;  If there is NO increment, exit.

	mov	ax,gl_dest.width_b
	or	ax,ax
	jnz	@f
	ret

;  Code generated depends on Y+ or Y- BLT

@@:	cmp	gl_direction,INCREASING
	jne	d_decrement

;======;
;  Y+  ;
;======;

d_increment:

	test	gl_dest.dev_flags,IS_DEVICE
	jnz	d_inc_dev

;-------------;
;  Y+ BITMAP  ;
;-------------;

d_inc_bitmap:

;  If the BLT doesn't span a segment, just generate increment

	mov	cx,gl_dest.seg_index
	jcxz	d_inc_bit2

;  The BLT spans a segment, so generate the segment code.


	mov	ax,I_MOV_AX_DI
	stosw				; mov ax,di

	GEN	I_CMP_AX_WORD_I
	mov	ax,gl_dest.fill_bytes
	add	ax,gl_dest.width_b
	neg	ax
	stosw				; cmp ax,-(fill_bytes+width_b)

	mov	al,I_JC
	xor	ah,ah
	stosw				; jb @f
	mov	bx,di

	mov	ax,gl_dest.fill_bytes
	or	ax,ax
	jz	d_inc_bit1

	mov	ax,I_ADD_DI_WORD_I
	stosw
	mov	ax,gl_dest.fill_bytes
	stosw				; add di,fill_bytes

d_inc_bit1:
	mov	ax,I_MOV_AX_ES
	stosw				; mov ax,es

	mov	al,I_ADD_AX_WORD_I
	stosb
	mov	ax,gl_dest.seg_index
	stosw				; add ax,seg_index

	mov	ax	,I_CMP_CX	     ;To avoid last segment load
	stosw				     ;test for end of outer loop
	mov	al	,1
	stosb
	mov	al	,I_JE
	stosb
	mov	al	,2
        stosb

	mov	ax,I_MOV_ES_AX
	stosw				; mov es,ax

	mov	ax,di
	sub	ax,bx
	mov	es:[bx-1],al		; @@:

d_inc_bit2:
	mov	ax,I_ADD_DI_WORD_I
	stosw
	mov	ax,gl_dest.width_b
	stosw				; add di,width_b

	ret

;-------------;
;  Y+ DEVICE  ;
;-------------;

d_inc_dev:

	mov	ax,I_ADD_DI_WORD_I
	stosw
	mov	ax,gl_dest.width_b
	stosw				; add di,width_b

	mov	al,I_SS_OVERRIDE
	stosb
	mov	ax,I_ADC_MEM_BYTE_I
	stosw
	mov	ax,gl_d_fixup
	stosw
	xor	al,al
	stosb

	ret


;======;
;  Y-  ;
;======;

d_decrement:
	test	gl_dest.dev_flags,IS_DEVICE
	jnz	d_dec_dev

;-------------;
;  Y- BITMAP  ;
;-------------;

d_dec_bit:

;  If the BLT doesn't span a segment just generate decrement

	mov	cx,gl_dest.seg_index
	jcxz	d_dec_bit2

;  The BLT spans a segment, so generate the segment update

	mov	ax,I_MOV_AX_DI
	stosw				; mov ax,di

	GEN	I_CMP_AX_WORD_I
	mov	ax,gl_dest.width_b
	stosw				; cmp ax,width_b

	mov	al,I_JNC
	xor	ah,ah
	stosw				; jnb @f
	mov	bx,di

	mov	ax,gl_dest.fill_bytes
	or	ax,ax
	jz	d_dec_bit1

	mov	ax,I_SUB_DI_WORD_I
	stosw
	mov	ax,gl_dest.fill_bytes
	stosw				; sub di,fill_bytes

d_dec_bit1:
	mov	ax,I_MOV_AX_ES
	stosw				; mov ax,es

	mov	al,I_SUB_AX_WORD_I
	stosb
	mov	ax,gl_dest.seg_index
	stosw				; sub ax,seg_index

	mov	ax	,I_CMP_CX	     ;To avoid last segment load
	stosw				     ;test for end of outer loop
	mov	al	,1
	stosb
	mov	al	,I_JE
	stosb
	mov	al	,2
	stosb

	mov	ax,I_MOV_ES_AX
	stosw				; mov es,ax

	mov	ax,di
	sub	ax,bx
	mov	es:[bx-1],al		; @@:

d_dec_bit2:
	mov	ax,I_SUB_DI_WORD_I
	stosw
	mov	ax,gl_dest.width_b
        stosw                           ; sub di,width_b

	ret

;-------------;
;  Y- DEVICE  ;
;-------------;

d_dec_dev:

	mov	ax,I_SUB_DI_WORD_I
	stosw
	mov	ax,gl_dest.width_b
	stosw				; sub di,width_b

	mov	al,I_SS_OVERRIDE
	stosb
	mov	ax,I_SBB_MEM_BYTE_I
	stosw
	mov	ax,gl_d_fixup
	stosw
	xor	al,al
	stosb

	ret

d_update	endp


sEnd	Code
 
 	end
