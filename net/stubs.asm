;
;   WINNET.ASM
;
;   Access to WINNET calls via user
;
;

memS=1
?WIN=1
?PLM=1

.xlist
include cmacros.inc
.list

WN_NOT_SUPPORTED    equ 1		; returned if no function exported

sBegin code
assumes cs,code

;-----------------------
;   NetStub
;
;   All we need to do is set ax to not supported and return, popping the
;   parameters
;
;   ?po gets set to zero in order to avoid a WHOLE LOT of "possible invalid
;   use of nogen" warning messages.
;
;   Realize that this is, after all, a hack, the purpose of which is to
;   reduce code.
;
NetStub macro

__pop	=   ?po
?po	=   0

&cBegin <nogen>

    mov     ax, WN_NOT_SUPPORTED
    ret     __pop

&cEnd <nogen>

endm


;-----------------
;   WNetHoldJob
;
cProc WNetHoldJob , <FAR, PUBLIC>

    parmD szQueue
    parmW idJob

NetStub

;--------------------
;   WNetReleaseJob
;
cProc WNetReleaseJob , <FAR, PUBLIC>

    parmD szQueue
    parmW idJob

NetStub

;---------------------
;   WNetCancelJob
;
cProc WNetCancelJob , <FAR, PUBLIC>

    parmD szQueue
    parmW idJob

NetStub

;--------------------
;   WNetSetJobCopies
;
cProc WNetSetJobCopies , <FAR, PUBLIC>

    parmD szQueue
    parmW idJob
    parmW nCopies

NetStub

;--------------------
;   WNetBrowseDialog
;
cProc WNetBrowseDialog , <FAR, PUBLIC>

    parmW   hwnd
    parmW   nFunction
    parmD   szPath
    parmD   lpnSize

NetStub

;------------------------
;   WNetGetError
;
cProc WNetGetError , <FAR, PUBLIC>

    parmD   lpnError

NetStub

;------------------------
;   WNetGetErrorText
;
cProc WNetGetErrorText , <FAR, PUBLIC>

    parmW   nError
    parmD   lpBuffer
    parmD   lpnSize

NetStub

;----------------------
;   WNetAbortJob
;
cProc WNetAbortJob , <FAR, PUBLIC>

    parmD   lpszQueue
    parmW   fh

NetStub

;----------------------
;   WNetEnable
;
cProc	WNetEnable , <FAR, PUBLIC>

NetStub

;-------------------------
;   WNetDisable
;
cProc	WNetDisable, <FAR, PUBLIC>

NetStub

sEnd code

end
