;----------------------------------------------------------------------------;
; Copyright (C) Microsoft Corporation 1985-1990. All Rights Reserved.        ;
;----------------------------------------------------------------------------;

FontSeg		segment at 0
fsVersion		dw	0
fsSize			dd	0
fsCopyright		db	60 dup (0)
fsType			dw	0	; Type field for the font
fsPoints		dw	0	; Point size of font
fsVertRes		dw	0	; Vertical digitization
fsHorizRes		dw	0	; Horizontal digitization
fsAscent		dw	0	; Baseline offset from char cell top
fsInternalLeading	dw	0	; Internal leading included in font
fsExternalLeading	dw	0	; Prefered extra space between lines
fsItalic		db	0	; Flag specifying if italic
fsUnderline		db	0	; Flag specifying if underlined
fsStrikeOut		db	0	; Flag specifying if struck out
fsWeight		dw	0	; Weight of font
fsCharSet		db	0	; Character set of font
fsPixWidth		dw	0	; Width field for the font
fsPixHeight		dw	0	; Height field for the font
fsPitchAndFamily	db	0	; Flag specifying pitch and family
fsAvgWidth		dw	0	; Average character width
fsMaxWidth		dw	0	; Maximum character width
fsFirstChar		db	0	; First character in the font
fsLastChar		db	0	; Last character in the font
fsDefaultChar		db	0	; Default character for out of range
fsBreakChar		db	0	; Character to define wordbreaks
fsWidthBytes		dw	0	; Number of bytes in each row
fsDevice		dd	0	; Offset to device name
fsFace			dd	0	; Offset to face name
fsBitsPointer		dd	0	; Bits pointer
fsBitsOffset		dd	0	; Offset to the begining of the bitmap
			even		; Word alignment for the offset table
fsFlags 		dd	0	; Bit Blags
fsAspace		dw	0	; Global A space, if any
fsBspace		dw	0	; Global B space, if any
fsCspace		dw	0	; Global C space, if any
fsColorPointer		dd	0	; offset to color table, if any
fsReserved		dd	4 dup(0)
fsCharOffset		db	0	; Area for storing the character offsets
FontSeg		ends