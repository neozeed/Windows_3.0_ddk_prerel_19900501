;-----------------------------------------------------------------------------;
;                         PLYBITM8.ASM                                        ;
;                         ------------                                        ;
; This file contains the draw for solid line on byte/pixel BITMAPs and the    ;
; byte/pixel BITMAP move  routines. The file is included in POLYLINE.ASM      ;
;                                                                             ;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.
comment ~

This part contains the slice drawing routines for the byte/pixel BITMAPs.
They are very similar to what we have for VRAM VGA except that now the
organization of the memory is different and also the ROP2 codes will be
handled differently.

                         HANDLING THE ROP2 CODES
                         -----------------------

Given any particular existing pixel value, the ROP2 codes combined with the Pen
color value may affect the pixel in the following four manners:

                       . some bits may be set to 0
                       . some bits may be set to 1
                       . some bits may be inverted
                       . some bits are left unchanged

As the information in the bit map is explicitly organized in terms of color
planes, its makes sense to handle a plane at a time. As each plane contributes
just one bit towards the ultimate pixel value, the ROP2 code depending upon
the particular pen color value may either set the target bit to 0 or 1, invert
it or leave it unchanged. The ROP2 code values have very conveniently been
allocated, if R is the ROP2 code (in the range 0 through 0fh) then the
following relationship holds:

                         if PEN COLOR BIT = 0
                         then let R1 = MOD(R,4)
                         else let R1 = IDIV(R,4)

                         switch (R1)

                          {
                          case 0:  SET BITMAP BIT to ZERO     ; break
                          case 1:  INVERT BIT MAP BIT         ; break
                          case 2:  LEAVE ALONE BIT MAP BIT    ; break
                          case 3:  SET BIT MAP BIT to ONE
                          }

We will use R1 to index into a table which has WORD entries, HIBYTE and LOBYTE
the algorithm followed is if D is the bit map byte containing the bit of our
interest and BL has the rotating bit mask,

                      . get rotating bit mask into AL and AH
                      . AND AL with LOBYTE and AH with HIBYTE
                      . INVERT AL
                      . AND D with AL
                      . XOR D with AH

The INVERT of AL with the subsequent AND ensures that we can set or reset a
particular bit or leave it alone without touching the other bits. The final
XOR does the inversion if necessary.

The following routines take care of the basic DRAW and MOVE routines. They
assume the following values have been calculated:

             wBitmapROP     -----   WORD containing the above XOR,AND mask
             wScan          -----   offset to the next scan line for same plane
             NumScans       -----   number of scan lines per segment
             FillBytes      -----   excess bytes at the end of the segment
             NextSegOff     -----   segment offset to the next segment

These variables must have the correct value, irrespective of whether we have
a small or a huge BITMAP ( NumScans should be 0 for small BITMAPS)

end comment ~
;                                                                             ;
; -by-  Amit Chatteree          Wed 14-Sep-1988    12:00:25                   ;
; -by-  Larry Coffey            1/89, Video 7
;                               Modified to work in VRAM's 256 color modes.   ;
; -by-  Irene Wu                2/89, Video 7                                 ;
;-----------------------------------------------------------------------------;
;                                                                             ;
;                       DRAW AND MOV ROUTINES FOR BITMAPS                     ;
;                       ---------------------------------                     ;
; INPUTS:                                                                     ;
;                                                                             ;
;            DS:DI          -----  current byte in BITMAP memory              ;
;               BL          -----  rotating bit mask for current pixel        ;
;               SI          -----  offset to next scan line (up or down)      ;
;               CX          -----  number of bits to draw                     ;
;       wBitmapROP          -----  XOR & AND MASK for current pen and ROP     ;
;        FillBytes          -----  extra bytes at the end of the segment      ;
;       NextSegOff          -----  segment offset to next segment             ;
;               DX          -----  number of scan lines left in the segment   ;
;                                                                             ;
; RETURNS:                                                                    ;
;            DS:DI          -----  updated current byte in BITMAP             ;
;               BL          -----  updated rotating bit mask                  ;
;               DX          -----  updated numver of scan lines left          ;
;                                    this value is maintained in ScanLeft     ;
;            AX,CX          -----  destroyed                                  ;
;                                                                             ;
; The DRAW routines for the POSITIVE_Y and DIAGONAL_1Q direction map on to the;
; ones for the NEGATIVE_Y and DIAGONAL_4Q after negating SI, the offset to    ;
; the next segment and the filler bytes value, and also scanleft should be    ;
; from the top of the bitmap memory                                           ;
;-----------------------------------------------------------------------------;
	PUBLIC Bm8_Negative_Y
Bm8_Negative_Y  proc    near

                mov     al,TmpColor
                dec     cx                  ; take out last pixel from loop
                jz      Bm8_Y_last_pixel
Bm8_Y_loop:
                call    BM8

; now move to the next scan line

                dec     dx                ; one more scan line taken up
                jnz     Bm8_Y_same_segment ; still in same seg,ent

		test	DeviceFlags, TYPE_IS_DEV
		jz	Bm8_update_selector

		; update bank
		mov	dx,NextSegOff
		or	dx,dx
		js	Bm8_Dec_Bank

		call	inc_bank_select
		jmp	short Bm8_update_offset

Bm8_Dec_Bank:	call	dec_bank_select
		jmp	short Bm8_update_offset

Bm8_update_selector:
                mov     dx,ds             ; get current segment
                add     dx,NextSegOff     ; positive or negative offset
                mov     ds,dx
Bm8_update_offset:
                add     di,FillBytes      ; bypass the filler bytes at end
                mov     dx,wScans         ; number of scans in full segment
Bm8_Y_same_segment:
                add     di,si             ; next scan in same segment
                loop    Bm8_Y_loop        ; process all the pixels

; do the last pixel

Bm8_Y_last_pixel:
                call    BM8
                ret

Bm8_Negative_Y  endp
;----------------------------------------------------------------------------;
	PUBLIC Bm8_Positive_Y
Bm8_Positive_Y  proc    near

                neg     NextSegOff        ; since we will be going to low addr
                neg     FillBytes

;    The no of scan lines left now to be taken fromm top

                neg     dx
                add     dx,wScans
                inc     dx                ; dx now has scansleft from top

;    now call Bm_Negative_Y

                call    Bm8_Negative_Y  ; line segment drawn

;    now DX has the number of scanlines from top, we will calculate the no of
;    scan lines left from thr bottom from DX

                neg     dx
                add     dx,wScans
                inc     dx                ; DX now has no of scans from bot

;    restore back the values which where negated

                neg     FillBytes
                neg     NextSegOff
                ret

Bm8_Positive_Y  endp
;---------------------------------------------------------------------------;
	PUBLIC Bm8_Positive_X
Bm8_Positive_X	proc	near

        mov     al,TmpColor
        dec     cx
        jz      bm8_x_do_last

bm8_x_output_loop:
        call    BM8
        inc     di
        loop    bm8_x_output_loop

bm8_x_do_last:
        call    BM8
        ret

Bm8_Positive_X	endp
;_________------------_____________-------------------________________


	PUBLIC	Bm8_Diagonal_4Q
Bm8_Diagonal_4Q proc	near

                mov     al,TmpColor
                dec     cx              ; leave out the last pixel
                jz      Bm8_diag_last_pixel
Bm8_Diag_loop:
                call    BM8

; move to next scan

                add     di,si
                inc     di              ; same Y, next X pixel  ;;;;;;
                dec     dx              ; dec no of scans left in dir of move
                jnz     bm8_diag_same_seg

		test	DeviceFlags, TYPE_IS_DEV
		jz	Bm8_Diag_Update_Selector

		mov	dx,NextSegOff
		or	dx,dx
		js	Bm8_Diag_Dec_Bank

		call	inc_bank_select
		jmp	short Bm8_Diag_Update_Offset

Bm8_Diag_Dec_Bank:
		call	dec_bank_select
		jmp	short Bm8_Diag_Update_Offset

Bm8_Diag_Update_Selector:
                mov     dx,ds
                add     dx,NextSegOff   ; offset of required segment
                mov     ds,dx
Bm8_Diag_Update_Offset:
                add     di,FillBytes    ; no of left over bytes
                mov     dx,wScans       ; new value of number of scans left
bm8_diag_same_seg:
                loop    Bm8_Diag_loop

; do the last pixel

Bm8_diag_last_pixel:

                call    BM8
                ret

Bm8_Diagonal_4Q endp
;-----------------------------------------------------------------------------;
	PUBLIC Bm8_Diagonal_1Q
Bm8_Diagonal_1Q proc	near

                neg     NextSegOff      ; offset to next segment
                neg     FillBytes       ; number of bytes left over
                neg     dx
                add     dx,wScans
                inc     dx              ; dx now has scans left from the top
                call    Bm8_Diagonal_4Q ; draw the slice

;    now DX has the number of scanlines from top, we will calculate the no of
;    scan lines left from thr bottom from DX

                neg     dx
                add     dx,wScans
                inc     dx                ; DX now has no of scans from bot
                neg     FillBytes       ; make them positive again
                neg     NextSegOff
                ret

Bm8_Diagonal_1Q endp
;-----------------------------------------------------------------------------;
	PUBLIC Bm8_Move_Negative_Y
Bm8_Move_Negative_Y proc        near

                add     di,si           ; next scan line
                dec     dx              ; reduce no of scanlines left
                jnz     bm8_move_in_same_seg_positive

		test	DeviceFlags, TYPE_IS_DEV
		jz	Bm8_Move_Update_Selector

		mov	dx,NextSegOff
		or	dx,dx
		js	Bm8_Move_Dec_Bank

		call	inc_bank_select
		jmp	short Bm8_Move_Update_Offset

Bm8_Move_Dec_Bank:
		call	dec_bank_select
		jmp	short Bm8_Move_Update_Offset

Bm8_Move_Update_Selector:
		mov	dx,ds
                add     dx,NextSegOff   ; nect segment
                mov     ds,dx

Bm8_Move_Update_Offset:
                add     di,FillBytes    ; skip over filler bytes
                mov     dx,wScans       ; no of scan lines left
bm8_move_in_same_seg_positive:
                ret

Bm8_Move_Negative_Y     endp
;-----------------------------------------------------------------------------;
	PUBLIC Bm8_Move_Positive_Y
Bm8_Move_Positive_Y proc        near

                add     di,si           ; previous scan line
                neg     dx
                add     dx,wScans       ; scan lines left in direction of move
                or      dx,dx
                jnz     bm8_move_in_same_seg_negative

		test	DeviceFlags, TYPE_IS_DEV
		jz	Bm8_MoveP_Update_Selector

		mov	dx,NextSegOff
		or	dx,dx
		jns	Bm8_MoveP_Dec_Bank

		call	inc_bank_select
		jmp	short Bm8_MoveP_Update_Offset

Bm8_MoveP_Dec_Bank:
		call	dec_bank_select
		jmp	short Bm8_MoveP_Update_Offset

Bm8_MoveP_Update_Selector:
		mov	dx,ds
		sub	dx,NextSegOff	; nect segment
                mov     ds,dx

Bm8_MoveP_Update_Offset:
                sub     di,FillBytes    ; skip over filler bytes
                mov     dx,wScans
bm8_move_in_same_seg_negative:
                neg     dx
                add     dx,wScans       ; no of scans left from bottom
                inc     dx              ; include current line in count
                ret

Bm8_Move_Positive_Y     endp
;----------------------------------------------------------------------------;
Bm8_Move_Positive_X proc        near

        inc     di
        ret

Bm8_Move_Positive_X     endp
;----------------------------------------------------------------------------;
Bm8_Move_Diagonal_4Q proc near

        inc     di
        call    Bm8_Move_Negative_Y
        ret

Bm8_Move_Diagonal_4Q    endp
;---------------------------------------------------------------------------;
Bm8_Move_Diagonal_1Q proc near

        inc     di
        call    Bm8_Move_Positive_Y
        ret

Bm8_Move_Diagonal_1Q    endp
;---------------------------------------------------------------------------;

;---------------------------------------------------------------------------;
BM8_ROP0	proc	near
		mov	BYTE PTR [di],0
                ret
BM8_ROP0	endp
;---------------------------------------------------------------------------;
BM8_ROP1	proc	near
		or	BYTE PTR [di],al
		not	BYTE PTR [di]
                ret
BM8_ROP1	endp
;---------------------------------------------------------------------------;
BM8_ROP2	proc	near
		mov	ah	,al
		not	ah
		and	BYTE PTR [di],ah
		ret
BM8_ROP2	endp
;---------------------------------------------------------------------------;
BM8_ROP3	proc	near
		mov	ah	,al
		not	ah
		mov	BYTE PTR [di],ah
		ret
BM8_ROP3	endp
;---------------------------------------------------------------------------;
BM8_ROP4	proc	near
		mov	ah	,BYTE PTR [di]
		not	ah
		and	ah	,al
		mov	BYTE PTR [di],ah
		ret
BM8_ROP4	endp
;---------------------------------------------------------------------------;
BM8_ROP5	proc	near
		not	BYTE PTR [di]
		ret
BM8_ROP5	endp
;---------------------------------------------------------------------------;
BM8_ROP6	proc	near
		xor	BYTE PTR [di],al
		ret
BM8_ROP6	endp
;---------------------------------------------------------------------------;
BM8_ROP7	proc	near
		and	BYTE PTR [di],al
		not	BYTE PTR [di]
                ret
BM8_ROP7	endp
;---------------------------------------------------------------------------;
BM8_ROP8	proc	near
		and	BYTE PTR [di],al
                ret
BM8_ROP8	endp
;---------------------------------------------------------------------------;
BM8_ROP9	proc	near
		xor	BYTE PTR [di],al
		not	BYTE PTR [di]
                ret
BM8_ROP9	endp
;---------------------------------------------------------------------------;
BM8_ROPA	proc	near
                ret
BM8_ROPA	endp
;---------------------------------------------------------------------------;
BM8_ROPB	proc	near
		mov	ah	,al
		not	ah
		or	BYTE PTR [di],ah
		ret
BM8_ROPB	endp
;---------------------------------------------------------------------------;
BM8_ROPC	proc	near
		mov	 BYTE PTR [di],al
		ret
BM8_ROPC	endp
;---------------------------------------------------------------------------;
BM8_ROPD	proc	near
		not	BYTE PTR [di]
		or	BYTE PTR [di] ,al
		ret
BM8_ROPD	endp
;---------------------------------------------------------------------------;
BM8_ROPE	proc	near
		or	BYTE PTR [di] ,al
		ret
BM8_ROPE	endp
;---------------------------------------------------------------------------;
BM8_ROPF	proc	near
		mov	BYTE PTR [di] ,0FFH
		ret
BM8_ROPF	endp


; TRUTH TABLE
;P D  0 1 2 3 4 5 6 7 8 9 a b c d e f
;0 0  0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
;0 1  0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1
;1 0  0 0 0 0 1 1 1 1 0 0 0 0 1 1 1 1
;1 1  0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1

