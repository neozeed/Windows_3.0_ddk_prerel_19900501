;-----------------------------------------------------------------------------;
;                         PLYSOLID.ASM                                        ;
;                         ------------                                        ;
; This file contains the draw for VRAM VGA (256 color) solid lines and the    ;
; VRAM VGA (256 color) move routines.                                         ;
; The file is included in POLYLINE.ASM                                        ;
;                                                                             ;
comment ~


Breshenham's algorithm requires moves in one of the eight basic directions,
which are:

        Positive_X              : towards increasing X value, horizontally
        Negative_X              : horizontally towards decreasing X value
        Positive_Y              : vertically towards top of display memory
        Negative_Y              : vertically towards end of display memory
        Diagonal_1Q             : diagonally in first quadrant
        Diagonal_2Q             : disgonally in second quadrant
        Diagonal_3Q             : diagonnaly in third quadrant
        Diagonal_4Q             : diagonnaly in fourth quadrant

We modify the end coordinates to ensure that lines never move to the left. This
manipulation allows us to dispense of with three of the 8 directions that
involve movement along negative X direction.

Furthermore, the routine for Positive_Y will exactly be similar to the one for
Negative_Y, if we negate the BYTES_PER_LINE parameter and hence move towards
lower VGA DISPLAY MEMORY address with the same ADD instruction. This is also
true for Diagonal_1Q routine, which with the same manipulation can jmp off
to the Diagonal_4Q routine. The offset to the next scan line will actually be
neagated before the draw routines, so we can dispense of with Positive_Y and
Diagonal_1Q. This is also true for the move routines.


The routines share a common set of parameters and return values

Inputs:

        DS:DI                   : points to current byte in display memory
*       BL                      : bit position in current byte
        CX                      : no of bytes to set on in the specific dir.
*       DX                      : address of the GRAPHICS CONTROLLER DATA reg.
        SI                      : no of bytes in one scan line


Outputs:

        DS:DI                   : points to the current byte after drawing line
*       BL                      : bit position after line drawing
        DX,SI                   : unchanged
        AX,CX,BH                : destroyed

? Does line color matter or is it always white ?
? For now, 256 color mode will write a 0fh for bright white line ?

Assumptions on VGA register values:

        . WRITE MODE 0   has been programmed
        . SET/RESET registers disabled
        . COMBINE function set to REPLACE, SHIFT function to NULL
        . GRAPHICS CONTROLLER ADDRESS REGISTER contains index of BIT MASK
          REGISTER (not necessary)
        . DAC set to proper colors
        . SEQUENCER bit map mak register enables write to all planes

The code for each direction has carefully been planned out to save on cycles
(at the expense of bytes)

end comment ~
;                                                                             ;
; -by-  Amit Chatterjee         Wed 14-Sep-1988   11:56:35                    ;
; -modified for V-RAM VGA- Larry Coffey  Sun 21-Jan-1989  17:20:10            ;
; -modified for V-RAM VGA- Irene Wu      2/89                                 ;
;-----------------------------------------------------------------------------;


comment ~

Negative_Y:

*                1. A small loop in the routine is used to draw 8 bits at a
                    time. However the loop is opened up to output each bit
                    separately and the code for each bit takes 4 BYTES.

*                2. The last bit is always set on after the loop, so CX
                    is decremented by 1 to start with.

*                3. CX is then divided by 8 to get a quotient and a remainder
                    The quotient would actually give the number of times the
                    SET_8_PIXELS loop is to be traversed. The Remainder gives
                    the last few bits (<8) to set on.

*                4. To avoid having to write code to set on the last few bits
                    the following trick was done:

                        CX in step 3 is decremented by 1 before the divide
                        and the remainder incremented after the divide, this
                        ensures that the remainder will never be 0 but may
                        be 8.

                        Remember the SET_8_PIXEL loop has 8 different points
                        ,each of 4 bytes, to set on each of the 8 bits.

                        The entry point into the loop is modified so that the
                        first pass actually draws the partial bits. To do this
                        simply multiply the remainder by 4 (4 bytes for each
                        bit to be set on) and enter at that offset from the
                        end of the loop.

                        Since the loop is also being used to draw the partial
                        bits, the quotient of the division is incremented by
                        one to account for the extra pass.

 * While these were true for the EGA routines, most have changed for the
   V-RAM VGA extended 256 color routines.

end comment ~

        public  Negative_Y
Negative_Y      proc    near

        test    SingleFlag,SINGLE_OK        ; is it really single pass ?
        jz      neg_y_twopass

        mov     al,TmpColor
        dec     cx
        jz      neg_y_30
neg_y_10:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        add     di,si
        jc      neg_y_40
neg_y_20:
        loop    neg_y_10
neg_y_30:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        ret
neg_y_40:
        call    inc_bank_select
        jmp     short neg_y_20

;---------------------------------------
neg_y_twopass:
        dec     cx
        jz      neg_y_t30
neg_y_t10:
        call    twopass_wt_data
        add     di,si
        jc      neg_y_t40
neg_y_t20:
        loop    neg_y_t10
neg_y_t30:
        call    twopass_wt_data
        ret
neg_y_t40:
        call    inc_bank_select
        jmp     short neg_y_t20

Negative_Y      endp
;----------------------------------------------------------------------------;
        public  Positive_Y
Positive_Y      proc    near

        neg     si
        test    SingleFlag,SINGLE_OK        ; is it really single pass ?
        jz      pos_y_twopass

        mov     al,TmpColor
        dec     cx
        jz      pos_y_30
pos_y_10:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        sub     di,si
        jc      pos_y_40
pos_y_20:
        loop    pos_y_10
pos_y_30:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        neg     si
        ret
pos_y_40:
        call    dec_bank_select
        jmp     short pos_y_20

;---------------------------------------
pos_y_twopass:
        dec     cx
        jz      pos_y_t30
pos_y_t10:
        call    twopass_wt_data
        sub     di,si
        jc      pos_y_t40
pos_y_t20:
        loop    pos_y_t10
pos_y_t30:
        call    twopass_wt_data
        neg     si
        ret
pos_y_t40:
        call    dec_bank_select
        jmp     short pos_y_t20

Positive_Y      endp
;----------------------------------------------------------------------------;
        public  Positive_X
Positive_X      proc    near
        push    es
        push    ds
        pop     es                      ; Put VGA selector in ES for STOS instructions

        test    SingleFlag,SINGLE_OK        ; is it really single pass ?
        jz      pos_x_twopass

        mov     al,TmpColor
pos_x_10:
;       mov     ah,[di]
;       stosb
        call    BM8
        inc     di
        loop    pos_x_10
pos_x_20:
        dec     di                      ; Point to last pixel written
        pop     es                      ; Note: if ES was an invalid selector,
                                        ;       an exception would occur here!
        ret

;---------------------------------------
pos_x_twopass:
pos_x_t10:
        call    twopass_wt_data
        inc     di
        loop    pos_x_t10

        jmp     short pos_x_20

Positive_X      endp
;-----------------------------------------------------------------------------;

comment ~

Diagonal_4Q

A simple loop outputs the bytes, each time going a byte to the right and a
byte down.

end comment ~

        public  Diagonal_4Q
Diagonal_4Q     proc    near

        test    SingleFlag,SINGLE_OK        ; is it really single pass ?
        jz      diag_4q_twopass

        mov     al,TmpColor
        dec     cx
        jz      diag_4q_30
diag_4q_10:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        inc     di
        add     di,si
        jc      diag_4q_40
diag_4q_20:
        loop    diag_4q_10
diag_4q_30:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        ret
diag_4q_40:
        call    inc_bank_select
        jmp     short diag_4q_20

;---------------------------------------
diag_4q_twopass:
        dec     cx
        jz      diag_4q_t30
diag_4q_t10:
        call    twopass_wt_data
        inc     di
        add     di,si
        jc      diag_4q_t40
diag_4q_t20:
        loop    diag_4q_t10
diag_4q_t30:
        call    twopass_wt_data
        ret
diag_4q_t40:
        call    inc_bank_select
        jmp     short diag_4q_t20

Diagonal_4Q     endp

;-----------------------------------------------------------------------------;
        public  Diagonal_1Q
Diagonal_1Q     proc    near

        neg     si
        test    SingleFlag,SINGLE_OK        ; is it really single pass ?
        jz      diag_1q_twopass

        mov     al,TmpColor
        dec     cx
        jz      diag_1q_30
diag_1q_10:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        inc     di
        sub     di,si
        jc      diag_1q_40
diag_1q_20:
        loop    diag_1q_10
diag_1q_30:
;       mov     ah,[di]
;       mov     byte ptr [di],al
        call    BM8
        neg     si
        ret
diag_1q_40:
        call    dec_bank_select
        jmp     short diag_1q_20

;---------------------------------------
diag_1q_twopass:
        dec     cx
        jz      diag_1q_t30
diag_1q_t10:
        call    twopass_wt_data
        inc     di
        sub     di,si
        jc      diag_1q_t40
diag_1q_t20:
        loop    diag_1q_t10
diag_1q_t30:
        call    twopass_wt_data
        neg     si
        ret
diag_1q_t40:
        call    dec_bank_select
        jmp     short diag_1q_t20

Diagonal_1Q     endp

;-----------------------------------------------------------------------------;
comment ~

        Now we will have the 3 routines to move the current point by one
        pixel in a specified direction

        Inputs:
                DS:DI           : points to current byte in display memory
*               BL              : indicates the current bit in the above byte
                SI              : no of bytes in one scan line

        Outputs:

                Updates DI and BL to reflect a 1 pixel move in the specific
                direction.


end comment ~
;----------------------------------------------------------------------------;
Move_Negative_Y proc    near

        add     di,si           ; move down one scan line
        jnc     mov_neg_y_10
        call    inc_bank_select
mov_neg_y_10:
        ret
Move_Negative_Y endp
;----------------------------------------------------------------------------;
Move_Positive_Y proc    near

        neg     si
        sub     di,si           ; move down one scan line
        jnc     mov_pos_y_10
        call    dec_bank_select
mov_pos_y_10:
        neg     si
        ret
Move_Positive_Y endp
;----------------------------------------------------------------------------;
Move_Positive_X proc    near
        inc     di                      ; Assume no bank xing in X-direction
        ret
Move_Positive_X endp
;----------------------------------------------------------------------------;
Move_Diagonal_4Q proc   near
        inc     di                      ; Assume no bank xing in X-direction
        add     di,si
        jnc     mov_diag_4q_10
        call    inc_bank_select
mov_diag_4q_10:
        ret
Move_Diagonal_4Q endp
;----------------------------------------------------------------------------;
Move_Diagonal_1Q proc   near
        neg     si
        inc     di                      ; Assume no bank xing in X-direction
        sub     di,si
        jnc     mov_diag_1q_10
        call    dec_bank_select
mov_diag_1q_10:
        neg     si
        ret
Move_Diagonal_1Q endp
;----------------------------------------------------------------------------;
