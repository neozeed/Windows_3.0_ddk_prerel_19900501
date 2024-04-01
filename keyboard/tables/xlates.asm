; ****** XLATES.ASM ************************************************
;	Contains XLATES.INC
; ******************************************************************

include cmacros.inc

sBegin CODE

assumes CS,CODE
; assumes DS,DATA

	public	xlatBeg, xlatSize

xlatSize	dw	CODEoffset xlatEnd - CODEoffset xlatBeg

    xlatBeg label byte

	include xlates.inc	; Olivetti Spain II ROM charset

    xlatEnd label byte

include date.inc

sEnd CODE

end
