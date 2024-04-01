page		60, 132
title		Library Initialisation
;===============================================================================
;		Filename	LOADLIB.ASM
;		Copyright	(C) 1989 by Research Machines
;		Copyright	(C) 1989, 1990 Microsoft Corp.
;
;===============================================================================
; REVISIONS:	24/02/1989	Initial version
;===============================================================================

		memM	equ	1			; Middle memory model
		?WIN	=	1			; Windows prolog/epilog
		?PLM	=	1			; Pascal calling convention


		.xlist
include 	cmacros.inc
include 	windows.inc
include 	wnet.inc
		.list

externFP	PostWarning

;===============================================================================
; ============= DATA SEGMENT ===================================================
;===============================================================================

sBegin		DATA

globalW 	hLibraryModule, 0, 1

sEnd		DATA

;===============================================================================
; ============= CODE SEGMENT ===================================================
;===============================================================================

sBegin		CODE
		assumes CS, CODE
		assumes DS, DATA

;===============================================================================
subttl		LibraryInitialisation
page
;===============================================================================

cProc		LibraryInit, <FAR, PUBLIC, NODATA>

cBegin
		mov	hLibraryModule, di

		mov	ax,00FFh      ; check if network installed
		int	2Ah
		mov	al,ah	      ; 0 in AH iff net not installed
		cmp	ax,0
		jnz	li_exit

		cCall	PostWarning

		sub	ax,ax		; return FALSE to unload
li_exit:
cEnd

cProc		WEP, <FAR, PUBLIC, NODATA>

;   parmW	fLeaving

cBegin	<nogen>

    xor     ax, ax
    retf    2

cEnd	<nogen>

;===============================================================================
; ============= END OF LOADLIB =================================================
;===============================================================================

sEnd		CODE

		end	LibraryInit
