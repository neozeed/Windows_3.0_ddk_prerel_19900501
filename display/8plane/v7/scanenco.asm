;
;	File:	SCANENCO.ASM
;	Date:	7/24/89
;	Author:	James Keller
;
;	This module handles the setup and rle encoding of a bitmap image.
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


cProc	frame, <FAR,WIN,PASCAL>,<ds,si,di,es>
include	bmc_main.var
cBegin	<nogen>
cEnd	<nogen>



public	scanline_encode_bitmap
scanline_encode_bitmap	proc	near

	les	bx	,lp_info_ext
	call	rlecom_get_external_type	   ;1,4,8, or 24 bits
	jc	scanline_encode_bitmap_done	   ;carry set then error

        mov     si      ,cx
	call	rlecom_init
	mov	cx	, encode_rle_table[si]	  ;specific routine tables
	mov	encode_rle	,cx
	mov	cx	,encode_absolute_table[si]
	mov	encode_absolute ,cx
	mov	cx	,encode_table[si]
	mov	scanline_encode ,cx

	call	scanenco_create_scanseg_table
	les	di ,lp_bits_ext
	mov	si	,OFFSET	scanline_segment_table

scanline_encode_allblocks:
	mov	bx	,cs:[si].sst_length		;num scans in next chunk
	or	bx	,bx				;if it is zero,
	je	scanline_encode_bitmap_done		;	then done
	push	si
	lds	si	,cs:[si].sst_offset		;get next chunk address
	call	scanline_blockencode			;rle encode the block
	pop	si
	add	si	,6				;next table entry
	jmp	scanline_encode_allblocks

scanline_encode_bitmap_done:
	ret

scanline_encode_bitmap	endp





;
;   scanenco_create_scanseg_table
;

scanenco_create_scanseg_table	proc	near

	mov	cx	,WORD PTR es:[bx].biHeight	;total number of
	lds	si	,lp_dest_dev			;  scanlines to convert
	mov	bx	,WORD PTR [si].bmBits		;offset to bitmap image
	mov	dx	,WORD PTR [si+2].bmBits 	;segment of bitmap image
	mov	si	,OFFSET scanline_segment_table
	or	bx	,bx				;make certain that no
	jns	scanline_save_segment_map		;segemnt overrun occurs
		sub	bx	,8000H			;in encoding of first
		add	dx	,800H			;scanline

scanline_save_segment_map:
	mov	cs:[si].sst_offset ,bx			;save offset
	mov	cs:[si].sst_segment ,dx			;save segment
	mov	di	,-1

scanline_one_less:
	inc	di					;count number of
	add	bx	,ax				;scanlines before a
	jnc	scanline_encode_noov			;segment overrun occurs
	mov	cs:[si].sst_length, di			;save this count
	sub	bx	,ax				;backup offset by 1 scan
	sub	bx	,8000H				;update offset, segment
	add	dx	,800H				;to prevent overrun
	add	si	,6				;next table entry
	jmp	scanline_save_segment_map

scanline_encode_noov:
	loop	scanline_one_less			;one less scan to go

	inc	di
        mov     cs:[si].sst_length, di                  ;save number of scans
	mov	WORD PTR cs:[si+6].sst_length, 0	;no more to consider
	ret

scanenco_create_scanseg_table	endp
 




;      The parameters for scanline_blockencode are as follows:
;      DS:SI   -       pointer to data stream to encode
;      ES:DI   -       pointer to storage for encoded data
;      BX      -       number of scan lines to encode
;      picture_width   byte width of the image we are encoding
;      nextscan_line   byte count of length of the scan line
;      worstcase_multiplier value to multiply by picture width to give
;			    the worstcase encoding length
;
;      scanline_blockencode can only encode data within a block i.e. a segment.
;      DS:SI is not allowed to cross segment boudaries. However, ES:DI CAN
;      cross segment boundaries.
;

scanline_blockencode   proc    near

scanline_newline:

       call    scanline_worstcase_encode

       mov     dx      ,scan_byte_count
       push    si
       call    scanline_encode
       pop      si

       mov     al      ,RLE_TYPE_ENDLINE
       stosb

       add      si     ,next_scan
       dec	bx
       jne	scanline_newline
       ret

scanline_blockencode   endp





;      The parameters for scanline_worstcase_encode are as follows:
;      ES:DI   -	   pointer to storage for encoded data
;      BX      -	   number of scan lines to encode
;      picture_width	   byte width of the image we are encoding
;      worstcase_length    worstcase encoding length of the scanline
;

scanline_worstcase_encode      proc    near

       mov     ax      ,worstcase_length
       add     ax      ,di
       jnc     scanline_worstcase_inseg
	       mov     al      ,RLE_TYPE_ENDSEG
	       stosb
	       mov     ax      ,es
	       add     ax      ,1000H
	       mov     es      ,ax
	       xor     di      ,di

scanline_worstcase_inseg:
       ret

scanline_worstcase_encode      endp


sEnd    DIMapSeg

END

