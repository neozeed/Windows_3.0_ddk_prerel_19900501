;----------------------------------------------------------------------------;
; 				rgb_to_ipc				     ;
;				----------				     ;
;   The given RGB triplet is converted into a 4 plane color index and the    ;
;   physical RGB triplet together with the color accelarator flags are       ;
;   returned.				                                     ;
;									     ;
;   It is this routine which maps the colors to the bit planes		     ;
;   of the EGA/VGA.						             ;
;									     ;
;   Ordering of the color in a dword is such that when stored in	     ;
;   memory, red is the first byte, green is the second, and blue	     ;
;   is the third.  The high order 8 bits may be garbage when passed	     ;
;   in, and should be ignored.						     ;
;									     ;
;   when in a register:     xxxxxxxxBBBBBBBBGGGGGGGGRRRRRRRR		     ;
;									     ;
;   when in memory:	    db	    red,green,blue			     ;
;									     ;
;									     ;
; Entry:								     ;
;	DS:SI --> RGB triplet to sum	    (for sum_RGB_colors)	     ;
;	AL     =  Red	value of triplet    (for sum_RGB_colors_alt)	     ;
;	AH     =  Green value of triplet    (for sum_RGB_colors_alt)	     ;
;	DL     =  Blue	value of triplet    (for sum_RGB_colors_alt)	     ;
; Returns:								     ;
;	BX		= Sum of the triplet				     ;
;									     ;
; The EGA/VGA supports 2 bits for every color and so the physical color can  ;
; only be one of 4 values. The mapping chosen by experimentation happens to  ;
; be:-									     ;
;		Physical Index            Color Vale(0-255)		     ;
;		--------------		  -----------------		     ;
;		     00				 0			     ;
;		     01				 128			     ;
;		     11				 255			     ;
; The intenstity bit,ie, the MS bit of the 4 bit index is common to all the  ;
; 3 planes and an index 10 for all will be used to siginify a dark grey with ;
; color value being 32 for all 3 colors.				     ;
;									     ;
;	AL		= Physical red color byte (0,32,128,255)	     ;
;	AH		= Physical green color byte (0,32,128,255)	     ;
;	DL		= Physical blue color byte (0,32,128,255)	     ;
;	DH:C0		= red	bit					     ;
;	DH:C1		= green bit					     ;
;	DH:C2		= blue	bit					     ;
;	DH:C3		= intensity bit					     ;
;	DH:MONO_BIT	= 0 if BX < BWThreashold			     ;
;			= 1 if BX >= BWThreashold			     ;
;	DH:ONES_OR_ZERO = 1 if C0:C3 are all 1's or all 0's		     ;
;	DH:GREY_SCALE	= 0						     ;
;	DH_SOLID_BRUSH	= 0						     ;
; Error Returns:							     ;
;	None								     ;
; Registers Preserved:							     ;
;	CX,SI,DI,DS,ES							     ;
; Registers Destroyed:							     ;
;	CX,FLAGS							     ;
; Calls:								     ;
;	None								     ;
; History:								     ;
;								             ;
;       Tue 29-Aug-1989 13:51:00 -by-  Amit Chatterjee [amitc]		     ;
;	For EGA, we never can return index 8 for any color because that is   ;
;       now being used just to invert gray at index 7. So on EGA what ever   ;
;       would be mapped to index 8 should map to index 7, whereas on VGA     ;
;	index 8 is still valid. The routine 'EGAindex8to7' defined in        ;
;       EGAHIRES.ASM and VGA.ASM does this.				     ;
;									     ;
;       Wed 12-Apr-1989 17:47:42 -by-  Amit Chatterjee [amitc]		     ;
;       Changed the color matcher totlly - this does not do the nearest color;
;       matching in the truest sense and goes more by a range matching test. ;
;       This is likely to change in the near future.			     ;
;       Also the special color for index 8 is (192,192,192).		     ;
;									     ;
;	Fri 25-Nov-1988 11:00:00 -by-  Amit Chatterjee [amitc]		     ;
;  	Created to adopt the 4 plane EGA/VGA model. The code is on the	     ;
;       lines of the code used in PM land.				     ;
;----------------------------------------------------------------------------;
	.286

	include	cmacros.inc
	include	macros.mac

	externFP	AllocCSToDSAlias

;----------------------------------------------------------------------------;
; we first define the equates for the fixed palette and color mapping for the;
; EGA and VGA drivers. The low nibble of the bytes  are the color indicxes & ;
; the high nibble has the accelarator bytes as discussed above.		     ;
;----------------------------------------------------------------------------;

PHY_COLOR_BYTE_00	equ	00100000b	; black
PHY_COLOR_BYTE_01	equ	00000001b	; dark red
PHY_COLOR_BYTE_02	equ	00000010b	; dark green
PHY_COLOR_BYTE_03	equ	00000011b	; mustard
PHY_COLOR_BYTE_04	equ	00000100b	; dark blue
PHY_COLOR_BYTE_05	equ	00000101b	; purple
PHY_COLOR_BYTE_06	equ	00000110b	; dark turquoise
PHY_COLOR_BYTE_07	equ	00010111b	; gray
PHY_COLOR_BYTE_08	equ	00001000b	; special blue
PHY_COLOR_BYTE_09	equ	00001001b	; red
PHY_COLOR_BYTE_10	equ	00001010b	; green
PHY_COLOR_BYTE_11	equ	00011011b	; yellow
PHY_COLOR_BYTE_12	equ	00001100b	; blue
PHY_COLOR_BYTE_13	equ	00011101b	; magenta
PHY_COLOR_BYTE_14	equ	00011110b	; cyan
PHY_COLOR_BYTE_15	equ	00111111b	; white

;----------------------------------------------------------------------------;
; The next set of equates define the physical color bytes for the types      ;
; supported by the driver.						     ;
;----------------------------------------------------------------------------;

PHY_COLOR_DATA_00	equ	00000000h	; black			
PHY_COLOR_DATA_01	equ	00000080h	; dark red		
PHY_COLOR_DATA_02	equ	00008000h	; dark green		
PHY_COLOR_DATA_03	equ	00008080h	; mustard		
PHY_COLOR_DATA_04	equ	00800000h	; dark blue		
PHY_COLOR_DATA_05	equ	00800080h	; purple		
PHY_COLOR_DATA_06	equ	00808000h	; dark turquoise	
PHY_COLOR_DATA_07	equ	00808080h	; gray			
PHY_COLOR_DATA_08	equ	00c0c0c0h	; dark gray		

PHY_COLOR_DATA_09	equ	000000FFh	; red			
PHY_COLOR_DATA_10	equ	0000FF00h	; green			
PHY_COLOR_DATA_11	equ	0000FFFFh	; yellow		
PHY_COLOR_DATA_12	equ	00FF0000h	; blue			
PHY_COLOR_DATA_13	equ	00FF00FFh	; pink (magenta)	
PHY_COLOR_DATA_14	equ	00FFFF00h	; cyan			
PHY_COLOR_DATA_15	equ	00FFFFFFh	; white			

;----------------------------------------------------------------------------;

sBegin	Data

sEnd	Data

	public	rgb_to_ipc
	public	IndexToColor

sBegin	Code
	assumes cs,Code

	externNP  	EGAindex8to7	;EGAHIRES.ASM/VGAHIRES.ASM

;----------------------------------------------------------------------------;
; A sorted list of the RGB colors for mapping from an index		     ;
;----------------------------------------------------------------------------;

adrgbIndex	equ	this dword
	dd	PHY_COLOR_DATA_00
	dd	PHY_COLOR_DATA_01
	dd	PHY_COLOR_DATA_02
	dd	PHY_COLOR_DATA_03
	dd	PHY_COLOR_DATA_04
	dd	PHY_COLOR_DATA_05
	dd	PHY_COLOR_DATA_06
	dd	PHY_COLOR_DATA_07
	dd	PHY_COLOR_DATA_08
	dd	PHY_COLOR_DATA_09
	dd	PHY_COLOR_DATA_10
	dd	PHY_COLOR_DATA_11
	dd	PHY_COLOR_DATA_12
	dd	PHY_COLOR_DATA_13
	dd	PHY_COLOR_DATA_14
	dd	PHY_COLOR_DATA_15


;----------------------------------------------------------------------------;
; a sorted list of color indeices together with their corresponding acclflags;
;----------------------------------------------------------------------------;

abindexaccl	equ	this byte

	db	PHY_COLOR_BYTE_00	
	db	PHY_COLOR_BYTE_01	
	db	PHY_COLOR_BYTE_02	
	db	PHY_COLOR_BYTE_03	
	db	PHY_COLOR_BYTE_04	
	db	PHY_COLOR_BYTE_05	
	db	PHY_COLOR_BYTE_06	
	db	PHY_COLOR_BYTE_07	
	db	PHY_COLOR_BYTE_08	
	db	PHY_COLOR_BYTE_09	
	db	PHY_COLOR_BYTE_10	
	db	PHY_COLOR_BYTE_11	
	db	PHY_COLOR_BYTE_12	
	db	PHY_COLOR_BYTE_13	
	db	PHY_COLOR_BYTE_14	
	db	PHY_COLOR_BYTE_15	
;----------------------------------------------------------------------------;									;
;   rgb_to_ipc accepts a logical RGB color value and returns the	     ;
;   device dependent, physical representation of bits necessary to	     ;
;   display the color closest to the specified color on the device.	     ;
;									     ;
; Entry:								     ;
;	DX:AX  =  RGB color (AL=Red, AH=green, DL=blue) 		     ;
; Returns:								     ;
;	AL		= Physical Red value				     ;
;	AH		= Physical Green value				     ;
;	DL		= Physical Blue value				     ;
;	DH		= Color Index:					     ;
;	DH:C0		= blue	intensity msb				     ;
;	DH:C1		= green intensity msb				     ;
;	DH:C2		= red	intensity msb				     ;
;	DH:C3		= intensity					     ;
;	DH:MONO_BIT	= 0 if BX < BWThreshold 			     ;
;			= 1 if BX >= BWThreshold			     ;
;	DH:ONES_OR_ZEROS= 1 if bits are 1111 or 0000			     ;
;	Sign bit clear (AX)						     ;
; Error Returns:							     ;
;	Sign bit set (AX) if error.  (Note:  No errors are possible in	     ;
;	this function, so always clear the sign bit -- calling code	     ;
;	checks it.)							     ;
; Registers Preserved:							     ;
;	BX,CX,SI,DI,DS,ES						     ;
; Registers Destroyed:							     ;
;	DX								     ;
; Calls:								     ;
;	None								     ;
; History:								     ;
;					                                     ;
; Fri 25-Nov-1988 11:00:00   -by-  Amit Chatterjee [amitc]		     ;
; Adopted the routine for 4 plane EGA/VGA drivers. PM uses plane 0 for Blue  ;
; and plane 2 for Red, for windows the convention is reverse. Changed the    ;
; tables to ensure plane 0 is red and 2 is blue.			     ;
;									     ;
;----------------------------------------------------------------------------;

LO_BREAK	equ	64    
HI_BREAK	equ	192
BREAK_128	equ	128

cProc	rgb_to_ipc,<NEAR,PUBLIC>,<bx,cx,si,di>

	localW	ErrorLo			;lo word of color distance
	localB	ErrorHi			;hi byte of color distance
	localB	desired_index		;the best index value
	localB	current_index		;the current palette being inspected

cBegin

;----------------------------------------------------------------------------;
; if all color values are < 64, map the color to black (index 0)	     ;
;----------------------------------------------------------------------------;

	cmp	al,LO_BREAK
	jae	not_black		;red >= 64
	cmp	ah,LO_BREAK
	jae	not_black		;green >= 64
	cmp	dl,LO_BREAK		
	jae	not_black		;blue >= 32

; all values are < 64

	xor	dh,dh			;index for black
	jmp	short index_obtained

not_black:

;----------------------------------------------------------------------------;
; if all colors are < 192 but atleast one >= 64 map colors to one low 	     ;
; intensity color, with color bit set on if color value >= 64 else set it off;
;----------------------------------------------------------------------------;

	cmp	al,HI_BREAK		
	jae	hi_intensity		;red >= 192
	cmp	ah,HI_BREAK
	jae	hi_intensity		;green >= 192
	cmp	dl,HI_BREAK	
	jae	hi_intensity		;blue < 192


lo_intensity:

; map colors to low intensity

	xor	dh,dh			;set color and intensity bits off
	mov	bl,LO_BREAK		;value to be tested against
	jmp	short test_n_set_bits

hi_intensity:

;----------------------------------------------------------------------------;
; if all color are 192, map them to special gray at index 8		     ;
;----------------------------------------------------------------------------;

	cmp	al,192
	jnz	do_hi_intensity
	cmp	ah,192
	jnz	do_hi_intensity
	cmp	dl,192
	jnz	do_hi_intensity

; map color to special gray

	mov	dh,8
	jmp	index_obtained

do_hi_intensity:

; map colors to hi intensity
	
	mov	dh,8h			;set intensity bit on
	mov	bl,HI_BREAK		;value to be compared against
test_n_set_bits:
	cmp	al,bl
	jb	lo_red			;red bit to remain off
	add	dh,1			;set red on
lo_red:
	cmp	ah,bl
	jb	lo_green		;green below threshold
	add	dh,2			;set green on
lo_green:
	cmp	dl,bl
	jb	index_obtained		;blue below threshold
	add	dh,4			;set blue on

index_obtained:

; on an EGA, we never want to return index 8 (it is used only for drawing
; the inverse of the border) so we will map it into index 7. For vga it is
; fine. The following routine is found in EGAHIRES.ASM and VGAHIRES.ASM

	call	EGAindex8to7		;for EGA: if dh = 8, return dh = 7
				        ;for VGA: nop.

; the color index is in DH, get the index with the accelarators set and the
; corresponding color values by table look up.

	mov	bl,dh			;the decided value of index 
	xor	bh,bh			;index into a byte table
	mov	dh,abindexaccl[bx]	;get index with accelarators set

; now get the color values corresponding to index

	shl	bx,1
	shl	bx,1			;index into DWORDS
	mov	al,byte ptr adrgbIndex[bx]	;get red
	mov	ah,byte ptr adrgbIndex[bx][1]	;get green
	mov	dl,byte ptr adrgbIndex[bx][2]	;get blue

; we are done


cEnd

    
;----------------------------------------------------------------------------;
;                         IndexToColor					     ;
;                         ------------					     ;
; This routine converts an index value into the corresponding hardware       ;
; supported color triplet.					             ;
;									     ;
; Inputs:								     ;
;	    DH    ---    color index (0 thru 15)                             ;
; Returns:								     ;
;	    AL    ---    red color byte					     ;
;           AH    ---    green color byte			             ;
;           DL	  ---    blue color byte				     ;
;           DH    ---    input index value and the accelarators				     ;
;           no other registers modified or destroyed			     ;
;									     ;
; -by-  Amit Chatterjee  Thu 01-Dec-1988    07:56:50			     ;
;----------------------------------------------------------------------------;


IndexToColor	proc	far

	assumes	ds,nothing
	assumes	es,nothing
	assumes	cs,Code


	push	bx				 ; save 
	mov	bl,dh			         ; get the index value
	and	bx,000fh		         ; only 0-15 valid
	shl	bx,1
	shl	bx,1			         ; will index into a dword table
	mov	al,byte ptr [bx][adrgbIndex]	 ; get red color
	mov	ah,byte ptr [bx][adrgbIndex+1]   ; get the green color
	mov	dl,byte ptr [bx][adrgbIndex+2]   ; blue color

; now get the accelarators

	mov	bl,dh				; index into a byte table
	mov	dh,[bx][abindexaccl]		; get the index and accl flags
	pop	bx				; restore

	ret

IndexToColor	endp

sEnd

end

