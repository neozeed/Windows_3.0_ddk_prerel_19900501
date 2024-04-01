;
; Raster operation macros for 256 color maps.
;
	page	64,131
	.list
	.model	small
	.data
	.code
;
;-------------------------=============================------------------------
;-------------------------====< Raster Operations >====------------------------
;-------------------------=============================------------------------
;
; O P A Q U E _ L O O P _ C O N T R O L
;
; Assumed Entry Conditions:
;	   dh holds the background color
;	   dl holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   ch holds the character width
;	   bp holds the character length
;
; Macro parameters:
;	rop_name	--  Name of operation, i.e., dpon,dpna,ddx,etc...
;	rop_macro	--  Macro name of raster operation
;	optimal_macro	--  adds some optimization to special cases
;
opaque_loop_control macro rop_name,rop_macro,optimal_macro
	local	xx1,xx2,xx3,xx4,xx5,xx6
xx1:	add	di,scan_increment	;; skip to next scan line
opaque_rop_&rop_name label near
	lodsb				;; fetch the pattern byte
	mov	bl,al			;; place byte in bl
	mov	cl,ch			;; load width counter (inner loop)
	optimal_macro	xx1		;; checks for optimal case, or skip
;;------ZERO CASE-----------------------
xx2:	shl	bl,1			;; push out next pel to carry
	jc	xx5			;; go to ONE CASE if set
xx3:	mov	al,dl			;; save the new background
	stosb				;;  color
	dec	cl			;; dec inner loop control
	jnz	xx2			;; go for more...
	dec	bp			;; dec outer loop control
	jnz	xx1			;; all done with this scan line
;;------ONES CASE-----------------------
xx4:	shl	bl,1			;; push out next pel to carry
	jnc	xx3			;; go to ZERO CASE if not set
xx5:	rop_macro			;; determines background color
	stosb				;; save the new color
	dec	cl			;; dec inner loop counter
	jnz	xx4			;; go for more...
	dec	bp			;; dec outer loop counter
	jnz	xx1			;; go for more...
;;------EXIT HOME...--------------------
	ret
	endm
;
;
; T R A N S P _ L O O P _ C O N T R O L
;
; Assumed Entry Conditions:
;	   dh holds the background color
;	   dl holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   ch holds the character width
;	   bp holds the character length
;
; Macro parameters:
;	rop_name	--  Name of operation, i.e., dpon,dpna,ddx,etc...
;	rop_macro	--  Macro name of raster operation
;	optimal_macro	--  adds some optimization to special cases
;
transp_loop_control macro rop_name,rop_macro,optimal_macro
	local	xx1,xx3,xx4,xx5,xx6,xx7
xx1:	add	di,scan_increment
transp_rop_&rop_name label near		;; entrypoint starts here...
	lodsb				;; fetch the pattern byte
	xchg	al,bl			;; place byte in bl
	mov	cl,ch			;; load width counter (inner loop)
	optimal_macro	xx1		;; checks for special cases
;;--------------ZERO CASE---------------;;
xx3:	shl	bl,1			;; carry clear = old color
	jc	xx6			;; transition to ONE CASE
xx4:	inc	di			;; skip this move 
	dec	cl			;; decrement inner loop count
	jnz	xx3			;; not done, continue inner loop
	dec	bp			;; dec outer loop
	jnz	xx1			;; done with outer, skip to next raster
	jmp	xx7			;; all done, exit out
;;---------------ONE CASE---------------;;
xx5:	shl	bl,1			;; carry set = new color
	jnc	xx4			;; transition to ZERO CASE
xx6:					;;
	rop_macro			;; expandable background raster op.
	stosb				;; save the new color
	dec	cl			;; decrement inner loop count
	jnz	xx5			;; not done, continue inner loop
	dec	bp			;; dec outer loop
	jnz	xx1			;; done with outer, skip to next raster
xx7:
	ret				;; exit home...
	endm
;
;----------------------====================================--------------------
;----------------------====< Background Determination >====--------------------
;----------------------====================================--------------------
;
; rop #0
ddx_rop		macro		;; dest = dest not dest
	xor	al,al
	endm
;
; rop #1
dpon_rop	macro		;; dest = not (dest or pattern)
	mov	al,dh
	or	al,es:[di]
	not	al
	endm
;
; rop #2
dpna_rop	macro		;; dest = dest and (not pattern)
	mov	al,dh
	not	al
	and	al,es:[di]
	endm
;
; rop #3
pn_rop		macro		;; dest = not pattern
	mov	al,dh
	not	al
	endm
;
; rop #4
pdna_rop	macro		;; dest = pattern and (not dest)
	mov	al,es:[di]
	not	al
	and	al,dh
	endm
;
; rop #5
dn_rop		macro		;; dest = not dest
	mov	al,es:[di]
	not	al
	endm
;
; rop #6
dpx_rop		macro		;; dest = dest xor pattern
	mov	al,dh
	xor	al,es:[di]
	endm
;
; rop #7
dpan_rop	macro		;; dest = not (dest and pattern)
	mov	al,dh
	and	al,es:[di]
	not	al
	endm
;
; rop #8
dpa_rop		macro		;; dest = dest and pattern
	mov	al,dh
	and	al,es:[di]
	endm
;
; rop #9
dpxn_rop	macro		;; dest = not (dest xor pattern)
	mov	al,dh
	xor	al,es:[di]
	not	al
	endm
;
; rop #10
d_rop		macro		;; dest = dest
	mov	al,es:[di]
	endm
;
; rop #11
dpno_rop	macro		;; dest = dest or (not pattern)
	mov	al,dh
	not	al
	or	al,es:[di]
	endm
;
; rop #12
p_rop		macro		;; dest = pattern
	mov	al,dh
	endm
;
; rop #13
pdno_rop	macro		;; dest = pattern or (not dest)
	mov	al,es:[di]
	not	al
	or	al,dh
	endm
;
; rop #14
dpo_rop		macro		;; dest = dest or pattern
	mov	al,es:[di]
	or	al,dh
	endm
;
; rop #15
ddxn_rop	macro		;; dest = not (dest xor dest)
	mov	al,0ffh
	endm
;
;------------------------===============================-----------------------
;------------------------====< Optimazation macros >====-----------------------
;------------------------===============================-----------------------
;
;  o p a q u e _ n u l l s  --  Check for null source color, then do a fast
;				write of new background colors.
opaque_nulls	macro	xx1
	local	xx2
	or	bl,bl			;; check to see if source is null
	jnz	xx2			;; not all null, go process each bit
	xchg	bl,ch			;; ch=0, bl=loop count
	mov	al,dl			;; load the new background color
	rep	stosb			;; splat!!!
	mov	ch,bl			;; restore the loop count
	jmp	short xx1		;; go for more...
xx2:
	endm
;
;  t r a n s p _ n u l l s  --  Check for null source color, then skip
;				to the next scan line.
transp_nulls	macro	xx1
	local	xx2
	or	bl,bl			;; check to see if source is null
	jnz	xx2			;; not all null, go process each bit
	xchg	bl,ch			;; ch=0, bl=loop count
	add	di,cx			;; skip over this scan line
	add	si,cx			;; skip over this font
	mov	ch,bl
	jmp	short xx1		;; go for more...
xx2:
	endm
;
;
;---------------------======================================-------------------
;---------------------====< Raster Operations Routines >====-------------------
;---------------------======================================-------------------
;
opaque_loop_control ddx,  ddx_rop,  opaque_nulls	; 00
opaque_loop_control dpon, dpon_rop, opaque_nulls	; 01
opaque_loop_control dpna, dpna_rop, opaque_nulls	; 02
opaque_loop_control pn,   pn_rop,   opaque_nulls	; 03
opaque_loop_control pdna, pdna_rop, opaque_nulls	; 04
opaque_loop_control dn,   dn_rop,   opaque_nulls	; 05
opaque_loop_control dpx,  dpx_rop,  opaque_nulls	; 06
opaque_loop_control dpan, dpan_rop, opaque_nulls	; 07
opaque_loop_control dpa,  dpa_rop,  opaque_nulls	; 08
opaque_loop_control dpxn, dpxn_rop, opaque_nulls	; 09
opaque_loop_control d,    d_rop,    opaque_nulls	; 0a
opaque_loop_control dpno, dpno_rop, opaque_nulls	; 0b
opaque_loop_control p,    p_rop,    opaque_nulls	; 0c
opaque_loop_control pdno, pdno_rop, opaque_nulls	; 0d
opaque_loop_control dpo,  dpo_rop,  opaque_nulls	; 0e
opaque_loop_control ddxn, ddxn_rop, opaque_nulls	; 0f
;
transp_loop_control ddx,  ddx_rop,  transp_nulls	; 00
transp_loop_control dpon, dpon_rop, transp_nulls	; 01
transp_loop_control dpna, dpna_rop, transp_nulls	; 02
transp_loop_control pn,   pn_rop,   transp_nulls	; 03
transp_loop_control pdna, pdna_rop, transp_nulls	; 04
transp_loop_control dn,   dn_rop,   transp_nulls	; 05
transp_loop_control dpx,  dpx_rop,  transp_nulls	; 06
transp_loop_control dpan, dpan_rop, transp_nulls	; 07
transp_loop_control dpa,  dpa_rop,  transp_nulls	; 08
transp_loop_control dpxn, dpxn_rop, transp_nulls	; 09
transp_loop_control d,    d_rop,    transp_nulls	; 0a
transp_loop_control dpno, dpno_rop, transp_nulls	; 0b
transp_loop_control p,    p_rop,    transp_nulls	; 0c
transp_loop_control pdno, pdno_rop, transp_nulls	; 0d
transp_loop_control dpo,  dpo_rop,  transp_nulls	; 0e
transp_loop_control ddxn, ddxn_rop, transp_nulls	; 0f
;
;
; Vector tables to the proper raster operations
;
opaque_rop_table	label	word
	dw	opaque_rop_ddx
	dw	opaque_rop_ddx
	dw	opaque_rop_dpon
	dw	opaque_rop_dpna
	dw	opaque_rop_pn
	dw	opaque_rop_pdna
	dw	opaque_rop_dn
	dw	opaque_rop_dpx
	dw	opaque_rop_dpan
	dw	opaque_rop_dpa
	dw	opaque_rop_dpxn
	dw	opaque_rop_d
	dw	opaque_rop_dpno
	dw	opaque_rop_p
	dw	opaque_rop_pdno
	dw	opaque_rop_dpo
	dw	opaque_rop_ddxn
;
if2
	.errnz	$-transp_rop_table
endif
;
transp_rop_table	label	word
	dw	transp_rop_ddx
	dw	transp_rop_ddx
	dw	transp_rop_dpon
	dw	transp_rop_dpna
	dw	transp_rop_pn
	dw	transp_rop_pdna
	dw	transp_rop_dn
	dw	transp_rop_dpx
	dw	transp_rop_dpan
	dw	transp_rop_dpa
	dw	transp_rop_dpxn
	dw	transp_rop_d
	dw	transp_rop_dpno
	dw	transp_rop_p
	dw	transp_rop_pdno
	dw	transp_rop_dpo
	dw	transp_rop_ddxn
;
;--------------------------=======================-----------------------------
;--------------------------====< end of rops >====-----------------------------
;--------------------------=======================-----------------------------
;
comment	|	;;; saving these for a rainy day...
;
; O P A Q U E _ L O O P _ C O N T R O L
;
; Assumed Entry Conditions:
;	   dh holds the background color
;	   dl holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   ch holds the character width
;	   bp holds the character length
;
;
opaque_loop_control macro rop_name,rop_macro
	local	outer_loop, inner_loop
opaque_rop_&rop_name label near
outer_loop:
	lodsb				;; fetch the pattern byte
	xchg	al,bl			;; place byte in bl
	mov	cl,ch			;; load width counter (inner loop)
inner_loop:
	rop_macro			;; determines background color
	shl	bl,1			;; set/clear carry
	sbb	ah,ah			;; ah = ff if new foreground
	xor	al,dl			;; either clear color, or leave it
	and	al,ah			;; either clear bg, or get it
	xor	al,dl			;; get final color
	stosb				;; save it
	dec	cl			;; decrement inner loop counter
	jnz	inner_loop
	add	di,scan_increment	;; point to next destination scan
	add	si,font_increment	;; font height-bp
	dec	bp
	jnz	outer_loop
	ret
	endm
;
;
; T R A N S P _ L O O P _ C O N T R O L
;
; Assumed Entry Conditions:
;	   dh holds the background color
;	   dl holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   ch holds the character width
;	   bp holds the character length
;
transp_loop_control macro rop_name,rop_macro
	local	xx1,xx2,xx3,xx4,xx5,xx6,xx7
xx1:	add	di,scan_increment
transp_rop_&rop_name label near		;; entrypoint starts here...
	lodsb				;; fetch the pattern byte
	xchg	al,bl			;; place byte in bl
	mov	cl,ch			;; load width counter (inner loop)
;;--------------ZERO CASE---------------;;
xx3:	shl	bl,1			;; carry clear = old color
	jc	xx6			;; transition to ONE CASE
xx4:	inc	di			;; skip this move 
	dec	cl			;; decrement inner loop count
	jnz	xx3			;; not done, continue inner loop
	dec	bp			;; dec outer loop
	jnz	xx1			;; done with outer, skip to next raster
	jmp	xx7			;; all done, exit out
;;---------------ONE CASE---------------;;
xx5:	shl	bl,1			;; carry set = new color
	jnc	xx4			;; transition to ZERO CASE
xx6:	rop_macro			;; expandable background raster op.
	stosb				;; save the new color
	dec	cl			;; decrement inner loop count
	jnz	xx5			;; not done, continue inner loop
	dec	bp			;; dec outer loop
	jnz	xx1			;; done with outer, skip to next raster
xx7:	ret				;; exit home...
	endm
;
	|		;;;; end of comment
;
	end
;