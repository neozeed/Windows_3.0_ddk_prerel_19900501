INCLUDE cmacros.inc
memS    EQU     1
?PLM=0
?WIN=1

sBegin  CODE
assumes CS,CODE
assumes DS,DGROUP
assumes ES,NOTHING

cProc FindBitmapBit,<FAR,PUBLIC>,<bx,cx,dx,si,di>
     parmD <pMem>
     parmW <iOffset>
cBegin
        lds si,pMem                     ; Set Memory address start

        mov bl,byte ptr[si]             ;
        mov cl,8
        shl bx,cl
        mov cx,iOffset                  ;
        shl bx,cl                       ; shift bits (planes) for result

        jnc NotSet                      ; if not set then return
        mov ax,1                        ;
        jmp Set                         ;

NotSet:
        xor ax,ax                       ;

Set:

cEnd

cProc CheckROPBit,<FAR,PUBLIC>,<bx,cx,dx,si,di>
     parmW <bSrc,bDst,bPat,bPix,wROP>
cBegin
        mov cl,2
        mov bx,bPat
        shl bx,cl
        mov dx,bx
        mov bx,bSrc
        shl bx,1
        add dx,bx
        mov bx,bDst
        add dx,bx

        mov cx,dx
        inc cx
        mov bx,wROP
        shr bx,cl
        jc  ROPSet
        xor ax,ax
        jmp ROPComp

ROPSet:
        mov ax,1

ROPComp:
        mov bx,bPix
        cmp bx,ax
        jz  ROPMatch
        xor ax,ax
        jmp ROPDone

ROPMatch:
        mov ax,1

ROPDone:

cEnd
sEnd    CODE
end
