page	,132
;----------------------------Module-Header------------------------------;
; Module Name: STRETCH.ASM
;
; StretchBLT at level of device driver.
;
; we will handle a stretchblt only if the following is true
;
;       source and dest is a COLOR bitmap
;       rop is SRCCOPY
;       scale factor is a integer multiple
;
; NOTES:
;       Does not handle mirroring in x or y
;       Should this function attempt to handle a non integer stretch?
;
;       >>>> DOES *NOT* handle palette translation, yet! <<<<
;
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.

        title   StretchBLT
        %out    StretchBlt

ifndef EDI?
ifdef FLAT
    EDI?        equ <edi>
    ESI?        equ <esi>
    MOVSB32?    equ <movs byte ptr es:[edi], byte ptr ds:[esi]>
    LODSB32?    equ <lods byte ptr ds:[esi]>
    STOSB32?    equ <stos byte ptr es:[edi]>
else
    EDI?        equ <di>
    ESI?        equ <si>
    MOVSB32?    equ <movsb>
    LODSB32?    equ <lodsb>
    STOSB32?    equ <stosb>
endif
endif

;	Define the portions of GDIDEFS.INC that will be needed by bitblt.

incLogical	= 1		;Include GDI logical object definitions
incDrawMode	= 1		;Include GDI DrawMode definitions

	.xlist
	include	CMACROS.INC
	include	GDIDEFS.INC
	include	DISPLAY.INC
	include	CURSOR.INC
        include MACROS.MAC
	include	NJUMPS.MAC
        include devconst.blt
        .list

        externA  COLOR_FORMAT           ;Format of device bitmaps (0801)
	externA  SCREEN_W_BYTES		;Screen width in bytes
        externA  __NEXTSEG              ;offset to next segment

ifdef PALETTES ;========================>

	externB PaletteModified 	; 0ffh IF palette modified
	externB	PaletteTranslationTable	; mem8 -> dev color translation
	externB	PaletteIndexTable	; dev -> mem8 color translation

endif ;=================================>

ifdef FLAT
else
        externNP set_bank_select
endif

ifdef	PALETTES
	externB  PaletteModified	;Set when palette is modified
	externFP TranslateBrush		;translates the brush
	externFP TranslateTextColor     ;translates text colors
endif

ifdef	EXCLUSION			;If cursor exclusion
	externNP exclude		;Exclude area from screen
	externNP unexclude		;Restore excluded area to screen
endif

sBegin	Data

	externB enabled_flag		;Non-zero if output allowed
        externW ScratchSel              ; the free selector

sEnd	Data

WORK_BUF_SIZE = SCREEN_WIDTH            ;size of work buffer
SRCCOPY_H     = 00CCh                   ;raster op dest=source

sBegin  Code
assumes cs,Code
assumes ds,Data
assumes es,nothing

cProc   StretchBlt,<FAR,PUBLIC,WIN,PASCAL>,<>
        parmD   lpDstDev                ;--> to destination bitmap descriptor
        parmW   DstX                    ;Destination origin - x coordinate
        parmW   DstY                    ;Destination origin - y coordinate
        parmW   DstXE                   ;x extent of the BLT
        parmW   DstYE                   ;y extent of the BLT
        parmD   lpSrcDev                ;--> to source bitmap descriptor
        parmW   SrcX                    ;Source origin - x coordinate
        parmW   SrcY                    ;Source origin - y coordinate
        parmW   SrcXE                   ;x extent of the BLT
        parmW   SrcYE                   ;y extent of the BLT
        parmD   Rop                     ;Raster operation descriptor
        parmD   lpPBrush                ;--> to a physical brush (pattern)
        parmD   lpDrawMode              ;--> to a drawmode
        parmD   lpClip                  ;Clip rect

        localW  dupX                    ;expansion factor in X
        localW  dupY                    ;expansion factor in Y

ifdef FLAT
else
	localW	next_src_scan		;Next source scan function
        localW  next_dst_scan           ;Next destination scan function
endif

        localW  read_scan               ;Read scan function
        localW  write_scan              ;Write scan function

        localB  gl_direction            ;Increment/decrement flag
INCREASING      equ     +1
DECREASING      equ     -1

        localB  local_enable_flag

        localB  gl_flag0
F0_Y_MIRROR         equ     01000000b   ;Mirroring in Y should be done
F0_X_MIRROR         equ     00100000b   ;Mirroring in X should be done
F0_SRC_AND_DST_DEV  equ     00010000b   ;Source and Destination is the device
F0_SRC_IS_DEV       equ     00001000b   ;Source is the device
F0_SRC_IS_COLOR     equ     00000100b   ;Source is color
F0_DEST_IS_DEV      equ     00000010b   ;Destination is the device
F0_DEST_IS_COLOR    equ     00000001b   ;Destination is color

        localV  gl_src,%(SIZE DEV)      ;Source device data
        localV  gl_dst,%(SIZE DEV)      ;Destination device data

        localV  work_buf,WORK_BUF_SIZE  ;Work buffer for 1 expanded scan

        localW  clip_test
        localW  clip_top
        localW  clip_bottom
        localW  clip_left
        localW  clip_right
        localW  clipXE
        localW  clipYE
cBegin
        push    ESI?
        push    EDI?

        mov     al,enabled_flag         ;Save enabled_flag while we still
        mov     local_enable_flag,al    ;  have DS pointing to Data

ifdef   PALETTES

	test	PaletteModified, 0ffh
        jnz     let_gdi_do_it

endif;  PALETTES

        ;
        ;   Is this a Blt we will support?
        ;
        ;       rop must be SRCCOPY
        ;       dest size must be greater that source size (expansion)
        ;       scale factor is a integer multiple
        ;       source and destination must be COLOR format
        ;

        mov     ax,wptr Rop[2]          ;Test the ROP first
        cmp     ax,SRCCOPY_H
        jnz     let_gdi_do_it

        mov     ax,DstXE
        mov     bx,SrcXE
        or      ax,ax
        js      let_gdi_do_it           ;mirroring is not cool
        cmp     bx,ax
        jae     let_gdi_do_it           ;if source > dest give it to GDI

        xor     dx,dx                   ;Calculate scale factor, if it is
        div     bx                      ;not even give it to GDI
        or      dx,dx
        jnz     let_gdi_do_it
        mov     dupX,ax

        mov     ax,DstYE
        mov     bx,SrcYE
        or      ax,ax
        js      let_gdi_do_it           ;mirroring is not cool
        cmp     bx,ax
        jae     let_gdi_do_it           ;if source > dest give it to GDI

        xor     dx,dx                   ;Calculate scale factor, if it is
        div     bx                      ;not even give it to GDI
        or      dx,dx
        jnz     let_gdi_do_it
        mov     dupY,ax

        xor     bh,bh                   ;copy_dev will accumulate flags here

        lds     si,lpSrcDev             ;Test the source device
        assumes ds,nothing
        lea     di,gl_src
        call    copy_dev

        mov     al,gl_src.dev_flags
        test    al,IS_COLOR             ;Mono bitmaps are GDI's turf
        jz      let_gdi_do_it

        lds     si,lpDstDev             ;Test the dest device
        assumes ds,nothing
        lea     di,gl_dst
        call    copy_dev

        mov     al,gl_dst.dev_flags
        test    al,IS_COLOR             ;Mono bitmaps are GDI's turf
        jz      let_gdi_do_it

        mov     gl_flag0,bh             ;Copy device set bh = gl_flag0

        ;
	;   Test the enable flag.
        ;
        test    bh,F0_SRC_IS_DEV+F0_DEST_IS_DEV
        jz      time_to_stretch

	test	local_enable_flag,0FFh
        jnz     time_to_stretch
        jz      stretchblt_ok

let_gdi_do_it:
        mov     ax,-1
        jmp     stretchblt_exit

;--------------------------------------------------------------------------;
;
;   We have a StretchBlt we are wiling to do!
;
;   ds:si   --> dest PDEVICE
;
;   dupX    expansion factor in X dir
;   dupY    expansion factor in Y dir
;
;--------------------------------------------------------------------------;
time_to_stretch:

        call    stretch_init
        jc      let_gdi_do_it

        call    stretch_y_expand

stretchblt_done:

ifdef EXCLUSION
        mov     al,gl_flag0
        test    al,F0_SRC_IS_DEV+F0_DEST_IS_DEV
        jz      @f
        call    unexclude
@@:
endif

stretchblt_ok:

        xor     ax,ax               ; return 0 to show we did it

stretchblt_exit:
        pop     EDI?
        pop     ESI?
cEnd

;--------------------------------------------------------------------------;
;
;   stretch_y_expand
;
;   a y expand StretchBlt (DstYE > SrcYE) is handled , currently the expand
;   factor is assumed constant.
;
;   Entry:
;       ds:si   --> start of source scan
;       es:di   --> start of destination scan
;   Returns:
;
;   Error Returns:
;       None
;   Registers Preserved:
;       ES,DI,BP
;   Registers Destroyed:
;       AX,BX,CX,DX,SI,flags
;   Calls:
;	None
;   History:
;
;--------------------------------------------------------------------------;

        ?_pub   stretch_y_expand
stretch_y_expand PROC NEAR

        ; write the first (clipped scan)
        ;
stretchblt_first:
        call    [read_scan]
        mov     cx,dupY
        sub     cx,clip_top

        dec     SrcYE
        jz      stretchblt_last_1

        call    [write_scan]

        dec     SrcYE
        jz      stretchblt_last

        ; write the non-clipped scans
        ;
stretch_scan_loop:
        call    [read_scan]
        mov     cx,dupY
        call    [write_scan]

        dec     SrcYE
        jnz     stretch_scan_loop

        ; write the last clipped scan
        ;
stretchblt_last:
        call    [read_scan]
        mov     cx,dupY
stretchblt_last_1:
        sub     cx,clip_bottom
        call    [write_scan]

        ret
stretch_y_expand ENDP

;--------------------------------------------------------------------------;
;
;   stretch_read_scan
;
;   (CX) scanlines from the source are read in and expanded,
;   the expanded version is placed in work_buf
;
;   Entry:
;       ds:si   --> begining of scan
;       cx          number of source scanlines to read
;   Returns:
;       work_buf contains expanded scanline
;       ds:si   --> advanced to next scan
;   Error Returns:
;       None
;   Registers Preserved:
;       ES,DI,BP
;   Registers Destroyed:
;       AX,BX,CX,DX,SI,flags
;   Calls:
;	None
;   History:
;
;--------------------------------------------------------------------------;
        ?_pub   stretch_read_scan_expand
stretch_read_scan_expand PROC NEAR
	push	es
        push    di
	mov	di,ss
	mov	es,di

ifdef FLAT
else
        ;
        ;   If both the source and dest are the device, we may need to
        ;   set the bank
        ;
        mov     al,gl_flag0
        test    al,F0_SRC_AND_DST_DEV
        jz      @f
        mov     dl,gl_src.init_page
        call    set_bank_select
@@:
endif; FLAT

        lea     di,work_buf
        mov     bx,SrcXE

read_scan_expand_loop:
        LODSB32?
        mov     cx,dupX
	mov	ah,al
;;;;;;;;REPSTOSB ss:[di]
	test di,0001h
        jz      @F
        stosb
        dec     cx
@@:
        shr     cx,1
        rep     stosw
        adc     cl,cl
        rep     stosb
;;;;;;;;ENDM
        dec     bx
        jnz     read_scan_expand_loop

ifdef FLAT
        movsx   eax,gl_src.next_scan
        add     esi,eax
else
        call    [next_src_scan]
endif

read_scan_expand_exit:
        pop     di
	pop	es
        ret
stretch_read_scan_expand ENDP

;--------------------------------------------------------------------------;
;
;   stretch_write_scan
;
;   output the scan in work_buf to the destination duplicating it CX times
;
;   Entry:
;       es:di   --> begining of output scan
;       cx          duplication count for scanline
;       work_buf contains expanded scanline to output
;   Returns:
;       es:di   --> advanced to next scan
;   Error Returns:
;       None
;   Registers Preserved:
;       ES,DI,BP
;   Registers Destroyed:
;       AX,BX,CX,DX,SI,flags
;   Calls:
;	None
;   History:
;
;--------------------------------------------------------------------------;
        ?_pub   stretch_write_scan
stretch_write_scan PROC NEAR
        push    ESI?

ifdef FLAT
else
        ;
        ;   If both the source and dest are the device, we may need to
        ;   set the bank
        ;
        mov     al,gl_flag0
        test    al,F0_SRC_AND_DST_DEV
        jz      @f
        mov     dl,gl_dst.init_page
        call    set_bank_select
@@:
endif; FLAT
        mov     bx,cx                       ; bx contains scanline count

write_scan_loop:
        lea     ESI?,work_buf
        add     si,clip_left
        mov     cx,clipXE

ifdef FLAT
        REPMOVSB es:[edi],ss:[esi]

        movsx   eax,gl_dst.next_scan
        add     edi,eax
else
;;;;;;;;REPMOVSB es:[di],ss:[si]
	test di,0001h
        jz      @F
	movs	byte ptr es:[di],byte ptr ss:[si]
        dec     cx
@@:
        shr     cx,1
	rep	movs word ptr es:[di],word ptr ss:[si]
	adc	cl,cl
	rep	movs byte ptr es:[di],byte ptr ss:[si]
;;;;;;;;ENDM

        call    [next_dst_scan]
endif; FLAT

        dec     bx
        jnz     write_scan_loop

write_scan_exit:
        pop     ESI?
        ret
stretch_write_scan ENDP

ifdef FLAT
else
;--------------------------------------------------------------------------;
;
;   next_src_scan
;
;   advance the source pointer ds:[si] to point to the next scan
;
;   Entry:
;       ds:si   --> end of current source scan
;   Returns:
;       ds:si   --> advanced to begining of next scan
;   Error Returns:
;       None
;   Registers Preserved:
;       ES,DI,BP
;   Registers Destroyed:
;       AX,BX,CX,DX,SI,flags
;   Calls:
;	None
;   History:
;
;--------------------------------------------------------------------------;

;
;   next_src_scan function used if the source is the device
;
next_src_scan_device PROC NEAR
        cmp     gl_direction,INCREASING
        jne     next_src_scan_device_dec

next_src_scan_device_inc:
        add     si,gl_src.next_scan
        jnc     next_src_scan_device_exit

        inc     gl_src.init_page
        jmp     short next_src_scan_device_set_page

next_src_scan_device_dec:
        sub     si,gl_src.next_scan
        jnc     next_src_scan_device_exit
        dec     gl_src.init_page

next_src_scan_device_set_page:
        mov     dl,gl_src.init_page
        call    set_bank_select

next_src_scan_device_exit:
        ret
next_src_scan_device ENDP

;
;   next_src_scan function used if the source is a bitmap
;
public	next_src_scan_bitmap
next_src_scan_bitmap PROC NEAR
	cmp	gl_direction,INCREASING
        jne     next_src_scan_bitmap_dec

next_src_scan_bitmap_inc:
	mov	bx	,si
        add     si,gl_src.next_scan
        cmp     gl_src.seg_index, 0
	je	next_src_scan_bitmap_exit
        mov     ax      ,si
	add	ax	,gl_src.fill_bytes
	cmp	bx	,ax
	jbe	next_src_scan_bitmap_exit

	mov	si	,ax
	mov	ax	,ds
        add     ax,__NEXTSEG
	mov	ds	,ax
        jmp     short next_src_scan_bitmap_exit

next_src_scan_bitmap_dec:
        sub     si,gl_src.next_scan
        jnc     next_src_scan_bitmap_exit
        cmp     gl_src.seg_index, 0
        je      next_src_scan_bitmap_exit

        sub     si,gl_src.fill_bytes
        mov     ax,ds
        sub     ax,__NEXTSEG
        mov     ds,ax

next_src_scan_bitmap_exit:
        ret
next_src_scan_bitmap ENDP

;--------------------------------------------------------------------------;
;
;   next_dst_scan
;
;   advance the destination pointer es:[di] to point to the next scan
;
;   Entry:
;       es:di   --> end of current destination scan
;   Returns:
;       es:di   --> advanced to begining of next scan
;   Error Returns:
;       None
;   Registers Preserved:
;       ES,DI,BP
;   Registers Destroyed:
;       AX,BX,CX,DX,SI,flags
;   Calls:
;	None
;   History:
;
;--------------------------------------------------------------------------;

;
;   next_dst_scan function used if the destination is the device
;
next_dst_scan_device PROC NEAR
        cmp     gl_direction,INCREASING
        jne     next_dst_scan_dec

next_dst_scan_inc:
        add     di,gl_dst.next_scan
        jnc     next_dst_scan_exit

        inc     gl_dst.init_page
        jmp     short next_dst_scan_set_page

next_dst_scan_dec:
        sub     di,gl_dst.next_scan
        jnc     next_dst_scan_exit
        dec     gl_dst.init_page

next_dst_scan_set_page:
        mov     dl,gl_dst.init_page
        call    set_bank_select

next_dst_scan_exit:
        ret

next_dst_scan_device ENDP

;
;   next_dst_scan function used if the destination is a bitmap
;
next_dst_scan_bitmap PROC NEAR
        cmp     gl_direction,INCREASING
        jne     next_dst_scan_bitmap_dec

next_dst_scan_bitmap_inc:
        mov     bx,di
        add     di,gl_dst.next_scan
        mov     ax,di
        add     ax,gl_dst.fill_bytes
        cmp     bx,ax
        jb      next_dst_scan_bitmap_exit

        mov     di,ax
        mov     ax,es
        add     ax,__NEXTSEG
        mov     es,ax

        jmp     short next_dst_scan_bitmap_exit

next_dst_scan_bitmap_dec:
        sub     di,gl_dst.next_scan
        jnc     next_dst_scan_bitmap_exit

        sub     di,gl_dst.fill_bytes
        mov     ax,es
        sub     ax,__NEXTSEG
        mov     es,ax

next_dst_scan_bitmap_exit:
        ret
next_dst_scan_bitmap ENDP
endif; FLAT

;--------------------------------------------------------------------------;
;
;   stretch_init
;
;   init local frame vars for StretchBlt
;
;   ENTRY:
;       ss:bp   --> stretchblt frame
;
;   EXIT:
;       CARRY if GDI should do the StretchBlt
;
;--------------------------------------------------------------------------;
stretch_init PROC NEAR

        lds     si,lpClip

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   Input Clipping
;
;   clip the StretchBlt source rectangle (SrcX,SrcY,SrcXE,SrcYE) to the
;   extents of the source Bitmap/Device
;
;   modify the passed clip rectangle (lpClip) to include the input clipping
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,SrcY       ; clip.top += max(0,SrcY) * dupY
        neg     ax
        max_ax  0
        mul     dupY
        add     [si].top,ax

        mov     ax,SrcX       ; clip.left += max(0,SrcX) * dupX
        neg     ax
        max_ax  0
        mul     dupX
        add     [si].left,ax

        mov     ax,SrcY       ; clip.bottom -= max(0,SrcY+SrcYE-BitmapHeight) * dupY
        add     ax,SrcYE
        sub     ax,gl_src.height
        max_ax  0
        mul     dupY
        sub     [si].bottom,ax

        mov     ax,SrcX       ; clip.right -= max(0,SrcX+SrcXE-BitmapWidth) * dupX
        add     ax,SrcXE
        sub     ax,gl_src.width_bits
        max_ax  0
        mul     dupY
        sub     [si].right,ax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   Output clipping
;
;   clip the StretchBlt destination rectangle (DstX,DstY,DstXE,DstYE)
;   to the clip retangle suplied by GDI (lpClip)
;
;   if clipping is required, set the following values:
;
;   clip_test    non zero if clipping is needed
;   clip_top     num scans clipped above
;   clip_bottom  num scans clipped below
;   clip_left    num pixels clipped on left
;   clip_right   num pixels clipped on right
;   clipXE       total pixels visible in a scan
;   clipYE       total scans visible
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        xor     bx,bx               ; Total clip error is kept in bx

        mov     ax,[si].top         ; clip_top = max(0,clip.top - DstY)
        sub     ax,DstY
        max_ax  0
        mov     clip_top,ax
        add     bx,ax

        mov     ax,DstY             ; clip_bottom = max(0,DstY + DstYE - clip.bottom)
        add     ax,DstYE
        sub     ax,[si].bottom
        max_ax  0
        mov     clip_bottom,ax
        add     bx,ax

        mov     ax,[si].left        ; clip_left = max(0,clip.left - DstX)
        sub     ax,DstX
        max_ax  0
        mov     clip_left,ax
        add     bx,ax
                                    ; clip_right = max(0,DstX + DstXE - clip.right)
        mov     ax,DstX
        add     ax,DstXE
        sub     ax,[si].right
        max_ax  0
        mov     clip_right,ax
        add     bx,ax

        mov     clip_test,bx
        or      bx,bx
        njz     stretch_no_clipping

        ;
        ;   Adjust the parameters of the stretch so we only do minimal work to
        ;   fill in the clipped area
        ;
        ?_pub stretch_clip_Y_top
stretch_clip_Y_top:
        mov     ax,clip_top
        mov     bx,dupY
        cmp     ax,bx
        jl      stretch_clip_Y_bottom

        xor     dx,dx
        mov     cx,ax               ; save clip_top
        div     bx                  ; dx = clip_top % dupY, ax = clip_top / dupY
        sub     cx,dx               ; cx = clip_top / dupY * dupY

        mov     clip_top,dx
        add     SrcY,ax
        sub     SrcYE,ax
        add     DstY,cx
        sub     DstYE,cx

stretch_clip_Y_bottom:
        mov     ax,clip_bottom
        mov     bx,dupY
        cmp     ax,bx
        jl      stretch_clip_X_left

        xor     dx,dx
        mov     cx,ax               ; save clip_bottom
        div     bx                  ; dx = clip_bottom % dupY, ax = clip_bottom / dupY
        sub     cx,dx               ; cx = clip_bottom / dupY * dupY

        mov     clip_bottom,dx
        sub     SrcYE,ax
        sub     DstYE,cx

stretch_clip_X_left:
        mov     ax,clip_left
        mov     bx,dupX
        cmp     ax,bx
        jl      stretch_clip_X_right

        xor     dx,dx
        mov     cx,ax               ; save clip_left
        div     bx                  ; dx = clip_left % dupY, ax = clip_left / dupY
        sub     cx,dx               ; cx = clip_left / dupY * dupY

        mov     clip_left,dx
        add     SrcX,ax
        sub     SrcXE,ax
        add     DstX,cx
        sub     DstXE,cx

stretch_clip_X_right:
        mov     ax,clip_right
        mov     bx,dupX
        cmp     ax,bx
        jl      stretch_no_clipping

        xor     dx,dx
        mov     cx,ax               ; save clip_bottom
        div     bx                  ; dx = clip_bottom % dupY, ax = clip_bottom / dupY
        sub     cx,dx               ; cx = clip_bottom / dupY * dupY

        mov     clip_right,dx
        sub     SrcXE,ax
        sub     DstXE,cx

stretch_no_clipping:
        mov     ax,DstXE            ; clipXE = DstXE - clip_left - clip_right
        sub     ax,clip_left
        sub     ax,clip_right
        mov     clipXE,ax

        mov     ax,DstYE            ; clipYE = DstYE - clip_top - clip_bottom
        sub     ax,clip_top
        sub     ax,clip_bottom
        mov     clipYE,ax

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   After clipping a destination scanline must be able to fit into
;   our work buffer or we can't do it
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,DstXE                ;if dest > work_buf give to GDI
        cmp     ax,WORK_BUF_SIZE
        nja     stretch_init_fail

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   Determine the direction of the Stretch
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

;  assume most favorable case

        mov     ah,INCREASING

;  source present
;  check if source and destination are same bitmaps

	mov	dx,off_lpSrcDev
        cmp     dx,off_lpDstDev
        jne     step_dir_found

	mov	dx,seg_lpSrcDev
        cmp     dx,seg_lpDstDev
	jne	step_dir_found

;  source and destination are the same
;  check if rectangles overlap

        mov     dx,SrcXE
        add     dx,SrcX
        cmp     dx,DstX                 ; src.x + src.dx <= dest.x ?
	jbe	step_dir_found		; --yes

        mov     dx,DstXE
        add     dx,DstX
        cmp     dx,SrcX                 ; dst.x + dst.dx <= src.x ?
	jbe	step_dir_found		; --yes

        mov     dx,SrcYE
        add     dx,SrcY
        cmp     dx,DstY                 ; src.y + src.dy <= dest.y ?
	jbe	step_dir_found		; --yes

        mov     dx,DstYE
        add     dx,DstY
        cmp     dx,SrcY                 ; dst.y + dst.dy <= src.y ?
	jbe	step_dir_found		; --yes
;
;   rectangles overlap
;
;   determine which direction to process by comparing the distances shown below
;   if A > B go forward else go backward.
;
;       +-----------------+
;       |Dst    ^         |
;       |       |------------------ A = SrcY - DstY
;       |       v         |
;       |    +------+     |
;       |    |Src   |     |
;       |    |      |     |
;       |    +------+     |
;       |       ^         |
;       |       |------------------ B = (DstY + DstYE) - (SrcY + SrcYE)
;       |       |         |
;       |       v         |     A - B = 2SrcY - 2DstY - DstYE - SrcYE
;       +-----------------+
;
        mov     bx,SrcY
        mov     cx,DstY
        add     bx,bx
        add     cx,cx
        sub     bx,cx
        sub     bx,DstYE
        sub     bx,SrcYE
        jge     step_dir_found

;  overlap requires move start at high end

	mov	ah,DECREASING

;  save the results

step_dir_found:
        mov     gl_direction,ah

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Set up the initial source and dest pointers
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     bx,SrcX
        mov     ax,SrcY
	cmp	gl_direction,INCREASING
	je	@f
        add     ax,SrcYE
        dec     ax
@@:
        lea     si,gl_src
        call    map_address

        mov     bx,DstX
        mov     ax,DstY
        add     bx,clip_left            ; figure clipping into start X
        add     ax,clip_top
	cmp	gl_direction,INCREASING
	je	@f
        add     ax,clipYE
        dec     ax
@@:
        lea     si,gl_dst
        call    map_address

        ;
        ;   if we are going backward, switch the meaning of clip_top
        ;   and clip_bottom
        ;
        cmp     gl_direction,INCREASING
	je	@f
        mov     ax,clip_top
        mov     bx,clip_bottom
        mov     clip_top,bx
        mov     clip_bottom,ax
@@:

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Calculate values for next_scan
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,gl_src.width_b
        mov     bx,SrcXE
	cmp	gl_direction,INCREASING
        je      @f
ifdef FLAT
        neg     ax              ; next_scan = -width_b - XE
else
        neg     bx              ; next_scan =  width_b + XE
endif
@@:
        sub     ax,bx
        mov     gl_src.next_scan,ax

        mov     ax,gl_dst.width_b
        mov     bx,clipXE
	cmp	gl_direction,INCREASING
        je      @f
ifdef FLAT
        neg     ax
else
        neg     bx
endif
@@:
        sub     ax,bx
        mov     gl_dst.next_scan,ax


; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;  Setup the read and write function pointers
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        mov     ax,CodeOFFSET stretch_read_scan_expand
        mov     bx,CodeOFFSET stretch_write_scan

        mov     read_scan,ax
        mov     write_scan,bx

ifdef FLAT
else
        mov     ax,CodeOFFSET next_src_scan_device
        mov     bx,CodeOFFSET next_dst_scan_device

        test    gl_src.dev_flags,IS_DEVICE
        jnz     @f
        mov     ax,CodeOFFSET next_src_scan_bitmap
@@:
        test    gl_dst.dev_flags,IS_DEVICE
        jnz     @f
        mov     bx,CodeOFFSET next_dst_scan_bitmap
@@:

        mov     next_src_scan,ax
        mov     next_dst_scan,bx

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   If both the source and dest are the device, set F0_SRC_AND_DST_DEV
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
        mov     al,gl_flag0
        and     al,F0_SRC_IS_DEV+F0_DEST_IS_DEV
        cmp     al,F0_SRC_IS_DEV+F0_DEST_IS_DEV
        jne     @f
        or      gl_flag0,F0_SRC_AND_DST_DEV
@@:
endif; FLAT

        subttl  Cursor Exclusion
	page
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;	Cursor Exclusion
;
;	If either device or both devices are for the display, then
;	the cursor must be excluded.  If both devices are the display,
;	then a union of both rectangles must be performed to determine
;	the exclusion area.
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

ifdef	EXCLUSION
	mov	al,gl_flag0
	and	al,F0_SRC_IS_DEV+F0_DEST_IS_DEV	;Are both memory bitmaps?
        jz      cursor_exclusion_end    ;  Yes, no exclusion needed

        mov     cx,DstX                 ;Assume only a destination on the
        mov     dx,DstY                 ;  display
        mov     si,DstXE
        mov     di,DstYE

        test    al,F0_SRC_IS_DEV        ;Is the source a memory bitmap?
        jz      cursor_exclusion_no_union;  Yes, go set right and bottom

	test	al,F0_DEST_IS_DEV	;  (set 'Z' if dest is memory)
	xchg	ax,cx			;  No, prepare for the union
	mov	bx,dx

        mov     cx,SrcX                 ;Set source org
        mov     dx,SrcY
        mov     si,SrcXE
        mov     di,SrcYE

	jz	cursor_exclusion_no_union;Dest is memory. Set right and bottom

;	The union of the two rectangles must be performed.  The top left
;	corner will be the smallest x and smallest y.  The bottom right
;	corner will be the largest x and the largest y added into the
;       extents

        add     si,cx                   ;si = SrcX + SrcXE
        add     di,dx                   ;di = SrcY + SrcYE

	cmp	cx,ax			;Get smallest x
	jle	cursor_exclusion_y	;CX is smallest
        mov     cx,ax                   ;AX is smallest

cursor_exclusion_y:
	cmp	dx,bx			;Get smallest y
	jle	cursor_exclusion_union	;DX is smallest
        mov     dx,bx                   ;BX is smallest

cursor_exclusion_union:
        mov     ax,DstX                 ;ax = DstX + DstXE
        add     ax,DstXE
        cmp     si,ax
        jge     @f
        mov     si,ax
@@:
        mov     ax,DstY                 ;ax = DstY + DstYE
        add     ax,DstYE
        cmp     di,ax
        jge     @f
        mov     di,ax
@@:
	jmp	short cursor_exclusion_do_it	;Go do exclusion

cursor_exclusion_no_union:
        add     si,cx                   ;Set right
	add	di,dx			;Set bottom

cursor_exclusion_do_it:
        dec     si                      ;Make the extents inclusive of the
	dec	di			;  last point

        call    exclude                 ;Exclude the area from the screen

endif	;EXCLUSION

cursor_exclusion_end:

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
;
;   load the pointers to the start and dest scans
;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

        lds     ESI?,gl_src.lp_init
        les     EDI?,gl_dst.lp_init

stretch_init_success:
        clc
        ret

stretch_init_fail:
        stc
        ret

stretch_init ENDP

;----------------------------Private-Routine----------------------------;
; copy_device
;
; Copy device information to frame.
;
; Entry:
;	DS:SI --> device
;       SS:DI --> frame DEV structure
;	BH     =  gl_flag0, accumulated so far
; Returns:
;	BH     =  gl_flag0, accumulated so far
;	Carry clear if no error
; Error Returns:
;	Carry set if error (bad color format)
; Registers Preserved:
;	BX,CX,DS,ES,BP
; Registers Destroyed:
;	AX,DX,SI,DI,flags
; Calls:
;	None
; History:
;  Sun 22-Feb-1987 16:29:09 -by-  Walt Moore [waltm]
; Created.
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

copy_	macro	si_item,di_item

	if	(size si_item)-(size di_item)
	.err2	si_item and di_item are different sizes
	endif

	if	si_item-si_off
	add	si,si_item-si_off
si_off	=	si_item
	endif

	if	di_item-di_off
	add	di,di_item-di_off
di_off	=	di_item
	endif

	&rept	(size si_item)/2
	movsw
	&endm

	if	(size si_item) and 1
	movsb
	endif

si_off	=	si_off+(size si_item)
di_off	=	di_off+(size di_item)

	endm
	
si_off	=	0
di_off	=	0

copy_dev        proc near
        smov    es,ss

	lodsw
	errnz	bmType-si_off
si_off	=	si_off+2

	cmp	ax,1			;Set 'C' if the device
	cmc
	rcl	bh,1			;Move in type
	errnz	F0_SRC_IS_DEV-00001000b
	errnz	F0_DEST_IS_DEV-0000010b

	copy_	bmWidth,width_bits
	copy_	bmHeight,height
	copy_	bmWidthBytes,width_b

	if	bmPlanes-si_off
	add	si,bmPlanes-si_off
si_off	=	bmPlanes
	endif
	lodsw				;Get Planes/pixels
si_off	=	si_off+2

	cmp	ax,0101H		;Monochrome?
	je	copy_dev_20		;  Yes	('C' clear)

	cmp	ax,COLOR_FORMAT		;Our color?
	jne	copy_dev_bad_clr_format	;  No, complain about color format

	stc				;  (show color)

copy_dev_20:
	rcl	bh,1			;Rotate in color flag
	errnz	F0_SRC_IS_COLOR-00000100b
	errnz	F0_DEST_IS_COLOR-00000001b

	copy_	bmBits,lp_bits
;;;;	copy_	bmWidthPlanes,plane_w
	copy_	bmSegmentIndex,seg_index
	copy_	bmScanSegment,scans_seg
	copy_	bmFillBytes,fill_bytes

	mov	al,bh			;Set IS_COLOR and IS_DEVICE
	and	al,IS_COLOR+IS_DEVICE	;  flags in the Device Flags
	errnz	IS_COLOR-F0_DEST_IS_COLOR ;Must be same bits

	if	dev_flags-di_off
	add	di,dev_flags-di_off
di_off	=	dev_flags
	endif
	stosb
di_off	=	di_off+1

	clc
	ret

copy_dev_bad_clr_format:
	stc
	ret

copy_dev	endp

;-----------------------------------------------------------------------
;
;  map_address - convert X,Y coordinate to initial pointer
;
; ENTRY:
;	AX = Y
;	BX = X
;	SS:SI -> local DEV structure
; EXIT:
;	DEV.lp_init set
;

map_address	proc	near

ifdef FLAT
        movzx   eax,ax
        movzx   ebx,bx
        movzx   edx,dx

        test    ss:[si].dev_flags,IS_COLOR
        jnz     @f
        shiftr  bx,3
@@:
        add     bx,ss:[si].OFF_lp_bits

        mov     dx,ss:[si].width_b
        mul     edx
        add     eax,ebx

        mov     ss:[si].OFF_lp_init,eax

	mov	dx,ss:[si].SEG_lp_bits
	mov	ss:[si].SEG_lp_init,dx

        ret
else
        test    ss:[si].dev_flags,IS_DEVICE
	jz	map_bitmap

;----------; 
;  DEVICE  ;
;----------; 

map_device:
	mov	dx,ss:[si].width_b
	mul	dx
	add	ax,bx
	adc	dl,0
        mov     ss:[si].init_page,dl

        call    set_bank_select

	mov	dx,ss:[si].SEG_lp_bits
	mov	ss:[si].SEG_lp_init,dx

	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
	ret

;----------; 
;  BITMAP  ;
;----------; 

map_bitmap:
	mov	dx,ss:[si].SEG_lp_bits
	mov	cx,ss:[si].seg_index
	jcxz	map_bitmapx

@@:	add	dx,cx
	sub	ax,ss:[si].scans_seg
	jnc	@b
	sub	dx,cx
	add	ax,ss:[si].scans_seg

map_bitmapx:
	mov	ss:[si].SEG_lp_init,dx

	test	ss:[si].dev_flags,IS_COLOR
	jnz	map_bitmap8

;---------------------;
;  MONOCHROME BITMAP  ;
;---------------------;

map_bitmap1:
	mov	dx,ss:[si].width_b
	mul	dx
	shiftr	bx,3
	add	ax,bx
	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
	ret


;----------------;
;  COLOR BITMAP  ;
;----------------;

map_bitmap8:
	mov	dx,ss:[si].width_b
	mul	dx
	add	ax,bx
	add	ax,ss:[si].OFF_lp_bits
	mov	ss:[si].OFF_lp_init,ax
        ret

endif; FLAT

map_address     endp

sEnd    Code

end
