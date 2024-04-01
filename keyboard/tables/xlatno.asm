; ****** XLATNO.ASM ************************************************
;	Contains XLATNO.INC
; ******************************************************************

include cmacros.inc

sBegin CODE

assumes CS,CODE
; assumes DS,DATA

	public	xlatBeg, xlatSize

xlatSize	dw	CODEoffset xlatEnd - CODEoffset xlatBeg

    xlatBeg label byte

	include xlatno.inc

    xlatEnd label byte

include date.inc

sEnd CODE

end
