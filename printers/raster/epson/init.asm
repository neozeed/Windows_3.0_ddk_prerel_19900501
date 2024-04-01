;
;   Startup code for DLL
;

memS = 1
?PLM = 1
?WIN = 1
?TF  = 1

.xlist
include cmacros.inc
.list

public __acrtused		    ; keep c lib stuff out.
__acrtused equ 0CACh

sBegin data

externW hInstance

sEnd

sBegin code

assumes cs,code
assumes ds,data

__astart proc far

    ;
    ;	conditions:
    ;	    DI - module handle
    ;	    DS - data segment
    ;	    CX - heap size
    ;	    ES:SI - command line
    ;

    mov     hInstance, di	   ; remember our instance handle
    mov     ax,1

    ;
    ;	if the driver uses a heap, add call to LocalInit()
    ;

    ret 			    ; return TRUE to kernel => we loaded ok.

__astart endp

sEnd

end __astart
