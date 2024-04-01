;
;	File:	SCANDECO.ASM
;	Date:	7/24/89
;	Author:	James Keller
;
;      The parameters for scanline_decode are:
;      DS:SI   -       pointer to run - length encoded data stream
;      ES:DI   -       pointer to location to store decoded data
;      nextscan_line   length of scan line in bytes
;      internal_is_1bit    this is 3 if internal storage is 1 bit else it is 0
;
;      This routine handles segment overruns in both the source (DS:SI) and
;      the destination (ES:DI) so no need to worry.
;

include	cmacros.inc
include	macros.mac
include	gdidefs.inc
include rlecom.inc
include rledat.inc
include rleext.inc


createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg
	assumes	cs, DIMapSeg


rle_type_table	label	word
dw	OFFSET	scanline_decode_rle
dw	OFFSET	scanline_decode_absolute
dw	OFFSET	scanline_decode_endline
dw	OFFSET	scanline_decode_endseg
dw	OFFSET	scanline_decode_endframe


cProc	frame, <FAR,WIN,PASCAL>,<ds,si,di,es>
include	bmc_main.var
cBegin	<nogen>
cEnd	<nogen>




public	scanline_decode_bitmap
scanline_decode_bitmap	proc	near

	push	ax
	push    bx
	push	cx
	push	dx

        les     bx      ,lp_info_ext
	call	rlecom_get_external_type	  ;1,4,8, or 24 bits
	jnc	scanline_decode_end_skip	  ;carry set then error
	jmp	scanline_decode_end

scanline_decode_end_skip:
	mov	si	,cx			  ;otherwise cx is the
	call	rlecom_init
	mov	ax, decode_rle_table[si]	  ;specific decode routine
	mov	decode_rle	,ax
	mov	ax	,decode_absolute_table[si]
	mov	decode_absolute	,ax

	mov	ax	,es			  ;creating the color table
	mov	ds	,ax			  ;get location into ds:si
        mov     si      ,bx
	add	si	,WORD PTR es:[bx].biSize
	mov	bx	,cx
	add	bx	,bx
	mov	cx	,color_table_size_table[bx] ;size of color table into cx
	mov	ax	,ss
	mov	es	,ax
	lea	di	,color_xlate		  ;destination into es:di
	rep	movsw				  ;create the color table

        lds     si ,lp_bits_ext
	les	di ,lp_bits_dev
	push	di

scanline_decode_next:

	lodsb
	mov	bx	,ax
	mov	bh	,0			;ah needs to be preserved
        shl     bx      ,1
	jmp	rle_type_table[bx]



scanline_decode_rle:

       lodsb
       mov     cl      ,al
       mov     ch      ,0
       call    scanline_segment_overflow
       lodsb
       call    decode_rle
       jmp     scanline_decode_next

scanline_decode_absolute:

       lodsb
       mov     cl      ,al
       mov     ch      ,0
       call    scanline_segment_overflow
       lodsb
       call    decode_absolute
       jmp     scanline_decode_next

scanline_decode_endline:

       pop     di
       add     di      ,next_scan
       push    di
       jnc     scanline_decode_next
       mov     ax      ,es
       add     ax      ,1000H
       mov     es      ,ax
       jmp     scanline_decode_next

scanline_decode_endseg:

       xor     si      ,si
       mov     ax      ,ds
       add     ax      ,1000H
       mov     ds      ,ax
       jmp     scanline_decode_next

scanline_decode_endframe:

       pop	di


scanline_decode_end:

       pop     dx
       pop     cx
       pop     bx
       pop     ax
       ret

scanline_decode_bitmap	endp



scanline_segment_overflow       proc    near

       mov     bx      ,di
       add     bx      ,cx
       jnc     scanline_segment_overflow_end
       mov     bx      ,es
       add     bx      ,800H
       mov     es      ,bx
       sub     di      ,8000H

scanline_segment_overflow_end:
       ret

scanline_segment_overflow       endp


sEnd	DIMapSeg

END
