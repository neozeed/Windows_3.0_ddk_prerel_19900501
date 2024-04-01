; ****** XLATIC.ASM ************************************************
;	Contains XLATIC.INC for Iceland
; ******************************************************************

include cmacros.inc

sBegin CODE

assumes CS,CODE
; assumes DS,DATA

	public	xlatBeg, xlatSize

xlatSize	dw	CODEoffset xlatEnd - CODEoffset xlatBeg

    xlatBeg label byte

	include xlatIC.inc

    xlatEnd label byte

include date.inc

sEnd CODE

end
