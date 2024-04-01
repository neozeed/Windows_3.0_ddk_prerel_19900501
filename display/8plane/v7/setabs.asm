;
;   File:   SETABS.ASM
;   Date:   7/26/89
;   Author: James Keller
;
;   This module holds all the RLE decoding routines.
;
;      All of the copyabs_eXiY routines have the following parameters:
;
;      DS:SI   -       pointer to an RL value (this value will be color mapped)
;      ES:DI   -       pointer to place where a "run" will be expanded
;      CX      -       number of bytes in which to store the run value
;


include	cmacros.inc
include	macros.mac
include	gdidefs.inc
include rledat.inc
include rleext.inc


createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg
	assumes	cs, DIMapSeg

cProc	frame, <FAR,WIN,PASCAL>,<ds,si,di,es>
include	bmc_main.var
cBegin	<nogen>
cEnd	<nogen>



public	copyabs_e1i1
copyabs_e1i1	proc	near

	cmp	ch	,8		;is this the start of a dest byte?
	je	copyabs_e1i1_bytealign	;if so, process the complete bytes

        lodsb
	cmp	cl	,ch		;if the run is not long enough to
	jnc	copyabs_e1i1_enough	;  fill a complete dest byte

	mov	dl	,al		;  then stick the short run
	shl	dx	,cl		;  into the byte,
	sub	ch	,cl		;  update the bit count,
	ret				;  and return

copyabs_e1i1_enough:
	mov	dl	,al		;stick the run in the byte
	xchg	cl	,ch
	shl	dx	,cl		;byte align the bits
	mov	es:[di] ,dh		;store the byte
	inc	di
	xchg	cl	,ch
	sub	cl	,ch		;num bits in run - num just inserted
	je	copyabs_e1i1_done	;if that's all then done

copyabs_e1i1_bytealign:
	cmp	cl	,8		;is there at least 1 destination byte?
	jc	copyabs_e1i1_part	;if not, process a partial byte

	mov	dl	,cl		;save remaining run count
	shiftr	cl	,3
	xor	ch	,ch
	rep	movsb

	mov	ch	,8
	mov	dh	,0
	mov	cl	,dl
	and	cl	,07H
	je	copyabs_e1i1_done

copyabs_e1i1_part:
	lodsb
	sub	ch	,cl
	mov	dl	,al
	shl	dx	,cl

copyabs_e1i1_done:
	ret

copyabs_e1i1	endp





public	copyabs_e4i1
copyabs_e4i1	proc	near

	lea	bx	,color_xlate

copyabs_e4i1_loop:
        lodsb
	mov	ah	,al
	shiftr	al	,3
	or	al	,1
	xlat	ss:[bx]
	shr	al	,1
	rcl	dl	,1
	dec	ch
	jne	copyabs_e4i1_second

        mov     es:[di] ,dl
	inc	di
	mov	ch	,8
	mov	dl	,0

copyabs_e4i1_second:
	dec	cl
	je	copyabs_e4i1_done

	mov	ah	,al
	shiftr	al	,3
	or	al	,1
	xlat	ss:[bx]
	shr	al	,1
	rcl	dl	,1
	dec	ch
	jne	copyabs_e4i1_third

        mov     es:[di] ,dl
	inc	di
	mov	ch	,8
	mov	dl	,0

copyabs_e4i1_third:
	dec	cl
	jne	copyabs_e4i1_loop

copyabs_e4i1_done:
	pop	bx
	ret

copyabs_e4i1	endp





public	copyabs_e8i1
copyabs_e8i1	proc	near

	push	bx
	push	bp
        lea     bx      ,color_xlate

copyabs_e8i1_loop:
	mov	bp	,bx
        lodsb
	shl	al	,1
	or	al	,1
	add	bp	,ax
	mov	al	,ss:[bp]
	shr	al	,1
	rcl	dl	,1
	dec	ch
	jne	copyabs_e8i1_second

        mov     es:[di] ,dl
	inc	di
	mov	ch	,8
	mov	dl	,0

copyabs_e8i1_second:
	dec	cl
	jne	copyabs_e8i1_loop

	pop	bp
	pop	bx
	ret

copyabs_e8i1	endp






public	copyabs_e24i1
copyabs_e24i1	 proc	 near

       push    dx

copyabs_e24i1_loop:
       lodsw			       ;get the run length value (low 2 bytes)
       mov     dl      ,[si]	       ;get high byte of run length value
       inc     si		       ;update RL record pointer
       xor     dh      ,dh	       ;another parameter for rgb_to_ipc???
       call    rgb_t_ipc	       ;maps 24 bit color to 1 bit color
       shr     ah      ,1	       ;get mono bit
       rcl     dl      ,1
       dec     ch
       jne     copyabs_e24i1_second

       mov     es:[di] ,dl
       inc     di
       mov     ch      ,8
       mov     dl      ,0

copyabs_e24i1_second:
       dec     cl
       jne     copyabs_e24i1_loop
       pop     dx
       ret

copyabs_e24i1	 endp





public	copyabs_e1i8
copyabs_e1i8	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table
       xor     al      ,al
       xlat    ss:[bx]
       mov     ah      ,al
       mov     al      ,2
       xlat    ss:[bx]
       mov     bx      ,ax
       xor     bl      ,bh

copyabs_e1i8_loop:
       lodsb
       mov     ah      ,al
       shl     ah      ,1
       sbb     al      ,al
       and     al      ,bl
       xor     al      ,bh
       stosb
       loop    copyabs_e1i8_loop

       pop     bx
       ret

copyabs_e1i8	endp





public	copyabs_e4i8
copyabs_e4i8	proc	near

       push    bx
       lea     bx      ,color_xlate    ;get address of color mapping table

copyabs_e4i8_loop:
       lodsb
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
       mov     bp      ,bx
       lodsb
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





public	copyabs_e24i8
copyabs_e24i8	 proc	 near

       push    dx

copyabs_e24i8_loop:
       lodsw                           ;get the run length value (low 2 bytes)
       mov     dl      ,[si]	       ;get high byte of the run length value
       inc     si		       ;update abs pointer
       call    rgb_t_ipc
       stosb
       loop    copyabs_e24i8_loop

       pop     dx
       ret

copyabs_e24i8	 endp



;----------------------------------------------------------------------------;
;   rgb_t_ipc								     ;
;	       converts	a RGB triplet into a a color byte for EGA adapters   ;
;									     ;
;  ENTRY:								     ;
;	 DL : (R)ed value (0,255)					     ;
;	 AH : (G)reen (0,255)						     ;
;	 AL : (B)lue  (0,255)						     ;
;									     ;
;  RETURN:								     ;
;  CALLS:								     ;
;	 sum_RGB_alt_far  defined in ROBJECT.ASM			     ;
;	 this routine actually returns the result in DH, with AL,AH,DL having;
;	 the physical color triplet					     ;
;									     ;
;	 we will ignore	AL,AH,DL and return the	result in AL.		     ;
;----------------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

rgb_t_ipc	proc	near

	xchg	al, dl		; get red into al and blue into	dl
	push	bx
	push	cx
	push	dx		; save the registers which are to be affected
	call	sum_RGB_alt_far	; do the conversion
	pop	dx		; new return, result in	ax
	pop	cx
	pop	bx		; restore the values
	ret

rgb_t_ipc	endp


sEnd    DIMapSeg

END
