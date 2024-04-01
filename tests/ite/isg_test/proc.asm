;-----------------------------------------------------------------------------;
; DATE/TIME ROUTINES                                                          :
;   This module contains the assembly routines for retrieving the date and/or :
;   time from MS-DOS.  These routines are declared FAR so that they may be    :
;   called from other segments.                                               :
;                                                                             :
; AUTHOR : Christopher Williams                                               :
; DATE   : April 07, 1989                                                     :
; SEGMENT: _TEXT                                                              :
;                                                                             :
; HISTORY: Apr 07, 1989 - created.                                            :
;                                                                             :
;-----------------------------------------------------------------------------;

.xlist
include cmacros.inc
memS    EQU     1
?PLM=1
?WIN=1
.list

assumes cs,CODE
assumes ds,nothing

sBegin  CODE
;-----------------------------------------------------------------------------;
; ABSOLUTE VALUE                                                              ;
;                                                                             ;
;   LPSTR FAR StringCopy(LPSTR,LPSTR);                                        ;
;                                                                             :
;      AFFECTS                                                                ;
;        (1) buffer pointed to by pDest.                                      ;
;                                                                             ;
; CALLED ROUTINES                                                             ;
;   -none-                                                                    ;
;                                                                             ;
; PARAMETERS                                                                  ;
;   LPSTR pSource - long pointer to source string (null terminated).          ;
;   LPSTR pDest   - long pointer to buffer to store string.                   ;
;                                                                             ;
; GLOBAL VARIABLES                                                            ;
;   -none-                                                                    ;
;                                                                             ;
; RETURNS                                                                     ;
;   LPSTR - beginning of destination string.                                  ;
;-----------------------------------------------------------------------------;
cProc AbsoluteValue,<FAR,PUBLIC>,<si,di>
     parmW pValue
cBegin
     mov  ax,pValue
     cwd
     xor  ax,dx
     neg  dx
     jnc  done
     inc  ax
done:
cEnd

;-----------------------------------------------------------------------------;
; STRING COPY                                                                 ;
;   This routine copies a string from pSource to pDest.  It first calc's the  ;
;   number of characters in the string plus the null terminator char.  Then   ;
;   copies the string to the pDest and returns the beginning of the string.   ;
;                                                                             ;
;   LPSTR FAR StringCopy(LPSTR,LPSTR);                                        ;
;                                                                             :
;      AFFECTS                                                                ;
;        (1) buffer pointed to by pDest.                                      ;
;                                                                             ;
; CALLED ROUTINES                                                             ;
;   -none-                                                                    ;
;                                                                             ;
; PARAMETERS                                                                  ;
;   LPSTR pSource - long pointer to source string (null terminated).          ;
;   LPSTR pDest   - long pointer to buffer to store string.                   ;
;                                                                             ;
; GLOBAL VARIABLES                                                            ;
;   -none-                                                                    ;
;                                                                             ;
; RETURNS                                                                     ;
;   LPSTR - beginning of destination string.                                  ;
;-----------------------------------------------------------------------------;
cProc StringCopy,<FAR,PUBLIC>,<cx,si,di>
     parmD pDest
     parmD pSource
cBegin
        cld                             ; Clear direction - good practice.

        les   di,pSource                ; Find the number of chars in string.
        xor   ax,ax                     ;       .       .       .       .
        mov   cx,0FFFFh                 ;       .       .       .       .
        repne scasb                     ;       .       .       .       .
        not   cx                        ; cx=size+1 -> account for null term.

        lds si,pSource                  ; Load string pointers and copy string.
        les di,pDest                    ;
        push es                         ; Save pDest
        push di                         ;
        rep movsb                       ; Copy ...

        pop ax                          ; return the beginning of pDest.
        pop dx                          ;

cEnd
sEnd    CODE
end
