;----------------------------------------------------------------------------;
; This file redirects the ExtTextOut and StrBlt calls to appropriate real or ;
; protected mode routine.						     ;
;----------------------------------------------------------------------------;


	.xlist
	include	cmacros.inc
	.list

sBegin	Data
sEnd	Data

	public	ExtTextOut
	public	StrBlt

	
sBegin	Code

	externD	ExtTextOutFunction
	externD	StrBltFunction
	

ExtTextOut  proc  far

	assumes	cs,Code
	assumes	ds,nothing
	assumes	es,nothing

	jmp	dword ptr [ExtTextOutFunction]

ExtTextOut  endp


StrBlt	proc	far

	jmp	dword ptr [StrBltFunction]

StrBlt	endp

sEnd 	Code

end

