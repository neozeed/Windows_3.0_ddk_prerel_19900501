	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	CURSOR.INC
;
; This file contains definitions for the cursor requirements of
; the EGAHires Display Driver.
;
; Created: 23-Feb-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.


CUR_HEIGHT	equ	32			;Height of cursor/icon
MAX_BUF_HEIGHT  equ     64                      ;Number of scans in work buffer

ifdef VRAM800
MEMORY_WIDTH    equ     800
MEM_BEG_OFFSET  equ     0
SCREEN_WIDTH    equ     800                     ;Screen width in pixels
SCREEN_HEIGHT   equ     600                     ;Screen height in pixels
SCAN_BYTES      equ     800                     ;# bytes in one scanline
V7VGA           equ     1                       ;optimize for V7 VGA
endif

ifdef VRAM480
MEMORY_WIDTH	equ	1024
MEM_BEG_OFFSET	equ	0
SCREEN_WIDTH	equ	640			;Screen width in pixels
SCREEN_HEIGHT	equ	480			;Screen height in pixels
SCAN_BYTES	equ	1024			;# bytes in one scanline
V7VGA           equ     1                       ;optimize for V7 VGA
endif

ifdef VRAM512
MEMORY_WIDTH	equ	1024
MEM_BEG_OFFSET	equ	0
SCREEN_WIDTH	equ	720			;Screen width in pixels
SCREEN_HEIGHT	equ	512			;Screen height in pixels
SCAN_BYTES	equ	1024			;# bytes in one scanline
V7VGA           equ     1                       ;optimize for V7 VGA
endif

ifdef VRAM768
MEMORY_WIDTH	equ	1024
MEM_BEG_OFFSET	equ	0
SCREEN_WIDTH	equ	1024			;Screen width in pixels
SCREEN_HEIGHT	equ	768			;Screen height in pixels
SCAN_BYTES	equ	1024			;# bytes in one scanline
V7VGA           equ     1                       ;optimize for V7 VGA
endif

ifdef VGA200
MEMORY_WIDTH    equ     320
MEM_BEG_OFFSET	equ	0
SCREEN_WIDTH    equ     320                     ;Screen width in pixels
SCREEN_HEIGHT   equ     200                     ;Screen height in pixels
SCAN_BYTES      equ     320                     ;# bytes in one scanline
IBMVGA          equ     1
endif

ifdef VRAM240
MEMORY_WIDTH    equ     512
MEM_BEG_OFFSET	equ	0
SCREEN_WIDTH    equ     320                     ;Screen width in pixels
SCREEN_HEIGHT   equ     240                     ;Screen height in pixels
SCAN_BYTES      equ     512                     ;# bytes in one scanline
V7VGA           equ     1                       ;optimize for V7 VGA
endif

SCAN_CHAR	equ	SCREEN_WIDTH/8		;# characters across screen
CUR_ICON_WIDTH	equ	4			;Width in bytes of cursor/icon
SAVE_WIDTH	equ	CUR_ICON_WIDTH+1	;Width of save area
BUF_WIDTH	equ	SAVE_WIDTH+CUR_ICON_WIDTH  ;Width of work buffer
SCAN_INC	equ	SCAN_CHAR-BUF_WIDTH	;Index to next scan line
MASK_LENGTH	equ	SAVE_WIDTH*CUR_HEIGHT	;#bytes in AND/XOR mask
FARTHEST	equ	(CUR_ICON_WIDTH+1)*8	;How far apart in X old and
						;  new cursor may be for
						;  both to fit in work buffer

X_RATE		equ	1			;X mickeys/pixel rate
Y_RATE		equ	2			;Y mickeys/pixel rate


INIT_CURSOR_X	equ	SCREEN_WIDTH/2		;Initial cursor x
INIT_CURSOR_Y	equ	SCREEN_HEIGHT/2 	;Initial cursor x


CUR_ROUND_LEFT	equ	0FFF0h			;Round down to a word
CUR_ROUND_RIGHT equ	0000Fh			;Round up   to a word

