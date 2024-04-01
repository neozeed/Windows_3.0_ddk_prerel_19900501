;----------------------------------------------------------------------------;
; This file contains the code for the following 2 functions in the palette   ;
; manager module:							     ;
;		   SetPaletteEntries    ---  Sets Palette & DAC registers    ;
;                  GetPaletteEntries    ---  Gets the DAC register values    ;
;----------------------------------------------------------------------------;

			public	SetPaletteEntries
			public	GetPaletteEntries

;----------------------------------------------------------------------------;
;		   	   SetPaletteEntries				     ;
;			   -----------------				     ;
;    Programs the VGA hardware palette registers and the DAC register array  ;
;    to refer to a specified set of colors.			             ;
;  									     ;
;    Parameters:					                     ;
;                wIndex	    --     Starting Palette Register number          ;
;                wCount     --     Count of the number of registers          ;
;                lpColors   --     a long pointer to a table which has the   ;
;                                  following structure:			     ;
;			           STRUC				     ;
;				        RGB   ColorAsked 		     ;
;				        RGB   ColorGiven                     ;
;				   END STRUC   				     ;
;				   with ColorAsked filled in		     ;
;							                     ;
;    Returns:  	 							     ;
;		 Number of Registers set				     ;
;		 Fills in the ColorGiven triplets in color table	     ;
;									     ;
;    History:								     ;
;                Modified to suite 4 plane ega/vga drivers.		     ;
;                -by- Amit Chatterjee [amitc]  Thu 01-Dec-1988  08:59:00     ;
;							                     ;
;		 Created.						     ;
;		 -by- Amit Chatterjee [amitc]  Thu 27-Oct-1988  11:14:30     ;
;----------------------------------------------------------------------------;

LAST_PALETTE_REG	equ	0fh   		; last palette register num
SIXTEENTRIPLES          equ     (LAST_PALETTE_REG + 1) * 3

RGB	STRUC
Red	db	0		; Red Value
Green	db	0		; Green Value
Blue	db	0		; Blue Value
Flag    db	0
RGB	ENDS


ColorTable	STRUC
ColorAsked	db	size RGB dup (?)	; Requested Color
; ColorGiven	db	size RGB dup (?)        ; Actual colors
ColorTable	ENDS

incDrawMode = 1				; include the drawmode definitions

	.xlist
	include	cmacros.inc
	include	macros.mac
	include gdidefs.inc
	include	display.inc
	.list

sBegin	Data
	externA	NUM_PALETTES 		; in .\vga.asm
	externA BW_THRESHOLD	        ; in .\vga.asm
	externW	PaletteTranslationTable ; in .\vga.asm
	externB	PaletteModified		; in .\vga.asm
	externB TextColorXlated         ; in .\vga.asm
	externB device_local_brush      ; in .\vga.asm
	externB device_local_drawmode   ; in .\vga.asm
	externB device_local_pen	; in .\vga.asm

sEnd	Data
createSeg _PALETTE,PaletteSeg,word,public,CODE
sBegin	PaletteSeg
        assumes cs,PaletteSeg
	assumes	ds,nothing
	assumes	es,nothing

cProc	SetPaletteEntries,<FAR,PUBLIC,WIN,PASCAL>,<si,di,dx>

	parmW	wIndex		; index of the first register
	parmW	wCount		; no of registers to program
	parmD	lpColorTable	; pointer to an array of color table entries

	localW	RegCopied	; no of registers copied

cBegin
	cCall	do_validations	; do validation on the passed in parameters

;----------------------------------------------------------------------------;
; the above call sets the carry flag in case the parameters are not correct. ;
; if parameters are valid, the registers CX and BX will have the following   ;
; values:								     ;
;		BX	---     start palette/DAC register number	     ;
;               CX      ---     number of register to set		     ;
; also, DS:SI & ES:DI   ---     will point to the passed in color structure  ;
;----------------------------------------------------------------------------;
	jnc	set_params_ok	; all parameters ok
	jmp	short SetPaletteEntries_Ret

set_params_ok:

;----------------------------------------------------------------------------;
; At this point,	                                                     ;
;                BX      ---    has the start register number		     ;
;		 CX      ---    count of registers to program		     ;
;                DS:SI   ---    points to the first asked color		     ;
;                ES:DI   ---    points to the place for the first given color;
;								             ;
; The asked colors have 8 bits per color where as the DAC registers use just ;
; 6 bits per entry: So a 8 bit value will be converted to a 6 bit value by   ;
; using the MS 6 bits.							     ;
;									     ;
; We will program the palette register with identity mapping (ie, palette    ;
; register no i will hold a value i) and put the color in the corresponding  ;
; DAC register.								     ;
;----------------------------------------------------------------------------;

	push	cx		; save count of registers to program
	push	bx		; save register number
	mov	bh,bl		; set palette color = register number
	mov	ax,1000h	; set palette register call

; AL = 0 = function code (set palette)
; BL =     Palette register number (0 through 7)
; BH =     value for the register

	int	10h		; palette register gets programmed

	lodsb			; get the red value
	shiftr	al,2		; get MS 6 bits
;	stosb			; this is the assigned color
	mov	dh,al

	lodsb			; get the green color
	shiftr	al,2		; get ms 6 bits
;	stosb			; save the passed back color
	mov	ch,al		; int 10 call parameter
	lodsb			; finally get the blue value
	shiftr	al,2		; our color is the ms 6 bits
;	stosb			; save it
	mov	cl,al

	pop	bx		; get back current register number
	mov	ax,1010h	; set DAC register call 
	int	10h		; set the particular register

	inc	bx  		; go to the next register
;	add	si,size RGB
	inc	si
	add	di,size RGB	; point to the respective next entries
	pop	cx		; count of registers to program
	loop	set_params_ok	; program all the requested registers

; all the registers have been programmed. Return the no of registers copied
; to the caller

SetPaletteEntries_Ret:
	mov	ax,RegCopied

cEnd
;----------------------------------------------------------------------------;

cProc	do_validations,<NEAR,PUBLIC>

cBegin

;----------------------------------------------------------------------------;
;  do some local validations. VGA has a set of 16 palette registers so the   ;
;  range of registers specified should fit in the range [0..15].	     ;
;----------------------------------------------------------------------------;

	mov	RegCopied,0		; no of registers copied
	mov	bx,wIndex		; load the index and count into registers
	mov	cx,wCount

	cmp	bx,LAST_PALETTE_REG
	ja	param_error		; it cant be > than the last register num
	mov	ax,bx			; we don't want the deatroy bx
	add	ax,wCount
	dec	ax			; ax has the no of the last reg to program
	cmp	ax,LAST_PALETTE_REG
	jbe	range_ok		; register range is ok
	mov	cx,LAST_PALETTE_REG
	sub	cx,wIndex
	add	cx,1			; the number tha actually will be copied
range_ok:

	lds	si,lpColorTable 	; DS:SI points to the first asked color
	mov	ax,ds
	or	ax,si			; test for a valid pointer
	jz	param_error		; can't work wih a NULL pointer
	les	di,lpColorTable
	add	di, size RGB		; ES:DI points to the place for GIVEN color
	mov	RegCopied,cx		; set the return value
	clc
	jmp	short do_validations_ret
param_error:
	stc				; indicate error
do_validations_ret:

cEnd

;----------------------------------------------------------------------------;
;		   	   GetPaletteEntries				     ;
;			   -----------------				     ;
;    Gets the current value of a set of DAC registers.			     ;
;							                     ;
;    Parameters:					                     ;
;                wIndex	    --     Starting Palette Register number          ;
;                wCount     --     Count of the number of registers          ;
;                lpColors   --     a long pointer to a table which has the   ;
;                                  following structure:			     ;
;			           STRUC				     ;
;				        RGB   ColorAsked 		     ;
;				        RGB   ColorGiven                     ;
;				   END STRUC   				     ;
;				   GDI could fill in the ColorAsked values   ;
;									     ;
;    Returns:  	 							     ;
;		 Number of Registers set				     ;
;		 fills in the ColorGiven triplets			     ;							     ;
;    History:								     ;
;		 Created.						     ;
;		 -by- Amit Chatterjee [amitc]  Thu 27-Oct-1988  14:26:20     ;
;----------------------------------------------------------------------------;

cProc	GetPaletteEntries,<FAR,PUBLIC,WIN,PASCAL>,<si,di>

	parmW	wIndex			; index of the first register
	parmW	wCount			; no of registers to get
	parmD	lpColorTable		; pointer to an array of color table entries

	localW	RegCopied               ; no of registers copied

; we will allocate an area in the stack to store the color values 

	localV	ColorBuffer,SIXTEENTRIPLES

cBegin
	cCall	do_validations	        ; validate the passed in parameters
	jnc	get_params_ok	        ; parameters look to be all valid
	jmp	short GetPaletteEntries_Ret

get_params_ok:
	push	es		        ; save the segment of the color table
	lea	dx,ColorBuffer	
	mov	ax,ss
	mov	es,ax		        ; ES:DX points to a buffer to read in DAC regs
	mov	ax,1017h	        ; int 10 code to read DAC register block
	int	10h		        ; required registers read into local buffer

; now transfer the RGB triplets back to the callers buffer

	pop	es		        ; ES:DI points to user buffer
	add	di,size RGB	        ; ES:DI poits to first ColorGiven field
	mov	ax,ss
	mov	ds,ax	
	lea	si,ColorBuffer	        ; DS:SI poits to the colors read in
color_xfer_loop:
	movsw			        ; move red and green
	movsb			        ; move blue
	add	di,size RGB	        ; go to the next ColorGiven field
	loop	color_xfer_loop	        ; transfer all the colors

; we have filled in the values of all the requested registers, now return

GetPaletteEntries_Ret:
	mov	ax,RegCopied	        ; no of registers copied

cEnd

sEnd	PaletteSeg

END

;----------------------------------------------------------------------------;
