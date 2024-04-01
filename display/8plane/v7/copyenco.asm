;
;
;	File:	COPYENCO.ASM
;	Author: James Keller
;	Date:	8/15/89
;
;   The routines in this module all have the same parameters:
;   DS:SI  -  ptr to a string to encode in RLE format
;   ES:DI  -  ptr to a place to store the encoded string
;   DX	   -  number of source bytes to put into RLE format
;


include cmacros.inc
include macros.mac
include gdidefs.inc
include getrle.inc
include getabs.inc
include rledat.inc
include rleext.inc


createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg




public	copyenco_i1e1, copyenco_i1e4, copyenco_i1e8, copyenco_i1e24
copyenco_i1e1	proc	near
copyenco_i1e4:
copyenco_i1e8:
copyenco_i1e24:

        mov     cx      ,max_rle_length_in_bytes
	cmp	cx	,dx
	jl	copyenco_i1e1_min
		mov	cx	,dx

copyenco_i1e1_min:

        lodsb
	mov	ah	,al
	inc	ah
	and	ah	,0FEH
	jne	copyenco_i1e1_absolute
		mov	BYTE PTR es:[di] ,RLE_TYPE_ENCODED
		inc	di
		call	encode_rle
		jmp	copyenco_i1e1_common

copyenco_i1e1_absolute:

	mov	BYTE PTR es:[di] ,RLE_TYPE_ABSOLUTE
	inc	di
	call	encode_absolute

copyenco_i1e1_common:

        sub     dx      ,cx
	jne	copyenco_i1e1
	ret

copyenco_i1e1	endp




public	copyenco_i8e1
copyenco_i8e1	proc	near

        mov     cx      ,max_rle_length_in_bytes
	cmp	cx	,dx
	jl	copyenco_i8e1_min
		mov	cx	,dx

copyenco_i8e1_min:

        lodsb
	cmp	al	,0FFH
	jne	coff
	cmp	BYTE PTR ds:[si] ,0FFH
	jne	copyenco_i8e1_absolute

con:		mov	BYTE PTR es:[di] ,RLE_TYPE_ENCODED
		inc	di
		call	copyrle_i8e1
		jmp	copyenco_i8e1_common

coff:	cmp	BYTE PTR ds:[si] ,0FFH
	jne	con

copyenco_i8e1_absolute:

	mov	BYTE PTR es:[di] ,RLE_TYPE_ABSOLUTE
	inc	di
	call	copyabs_i8e1

copyenco_i8e1_common:

        sub     dx      ,cx
	jne	copyenco_i8e1
	ret

copyenco_i8e1	endp




public	copyenco_i8e4
copyenco_i8e4	proc	near

        mov     cx      ,max_rle_length_in_bytes
	cmp	cx	,dx
	jl	copyenco_i8e4_min
		mov	cx	,dx

copyenco_i8e4_min:

        lodsb
	and	al	,0FH
	mov	ah	,ds:[si]
	and	ah	,0FH
	cmp	al	,ah
	jne	copyenco_i8e4_absolute
		mov	BYTE PTR es:[di] ,RLE_TYPE_ENCODED
		inc	di
		call	copyrle_i8e4
		jmp	copyenco_i8e4_common

copyenco_i8e4_absolute:

	mov	BYTE PTR es:[di] ,RLE_TYPE_ABSOLUTE
	inc	di
	call	copyabs_i8e4

copyenco_i8e4_common:

        sub     dx      ,cx
	jne	copyenco_i8e4
	ret

copyenco_i8e4	endp




public	copyenco_i8e8, copyenco_i8e24
copyenco_i8e8  proc    near
copyenco_i8e24:

        mov     cx      ,max_rle_length_in_bytes
	cmp	cx	,dx
	jl	copyenco_i8e8_min
		mov	cx	,dx

copyenco_i8e8_min:

        lodsb
	cmp	al	,ah
	jne	copyenco_i8e8_absolute
		mov	BYTE PTR es:[di] ,RLE_TYPE_ENCODED
		inc	di
		call	encode_rle
		jmp	copyenco_i8e8_common

copyenco_i8e8_absolute:

	mov	BYTE PTR es:[di] ,RLE_TYPE_ABSOLUTE
	inc	di
	call	encode_absolute

copyenco_i8e8_common:

        sub     dx      ,cx
	jne	copyenco_i8e8
	ret

copyenco_i8e8	endp


sEnd	DIMapSeg

END

