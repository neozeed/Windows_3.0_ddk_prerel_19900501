;-----------------------------------------------------------------------------;
; TESTING DynaLink Entry Module                                               ;
;                                                                             ;
; AUTHOR : Christopher Williams                                               ;
; DATE   : Jan 05, 1990                                                       ;
; SEGMENT: _TEXT                                                              ;
;                                                                             ;
;-----------------------------------------------------------------------------;

.xlist
include cmacros.inc
?PLM=1
?WIN=1
memS EQU 1
assumes cs,CODE
.list

externFP <LocalInit>
externFP <__acrtused>
externFP <LibMain>

sBegin  CODE
;-----------------------------------------------------------------------------;
; TESTING DynaLink Entry Point                                                ;
;                                                                             ;
; CALLED ROUTINES                                                             ;
;   LocalInit() - Kernel routine to initialize the Local Heap for the library.;
;                                                                             ;
; PARAMETERS                                                                  ;
;   DI - Instance Handle for the module.                                      ;
;   DS - Segement Address for the Library Data-Seg.                           ;
;   CX - Size of the Local Heap.                                              ;
;                                                                             ;
; GLOBAL VARIABLES                                                            ;
;   HANDLE hInst - Instance handle for the library.                           ;
;                                                                             ;
; RETURNS                                                                     ;
;   int - returns 0 if the heap cannot be initialized, or it returns the      ;
;         value from the LocalInit.                                           ;
;-----------------------------------------------------------------------------;
cProc LoadLib,<FAR,PUBLIC>,<si,di>
cBegin
     jcxz LoadLibDone                   ; If Heap == 0, return 0

     xor ax,ax                          ; Clear return register
     push ds                            ; Setup stack for the call to the
     push ax                            ;   LocalInit() function.
     push cx                            ;
     call LocalInit                     ; Call LocalInit()
     or   ax,ax                         ;
     jz   LoadLibDone;                  ;

     push di                            ; Setup stack for the call to the
     push ds                            ;   LibMain() function.
     push cx                            ;
     push es                            ;
     push si                            ;
     call LibMain                       ; Call Libmain()

LoadLibDone:                            ; Return AX to Windows
cEnd

sEnd CODE
end LoadLib
