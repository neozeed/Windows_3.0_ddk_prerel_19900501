;
;   strblt2 -
;
;   This file contains a color version of dmStrBlt() which is designed to
;   work around the font file format changes that have occurred without
;   the color library being updated.  The real (optimal) solution is to
;   rewrite dmStrBlt() in accordance with the latest format, but this is
;   a simple if slower solution.
;
;   THIS SHOULD BE FIXED AS SOON AS SOMEONE HAS TIME SO WE HAVE A REAL
;   COLOR STRBLT!!!!!
;

memS = 1
?plm = 1
?win = 1

incDrawmode = 1 		; required for gdidefs

.xlist
include cmacros.inc
include gdidefs.inc
.list

rgb struc
    Red     db	?
    Green   db	?
    Blue    db	?
rgb ends

externFP    gdi_dmStrBlt	; GDI's monochrome dmStrBlt()

sBegin code

assumes cs,code
assumes ds,data

;
;   dmStrBlt -
;
;   Dot matrix (banding bitmap) support string blt.  Formats a string
;   into a bitmap using the specified GDI raster font.	See the device
;   driver kit documentation for more detailed specification.
;
;   The dmStrBlt in GDI is a monochrome-only version.  This version will
;   support monochrome as well as 3-plane RGB format bitmaps by calling
;   the monochrome strblt for each of the three planes.
;
;   Assumes the physical colors are three-byte RGB's restricted to each
;   color being 0 or 0FFh.  Also assumes that the banding bitmap is smaller
;   than 64K.
;

cProc	dmStrBlt, <FAR, PUBLIC>, <si, di>

    parmD   lpDestDev		; destination bitmap
    parmW   DestXOrg		; x position at which to print
    parmW   DestYOrg		; y position
    parmD   lpClipRect		; pointer to clipping rectangle
    parmD   lpString		; pointer to string
    parmW   count		; count of chars, <0 if getting extent
    parmD   lpFont		; pointer to raster font
    parmD   lpDrawMode		; DC information
    parmD   lpTextXForm 	; font variations

    localW  RedSave		; fg (hibyte) and bg (lo) colors, red
    localW  GreenSave		; green component
    localW  BlueSave		; blue component
    localW  BrkErrSave		; break error

    localV  rcClip, <size RECT>

cBegin

    mov     ax, count
    or	    ax,ax		; if negative, only getting extent,
    js	    sb_passthru 	; so just call ordinary dmStrBlt

    les     bx, lpDestDev	; get pointer to destination
    cmp     word ptr es:bmPlanes[bx], 0101h	; monochrome?
    je	    sb_passthru 	; if so, call ordinary dmStrBlt

    cmp     word ptr es:bmPlanes[bx], 0103h	; RGB?
    je	    sb_color

sb_fail:
    sub     ax, ax
    mov     dx, 8000h		; if error, return 8000:0000
    jmp     sb_exit

sb_passthru:

    mov     sp,bp		; assumes si, di, ds not changed.
    pop     bp
    dec     bp			; since far call
    jmp     gdi_dmStrBlt

sb_color:

    ;	Fake a monochrome bitmap out of the color bitmap
    ;
    mov     word ptr es:bmPlanes[bx], 0101h	; make monochrome

    mov     dx, word ptr es:bmHeight[bx]	; triple hieght
    shl     dx,1
    add     word ptr es:bmHeight[bx], dx
    shr     dx,1				; and save original

    ;	Copy the clipping rectangle,
    push    ds
    push    es
    push    ss
    pop     es
    lea     di, rcClip

    lds     si, lpClipRect
    mov     cx, ds
    or	    cx, si				; is there a clip rect?
    jz	    sb_defcliprect			; no, default one
    cld
    mov     cx, 4
    rep     movsw				; copy the clip rect
    jmp     short sb_clipdone

sb_defcliprect:
    sub     ax,ax				; 0 goes in
    stosw					; left
    stosw					; top
    pop     ds					; ds -> bitmap
    push    ds					; save again
    mov     ax, ds:bmWidth[bx]			; get width
    stosw					; to right
    mov     ax,dx				; hieght
    stosw					; in bottom

sb_clipdone:
    pop     es					; restore seg of bitmap
    pop     ds					; restore dgroup

sb_savehieght:
    mov     di, dx				; height -> di

    ;	Save the background and foreground colors in the drawmode
    ;
    les     bx,lpDrawMode
    mov     al, byte ptr es:bkColor[bx][Green]
    mov     ah, byte ptr es:textColor[bx][Green]
    mov     GreenSave, ax
    mov     al, byte ptr es:bkColor[bx][Blue]
    mov     al, byte ptr es:textColor[bx][Blue]
    mov     BlueSave, ax
    mov     al, byte ptr es:bkColor[bx][Red]
    mov     ah, byte ptr es:textColor[bx][Red]
    mov     RedSave, ax

    ;	save the break error so it can be restored
    mov     ax, es:BreakErr[bx]
    mov     BrkErrSave, ax

    ;	now set the colors for the red plane
    mov     byte ptr es:bkColor[bx][Green], al
    mov     byte ptr es:bkColor[bx][Blue], al
    mov     byte ptr es:textColor[bx][Green], ah
    mov     byte ptr es:textColor[bx][Blue], al

    call    realstrblt
    or	    ax,ax
    jnz     sb_dogreen
    cmp     dx, 8000h
    jz	    sb_restore

sb_dogreen:
    ; now set up for green plane
    add     DestYOrg, di			; offset Y by bitmap hieght
    add     word ptr rcClip[top], di		; offset clip rect top
    add     word ptr rcClip[bottom], di 	; offset clip rect bottom

    ; get green color.
    mov     ax, GreenSave
    les     bx, lpDrawMode
    mov     byte ptr es:bkColor[bx][Red], al
    mov     byte ptr es:bkColor[bx][Blue], al
    mov     byte ptr es:textColor[bx][Red], ah
    mov     byte ptr es:textColor[bx][Blue], al

    ; restore break error so format dda will work out
    mov     ax, BrkErrSave
    mov     es:BreakErr[bx], ax

    call    realstrblt
    or	    ax,ax
    jnz     sb_doblue
    cmp     dx, 8000h
    jz	    sb_restore

sb_doblue:

    ; now set up for blue plane
    add     DestYOrg, di			; offset Y by bitmap hieght
    add     word ptr rcClip[top], di		; offset clip rect top
    add     word ptr rcClip[bottom], di 	; offset clip rect bottom

    ; get blue color.
    mov     ax, BlueSave
    les     bx, lpDrawMode
    mov     byte ptr es:bkColor[bx][Red], al
    mov     byte ptr es:bkColor[bx][Green], al
    mov     byte ptr es:textColor[bx][Red], ah
    mov     byte ptr es:textColor[bx][Green], ah

    ; restore break error so format dda will work out
    mov     ax, BrkErrSave
    mov     es:BreakErr[bx], ax

    call    realstrblt

    ; leave break error and return value as it was for this last call

sb_restore:

    ; now restore the bitmap to its color dimensions
    les     bx, lpDestDev
    mov     word ptr es:bmPlanes[bx], 0103h	; make RGB bitmap again
    mov     word ptr es:bmHeight[bx], di	; restore height

    ;	restore the colors in the drawmode
    les     bx, lpDrawMode
    mov     cx, RedSave 			; restore red colors
    mov     byte ptr es:bkColor[bx][Red], cl
    mov     byte ptr es:textColor[bx][Red], ch
    mov     cx, GreenSave			; green
    mov     byte ptr es:bkColor[bx][Green], cl
    mov     byte ptr es:textColor[bx][Green], ch
    mov     cx, BlueSave			; blue
    mov     byte ptr es:bkColor[bx][Blue], cl
    mov     byte ptr es:textColor[bx][Blue], cl

    ; and return GDI's return value

sb_exit:

cEnd

realstrblt  proc    near

    pop     bx					; save return offset
    push    SEG_lpDestDev
    push    OFF_lpDestDev
    push    DestXOrg
    push    DestYOrg
    push    ss
    lea     ax, rcClip
    push    ax					; copy of clipping rect
    push    SEG_lpString
    push    OFF_lpString
    push    count
    push    SEG_lpFont
    push    OFF_lpFont
    push    SEG_lpDrawMode
    push    OFF_lpDrawMode
    push    SEG_lpTextXForm
    push    OFF_lpTextXForm

    push    cs					; segment of far return
    push    bx					; offset

    jmp     gdi_dmStrBlt

realstrblt  endp

sEnd

end
