;
;   File:   SETRLE.ASM
;   Date:   7/26/89
;   Author: James Keller
;
;   This module holds all the RLE decoding routines.
;
;      The parameters for copyrle_exi1 are:
;
;      AL      -       00 or FF (bit value repeated 8 times)
;      CX      -       number of pixels (bits) in the run
;      ES:DI   -       pointer to memory where run is to be expanded
;
;
;      The parameters for copyrle_exi8 are:
;
;      AL      -       value to "run out" through memory
;      AH      -       same as AL    AH = AL
;      CX      -       number of pixels (bytes) in the run
;      ES:DI   -       pointer to memory where run is to be expanded
;
;
;      All of the remaining routines have the following parameters:
;
;      DS:SI   -       pointer to an RL value (this value will be color mapped)
;      ES:DI   -       pointer to place where a "run" will be expanded
;      CX      -       length of the run
;

include	cmacros.inc
include	macros.mac
include	gdidefs.inc
include rledat.inc
include rlecom.inc

	externA 	__NEXTSEG

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg
	assumes	cs, DIMapSeg

cProc	frame, <FAR,WIN,PASCAL>,<ds,si,di,es>
include	bmc_main.var
cBegin	<nogen>
cEnd	<nogen>





public	scanline_decode_bitmap
scanline_decode_bitmap	proc	near

        push    ax
	push    bx
	push	cx
	push	dx

        les     bx      ,lp_info_ext
	call	rlecom_init			  ;1,4,8, or 24 bits
	jnc	scanline_decode_end_skip	  ;carry set then error
	jmp	scanline_decode_end

scanline_decode_end_skip:

	mov	si	,ax			  ;ax is the index of the
	add	si	,si			  ;specific decode routine
	mov	ax, decode_rle_table[si]
	mov	decode_rle	,ax
	mov	ax	,decode_absolute_table[si]
	mov	decode_absolute	,ax

        lds     si      ,lp_info_ext
	add	si	,wptr es:[bx].biSize
	mov	ax, ss			; get stack segment into es
	mov	es, ax
	lea	di, color_xlate
	rep	movsw			; word indices

        lds     si ,lp_bits_ext
	les	di ,lp_bits_dev
	jmp	scanline_decode_end_of_line

	mov	bx	,top_of_dib
	call	find_visible_top



public	scanline_decode_next
scanline_decode_next:

	call	find_visible_left
	add	di	,bx
	jnc	sc
	mov	ax	,es
	add	ax	,__NEXTSEG
	mov	es	,ax

sc:
        call    getword

	cmp	al	,0
	jne	scanline_decode_rle

	cmp	ah	,0
        je      scanline_decode_end_of_line

	cmp	ah	,1
	je	scanline_decode_end

	cmp	ah	,2
	je	scanline_decode_skip

	jmp	short	scanline_decode_absolute


public	scanline_decode_rle
scanline_decode_rle:

        mov     cl      ,al
	mov	ch	,0
	mov	al	,ah
	mov	bx	,right_of_
        cmp     cx      ,


        call    decode_rle
	jmp	scanline_decode_next

public	scanline_decode_absolute
scanline_decode_absolute:

	mov	cl	,al
	mov	ch	,0

        call    decode_absolute
	inc	si
	jne	scanline_decode_abs_1
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si

scanline_decode_abs_1:
	and	si	,0FFFEH
        jmp     scanline_decode_next

public scanline_decode_end_of_line
scanline_decode_end_of_line:

	mov	bx	,di
	add	bx	,bitmap_width
	jnc	scanline_decode_end_of_line_0
	mov	bx	,es
	add	bx	,__NEXTSEG
	mov	es	,bx
	sub	di	,di

scanline_decode_end_of_line_0:
        jmp     scanline_decode_next

public	scanline_decode_skip
scanline_decode_skip:

	jmp	scanline_decode_next

public	scanline_decode_end
scanline_decode_end:

	pop	dx
	pop	cx
	pop	bx
	pop	ax
        ret

scanline_decode_bitmap	endp




public	find_visible_top
find_visible_top	proc	near

find_visible_top_loop:
        call    getword
	cmp	al	,0
	jne	find_visible_top_loop

find_visible_top_endline:
	cmp	ah	,0
	jne	find_visible_top_endframe
	dec	bx
	jne	find_visible_top_loop
	ret

find_visible_top_endframe:
	cmp	ah	,1
	jne	find_visible_top_skip
	stc
	ret

find_visible_top_skip:
	jmp	find_visible_top

find_visible_top_abs:
	mov	al	,ah
	sub	ah	,ah
	mov	cx	,rle_size
	inc	ax
        shr     ax      ,1

find_visible_top_abs0:
	mov	cx	,si
	and	cx	,1
	add	ax	,cx
        add     si      ,ax
	jnc	find_visible_top_abs1
	mov	ax	,ds
	add	ax	,__NEXTSEG
	mov	ds	,ax

find_visible_top_abs1:
	jmp	find_visible_top

find_visible_top	endp




public	find_visible_left
find_visible_left	 proc	 near

	sub	bx	,bx

find_visible_left_loop:
        call    getword
	cmp	al	,0
	je	find_visible_left_endline

find_visible_left_rle:
	sub	ah	,ah
	add	ax	,bx
	cmp	ax	,left_of_dib
	jbe	find_visible_left_rle0
	add	bx	,ax

find_visible_left_rle0:
	MODIFY DS:SI
        ret

find_visible_left_endline:
	cmp	ah	,0
	jne	find_visible_left_endframe
	ret

find_visible_left_endframe:
	cmp	ah	,1
	jne	find_visible_left_skip
	ret

find_visible_left_skip:
	jmp	find_visible_left_loop

find_visible_left_abs:
	mov	al	,ah
	sub	ah	,ah
        mov     cx      ,rle_size
	inc	ax
        shr     ax      ,1
	add	ax	,bx
	cmp	ax	,left_of_dib
	jbe	find_visible_left_abs1

find_visible_left_abs0:
	add	bx	,ax
        mov     cx      ,si
	and	cx	,1
	add	ax	,cx
        add     si      ,ax
	jnc	find_visible_left_loop
	mov	ax	,ds
	add	ax	,__NEXTSEG
	mov	ds	,ax
	jmp	find_visible_left

find_visible_left_abs1:
        MODIFY DS:SI
        ret

find_visible_left	endp













public	copyrle_exi1
copyrle_exi1	proc	near

	cmp	ch	,8		;is this the start of a dest byte?
	je	copyrle_exi1_bytealign	;if so, process the complete bytes
	cmp	cl	,ch		;if the run is not long enough to
	jnc	copyrle_exi1_enough	;  fill a complete dest byte

	mov	dl	,al		;  then stick the short run
	shl	dx	,cl		;  into the byte,
	sub	ch	,cl		;  update the bit count,
	ret				;  and return

copyrle_exi1_enough:
	mov	dl	,al		;stick the run in the byte
	xchg	cl	,ch
	shl	dx	,cl		;byte align the bits
	mov	es:[di] ,dh		;store the byte
	inc	di

        jne     copyrle_exi1_0
	mov	di	,es
	add	di	,__NEXTSEG
	mov	es	,di
	sub	di	,di
copyrle_exi1_0:

	xchg	cl	,ch
	sub	cl	,ch		;num bits in run - num just inserted
	je	copyrle_exi1_done	;if that's all then done

copyrle_exi1_bytealign:
	cmp	cl	,8		;is there at least 1 destination byte?
	jc	copyrle_exi1_part	;if not, process a partial byte

	mov	dl	,cl		;save remaining run count
	shiftr	cl	,3
	xor	ch	,ch
	rep	stosb

	mov	ch	,8
	mov	dh	,0
	mov	cl	,dl
	and	cl	,07H
	je	copyrle_exi1_done

copyrle_exi1_part:
	sub	ch	,cl
	mov	dl	,al
	shl	dx	,cl

copyrle_exi1_done:
	ret

copyrle_exi1	endp



public	copyrle_e4i1
copyrle_e4i1	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table
       shiftl  al      ,1	       ;form index into
       or      al      ,1	       ;    color table
       xlat    ss:[bx]		       ;
       shr     al      ,1	       ;get mono bit
       sbb     al      ,al	       ;spread it out to a byte
       call    copyrle_exi1	       ;run the value throughtout the length
       pop     bx
       ret

copyrle_e4i1	endp



public	copyrle_e8i1
copyrle_e8i1	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table
       mov     ah      ,0              ;prepare for color mapping
       shiftl  ax      ,1	       ;form index into
       or      ax      ,1	       ;    color table
       add     bx      ,ax	       ;
       mov     al      ,ss:[bx]        ;al now holds the remapped color
       shr     al      ,1	       ;get mono bit
       sbb     al      ,al	       ;spread it out to a byte
       call    copyrle_exi1	       ;run the value throughtout the length
       pop     bx
       ret

copyrle_e8i1	endp




public	copyrle_exi8
copyrle_exi8   proc    near

	push	ax
	push	bx

	test	di	,1		;if di is on an odd boundary
	je	copyrle_exi8_3
	stosb				;store the odd byte
	dec	cx			;decrement the count
	xchg	al	,ah		;swap low and high for word storage

copyrle_exi8_3:
	shr	cx	,1
	rep	stosw			;store all word aligned values
	rcl	cx	,1
	rep	stosb

copyrle_exi8_5:
        pop     bx
	pop	ax
        ret

copyrle_exi8   endp


public  copyrle_e4i8
copyrle_e4i8	proc	near

	push	bx
	lea	bx	,color_xlate	;get address of color mapping table
	mov	ah	,al
	shiftr	al	,3		;form index into
	and	al	,1EH
	xlat	ss:[bx] 		;get the remapped color value
	xchg	ah	,al
	shl	al	,1
	and	al	,1EH
        xlat    ss:[bx]
	call	copyrle_exi8
	pop	bx
	ret

copyrle_e4i8	endp

public	copyrle_e8i8
copyrle_e8i8	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table
       xor     ah      ,ah
       shiftl  ax      ,1	       ;form index into
       add     bx      ,ax	       ;   a word table
       mov     al      ,ss:[bx]        ;get the remapped color value
       mov     ah      ,al	       ;two identical bytes for quick storage
       call    copyrle_exi8
       pop     bx
       ret

copyrle_e8i8	endp



public	copyabs_e4i1
copyabs_e4i1	proc	near

	push	bx
        lea     bx      ,color_xlate

copyabs_e4i1_newbyte:
        mov     ch      ,8
	mov	dl	,0

copyabs_e4i1_loop:
        lodsb
	or	si	,si
	jne	copyabs_e4i1_0
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si

copyabs_e4i1_0:
        mov     ah      ,al
	shiftr	al	,3
	or	al	,1
	xlat	ss:[bx]
	shr	al	,1
	rcl	dl	,1
	dec	cl
	je	copyabs_e4i1_done

        mov     al      ,ah
	shiftr	al	,3
	or	al	,1
	xlat	ss:[bx]
	shr	al	,1
	rcl	dl	,1
	dec	cl
	je	copyabs_e4i1_done
        dec     ch
	jne	copyabs_e4i1_loop

        mov     es:[di] ,dl
	inc	di
	jmp	copyabs_e4i1_newbyte

copyabs_e4i1_done:
	mov	es:[di] ,dl
	inc	di
        pop     bx
	ret

copyabs_e4i1	endp





public	copyabs_e8i1
copyabs_e8i1	proc	near

	push	bx
	push	bp
        lea     bx      ,color_xlate

copyabs_e8i1_newbyte:
        mov     ch      ,8
	mov	dl	,0

copyabs_e8i1_loop:
	mov	bp	,bx
        lodsb
	or	si	,si
	jne	copyabs_e8i1_0
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si

copyabs_e8i1_0:
        shl     al      ,1
	or	al	,1
	add	bp	,ax
	mov	al	,ss:[bp]
	shr	al	,1
	rcl	dl	,1
	dec	cl
	je	copyabs_e8i1_done
	dec	ch
	jne	copyabs_e8i1_loop

        mov     es:[di] ,dl
	inc	di
	jmp	copyabs_e8i1_newbyte

copyabs_e8i1_done:
	mov	es:[di] ,dl
	inc	di
	pop	bp
	pop	bx
	ret

copyabs_e8i1	endp





public	copyabs_e4i8
copyabs_e4i8	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table

copyabs_e4i8_loop:
	lodsb
	or	si	,si
	jne	copyabs_e4i8_0
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si

copyabs_e4i8_0:
       mov     ah      ,al
       shiftr  al      ,3
       and     al      ,0FEH
       xlat    ss:[bx]
       stosb
       dec     cx
       je      copyabs_e4i8_done

       mov     al      ,ah
       and     al      ,0FH
       shl     al      ,1
       xlat    ss:[bx]
       stosb
       loop    copyabs_e4i8_loop

copyabs_e4i8_done:
       pop     bx
       ret

copyabs_e4i8	endp





public	copyabs_e8i8
copyabs_e8i8	proc	near

       push    bx
       push    bp
       lea     bx      ,color_xlate    ;get address of color mapping table

copyabs_e8i8_loop:
	mov	bp	,bx
	lodsb
	or	si	,si
	jne	copyabs_e8i8_0
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si

copyabs_e8i8_0:
       xor     ah      ,ah
       shiftl  ax      ,1	       ;form index into
       add     bp      ,ax	       ;   a word table
       mov     al      ,ss:[bp]        ;get the remapped color value
       stosb
       loop    copyabs_e8i8_loop

       pop     bp
       pop     bx
       ret

copyabs_e8i8	endp


sEnd	DIMapSeg

END

