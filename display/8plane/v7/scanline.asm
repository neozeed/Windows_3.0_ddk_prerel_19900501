;
;	FILE:	SCANLINE.ASM
;	DATE:	1/14/89
;	AUTHOR: Jim Keller
;
;	The new improved scanline.
;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.



incLogical	= 1			; Include control for gdidefs.inc
incDrawMode	= 1			; Include control for gdidefs.inc
incOutput	= 1			; Include control for gdidefs.inc

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include display.inc
	include macros.mac
	.list



ifdef PALETTES
	externB PaletteModified 	; Set when palette is modified
        externFP TranslateBrush         ; 'on-the-fly' translation of brush
        externFP TranslatePen           ; 'on-the-fly' translation of pen
        externFP TranslateTextColor     ; 'on-the-fly' translation of textcol
endif

ifdef	EXCLUSION
	externFP exclude_far		; Exclude area from screen
	externFP unexclude_far		; Clear excluded area
endif


rscan_type	STRUC

rsiterate       db      0       ;flag: if non 0 then in a Begin/End loop
rstype          db      0       ;dev type:0 = screen, 1 = clr mem, 2 mono mem
rsvalid 	db	0
rscolor_pattern db      SIZE_PATTERN * SIZE_PATTERN  DUP(?)
rsmono_pattern  db      SIZE_PATTERN DUP (?)
rsstyle         dw      0
rsaccel         db      0
rsxpar_mask     db      SIZE_PATTERN DUP(?)
rsoutput        dw      0       ;rop and bg mode specific output routine

rscan_type      ENDS


sBegin  Data

	externB enabled_flag		; Non-zero if output allowed


public	rscan
rscan		db	100 DUP (?)


MONO_TYPE	EQU	0
COLOR_TYPE	EQU	1
SCREEN_TYPE	EQU	2


sEnd	Data


externFP far_set_bank_select
externFP far_pattern_preprocessing
externFP exclude_far
externFP unexclude_far


createSeg _SCANLINE,ScanlineSeg,word,public,CODE
sBegin	ScanlineSeg
assumes cs,ScanlineSeg


mono_rop_and_mode_output	LABEL	WORD

dw	 mono_xpar_rop_0
dw       mono_xpar_rop_1
dw       mono_xpar_rop_2
dw       mono_xpar_rop_3
dw       mono_xpar_rop_4
dw       mono_xpar_rop_5
dw       mono_xpar_rop_6
dw       mono_xpar_rop_7
dw       mono_xpar_rop_8
dw       mono_xpar_rop_9
dw       mono_xpar_rop_a
dw       mono_xpar_rop_b
dw       mono_xpar_rop_c
dw       mono_xpar_rop_d
dw       mono_xpar_rop_e
dw       mono_xpar_rop_f

dw       mono_opaque_rop_0
dw	 mono_opaque_rop_1
dw	 mono_opaque_rop_2
dw	 mono_opaque_rop_3
dw	 mono_opaque_rop_4
dw	 mono_opaque_rop_5
dw	 mono_opaque_rop_6
dw	 mono_opaque_rop_7
dw	 mono_opaque_rop_8
dw	 mono_opaque_rop_9
dw	 mono_opaque_rop_a
dw	 mono_opaque_rop_b
dw	 mono_opaque_rop_c
dw	 mono_opaque_rop_d
dw	 mono_opaque_rop_e
dw	 mono_opaque_rop_f

color_rop_and_mode_output       LABEL   WORD

dw	 color_xpar_rop_0
dw	 color_xpar_rop_1
dw	 color_xpar_rop_2
dw	 color_xpar_rop_3
dw	 color_xpar_rop_4
dw	 color_xpar_rop_5
dw	 color_xpar_rop_6
dw	 color_xpar_rop_7
dw	 color_xpar_rop_8
dw	 color_xpar_rop_9
dw	 color_xpar_rop_a
dw	 color_xpar_rop_b
dw	 color_xpar_rop_c
dw	 color_xpar_rop_d
dw	 color_xpar_rop_e
dw	 color_xpar_rop_f

dw	 color_opaque_rop_0
dw       color_opaque_rop_1
dw       color_opaque_rop_2
dw       color_opaque_rop_3
dw       color_opaque_rop_4
dw       color_opaque_rop_5
dw       color_opaque_rop_6
dw       color_opaque_rop_7
dw       color_opaque_rop_8
dw       color_opaque_rop_9
dw       color_opaque_rop_a
dw       color_opaque_rop_b
dw       color_opaque_rop_c
dw       color_opaque_rop_d
dw       color_opaque_rop_e
dw       color_opaque_rop_f




;
; DO_SCANLINES
;
; Entry:
;	CX holds the data segment from 'output'
; Return:
;	AX = Non-zero to show success
; Error Returns:
;	None
; Registers Preserved:
;	SI,DI,ES,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
;







cProc   do_scanlines,<FAR,PUBLIC,WIN,PASCAL>,<si,di,es,ds>
        parmD   lp_dst_dev              ; --> to the destination
        parmW   style                   ; Output operation
        parmW   count                   ; # of points
        parmD   lp_points               ; --> to a set of points
        parmD   lp_phys_pen             ; --> to physical pen
        parmD   lp_phys_brush           ; --> to physical brush
        parmD   lp_draw_mode            ; --> to a Drawing mode
        parmD   lp_clip_rect            ; --> to a clipping rectange if <> 0

        localB  current_mono_pattern
        localB  current_xpar_mask
        localV  current_color_pattern,8
        localW  start_of_scan           ;address of start of scanline
        localW  draw_output             ;rop and bg mode specific routine
	localW	my_data_seg
	localW	exclude_flag

cBegin

	mov	ds,cx			; CX holds the data segment

	cld				; We are forever doing this

	mov	ax	,ds
	mov	es	,ax

	mov	my_data_seg ,ax
	assumes ds,nothing
	assumes es,Data

	lds	si	,lp_points
	mov	ax	,ds
	or	ax	,si
	je	do_scanline_begin

	cmp	es:rscan.rsiterate, 1	;if we are in a begin/end scanline
	jne	do_scanline_begin0	;   loop then do not need init stuff

bracketed_do_scanline:
	cmp	es:[rscan.rstype] ,MONO_TYPE
	je	mono_main_loop_pre
	cmp	es:[rscan.rstype] ,COLOR_TYPE
	jmp	color_main_loop_pre	;this will become screen main loop

public do_scanline_begin
do_scanline_begin:
	mov	es:[rscan.rsiterate], 1 ;Flag start of scanline loop

do_scanline_begin0:
	mov	es:[rscan.rsvalid] ,0
	lds	si	,lp_dst_dev
	cmp	[si].bmType, 0		;if the bitmap type is 0 then
	jne	do_scanline_screen_ws	;  the device is the screen
	cmp	[si].bmBitsPixel, 8	;if there are 8 bits/pixel then
	jne	do_scanline_mono	;  the device is color memory
	jmp	do_scanline_color

do_scanline_screen_ws:
        jmp     do_scanline_screen


public	do_scanline_mono
do_scanline_mono:

	mov	bl	,MONO_TYPE
	mov	es:[rscan.rstype] ,bl	;mark the type of the dst dev

	mov	bh	,00H		;get_fill_data returns with ES = Data
	call	get_fill_data		;returns bx as the background mode
                                        ;        ax is the rop # [0-F]
	shiftl	bx	,4
	add	bx	,ax
	shiftl	bx	,1
	and	bx	,3FH
	mov	ax	,my_data_seg
	mov	es	,ax
	mov	ax	,cs:[mono_rop_and_mode_output][bx]
	mov	es:[rscan.rsoutput] ,ax

	lds	si	,lp_points
	mov	ax	,ds
	or	ax	,si
	jne	mono_main_loop_pre

do_scanline_mono_done_ws:
	jmp	do_scanline_done

mono_main_loop_pre:
	cmp	es:[rscan.rsvalid] ,1
	je	mono_main_loop
	jmp	do_scanline_done

public	mono_main_loop
mono_main_loop:
	dec	count
	je	do_scanline_mono_done_ws	;   then done

	mov	ax	,es:[rscan.rsoutput]
	mov	draw_output ,ax

	lds	si	,lp_points
						  ;use ycoord to determine
	mov	bx	,[si].ycoord		  ; which byte of the xpar
	mov	ax	,bx
	and	bx	,7			  ; mask and mono pattern to
	mov	cl	,es:[rscan.rsxpar_mask][bx]  ; use. Then place these
	mov	current_xpar_mask, cl		  ; variables on the stack.
	mov	cl	,es:[rscan.rsmono_pattern][bx]
	mov	current_mono_pattern, cl

	les	di	,lp_dst_dev		  ;This section computes a
	assume	es:nothing			  ;  the starting address of
        mov     bx      ,es:[di].bmSegmentIndex
	mov	cx	,es:[di].bmScanSegment	  ;  the destination scanline
	push	WORD PTR es:[di].bmWidthBytes
	les	di	,es:[di].bmBits
        or      bx      ,bx
	je	mono_main_loop_small
	mov	dx	,es

mono_main_loop_huge:
	add	dx	,bx
	sub	ax	,cx
	jae	mono_main_loop_huge

	sub	dx	,bx
	mov	es	,dx
	add	ax	,cx

mono_main_loop_small:
	pop	dx
	mul	dx
	add	di	,ax
	mov	start_of_scan ,di
	add	si	,4

public	mono_main_draw_loop
mono_main_draw_loop:

	lodsw
	mov	dx	,ax
	lodsw				;dx:ax = xstart:xend
	mov	di	,dx		;compute start addr of interval
	shiftr	di	,3
	add	di	,start_of_scan	;compute left byte masks
	mov	bx	,dx		;
	and	bx	,7
	mov	cl	,cs:[mono_left_edge_mask][bx]
	mov	bx	,ax		;compute right edge byte mask
	and	bx	,7
	mov	ch	,cs:[mono_right_edge_mask][bx]
	and	dx	,not 7		;compute number of whole bytes
	and	ax	,not 7
	sub	ax	,dx
	shiftr	ax	,3
	jne	mono_main_width_bigger_than_a_byte

	and	cl	,ch		;width less than 1 byte, so combine
	mov	ch	,0		;the left and right masks into the
	inc	ax			;left mask and set right mask to 0

mono_main_width_bigger_than_a_byte:
	dec	ax
	mov	bx	,cx		;bh:bl = right:left edge masks
	mov	cx	,ax
	mov	ah	,current_mono_pattern
	and	ah	,current_xpar_mask
	call	draw_output		;do the output rop routine

	dec	count
	jne	mono_main_draw_loop
	jmp	do_scanline_done



mono_left_edge_mask	LABEL	BYTE
db	0FFH, 07FH, 03FH, 01FH, 00FH, 007H, 003H, 001H

mono_right_edge_mask	LABEL	BYTE
db	000H, 080H, 0C0H, 0E0H, 0F0H, 0F8H, 0FCH, 0FEH




	assumes ds,nothing
	assumes es,Data

public	do_scanline_color
do_scanline_color:

	mov	exclude_flag ,0
	mov	bl	,COLOR_TYPE
	mov	es:[rscan.rstype] ,bl	;mark the type of the dst dev
	jmp	do_scanline_skip_screen

do_scanline_screen:
	mov	bl	,SCREEN_TYPE
	mov	es:[rscan.rstype] ,bl	;mark the type of the dst dev

do_scanline_skip_screen:
	mov	bh	,0FFH
	call	get_fill_data		;returns bx as the background mode
					;	 ax is the rop # [0-F]
	shiftl	bx	,4
	add	bx	,ax
	shiftl	bx	,1
        and     bx      ,3FH
	mov	ax	,my_data_seg
	mov	es	,ax
	mov	ax	,cs:[color_rop_and_mode_output][bx]
	mov	es:[rscan.rsoutput] ,ax

	lds	si	,lp_points
	mov	ax	,ds
	or	ax	,si
	jne	color_main_loop_pre


do_scanline_color_done_ws:
	jmp	do_scanline_color_done

color_main_loop_pre:
	cmp	es:[rscan.rsvalid] ,1
	je	color_main_loop
        jmp     do_scanline_done

public  color_main_loop
color_main_loop:
	dec	count
	je	do_scanline_color_done_ws    ;	 then done

	mov	ax	,es:[rscan.rsoutput]
        mov     draw_output ,ax

	mov	exclude_flag ,0
	cmp	es:[rscan.rstype] ,SCREEN_TYPE
	jne	color_main_loop_noex
	mov	exclude_flag ,1
	lds	si	,lp_points
	mov	dx	,[si].ycoord
	mov	di	,dx
	mov	bx	,count
	shiftl	bx	,2
	mov	cx	,[si+4].xcoord
	mov	si	,[bx + si].ycoord
	push	es
	call	exclude_far
	pop	es

color_main_loop_noex:
	lds	si	,lp_points
	mov	bx	,[si].ycoord		  ;use ycoord to determine
	mov	ax	,bx			  ; which byte of the xpar
	and	bx	,7
	mov	cl	,es:[rscan.rsxpar_mask][bx]  ; mask and mono pattern to
	mov	current_xpar_mask, cl		  ; use. Then place these
						  ; variables on the stack.
	shiftl	bx	,3
	mov	cx	,WORD PTR es:[rscan.rscolor_pattern][bx + 0]
	mov	WORD PTR [current_color_pattern + 0], cx
	mov	cx	,WORD PTR es:[rscan.rscolor_pattern][bx + 2]
	mov	WORD PTR [current_color_pattern + 2], cx
	mov	cx	,WORD PTR es:[rscan.rscolor_pattern][bx + 4]
	mov	WORD PTR [current_color_pattern + 4], cx
	mov	cx	,WORD PTR es:[rscan.rscolor_pattern][bx + 6]
	mov	WORD PTR [current_color_pattern + 6], cx

	mov	bl	,es:[rscan.rstype]
	les	di	,lp_dst_dev		  ;This section computes a
	assume	es:nothing			  ; the starting address of
	push	WORD PTR es:[di].bmWidthBytes

	cmp	bl	,SCREEN_TYPE
	jne	color_main_loop_memory
        les     di      ,es:[di].bmBits
	pop	dx				;these computations will
	mul	dx				;set the dest address if
	add	di	,ax		 ;the screen is the dest
	adc	dx	,0
	mov	start_of_scan ,di
	call	far_set_bank_select
	add	si	,4
	mov	bx	,si
	jmp	color_main_draw_loop

color_main_loop_memory:
        mov     cx      ,es:[di].bmScanSegment    ; the destination scanline
        mov     bx      ,es:[di].bmSegmentIndex
        les     di      ,es:[di].bmBits
	or	bx	,bx
	je	color_main_loop_small
	mov	dx	,es

color_main_loop_huge:
	add	dx	,bx			;these computations will
	sub	ax	,cx			;set the dest address if
	jae	color_main_loop_huge		;color memory is the dest

	sub	dx	,bx
	mov	es	,dx
	add	ax	,cx

color_main_loop_small:
	pop	dx
	mul	dx
	add	di	,ax
	mov	start_of_scan ,di
        add     si      ,4
	mov	bx	,si

public	color_main_draw_loop
color_main_draw_loop:

	mov	ax	,[bx]			;ax = xstart
	add	bx	,2
	mov	cx	,ax			;save it
	and	cx	,7			;compute the pattern rotation
	lea	si	,current_color_pattern
	add	si	,cx
	mov	dl	,1			;dl will have 1 in the bit
	rol	dl	,cl			;position corresponding to
	mov	dh	,current_xpar_mask	;the current pattern byte
	rol	dh	,cl			;rotate the xpar mask
	mov	cx	,[bx]			;cx = xend
	add	bx	,2
	sub	cx	,ax			;cx = xend - xstart
	je	color_main_draw_loop_skip
	mov	di	,start_of_scan
	add	di	,ax
	call	draw_output

color_main_draw_loop_skip:
	dec	count
	jne	color_main_draw_loop

do_scanline_color_done:

	cmp	exclude_flag ,1
	jne	do_scanline_done
	call	unexclude_far

do_scanline_done:

cEnd

	assume	ds:nothing, es:nothing

cProc	end_scanline,<FAR,PUBLIC,WIN,PASCAL>,<si,di,es,ds>
	parmD	lp_dst_dev		; --> to the destination
	parmW	style			; Output operation
	parmW	count			; # of points
	parmD	lp_points		; --> to a set of points
	parmD	lp_phys_pen		; --> to physical pen
	parmD	lp_phys_brush		; --> to physical brush
	parmD	lp_draw_mode		; --> to a Drawing mode
	parmD	lp_clip_rect		; --> to a clipping rectange if <> 0

cBegin
	mov	ds,cx			; cx holds ds from 'output' fixup
	mov	ds:[rscan.rsiterate] ,0

cEnd




;
;	get_fill_data
;
;
;	Entry:	 CURRENTLY NO ENTRY CONDITIONS: IGNORE BELOW
;
;		if ah is equal to FF
;
;                        1) The color brush pattern will be inverted as
;			    it is copied to local storage
;
;                        2) The mono brush pattern will be inverted as
;			    it is copied to local storage
;
;                        3) The transparency mask will be inverted as
;			    it is copied to local storage.
;
;		if ah equals 0 then no inversion occurs.
;
;	The routine will only copy the necessary data to local storage
;	depending upon the background mode (opaque or transparent) and the
;	type of the destination (mono or color).
;

	assumes ds,nothing
	assumes es,Data

	public	get_fill_data
get_fill_data	proc near

	lds	si	,lp_phys_brush
	mov	ax	,ds
	or	ax	,si
	jne	get_fill_brush

get_fill_pen:

        lds     si,lp_phys_pen                  ; --> physical pen
	mov	ax	,ds
	or	ax	,si
	je	get_fill_pen_done

	cmp	[si].oem_pen_style,LS_NOLINE
	je	get_fill_pen_done

	mov	es:[rscan.rsvalid] ,1		;mark data structure valid
	cmp	es:PaletteModified,0ffh ; was the palette modified ?
	jne	no_pen_xlat_needed
	cmp	bl	,SCREEN_TYPE
	jne	no_pen_xlat_needed

	mov	ds	,my_data_seg
	arg	lp_phys_pen
        cCall   TranslatePen                 ; translate the pen
        mov     ds      ,dx
        mov     si      ,ax                  ; load the local pen pointer

no_pen_xlat_needed:

	mov	ax	,wptr [si].oem_pen_pcol.pcol_Clr
	mov	cx	,SIZE_PATTERN * SIZE_PATTERN
	mov	di	,DataOFFSET rscan.rscolor_pattern
	rep	stosb

	mov	cx	,SIZE_PATTERN
	mov	al	,0FFH
	mov	di	,DataOFFSET rscan.rsxpar_mask
	rep	stosb

	shr	ah	,1			;MONO BIT of the pen color
	sbb	al	,al			; determines the mono pattern
	mov	cx	,SIZE_PATTERN
	mov	di	,DataOFFSET rscan.rsmono_pattern
        rep     stosb

get_fill_pen_done:
	mov	bx	,OPAQUE - 1
	lds	si	,lp_draw_mode
	mov	ax	,[si].Rop2
	dec	ax
        ret


get_fill_brush:

	mov	ax,[si].oem_brush_style ; Get brush style
	cmp	ax,MaxBrushStyle	; Legal?
	ja	get_fill_brush_1	; Outside range, return error
	cmp	ax,BS_HOLLOW		; Hollow?
	je	get_fill_brush_1	; Yes, return now.

	mov	es:[rscan.rsvalid] ,1
	cmp	es:PaletteModified,0ffh ; was the palette modified ?
	jne	no_brush_xlat_needed
	cmp	bl	,SCREEN_TYPE
	jne	no_brush_xlat_needed

	mov	ds	,my_data_seg
	arg	lp_draw_mode
        cCall   TranslateTextColor      ; translate foreground/background cols
        mov     seg_lp_draw_mode,dx
        mov     off_lp_draw_mode,ax     ; load the local pen pointer

	mov	ds	,my_data_seg
	arg	lp_phys_brush		;this call preserves es,si,di,bx,cx
	cCall	TranslateBrush		; translate the brush
	mov	ds	,dx
        mov     si      ,ax                  ; load the local pen pointer

no_brush_xlat_needed:
	mov	cx	,es
	mov	ax	,DataOFFSET rscan.rscolor_pattern
	les	di	,lp_draw_mode
	assume	es:nothing
	push	bp
	mov	bp	,my_data_seg
	call	far_pattern_preprocessing
	pop	bp

get_fill_brush_1:
	lds	si	,lp_draw_mode
	mov	bx	,[si].bkMode
	dec	bx
	mov	ax	,[si].Rop2
	dec	ax

	lds	si	,lp_phys_brush
        cmp     [si].oem_brush_style ,BS_HATCHED
        je      get_fill_brush_0

	push	ax
	mov	ax	,my_data_seg
        mov     es      ,ax
	mov	cx	,SIZE_PATTERN
	mov	al	,0FFH
	mov	di	,DataOFFSET rscan.rsxpar_mask
        rep     stosb
	pop	ax
	mov	bx	,OPAQUE - 1

get_fill_brush_0:

	ret

get_fill_data	endp

	include scanmono.inc
	include scancolo.inc

_SCANLINE	ends

       end

