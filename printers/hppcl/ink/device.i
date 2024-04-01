; **[f******************************************************************
; * device.i - 
; *
; * Copyright (C) 1988-1989 Aldus Corporation
; * Copyright (C) 1988-1990 Microsoft Corporation.
; *			    All rights reserved.
; * Company confidential.
; *
; **f]*****************************************************************/

;/*********************************************************************
; *
; *  05 sep 89	peterbe	Added epLineBuf to DEVICE.
; *   1-18-89	 jimmat  Now space for epBuf is only allocated if the printer
; *			 is in landscape mode.
; */

NAME_LEN	equ	32			;These constants
SPOOL_SIZE	equ	2048			;   are defined in
MAX_BAND_WIDTH	equ	3510			;   HPPCL.H
BYTESIZE	equ	8

DEV_LAND	equ	8889H			;defined in DEVICE.H

HPJET		equ	0001H			;defined in RESOURCE.H
NOBITSTRIP	equ	0040H
DINA4		equ	21
B5		equ	23

NULL		equ	0			;These constants
TRUE		equ	1			;   are defined in
FALSE		equ	0			;   PRINTER.H
SUCCESS		equ	0
OPTIONS_DPTEKCARD equ 1				;defined in RESOURCE.H
OPTIONS_RESETJOB  equ 2
OPTIONS_FORCESOFT equ 4
OPTIONS_VERTCLIP  equ 8

FAILED		equ	-1			;defined here only for DUMP.A
NUMARRAYS	equ	24			; increased from 10 for LJ2000
MINBYTES	equ	32


PAPERFORMAT		struc			;This data structure
						;   is defined in
	xPhys		dw	0		;   DEVICE.H
	yPhys		dw	0
	xImage		dw	0
	yImage		dw	0
	xPrintingOffset	dw	0
	yPrintingOffset	dw	0
	select		db	16 	dup (0)

PAPERFORMAT		ends


JUSTBREAKREC		struc			;This data structure
						;   is defined in
	extra		dw	0		;   DEVICE.H
	rem		dw	0
	err		dw	0
	count		dw	0
	ccount		dw	0

JUSTBREAKREC		ends


DEVICE			struc			;This data structure
						;   is defined in
	epType		dw	0		;   DEVICE.H
	epBmpHdr	db	SIZE BITMAP		dup(0)
	epPF		db	SIZE PAPERFORMAT	dup(0)
	ephDC		dw	0
	epMode		dw	0
	epNband		dw	0
	epXOffset	dw	0
	epYOffset	dw	0
	epJob		dw	0
	epDoc		dw	0
	epPtr		dw	0
	epXerr		dw	0
	epYerr		dw	0
	ephMd		dw	0
	epECtl		dw	0
	epCurx		dw	0
	epCury		dw	0
	epNumBands	dw	0
	epLastBandSz	dw	0
	epScaleFac	dw	0
	epCopies	dw	0
	epTray		dw	0
	epPaper		dw	0
	epFontSub	db	0
	epPgSoftNum	dw	0
	epTotSoftNum	dw	0
	epMaxPgSoft	dw	0
	epMaxSoft	dw	0
	epGDItext	db	0
	epOpaqText	db	0
	epGrxRect	db	SIZE RECT		dup (0)
	epCaps		dw	0
	epOptions	dw	0
	epDuplex	dw	0
	epPageCount	dw	0
	epAvailMem	dd	0
	epFreeMem	dd	0
	epTxWhite	dw	0
	epHFntSum	dw	0		;HANDLE from PRINTER.H
	epLPFntSum	dd	0		;LPSTR  from PRINTER.H
	epJust		dw	0		;JUSTBREAKTYPE
	epJustWB	db	SIZE JUSTBREAKREC	dup (0)
	epJustLTR	db	SIZE JUSTBREAKREC	dup (0)
	epHWidths	dw	0		;HANDLE from PRINTER.H
	epDevice	db	NAME_LEN		dup (0)
	epPort		db	NAME_LEN		dup (0)
	epSpool		db	SPOOL_SIZE		dup (0)
	epBuf		dw	0		; was a buffer, now an OFFSET
	epLineBuf	dw	0		; special graphics buffer
	epBmp		db	0

DEVICE			ends


ESCTYPE			struc			;This data structure
						;   is defined in HPPCL.H
	escx		db	0		;esc is a "reserved" word in
	start1		db	0		;  5.0 asm. expanded to escx.
	start2		db	0
	num		db	10	dup (0)

ESCTYPE			ends

ESCtypeSIZE	equ	SIZE ESCTYPE


POSARRAY	struc				;defined here only for DUMP.A

	startpos	dw	0
	endpos		dw	0

POSARRAY	ends

PosArraySIZE	equ	SIZE POSARRAY
