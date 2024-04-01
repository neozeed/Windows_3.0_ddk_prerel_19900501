        title   lmemcpy.asm

;***************************************************************
;*
;*  PSCRIPT.DRV
;*
;*  void FAR PASCAL lmemcpy(lpbDst, lpbSrc, cb);
;*
;****************************************************************

?WIN = 1

.xlist
include cmacros.inc
.list

sBegin  CODE

assumes CS,CODE
assumes DS,DATA

cProc   lmemcpy,<PUBLIC,FAR>,<di,si,ds>
        parmD   lpDest
        parmD   lpSrc
        parmW   cnt
cBegin
        les     di,lpDest
        lds     si,lpSrc
        mov     cx,cnt
        repne   movsb
cEnd

sEnd    CODE

end



