        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	VGA.ASM
;
;   This module contains functions and definitions specific to
;   the VGA Display Driver.
;
; Created: 22-Feb-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:	physical_enable
;			physical_disable
;
; Public Data:
;		PHYS_DEVICE_SIZE		info_table_base
;		BW_THRESHOLD			physical_device
;		COLOR_FORMAT			rot_bit_tbl
;		SCREEN_W_BYTES			color_table
;		SCREEN_WIDTH			Code_palette
;		SCREEN_HEIGHT			
;		COLOR_TBL_SIZE			
;		COLOR_DONT_CARE
;		SSB_START_SCAN
;		SSB_TOTAL_SCANS
;		ScreenSelector
;
;		HYPOTENUSE
;		Y_MAJOR_DIST
;		X_MAJOR_DIST
;		Y_MINOR_DIST
;		X_MINOR_DIST
;		MAX_STYLE_ERR
;
;		 H_HATCH_BR_0, H_HATCH_BR_1, H_HATCH_BR_2, H_HATCH_BR_3
;		 H_HATCH_BR_4, H_HATCH_BR_5, H_HATCH_BR_6, H_HATCH_BR_7
;		 V_HATCH_BR_0, V_HATCH_BR_1, V_HATCH_BR_2, V_HATCH_BR_3
;		 V_HATCH_BR_4, V_HATCH_BR_5, V_HATCH_BR_6, V_HATCH_BR_7
;		D1_HATCH_BR_0,D1_HATCH_BR_1,D1_HATCH_BR_2,D1_HATCH_BR_3
;		D1_HATCH_BR_4,D1_HATCH_BR_5,D1_HATCH_BR_6,D1_HATCH_BR_7
;		D2_HATCH_BR_0,D2_HATCH_BR_1,D2_HATCH_BR_2,D2_HATCH_BR_3
;		D2_HATCH_BR_4,D2_HATCH_BR_5,D2_HATCH_BR_6,D2_HATCH_BR_7
;		CR_HATCH_BR_0,CR_HATCH_BR_1,CR_HATCH_BR_2,CR_HATCH_BR_3
;		CR_HATCH_BR_4,CR_HATCH_BR_5,CR_HATCH_BR_6,CR_HATCH_BR_7
;		DC_HATCH_BR_0,DC_HATCH_BR_1,DC_HATCH_BR_2,DC_HATCH_BR_3
;		DC_HATCH_BR_4,DC_HATCH_BR_5,DC_HATCH_BR_6,DC_HATCH_BR_7
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;
STOP_IO_TRAP	equ 4000h		; stop io trapping
START_IO_TRAP	equ 4007h		; re-start io trapping

incDevice = 1				;Include control for gdidefs.inc
incDrawMode = 1                         ;Include DRAWMODE structure

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include ega.inc
	include	egamem.inc
	include display.inc
	include macros.mac
	include	cursor.inc
	.list

	externA	ScreenSelector		; an import from the kernel
	externA	__WinFlags		; an import from kernel
	externA WINR			; flag bits in __WinFlags (enable.asm)
	externA	WINP			; flag bits in __WinFlags (enable.asm)
	externA	WIN286			; flag bits in __WinFlags (enable.asm)
	externA WIN386			; flag bits in __WinFlags (enable.asm)
	externFP AllocSelector		; create a new selector
	externFP FreeSelector		; free the selector
	externFP AllocCSToDSAlias	; CS -> DS (new)

;------------------------------ Macro ----------------------------------;
;	cdw - conditional dw
;
;	cdw is used to conditionally define a word to one of the
;	given values.
;
;	usage:
;		cdw	v1,v2
;
;	v1 will be used if LARGE_SCREEN_SIZE if defined.
;	v1 will be used otherwise.
;-----------------------------------------------------------------------;

cdw	macro	v1,v2
ifdef	LARGE_SCREEN_SIZE
	dw	v1
else
	dw	v2
endif
	endm


	externFP init_hw_regs		;Initialize ega state code

	public	PHYS_DEVICE_SIZE	;Number of bytes in physical device
	public	BW_THRESHOLD		;Black/white threshold
	public	COLOR_FORMAT		;Color format
	public	SCREEN_W_BYTES		;Screen width in bytes
	public	SCREEN_WIDTH		;Screen width in pixels
	public	SCREEN_HEIGHT		;Screen height in scans
	public	COLOR_TBL_SIZE		;Number of entries in color table
	public	COLOR_DONT_CARE 	;Value for color don't care register
	public	ScreenSelector		;Segment of Regen RAM
	public	ScratchSel
ifdef PALETTES
	public	NUM_PALETTES		;Number of palette registers
	public	PaletteSupported	;whether palette manager supported
endif
	public	physical_enable 	;Enable routine
	public	physical_disable	;Disable

	public	ssb_device		;SaveScreenBitmap version of above
	public	physical_device 	;Physical device descriptor
	public	info_table_base 	;GDIInfo table base address
	public	Code_palette

	public	SSB_START_SCAN
	public	SSB_TOTAL_SCANS		

	public	HYPOTENUSE
	public	Y_MAJOR_DIST
	public	X_MAJOR_DIST
	public	Y_MINOR_DIST
	public	X_MINOR_DIST
	public	MAX_STYLE_ERR

	public	PixelSeg_color_table	;Table of physical colors
	public	BlueMoonSeg_color_table ;Table of physical colors

	public	 H_HATCH_BR_0, H_HATCH_BR_1, H_HATCH_BR_2, H_HATCH_BR_3
	public	 H_HATCH_BR_4, H_HATCH_BR_5, H_HATCH_BR_6, H_HATCH_BR_7
	public	 V_HATCH_BR_0, V_HATCH_BR_1, V_HATCH_BR_2, V_HATCH_BR_3
	public	 V_HATCH_BR_4, V_HATCH_BR_5, V_HATCH_BR_6, V_HATCH_BR_7
	public	D1_HATCH_BR_0,D1_HATCH_BR_1,D1_HATCH_BR_2,D1_HATCH_BR_3
	public	D1_HATCH_BR_4,D1_HATCH_BR_5,D1_HATCH_BR_6,D1_HATCH_BR_7
	public	D2_HATCH_BR_0,D2_HATCH_BR_1,D2_HATCH_BR_2,D2_HATCH_BR_3
	public	D2_HATCH_BR_4,D2_HATCH_BR_5,D2_HATCH_BR_6,D2_HATCH_BR_7
	public	CR_HATCH_BR_0,CR_HATCH_BR_1,CR_HATCH_BR_2,CR_HATCH_BR_3
	public	CR_HATCH_BR_4,CR_HATCH_BR_5,CR_HATCH_BR_6,CR_HATCH_BR_7
	public	DC_HATCH_BR_0,DC_HATCH_BR_1,DC_HATCH_BR_2,DC_HATCH_BR_3
	public	DC_HATCH_BR_4,DC_HATCH_BR_5,DC_HATCH_BR_6,DC_HATCH_BR_7


;-----------------------------------------------------------------------;
;	The hatched brush pattern definitions
;-----------------------------------------------------------------------;

H_HATCH_BR_0	equ	00000000b	;Horizontal Hatched brush
H_HATCH_BR_1	equ	00000000b
H_HATCH_BR_2	equ	00000000b
H_HATCH_BR_3	equ	00000000b
H_HATCH_BR_4	equ	11111111b
H_HATCH_BR_5	equ	00000000b
H_HATCH_BR_6	equ	00000000b
H_HATCH_BR_7	equ	00000000b

V_HATCH_BR_0	equ	00001000b	;Vertical Hatched brush
V_HATCH_BR_1	equ	00001000b
V_HATCH_BR_2	equ	00001000b
V_HATCH_BR_3	equ	00001000b
V_HATCH_BR_4	equ	00001000b
V_HATCH_BR_5	equ	00001000b
V_HATCH_BR_6	equ	00001000b
V_HATCH_BR_7	equ	00001000b

D1_HATCH_BR_0	equ	10000000b	;\ diagonal brush
D1_HATCH_BR_1	equ	01000000b
D1_HATCH_BR_2	equ	00100000b
D1_HATCH_BR_3	equ	00010000b
D1_HATCH_BR_4	equ	00001000b
D1_HATCH_BR_5	equ	00000100b
D1_HATCH_BR_6	equ	00000010b
D1_HATCH_BR_7	equ	00000001b

D2_HATCH_BR_0	equ	00000001b	;/ diagonal hatched brush
D2_HATCH_BR_1	equ	00000010b
D2_HATCH_BR_2	equ	00000100b
D2_HATCH_BR_3	equ	00001000b
D2_HATCH_BR_4	equ	00010000b
D2_HATCH_BR_5	equ	00100000b
D2_HATCH_BR_6	equ	01000000b
D2_HATCH_BR_7	equ	10000000b

CR_HATCH_BR_0	equ	00001000b	;+ hatched brush
CR_HATCH_BR_1	equ	00001000b
CR_HATCH_BR_2	equ	00001000b
CR_HATCH_BR_3	equ	00001000b
CR_HATCH_BR_4	equ	11111111b
CR_HATCH_BR_5	equ	00001000b
CR_HATCH_BR_6	equ	00001000b
CR_HATCH_BR_7	equ	00001000b

DC_HATCH_BR_0	equ	10000001b	;X hatched brush
DC_HATCH_BR_1	equ	01000010b
DC_HATCH_BR_2	equ	00100100b
DC_HATCH_BR_3	equ	00011000b
DC_HATCH_BR_4	equ	00011000b
DC_HATCH_BR_5	equ	00100100b
DC_HATCH_BR_6	equ	01000010b
DC_HATCH_BR_7	equ	10000001b



;-----------------------------------------------------------------------;
;	Line style definitions for the EGA Card
;
;	Since the style update code in the line DDA checks for a sign,
;	the values chosen for distances, HYPOTENUSE, and MAX_STYLE_ERR
;	must not be bigger than 127+min(X_MAJOR_DIST,Y_MAJOR_DIST).  If
;	this condition is met, then the sign bit will always be cleared
;	on the first subtraction after every add-back.
;-----------------------------------------------------------------------;


HYPOTENUSE	=	51		;Distance moving X and Y
Y_MAJOR_DIST	=	36		;Distance moving Y only
X_MAJOR_DIST	=	36		;Distance moving X only
Y_MINOR_DIST	=	HYPOTENUSE-X_MAJOR_DIST
X_MINOR_DIST	=	HYPOTENUSE-Y_MAJOR_DIST
MAX_STYLE_ERR	=	HYPOTENUSE*2	;Max error before updating
					;  rotating bit mask



;-----------------------------------------------------------------------;
;	The black/white threshold is used to determine the split
;	between black and white when summing an RGB Triplet
;-----------------------------------------------------------------------;


BW_THRESHOLD	equ	(3*0FFh)/2
page

sBegin	Data

globalB do_int_30,0			;int 30 call necessary or not
globalB	Line43,0		       	;43 line mode
globalD LineAddr,0
globalW ScratchSel,0			;have a scratch selector
globalW	is_protected,__WinFlags		;LSB=1 iff protected mode
globalW ssb_mask,0FFFFh 		;Mask for save screen bitmap bit
globalB enabled_flag,0			;Display is enabled if non-zero

ifdef PALETTES

NUM_PALETTES	equ	16		; number of palettes
PaletteSupported equ	1		; palette manager not supported
globalB	device_local_brush ,0,<SIZE oem_brush_def>; for translate palette
globalW PaletteTranslationTable,0, NUM_PALETTES	  ; the tranlate table
globalB PaletteModified,0			  ; table tampered ?
globalB TextColorXlated,0			  ; text colors translated
globalB device_local_drawmode,0,<SIZE DRAWMODE>   ; local drawmode structure
globalB device_local_pen,0,<SIZE oem_pen_def>     ; local pen definitions

endif

globalB bEquipmentFlag, 0		;BYTE at 40:10 in ROM BIOS save area

sEnd	Data
page

createSeg _INIT,InitSeg,word,public,CODE
sBegin	InitSeg
assumes cs,InitSeg


SCREEN_W_BYTES	equ	SCAN_BYTES*1	;("*1" to get to public symbol table)
COLOR_FORMAT	equ	(0100h + NUMBER_PLANES)
COLOR_DONT_CARE equ	0f00h + GRAF_CDC;Value for color don't care register

;-----------------------------------------------------------------------;
;	PhysDeviceSize is the number of bytes that the enable routine
;	is to copy into the passed PDevice block before calling the
;	physical_enable routine.  For this driver, the length is the
;	size of the bitmap structure.
;-----------------------------------------------------------------------;

PHYS_DEVICE_SIZE equ	size BITMAP



;-----------------------------------------------------------------------;
;	Allocate the physical device block for the EGA Card.
;	For this driver, physical devices will be in the same format
;	as a normal bitmap descriptor.	This is very convienient since
;	it simplifies the structures that the code must work with.
;
;	The bmWidthPlanes field will be set to zero to simplify some
;	of the three plane code.  By setting it to zero, it can be
;	added to a memory bitmap pointer without changing the pointer.
;	This allows the code to add this in regardless of the type of
;	the device.
;
;	The actual physical block will have some extra bytes stuffed on
;	the end (IntPhysDevice structure), but only the following is static
;-----------------------------------------------------------------------;



;	The following constants keep the parameter list to BITMAP within
;	view on an editing display 80 chars wide.

SCRSEL		equ	ScreenSelector
P		equ	 COLOR_FORMAT AND 000FFh	;# color planes
B		equ	(COLOR_FORMAT AND 0FF00h) SHR 8	;# bits per pixel
H		equ	SCREEN_HEIGHT			;new display height
W		equ	SCREEN_WIDTH			;display width, pels
WB		equ	SCREEN_W_BYTES			;display width, bytes

physical_device BITMAP <SCRSEL,W,H,WB,P,B,0A0000000H,0,0,0,0,0,0,0>



;-----------------------------------------------------------------------;
;	The GDIInfo data Structure.  The specifics of the EGA
;	mode are passed to GDI via this structure.
;-----------------------------------------------------------------------;

	public	drivers_TC_caps
	public	drivers_RC_caps

info_table_base label byte

	dw	300h			;version number
	errnz	dpVersion

	dw	DT_RASDISPLAY		;Device classification
	errnz	dpTechnology-dpVersion-2

	cdw	240,208 		;Horizontal size in millimeters
	errnz	dpHorzSize-dpTechnology-2

	cdw	180,156 		;Vertical size in millimeters
	errnz	dpVertSize-dpHorzSize-2

	dw	SCREEN_WIDTH		;Horizontal width in pixels
	errnz	dpHorzRes-dpVertSize-2

	dw	SCREEN_HEIGHT		;Vertical width in pixels
	errnz	dpVertRes-dpHorzRes-2

	dw	1			;Number of bits per pixel
	errnz	dpBitsPixel-dpVertRes-2

	dw	4			;Number of planes
	errnz	dpPlanes-dpBitsPixel-2

	dw	-1			;Number of brushes the device has
	errnz	dpNumBrushes-dpPlanes-2 ;  (Show lots of brushes)

	dw	16*5			;Number of pens the device has
	errnz	dpNumPens-dpNumBrushes-2;  (16 colors * 5 styles)

	dw	0			;Reserved

	dw	0			;Number of fonts the device has
	errnz	dpNumFonts-dpNumPens-4

	dw	16			;Number of colors in color table
	errnz	dpNumColors-dpNumFonts-2

	dw	size int_phys_device	;Size required for device descriptor
	errnz	dpDEVICEsize-dpNumColors-2

	dw	CC_NONE 		;Curves capabilities
	errnz	dpCurves-dpDEVICEsize-2

	dw	LC_POLYLINE+LC_STYLED	;Line capabilities
	errnz	dpLines-dpCurves-2

	dw	PC_SCANLINE		;Polygonal capabilities
	errnz	dpPolygonals-dpLines-2

drivers_TC_caps	label	word
	dw	TC_CP_STROKE+TC_RA_ABLE+TC_EA_DOUBLE ;Text capabilities
	errnz	dpText-dpPolygonals-2

	dw	CP_RECTANGLE 		;Clipping capabilities
	errnz	dpClip-dpText-2

drivers_RC_caps	label word
ifdef PALETTES
					;BitBlt capabilities
	dw	RC_BITBLT+RC_BITMAP64+RC_GDI20_OUTPUT+RC_SAVEBITMAP+RC_DI_BITMAP+RC_DIBTODEV+RC_PALETTE+RC_BIGFONT
else
	dw	RC_BITBLT+RC_BITMAP64+RC_GDI20_OUTPUT+RC_SAVEBITMAP+RC_DI_BITMAP+RC_DIBTODEV+RC_BIGFONT
endif
	errnz	dpRaster-dpClip-2

	dw	X_MAJOR_DIST		;Distance moving X only
	errnz	dpAspectX-dpRaster-2

	dw	Y_MAJOR_DIST		;Distance moving Y only
	errnz	dpAspectY-dpAspectX-2

	dw	HYPOTENUSE		;Distance moving X and Y
	errnz	dpAspectXY-dpAspectY-2

	dw	MAX_STYLE_ERR		;Length of segment for line styles
	errnz	dpStyleLen-dpAspectXY-2


	errnz	dpMLoWin-dpStyleLen-2	;Metric  Lo res WinX,WinY,VptX,VptY
	cdw	15,2080 		;  HorzSize * 10
	cdw	15,1560 		;  VertSize * 10
	cdw	4,640			;  HorizRes
	cdw	-4,-480 		;  -VertRes


	errnz	dpMHiWin-dpMLoWin-8	;Metric  Hi res WinX,WinY,VptX,VptY
	cdw	150,20800		;  HorzSize * 100
	cdw	150,15600		;  VertSize * 100
	cdw	4,640			;  HorizRes
	cdw	-4,-480 		;  -VertRes


	errnz	dpELoWin-dpMHiWin-8	;English Lo res WinX,WinY,VptX,VptY
	cdw	375,325 		;  HorzSize * 1000
	cdw	375,325 		;  VertSize * 1000
	cdw	254,254 		;  HorizRes * 254
	cdw	-254,-254		;  -VertRes * 254


	errnz	dpEHiWin-dpELoWin-8	;English Hi res WinX,WinY,VptX,VptY
	cdw	3750,1625		;  HorzSize * 10000
	cdw	3750,1625		;  VertSize * 10000
	cdw	254,127 		;  HorizRes * 254
	cdw	-254,-127		;  -VertRes * 254


	errnz	dpTwpWin-dpEHiWin-8	;Twips		WinX,WinY,VptX,VptY
	cdw	5400,2340		;  HorzSize * 14400
	cdw	5400,2340		;  VertSize * 14400
	cdw	254,127 		;  HorizRes * 254
	cdw	-254,-127		;  -VertRes * 254


	dw	96			;Logical Pixels/inch in X
	errnz	dpLogPixelsX-dpTwpWin-8

	dw	96			;Logical Pixels/inch in Y
	errnz	dpLogPixelsY-dpLogPixelsX-2

	dw	DC_IgnoreDFNP		;dpDCManage
	errnz	dpDCManage-dpLogPixelsY-2

	dw	0			;Reserved fields
	dw	0
	dw	0
	dw	0
	dw	0

	dw	0
	dw	0
	dw	0

	errnz	<(offset $)-(offset info_table_base)-(size GDIINFO)>

ifdef PALETTES
; start of entries in version 3.0 of this structure

	dw	16 			; number of palette entries
	errnz	dpNumPalReg-dpDCManage-12

	dw	18			; DAC resolution for RGB
	errnz	dpColorRes-dpNumPalReg-2

endif

page

;---------------------------Public-Routine------------------------------;
; physical_enable
;
;   VGA 640x480 graphics mode is enabled.  The VGA's Color-Don't-Care
;   register and palettes are set for an 8-color mode of operation.
;   The EGA state restoration code is initialized.
;
; Entry:
;	ES:DI --> ipd_format in our pDevice
;	DS:    =  Data
; Returns:
;	AX = non-zero to show success
; Error Returns:
;	AX = 0
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,ES,DS,FLAGS
; Calls:
;	INT 10h
;	init_hw_regs
; History:
;	
;       Tue 29-Aug-1989 13:45:00 -by-  Amit Chatterjee [amitc]
;	added a copuple of routines in _TEXT segment (EGAindex8to7 &
;	RGB192Brush) which are called from ROBJECT.ASM and COLOR.ASM
;	and are different for EGA. (This is a hack for the special gray
;	in VGA/EGA)
;
;	Tue 18-Aug-1987 18:09:00 -by-  Walt Moore [waltm]
;	Added enabled_flag
;
;	Thu 26-Feb-1987 13:45:58 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing

physical_enable proc near
	
;----------------------------------------------------------------------------;
; allocate the scratch selector here				             ;
;----------------------------------------------------------------------------;

	push	es
	xor	ax,ax
	cCall	AllocSelector,<ax>	; get a free selector
	mov	ScratchSel,ax		; save it


; also change the slector in ssb_device at this point

	mov	ax, seg _TEXT
	cCall	AllocCSToDSAlias, <ax>
	mov	es, ax
	assumes	es, Code
	mov	ax,ScreenSelector
	mov	word ptr es:[ssb_device.bmType], ax
	mov	word ptr es:[ssb_device.bmBits+2], ax
	mov	ax,es
	xor	bx,bx
	mov	es,bx			;invalidate es before freeing it
	cCall	FreeSelector,<ax>
	pop	es
;-----------------------------------------------------------------------------;


	mov	al,3			;we will save old mode as mode 3
	stosb

; get the number of lines being displayed currently

	push	bp
	push	es
	push	ds
	pop	es
	lea	bp,LineAddr
	mov	ax,1130h
	mov	bx,0
	int	10h
	xor	al,al
	inc	dl
	cmp	dl,25
	jz	Line25
	mov	al,dl
Line25:
	mov	Line43,al
	mov	ax, 40h 		;now get the equipment flag and if
	mov	es, ax			;a monochrome adaptor is currently
	mov	al, es:[10h]		;used, select the color monitor so
	mov	bEquipmentFlag, al	;that the subsequent int 10h suceeds
	and	al, 0efh
	mov	es:[10h], al
	pop	es
	pop	bp

	mov	ax,0012h		;Set color graphics
	int	10h

	mov	ax,0F00h		;See if it was set
	int	10h
	cmp	al,12h
	mov	ax,0
	jne	phys_enable_20		;Mode wasn't set up.
	mov	enabled_flag,0FFh	;Show enabled

;	Graphics mode was set up.  Now set up the EGA's
;	registers for the special mode used by this driver.

	mov	ax,cs			;--> palette table
	mov	es,ax
	assumes es,nothing
	mov	dx,InitSegOFFSET palette
	mov	ax,1002h
	int	10h			;Set the palette up

;---------------------------------------------------------------------------;
; now program the DAC, so that index 7 (pallete value 07) has (19,1a,1b) &  ;
; index 8 (palette value 0fh) has (28,29,2a)			            ;
;---------------------------------------------------------------------------;

        mov     cx,2223h
        mov     dx,2100h
	mov	bx,7
	mov	ax,1010h
        int     10h                     ;index 7 = RGB(21,22,23)

        mov     cx,3132h
        mov     dx,3000h
	mov	bx,0fh
	mov	ax,1010h
        int     10h                     ;index 8 = RGB(30,31,32)

;----------------------------------------------------------------------------;

	mov	do_int_30,1		;int 30 call to be done before init
	mov	ax,ScreenSelector	;init_hw_regs will require this
	mov	es,ax
	assumes es,nothing
	call	init_hw_regs
	mov	do_int_30,0		;no more int 30 calls necessary


	assumes	es,EGAMem


;	Check for extra EGA memory.  If present, then one bitmap at a 
;	time can be sent there to speed up saving and restoring.

	xor	cl,cl			;Assume we don't have 256K
	test	ssb_mask,RC_SAVEBITMAP
	jz	phys_enable_15		;Can't use speed-up
	mov	cl,SHADOW_EXISTS	;We've got it

phys_enable_15:
	mov	shadow_mem_status,cl
;----------------------------------------------------------------------------;
; at this point notify kernel that driver is cable of doing a save/restore   ;
; of its state registers and kernel should stop I/O trapping.		     ;
; Do this only if we are in protected mode.				     ;
;----------------------------------------------------------------------------;
	
	test	is_protected,WINP 	;will be set if in protected mode
	jz	phys_enable_ok		;not in protectde mode
	mov	ax,STOP_IO_TRAP		
	int	2fh	      
phys_enable_ok:
	mov	ax,1			;success code

;----------------------------------------------------------------------------;

phys_enable_20:
	ret

physical_enable endp
page

;---------------------------Public-Routine------------------------------;
; physical_disable
;
;   VGA 640x480 graphics mode is exited.  The previous mode of the
;   adapter is restored.
;
; Entry:
;	DS:SI --> int_phys_device
;	ES:    =  Data
; Returns:
;	AX = non-zero to show success
; Error Returns:
;	None
; Registers Preserved:
;	BP
; Registers Destroyed:
;	AX,BX,CX,DX,SI,DI,ES,DS,FLAGS
; Calls:
;	INT 10h
;	init_hw_regs
; History:
;	Tue 18-Aug-1987 18:09:00 -by-  Walt Moore [waltm]
;	Added enabled_flag
;
;	Thu 26-Feb-1987 13:45:58 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,Data

physical_disable proc near

;----------------------------------------------------------------------------;
; disbale the selector here				                     ;
;----------------------------------------------------------------------------;

	push	es			; save
	mov	ax,ScratchSel		; get the scratch selector
	cCall	FreeSelector,<ax>	; free it
	pop	es			;restore ES

;----------------------------------------------------------------------------;

; if we have 43 line mode we must set the 350 scan line mode.

	push	ds
	mov	ax, 40h 		;restore the original equipment flag
	mov	ds, ax			;value in ROM BIOS save area
	mov	al, bEquipmentFlag
	mov	ds:[10h], al
	pop	ds

	xor	ah,ah
	mov	enabled_flag,ah 	;Show disabled

	and	al, 30h
	cmp	al, 30h 		;were we in 80x25 bw text mode?
	jne	restore_color_text	;no

	mov	al, 7			;switch back into 80 by 25 line
	int	10h			;bw text mode.
	jmp	short LineOk

restore_color_text:
	cmp	Line43,43		;43 line mode ?
	jnz	@f			;no.
	mov	ax,1201h		;350 scan line mode
	mov	bl,30h			;set scan lines
	int	10h
@@:
	mov	al,[si].ipd_format
	sub	ah, ah
	int	10h

; restore 43 line mode if it existed at start

	cmp	Line43,0	
	jz	LineOk
	mov	ax,1112h
	mov	bl,0
	int	10h
	mov	ax,0100h
	mov	cx,0607h
	int	10h
LineOk:

;----------------------------------------------------------------------------;
; at this point as the kernel to do the io trapping again, provided we are in;
; protected mode.							     ;
;----------------------------------------------------------------------------;
	
	test	is_protected,WINP	;will be set if we are in prot mode
	jz	phys_disable_ret	;we are in real mode
	mov	ax,START_IO_TRAP
	int	2fh			;start i/o trapping
phys_disable_ret:
;----------------------------------------------------------------------------;
	
	mov	al,1
	ret

physical_disable endp
page

;-----------------------------------------------------------------------;
;	Palette contains the palette values for the EGA card
;	to give the desired colors.  The planes have been
;	changed from the normal EGA plane definition to be
;	C0 = red, C1 = green, C2 = blue.  The intensity bit
;	will be set for all the colors.
;-----------------------------------------------------------------------;

palette label	byte

	db	00h			;Black
	db	0ch			;dark Red
	db	0ah			;dark Green
	db	0eh			;mustard
	db	31h			;dark Blue
	db	15h			;purple 
	db	23h			;turquoise
	db	07h			;gray
	db	0fh			;blend of blue
	db	24h			;Red
	db	12h			;Green
	db	36h			;Yellow
	db	09h			;Blue
	db	2Dh			;Magenta
	db	1Bh			;Cyan
	db	3Fh			;White
	db	0			;Overscan will be black
;
sEnd	InitSeg
page

;-----------------------------------------------------------------------;
;	Color Table contains the color table definition.  The color
;	table is used for the GetColorTable Escape function and for
;	pen and brush enumeration.
;
;	This table must match the palette register values issued to
;	the EGA palette registers to get the colors in the color table.
;
;	The table is also used to take the color index which is
;	created for a GetPixel and turn it into a color which
;	sum_RGB_colors_alt can deal with.
;-----------------------------------------------------------------------;


COLOR_TBL_SIZE	equ	16		;16 entries in the table


createSeg _PIXEL,PixelSeg,word,public,CODE
sBegin	PixelSeg

PixelSeg_color_table	label	dword

;		dd	xxbbggrr
 		dd	00000000h	;Black
		dd	00000080h	;Dark Red
		dd	00008000h	;Dark Green
		dd	00008080h	;Mustard
		dd	00800000h	;Dark Blue
		dd	00800080h	;Purple
		dd	00808000h	;Turquoise
		dd	00808080h	;dark Gray
		dd	00c0c0c0h	;light gray
		dd	000000ffh	;Red
		dd	0000ff00h	;Green
		dd	0000ffffh	;Yellow
		dd	00ff0000h	;Blue
		dd	00ff00ffh	;Magenta
		dd	00FFFF00h	;Cyan
		dd	00ffffffh	;White

sEnd	PixelSeg
page

createSeg _BLUEMOON,BlueMoonSeg,word,public,CODE
sBegin	BlueMoonSeg

BlueMoonSeg_color_table label	dword

;		dd	xxbbggrr
		dd	00000000h	;Black
		dd	00000080h	;Dark Red
		dd	00008000h	;Dark Green
		dd	00008080h	;Mustard
		dd	00800000h	;Dark Blue
		dd	00800080h	;Purple
		dd	00808000h	;Turquoise
		dd	00808080h	;dark Gray
		dd	00c0c0c0h	;light gray
		dd	000000ffh	;Red
		dd	0000ff00h	;Green
		dd	0000ffffh	;Yellow
		dd	00ff0000h	;Blue
		dd	00ff00ffh	;Magenta
		dd	00FFFF00h	;Cyan
		dd	00ffffffh	;White

sEnd	BlueMoonSeg


sBegin	Code
assumes cs,Code



;-----------------------------------------------------------------------;
;	This is the same physical device structure as defined above
;	except that the number of scan lines has been increased to
;	encompass shadow memory.  We have to do this for the restore
;	operation of the SaveScreenBitmap function, because BitBLT
;	clips the source to the device extents,	meaning that the entire
;	saved rectangle would be lost.
;
;       On a VGA we have usable shadow memory from 0c000H to 0FFFFh. Starting
;	at offset 0, offset 0c030h is start of 615th scan and the extent of
;	the save area will be from 0c030h to 0ffffh or 204 scans.
;       Total no of scans in this fake device will be 615 + 204 = 819.
;
;	New vertical dimension of VGA = 615 + 204 = 819
;-----------------------------------------------------------------------;


SSB_START_SCAN	equ	615
SSB_TOTAL_SCANS equ	204
NH		equ	SSB_START_SCAN + SSB_TOTAL_SCANS;new display height

ssb_device BITMAP <SCRSEL,W,NH,WB,P,B,0A0000000H,0,0,0,0,0,0,0>
;-----------------------------------------------------------------------;
;	Palette contains the palette values for the EGA card
;	to give the desired colors.  The planes have been
;	changed from the normal EGA plane definition to be
;	C0 = red, C1 = green, C2 = blue.  The intensity bit
;	will be set for all the colors.
;-----------------------------------------------------------------------;

Code_palette label	byte

	db	00h			;Black
	db	0ch			;dark Red
	db	0ah			;dark Green
	db	0eh			;mustard
	db	31h			;dark Blue
	db	15h			;purple 
	db	23h			;turquoise
	db	07h			;gray
	db	0fh			;blend of blue
	db	24h			;Red
	db	12h			;Green
	db	36h			;Yellow
	db	09h			;Blue
	db	2Dh			;Magenta
	db	1Bh			;Cyan
	db	3Fh			;White
	db	0			;Overscan will be black

public	PMatchTable1
public	PMatchTable2
public	PMatchTable3
public	PIndexTable1
public	PIndexTable2
public	PIndexTable3
public	PAccelTable
public	PColorTable
public	NUMBER_CL1_COLORS
public	NUMBER_CL2_COLORS
public	NUMBER_CL3_COLORS

NUMBER_CL1_COLORS	equ	3
NUMBER_CL2_COLORS	equ	3
NUMBER_CL3_COLORS	equ	4

PMatchTable1	label	byte
PMatchTable2	label	byte
	db	0
	db	80h
	db	0ffh

PMatchTable3	label	byte
	db	0
	db	80h
	db	0c0h
	db	0ffh

PIndexTable1	label	byte
	db	0
	db	1
	db	9

PIndexTable2	label	byte
	db	0
	db	3
	db	0bh

PIndexTable3	label	byte
	db	0
	db	7
	db	8
	db	0fh

PAccelTable label   byte
	db	20h			;black
	db	00h
	db	00h
	db	00h
	db	00h
	db	00h
	db	00h
	db	00h
	db	10h			;light grey
	db	00h			;red
	db	10h			;green
	db	10h			;yellow
	db	00h			;blue
	db	10h			;magenta
	db	10h			;cyan
	db	30h			;white

PColorTable label   byte
	db	0, 0, 0
	db	80h, 0, 0
	db	0, 80h, 0
	db	80h, 80h, 0
	db	0, 0, 80h
	db	80h, 0, 80h
	db	0, 80h, 80h
	db	80h, 80h, 80h
	db	0c0h, 0c0h, 0c0h
	db	0ffh, 0, 0
	db	0, 0ffh, 0
	db	0ffh, 0ffh, 0
	db	0, 0, 0ffh
	db	0ffh, 0, 0ffh
	db	0, 0ffh, 0ffh
	db	0ffh, 0ffh, 0ffh

;----------------------------------------------------------------------------;
; define two hack routines for EGA color tranalation. The first one is a     ;
; NOP for VGA, and the next one builds the color planes of a solid brush with;
; index 8.								     ;
; (these operate differently under EGA.					     ;
;----------------------------------------------------------------------------;
	
	public	EGAindex8to7
	public	RGB192Brush

EGAindex8to7	proc near

	ret

EGAindex8to7	endp

RGB192Brush	proc near

	xor	ax,ax			;value for the 3 color planes
	mov	cx,12			;12 words for 3 planes
	rep	stosw			;fill in the color planes
	dec	ax			;intensity is all 0's
	mov	cx,4			;4 word here
	rep	stosw			;do the intensity plane
	ret

RGB192Brush	endp
;----------------------------------------------------------------------------;

sEnd	Code
end

