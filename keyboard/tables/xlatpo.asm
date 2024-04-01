; ****** XLATPO.ASM ************************************************
;	Contains XLATPO.INC
; ******************************************************************

include cmacros.inc

sBegin CODE

assumes CS,CODE
; assumes DS,DATA

	public	xlatBeg, xlatSize

xlatSize	dw	CODEoffset xlatEnd - CODEoffset xlatBeg

    xlatBeg label byte

	include xlatpo.inc

    xlatEnd label byte

include date.inc

sEnd CODE

end
