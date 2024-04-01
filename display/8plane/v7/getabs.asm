;
;
;      File:   GETABS.ASM
;      Author: James Keller
;      Date:   7/16/89
;
;      All the copyabs_iXeY routines have the same parameters.
;
;      DS:SI   -       ptr to the data sequence to be run length encoded
;      ES:DI   -       ptr to the location to store the run length encoding
;      CX      -       maximum number of bytes to run length encode + 1
;      AL      -       run length value


include cmacros.inc
include	macros.mac
include gdidefs.inc
include	rledat.inc
include	rleext.inc

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg
	assumes	cs, DIMapSeg




public	copyabs_i1e1
copyabs_i1e1   proc    near

       push    bx
       push    bp
       mov     bp      ,di
       mov     bx      ,cx             ;save maximum count
       inc     di		       ;update RL record ptr
       dec     si

ci1e1_next_byte:
       movsb			       ;store source byte in the absolute run
       mov     al      ,ds:[si]        ;get next byte (contains 8 source pixels)
       inc     al		       ;sends  0 -> 1 and FF -> 0
       and     al      ,0FEH	       ;if al is not a 0 or a 1 then pixels are
       loopne  ci1e1_next_byte	       ;  still mixed -- use absolute encoding

       sub     bx      ,cx
       mov     cx      ,bx
       shiftl  bx      ,3
       mov     es:[bp] ,bl	       ;set the absolute record length

       pop     bp
       pop     bx
       ret

copyabs_i1e1   endp





public	copyabs_i1e4
copyabs_i1e4   proc    near

       push    bx
       push    bp
       push    dx
       mov     bp      ,di
       mov     dx      ,cx	       ;save maximum count
       inc     di		       ;update RL record ptr

ci1e4_next_byte:
       mov     bx      ,ax
       shiftr  bx      ,3	       ;get top nibble of source byte
       and     bx      ,01EH	       ;convert to a word index
       mov     bx      ,jxlat_1_to_4[bx]
       xchg    ax      ,bx
       stosw			       ;store 4 pixels: each is 4 bits/pixel

       add     bx      ,bx
       and     bx      ,01EH           ;low nibble of source byte to word index
       mov     ax      ,jxlat_1_to_4[bx]
       stosw			       ;store 4 pixels: each is 4 bits/pixel

       lodsb                           ;get next 8 source pixels
       mov     ah      ,al
       inc     ah		       ;if next 8 pixels are not all the same
       and     ah      ,0FEH	       ;   i.e. al != 0   and al != FF
       loopne  ci1e4_next_byte	       ;   then stay in absolute encoding
       dec     si		       ;last pixel is not part of absolute run

       sub     dx      ,cx	       ;compute number of bytes in absolute run
       mov     cx      ,dx
       shiftl  dx      ,3	       ;8 times as many destination pixels
       mov     es:[bp] ,dl	       ;set the absolute record length

       pop     dx
       pop     bp
       pop     bx
       ret

copyabs_i1e4   endp





public	copyabs_i1e8
copyabs_i1e8   proc    near

       push    bx
       push    bp
       push    dx
       mov     bp      ,di
       mov     bx      ,cx	       ;save maximum count
       inc     di		       ;update RL record ptr
       mov     dh      ,al	       ;prepare to expand 1 bit to 8 bits

ci1e8_next_byte:
       mov     dl      ,4

ci1e8_next_pixel:
       rcl     dh      ,1
       sbb     al      ,al
       rcl     dh      ,1
       sbb     ah      ,ah
       stosw
       dec     dl
       jne     ci1e8_next_pixel

       lodsb
       mov     dh      ,al	       ;prepare to expand 1 bit to 8 bits
       inc     al                      ;if next 8 pixels are not all the same
       and     al      ,0FEH	       ;   i.e. al != 0   and al != FF
       loopne  ci1e8_next_byte	       ;   then stay in absolute encoding
       dec     si		       ;last pixel is not part of absolute run

       sub     bx      ,cx	       ;compute number of bytes in absolute run
       mov     cx      ,bx
       shiftl  bx      ,3	       ;8 times as many destination pixels
       mov     es:[bp] ,bl	       ;set the absolute record length

       pop     dx
       pop     bp
       pop     bx
       ret

copyabs_i1e8   endp






public	copyabs_i1e24
copyabs_i1e24	proc	near

       push    bx
       push    bp
       push    dx
       mov     bp      ,di
       mov     bx      ,cx	       ;save maximum count
       inc     di		       ;update RL record ptr
       mov     dh      ,al	       ;prepare to expand 1 bit to 8 bits

ci1e24_next_byte:
       mov     dl      ,4

ci1e24_next_pixel:
       rcl     dh      ,1
       sbb     al      ,al
       rcl     dh      ,1
       sbb     ah      ,ah
       stosw
       stosw
       stosw
       dec     dl
       jne     ci1e24_next_pixel

       lodsb
       mov     dh      ,al	       ;prepare to expand 1 bit to 8 bits
       inc     al                      ;if next 8 pixels are not all the same
       and     al      ,0FEH	       ;   i.e. al != 0   and al != FF
       loopne  ci1e24_next_byte        ;   then stay in absolute encoding
       dec     si		       ;last pixel is not part of absolute run

       sub     bx      ,cx	       ;compute number of bytes in absolute run
       mov     cx      ,bx
       shiftl  bx      ,3	       ;8 times as many destination pixels
       mov     es:[bp] ,bl	       ;set the absolute record length

       pop     dx
       pop     bp
       pop     bx
       ret

copyabs_i1e24	endp





public	copyabs_i8e1
copyabs_i8e1   proc    near

       push    bx
       mov     bx      ,cx	       ;save maximum count
       mov     ah      ,al
       xor     al      ,0FFH           ;was first pixel a one?
       jne     ci8e1_look_for_one      ;if not, then look for a one

ci8e1_look_for_zero:
       lodsb			       ;get next byte in data sequence
       xor     al      ,0FFH	       ;if it is a "zero" (i.e. anything but FF)
       loopne  ci8e1_look_for_one      ;  then keep going and look for a one
       jmp     ci8e1_end_search        ;two consecutive pixels matched

ci8e1_look_for_one:
       lodsb			       ;get next byte in data sequence
       xor     al      ,0FFH	       ;if it is a "one" (i.e. an FF)
       loope   ci8e1_look_for_zero     ;  then keep going and look for a zero

ci8e1_end_search:
       dec     si
       sub     bx      ,cx             ;compute length of absolute run
       mov     es:[di] ,bl	       ;set the absolute record length
       inc     di		       ;update the RL record pointer

       mov     cx      ,bx             ;get number of pixels in absolute run
       add     ah      ,1	       ;these two lines map
       sbb     al      ,al	       ;   FF to FF and everything else to 0
       xor     al      ,01010101B      ;absolute run must alternate pixels
       add     cx      ,7
       shiftr  cx      ,3	       ;compute number of bytes to store
       rep     stosb		       ;store the absolute run
       mov     cx      ,bx	       ;restore the return value
       pop     bx
       ret

copyabs_i8e1   endp





public	copyabs_i8e4
copyabs_i8e4   proc    near

       push    bp
       push    bx
       mov     bp      ,di	       ;location to store absolute run length
       inc     di		       ;point to run length record data area
       mov     bx      ,cx             ;save maximum bytes to encode
       and     al      ,0FH	       ;only low four bits will be needed
       dec     cx		       ;
       jcxz    ci8e4                   ;AL is the only byte left in the run

ci8e4_next_pixel:
       mov     ah      ,al	       ;save previous pixel
       lodsb			       ;get next pixel
       and     al      ,0FH	       ;do compression mapping to 4 bits
       cmp     al      ,ah	       ;are we still on the same RL value?
       je      ci8e4_found_pixel_match ;if so, then done with absolute run

       shiftl  ah      ,4	       ;get first pixel into high nibble
       xchg    al      ,ah	       ;save most recent pixel in ah
       or      al      ,ah	       ;place two pixels in one byte
       stosb			       ;store the pixels
       dec     cx                      ;another byte has been run-length coded
       je      ci8e4_maximum_absolute  ;if no more bytes to encode, then done
 
       lodsb			       ;get next byte
       and     al      ,0FH	       ;do compression mapping to 4 bits
       cmp     al      ,ah	       ;are we still on the same RL value?
       je      ci8e4_found_pixel_match ;if so, then done with absolute run

       loop    ci8e4_next_pixel        ;get second pixel to combine into byte

ci8e4: shiftl  al      ,4              ;if no more byte to encode, then save
       stosb			       ;  first pixel in high nibble
       jmp     ci8e4_maximum_absolute

ci8e4_found_pixel_match:
       sub     si      ,2
       inc     cx

ci8e4_maximum_absolute:
       sub     bx      ,cx             ;compute the run length
       mov     cx      ,bx
       mov     es:[bp] ,bl	       ;save the run length

       pop     bx
       pop     bp
       ret

copyabs_i8e4   endp






public	copyabs_i8e8
copyabs_i8e8   proc    near

       push    bx
       push    di
       mov     di      ,si
       dec     si
       push    si
       push    es
       mov     bx      ,ds
       mov     es      ,bx	       ;ES:DI = DS:SI + 1
       mov     bx      ,cx	       ;save maximum bytes to encode

       repne   cmpsb		       ;find first adjacent pixels which match
       sub     cx      ,1	       ;these two lines map 0 -> 1 and all else
       adc     cx      ,1	       ;  to itself

       pop     es		       ;restore all the pointers
       pop     si		       ;
       pop     di		       ;

       sub     bx      ,cx	       ;compute absolute run length
       mov     es:[di] ,bl	       ;save RL record length
       inc     di		       ;update RLE pointer

       mov     cx      ,bx             ;cx has RL absolute run length
       rep     movsb		       ;store bytes in absolute run
       mov     cx      ,bx	       ;restore cx
       pop     bx
       ret

copyabs_i8e8   endp





public	copyabs_i8e24
copyabs_i8e24	proc	near

       push    bx
       push    dx
       push    di
       push    si
       push    es
       mov     di      ,si
       mov     bx      ,ds
       mov     es      ,bx	       ;ES:DI = DS:SI + 1
       mov     dx      ,cx	       ;save the maximum bytes to encode
       dec     si		       ;count the byte already in AL

       repne   cmpsb		       ;find first adjacent pixels which match
       sub     cx      ,1
       adc     cx      ,1

       pop     es                      ;restore all the pointers
       pop     si
       pop     di

       sub     dx      ,cx             ;compute absolute run length
       mov     cx      ,dx
       mov     es:[di] ,dl	       ;save RL record length
       inc     di		       ;update RL ptr

ci8e24_next_pixel:
       sub     ah      ,ah
       mov     bx      ,ax
       add     bx      ,ax
       add     bx      ,ax	       ;form index into table of 3 byte entries
       mov     ax      ,word ptr i8e24_color_table[bx]
       stosw			       ;store first two bytes of 24 bit color
       mov     al      ,BYTE PTR i8e24_color_table[bx]
       stosb			       ;store last byte of 24 bit color
       lodsb			       ;get next pixel to map
       dec     dx		       ;one less byte left in this absolute run
       jne     ci8e24_next_pixel       ;go map the next pixel
       dec     si		       ;did not need to get the last pixel

       pop     dx
       pop     bx
       ret

copyabs_i8e24  endp

sEnd

END
