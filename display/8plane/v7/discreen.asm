        page    ,132
;----------------------------------------------------------------------------;
;                            DIScreeBlt                                      ;
;                            ----------                                      ;
;  Blts a portion of a Device Independent Bitmap directly onto the screen,or ;
;  viceversa.                                                                ;
;                                                                            ;
;               . The Device Independent Bitmap can have 1/4/8/24 bits/pel   ;
;                 but only one plane.                                        ;
;               . length in byte of 1 scan does not exceed 64k               ;
;               . The target device can be a mono chrome or 3/4 plane EGA/VGA;
;                 display.                                                   ;
;               . No RasterOperations are supported and direct copy is done. ;
;               . Bound checking of the source rectangle is done against the ;
;                 screen extents.                                            ;
;               . Returns 1 in AX to indicate success (0 => failure)         ;
;                                                                            ;
;  History.                                                                  ;
;     . Created.                                                             ;
;       -by-  Amit Chatterjee  [amitc]   on   Dec-06-1988  07:25:52          ;
;     . Modified.                                                            ;
;       -by-  Irene Wu, Video 7          2/89                                ;
;----------------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC."

	.xlist
	include	cmacros.inc
	include	gdidefs.inc
	include	display.inc
	include	macros.mac
	include	cursor.inc
	include rlecom.inc
	include rledat.inc
	.list

;----------------------------------------------------------------------------;
; define the equates and externAs here.                                      ;
;----------------------------------------------------------------------------;

	externA		__NEXTSEG	; offset to next segment

	externA	ScreenSelector		; video segment address
	externA	COLOR_FORMAT		; own color format

MAP_IS_HUGE	equ	10000000b	; source maps spans segments

;----------------------------------------------------------------------------;

	externFP	sum_RGB_alt_far
	externFP	exclude_far		; xeclude cursor from blt area
	externFP	unexclude_far		; redraw cursor
	externFP	far_set_bank_select

sBegin Data

	externB	enabled_flag
	externD 	GetColor_addr		; DeviceColorMatch vector

sEnd Data


createSeg   _DIMAPS,DIMapSeg,word,public,code
sBegin DIMapSeg
	assumes cs,DIMapSeg
	assumes ds,Data
	assumes es,nothing


InitProc	label	word	; has the init routine addresses
	dw	DIMapSegOFFSET init_1_bits_per_pel
	dw	DIMapSegOFFSET init_4_bits_per_pel
	dw	DIMapSegOFFSET init_8_bits_per_pel
	dw	DIMapSegOFFSET init_24_bits_per_pel


cProc	DIBScreenBlt,<FAR,PUBLIC,WIN,PASCAL>,<si,di,es,ds>

	parmD	lp_pdevice		; pointer to device structure
	parmW	ScrXorg			; Screen origin X coordinate
	parmW	ScrYorg			; Screen origin Y coordinate
	parmW	StartScan
	parmW	Num_Scans
	parmD	lp_cliprect
	parmD	lp_drawmode
	parmD	lp_bits			; pointer to DI bits
        parmD   lp_bi                   ; pointer to bitmap info block
        parmD   lpColorInfo             ; GDI supplied value for color matching

	localW	xExt			; X extent of BLT
	localW	yExt			; Y extent of BLT
	localW	MapXorg			; Map origin X coordinate
	localW	MapYorg


	localB	fbFlags			; flag bytes

	localW	MapWidth		; width of map in pels
	localW	MapHeight		; height of map in scans
	localW	MapBitCount		; no of bits per map pel
	localB	MapBitSrl		; 1->0,4->1,8->2,24->3
	localW	next_map_scan		; offset to next map scan

	localD	lp_screen		; pointer to start byte of screen
	localW	scr_width
	localW	scr_height

	localW	bank_wrap_flag
        localW  top_of_dib
	localW	bottom_of_dib
	localW	left_of_dib
	localW	right_of_dib
	localW	rle_xorg
	localW	rle_yorg
	localW	rle_type
	localW	encode_rle		;ptr to specific encoding routine
	localW	encode_absolute 	;ptr to specific encoding routine
	localW	decode_rle		;ptr to specific decoding routine
	localW	decode_absolute 	;ptr to specific decoding routine

        localW  next_screen_scan        ; offset to next screen scan

	localV	color_xlate,512 	; the local color xlate table

        localD  DeviceColorMatch        ; GDI color match routine

	localW	nibble_permutation_sign ; for 4bpp dibs whether the high or low
					; nibble is left edge of the cliprect
        localW  left_shift_amount       ; alignment shift count for mono dibs
	localW	first_byte_count	; 8 - left_shift_amount
	localW	inner_loop_count	; xExt-first_byte_count-last_byte_count
	localW	last_byte_count 	; number mono pels (bits) in last byte
	localW	full_proc		; routine to call to blt a byte/scan
	localW	get_byte		; holds address of fetch proc
	localW	SourceBytesPerScanBlt	; no of bytes in map for 1 scan blt
	localB	bank_select		; track the current bank of display

	localW	FoundRGB
        localW  AllocRGB
	localV	LastRGBl,1024		 ; holds last R-G-B from src/GDI
	localV	LastRGBh,1024		 ; holds last R-G-B from src/GDI

cBegin

WriteAux <'DIBScreenBlt'>		;***** DEBUG *****

	cld
	mov	al,enabled_flag		; error if screen is not enabled
	or	al,al
	jz	parameter_error_relay	; will not blt now

;----------------------------------------------------------------------------;
; do a few validations at this point:                                        ;
;               .  pointer to bits must not be NULL                          ;
;               .  destination must be a device and not a bitmap             ;
;               .  calculate the actual blt rectangle on the screen, this    ;
;                  has to be clipped against the clip rectangle and can't    ;
;                  be NULL                                                   ;
;----------------------------------------------------------------------------;

	les	di,lp_bits		; get the pointers to the bits
	assumes	es,nothing

; test for souce pointer to be non NULL

	mov	ax,es
	or	ax,di			; NULL implies error
	jz	parameter_error_relay	; can't support NULL pointer

	les	di,lp_pdevice		; get the pdevice structure
	assumes	es,nothing

; test for target to be a screen

	mov	cx,es:[di].bmType	; test the type of the structure
	jcxz	parameter_error_relay	; can't support memory bitmaps here

; set mono chrome or color destination flag

	xor	al,al
	mov	fbFlags,al		; reset the flag byte
	mov	ax,word	ptr es:[di].bmPlanes
	cmp	ax,COLOR_FORMAT		; our screen format?
	jnz	parameter_error_relay	; don't support this format

; save screen-relevant parameters

	mov	ax,wptr	es:[di].bmBits[0]
	mov	off_lp_screen,ax
	mov	ax,wptr	es:[di].bmBits[2]
	mov	seg_lp_screen,ax

	mov	ax,es:[di].bmWidthBytes
	mov	next_screen_scan,ax
	mov	ax,es:[di].bmWidth
	mov	scr_width,ax
	mov	ax,es:[di].bmHeight
	mov	scr_height,ax

; calculate the exclusion area

	lds	si,lp_bi		; pointer to the bitmap header
	mov	ax,wptr [si].biWidth	; the horixontal extent of map
	mov	MapWidth,ax		; save it
	mov	ax,wptr [si].biHeight	; the height of the map
	mov	MapHeight,ax		; save it
	mov	ax,[si].biBitCount	; get the bits per pel
	mov	MapBitCount,ax		; save it

;----------------------------------------------------------------------------;
; validate the bits/pel count at this point. It has to be 1/4/8 or 24        ;
;----------------------------------------------------------------------------;
	xor	cx,cx			; will build up the serial no
	IRP	x,<1,4,8,24>
	cmp	ax,x
	je	bit_count_ok		; valid count
	inc	cl
	ENDM
parameter_error_relay:
	jmp	parameter_error

bit_count_ok:
	mov	MapBitSrl,cl
;----------------------------------------------------------------------------;
	cmp	[si].biPlanes,1		; no of color planes has to be 1
	jnz	parameter_error_relay	; its an error


	?_pub	clip_dib
clip_dib:
        mov     ax      ,ScrXorg
	mov	si	,scr_width
	cmp	ax	,si
	jg	parameter_error_relay
	neg	si
	cmp	ax	,si
	jl	parameter_error_relay

	mov	ax	,ScrYorg
	mov	si	,scr_height
	cmp	ax	,si
	jg	parameter_error_relay
	neg	si
	cmp	ax	,si
	jl	parameter_error_relay

	mov	ax	,StartScan
	cmp	ax	,MapHeight
	jnc	parameter_error_relay

	add	ax	,Num_Scans
	cmp	ax	,MapHeight
	ja	parameter_error_relay

	les	bx, lp_cliprect
	mov	ax	,es
	or	ax	,bx
	jz	parameter_error_relay

	mov	si, ScrYorg
	mov	di, ScrXorg
	mov	rle_xorg ,di
	mov	rle_yorg ,si

; top of dib = max(ScrYorg + StartScan, clip_top)

	mov	ax	,si
	add	ax	,StartScan
	cmp	ax	,es:[bx].top
	jg	gottop
	mov	ax	,es:[bx].top
gottop: mov	top_of_dib ,ax


; bottom of dib = min(ScrYorg + MapHeight, scr_height, clip_bottom)

	mov	ax	,si
	add	ax	,MapHeight
	cmp	ax	,scr_height
	jc	bot0
	mov	ax	,scr_height
bot0:	cmp	ax	,es:[bx].bottom
	jc	gotbot
	mov	ax	,es:[bx].bottom
gotbot: mov	bottom_of_dib ,ax


; left of dib = max(ScrXorg, clip_left)

	mov	ax	,di
	or	ax	,ax
	cmp	ax	,es:[bx].left
	jg	gotlef
	mov	ax	,es:[bx].left
gotlef: mov	left_of_dib ,ax


; right of dib = min(ScrXorg + MapWidth, scr_width, clip_right)

	mov	ax	,di
	add	ax	,MapWidth
	cmp	ax	,scr_width
	jc	rig0
	mov	ax	,scr_width
rig0:	cmp	ax	,es:[bx].right
	jc	gotrig
	mov	ax	,es:[bx].right
gotrig: mov	right_of_dib ,ax


        mov     ax      ,bottom_of_dib
	sub	ax	,top_of_dib
	jle	parameter_error
	cmp	ax	,Num_Scans
	jle	gotyXt
	mov	ax	,Num_Scans
gotyXt: mov	yExt	,ax

        mov     ax      ,right_of_dib
	sub	ax	,left_of_dib
	jle	parameter_error
        mov     xExt    ,ax

	mov	ax	,MapHeight
	sub	ax	,bottom_of_dib
	add	ax	,ScrYorg
	mov	MapYorg ,ax

	mov	ax	,left_of_dib
	sub	ax	,ScrXorg
	mov	MapXorg ,ax

        mov     ax      ,top_of_dib
	mov	ScrYorg ,ax

	mov	ax	,left_of_dib
	mov	ScrXorg ,ax
	jmp	clip_complete


parameter_error:
	xor	ax,ax
	jmp	DIScreenBlt_Ret		; error return
	db	"COPYRIGHT FEBRUARY 1990 HEADLAND TECHNOLOGY, INC."

clip_complete:
	les	bx     ,lp_bi
	mov	ax     ,WORD PTR es:[bx].biCompression
	mov	rle_type ,ax
	cmp	ax     ,BI_RLE8 	; RLE_FORMAT_8
	je	encoded
	cmp	ax     ,BI_RLE4 	; RLE_FORMAT_4
	jne	not_encoded
						;
encoded:
	jmp	rle_to_screen

not_encoded:


;----------------------------------------------------------------------------;
; we shall also calculate the no of byte in the source map that correspond to;
; xExt bits, as we shall use this information to test whther we cross a segm-;
; -ent on the source during the blt of xExt pels on a scan.                  ;
;----------------------------------------------------------------------------;

	mov	ax,xExt			; get the no of bits in 1 scan blt
	mul	MapBitCount		; multiply be no of bits per pel

; our assumption is that one scan does not cross a segnent, so ignore dx
; GLM - assumption is that one scan < 8K (i.e.,	64K bits)

	shiftr	ax,3			; get the number of bytes
	add	ax,1			; take the cealing
	mov	SourceBytesPerScanBlt,ax; save it for use later


;----------------------------------------------------------------------------;
; exclude the cursor from the blt area of the screen.                        ;
;----------------------------------------------------------------------------;

ifdef EXCLUSION

	mov	cx,ScrXorg		; X1
	mov	dx,ScrYorg		; Y1
	mov	si,right_of_dib 	; X2
	mov	di,bottom_of_dib	; Y2
	call	exclude_far		; exclude cursor from blt area

endif

;----------------------------------------------------------------------------;
; calculate the offset to the next scan in the map (scans there are DWORD    ;
; alligned ).                                                                ;
;----------------------------------------------------------------------------;

	mov	ax,MapWidth		; get the no of pels in a scan
	mul	MapBitCount		; bits per pel
	add	ax,31
	and	ax,not 31		; ax has multiple of 32 bits

; assume that the offset to the next scan fits in  a word, ignore DX

	shiftr	ax,3			; get the no of bytes
	mov	next_map_scan,ax	; save it

;----------------------------------------------------------------------------;
; calculate the offset to the start byte in the map and the screen and the   ;
; position of the first pel in the screen byte.                              ;
;----------------------------------------------------------------------------;


; NOTE: THIS WILL BE RADICALLY DIFFERENT FOR RLE DIBs

	mov	ax,MapXorg		; get the X offset of the map
	mul	MapBitCount
	mov	bx,ax
	shiftr	bx,3			; number of bytes into scan line

	mov	ax,MapYorg		; get the Y origin of the map
	mul	next_map_scan		; the result is in DX:AX

	add	ax,bx
	adc	dl,dh

	jz	update_map_start_offset	; if no segment overflow

; update the start segment also

	mov	bx,seg_lp_bits		; the segment part of lp_bits
map_segment_update:
	add	bx,__NEXTSEG		; make it go to next segment
	dec	dl
	jnz	map_segment_update
	mov	seg_lp_bits,bx		; update the segment part of ptr
update_map_start_offset:
	add	off_lp_bits,ax		; update the offset
	sbb	ax	,ax
	and	ax	,__NEXTSEG
	add	seg_lp_bits, ax

; lp_bits now points to the first byte of the map in the blt area

;----------------------------------------------------------------------------;
;  the starting scan no might not have crossed a segment but the blt area    ;
;  might still cross a segment, test for that.                               ;
;----------------------------------------------------------------------------;

	mov	ax,yExt			; get the Y ext of the blt
	mul	next_map_scan		; bytes per scan on the map
	add	ax,off_lp_bits		; add the first byte offset
	adc	dl,dh			; do a 32 bit addition
	jz	blt_area_in_1_segment
	or	fbFlags,MAP_IS_HUGE	; will have to test for segment update
blt_area_in_1_segment:

;----------------------------------------------------------------------------;
; now calculate the address of the start byte in the screen blt area.        ;
;----------------------------------------------------------------------------;

	mov	ax,ScrYorg		; get the Y origin (top scan)
	add	ax,yExt			; start from bottom
	dec	ax
	mul	next_screen_scan	; no of bytes in a scan
					; dx:ax = bank:offset

	add	ax,ScrXorg		;shouldn't be a carry with 1K scanlines
	add	ax,off_lp_screen	; this is the start offset
	adc	dl,dh
	mov	bank_select,dl

	mov	off_lp_screen,ax	; save it
	call	far_set_bank_select	; set bank to dl

; lp_screen now has the pointer to the first byte in the screen blt area

;----------------------------------------------------------------------------;
; now get the address of the full and partial byte proces that we will be    ;
; using depending on the bits per pel.                                       ;
;----------------------------------------------------------------------------;

	les	bx,lp_bi		; ES:BX points to the info block
	mov	di,bx
	add	di,wptr	es:[bx].bcSize	; ES:DI has the color table
	xor	bx,bx
	mov	bl,MapBitSrl		; get the bits per pel serial num
	shl	bx,1			; look up a word table
	mov	bx,InitProc[bx]		; get the appropriate init procs addr
	jmp	bx			; do the format specific inits

;----------------------------------------------------------------------------;
;  the format specific initialization routines follow here.                  ;
;  ES:DI poits to the color table.                                           ;
;----------------------------------------------------------------------------;

init_1_bits_per_pel:

	mov	ax,DIMapSegOFFSET copy_1_bp_full
	mov	full_proc,ax

	mov	ax	,MapXorg
	mov	cx	,xExt
	and	ax	,07H
	mov	left_shift_amount ,ax

        neg     ax
	add	ax	,8
	cmp	ax	,cx
	jb	@F
	mov	ax	,cx
@@:
        mov     first_byte_count ,ax
	sub	cx	,ax
	mov	ax	,cx
	and	ax	,not 07H
	mov	inner_loop_count ,ax
	and	cx	,07H
	mov	last_byte_count ,cx

; fill in the color table with the two color values

	mov	cx,2
	call	create_color_table	; translates the color into indices
	jmp	init_done

init_4_bits_per_pel:

	mov	ax,DIMapSegOFFSET copy_4_bp_full
	mov	full_proc,ax

; fill in the color table with the 16 color values

	mov	ax	,MapXorg
	and	ax	,1
	mov	nibble_permutation_sign ,ax

	mov	cx,16
	call	create_color_table	; translates the color into indices
	jmp	init_done

init_8_bits_per_pel:

	mov	ax,DIMapSegOFFSET copy_8_bp_full
	mov	full_proc,ax

; fill in the color table with the 256 color values

	mov	cx,256
	call	create_color_table	; translates the color into indices
	jmp	init_done

init_24_bits_per_pel:

	mov	ax,DIMapSegOFFSET copy_24_bp_full
	mov	full_proc,ax
	push	ds
	mov	ax, _DATA
	mov	ds,ax
	assumes ds,Data
	lds	ax,ds:[GetColor_addr]	; fetch the GDI vector
	mov	wptr DeviceColorMatch[0],ax
	mov	wptr DeviceColorMatch[2],ds
	pop	ds
	assumes	ds,nothing

	push	es

	mov	ax	,ss
	mov	es	,ax
	sub	ax	,ax
        mov     AllocRGB,ax
	mov	FoundRGB,ax

	lea	di	,LastRGBl
	mov	cx	,512
	rep	stosw

        push    ax
	push	ax
        push    wptr lpColorInfo[2]
	push	wptr lpColorInfo[0]
	call	DeviceColorMatch

	mov	cx	,ss
	mov	es	,cx
	lea	di	,LastRGBh
	mov	cx	,512
	mov	ah	,al
	xor	al	,al
	rep	stosw

        pop     es
        jmp     init_done



;----------------------------------------------------------------------------;
; the color convert routine reads a color triplet from the map specific color;
; table - converts it into an index and stores it in a table on the stack. It;
; does the conversion for the count of triplets passed in CX                 ;
;----------------------------------------------------------------------------;

create_color_table	proc	near

; CX     ---  count of triplets to converts
; ES:DI  ---  place where the triplets are stored
	push	si
	push	di

	lea	si,color_xlate		; index into xlate table on stack
xlate_next_color:
	mov	al,es:[di]
	mov	ss:[si],al
	add	di,2
	inc	si
	loop	xlate_next_color

	pop	di
	pop	si
	ret

create_color_table	endp

;----------------------------------------------------------------------------;

init_done:

;----------------------------------------------------------------------------;
; The sorce bytes in a scan may cross a segment boundary only if the flag -  ;
; MAP_IS_HUGE is set.                                                        ;
;                                                                            ;
; If the flag is set a test will be made at the start of every scan line to  ;
; see whether that scan crosses the segment boundary or not. If it does cross;
; a segment boundary, every byte will be fetched with a test to detect the   ;
; segment cross if not then 'LODSB' will be used to get the byte.            ;
;                                                                            ;
; We assume that a scan will not cross two segment boundaries.               ;
;----------------------------------------------------------------------------;

	lds	si,lp_bits		; the strting point of the bits
	assumes	ds,nothing
	les	di,lp_screen		; the start on the screen
	assumes	es,nothing
	lea	bx,color_xlate		; address of the xlate table

	mov	cx,yExt			; the number of scans to copy

blt_next_scan:

; assume that segment crossings will not be required
	mov	get_byte,DIMapSegOFFSET get_byte_wo_test

	test	fbFlags,MAP_IS_HUGE	; will the blt cross a segment
	jz	blt_next_scan_continue	; segment crossing check not required

; test to see if the current scan will span a segment

	mov	ax,si			; set current offset
	xor	dx,dx
	add	ax,SourceBytesPerScanBlt; no of bytes involved in 1 scan blt
	adc	dl,dh			; collect the carry if any
	jz	blt_next_scan_continue	; this scan will be in segment

; the scan will cross a segment somewhere, so use the get_byte proc which tests
; for segment crossings

	mov	get_byte,DIMapSegOFFSET get_byte_with_test

blt_next_scan_continue:
	push	cx			; save scan loop count
	push	ds			; save map segment
	push	si
	push	di			; save the source and target pointers

	mov	cx,xExt			; the no of pels to blt
	call	full_proc		; do the blt of a scan

	pop	di
	pop	si			; get back the pointers
	pop	ds
	pop	cx			; get back the no of scans left to blt

	sub	di,next_screen_scan	; we map top to bottom
	jnc	new_scan_in_bank

	dec	bank_select
	mov	dl,bank_select
	call	far_set_bank_select

new_scan_in_bank:
	add	si,next_map_scan	; update to the next scan
	jnc	new_scan_in_segment	; the new scan is in segment

; the new scan crosses a segment, has to be in the next segment, so take DS
; into the next segment

	mov	ax,ds			; get current DS
	add	ax,__NEXTSEG		; update to the next segment
	mov	ds,ax

new_scan_in_segment:
	loop	blt_next_scan		; blt all the scans
	jmp	reset_registers		; reset the register and exit



;----------------------------------------------------------------------------;
;                       copy_1_bp_full                                       ;
;                       --------------                                       ;
;  handles the blt of a scan or a part of it for 1 bits per pixel case       ;
;                                                                            ;
;  Entry:                                                                    ;
;               DS:SI     --      current byte in the map                    ;
;               ES:DI     --      current byte in the screen                 ;
;                  BX     --      address of color_xlate table on stack      ;
;                  CX     --      number of pels to convert                  ;
;                                                                            ;
;  Returns:                                                                  ;
;               DS:SI,ES:DI --    next bytes in respective areas             ;
;                     BX,DX --    unchanged                                  ;
;                     AL,CX --    destroyed                                  ;
;----------------------------------------------------------------------------;


copy_1_bp_full	proc	near

	call	get_byte
	mov	ah	,al
	mov	cx	,left_shift_amount
	shl	ah	,cl
	mov	cx	,first_byte_count

copy_1_bp_first_byte:

	xor	al,al
	shl	ah,1			; get the next pel into carry
	rcl	al,1
	xlat	ss:[bx]			; get the converted value
	stosb
	loop	copy_1_bp_first_byte	; convert the byte

	mov	cx	,inner_loop_count	; guaranteed cx = 0 modulo 8
	jcxz	copy_1_bp_last_byte_prep

copy_1_bp_full_loop:
	call	get_byte		; get the first byte
	mov	ah	,al
	push	cx
	mov	cx	,8

copy_1_bp_same_byte:

	xor	al,al
	shl	ah,1			; get the next pel into carry
	rcl	al,1
	xlat	ss:[bx]			; get the converted value
	stosb
	loop	copy_1_bp_same_byte	; convert the byte

	pop	cx			; get back remaining bits
	sub	cx	,8		; 8 were just converted
	ja	copy_1_bp_full_loop	; CX <= 0 means we have done all

copy_1_bp_last_byte_prep:
	mov	cx	,last_byte_count
	jcxz	copy_1_bp_done
        call    get_byte
	mov	ah	,al

copy_1_bp_last_byte:

	xor	al,al
	shl	ah,1			; get the next pel into carry
	rcl	al,1
	xlat	ss:[bx]			; get the converted value
	stosb
	loop	copy_1_bp_last_byte	; convert the byte

copy_1_bp_done:
        ret

copy_1_bp_full	endp

;----------------------------------------------------------------------------;
;                       copy_4_bp_full                                       ;
;                       --------------                                       ;
;  handles the blt of a scan or a part of it for 4 bits per pixel case       ;
;                                                                            ;
;  Entry:                                                                    ;
;               DS:SI     --      current byte in the map                    ;
;               ES:DI     --      current byte in the screen                 ;
;                  BX     --      address of color_xlate table on stack      ;
;                  CX     --      number of pels to convert                  ;
;                                                                            ;
;  Returns:                                                                  ;
;               DS:SI,ES:DI --    next bytes in respective areas             ;
;                     BX,DX --    unchanged                                  ;
;                     AL,CX --    destroyed                                  ;
;----------------------------------------------------------------------------;

copy_4_bp_full	proc	near

	test	nibble_permutation_sign ,1	; if sign is odd then do low
	je	copy_4_bp_full_loop		;    nibble of byte
	call	get_byte

copy_4_bp_full_lownib:
	and	al,0fh			; get the lower 4 bits
	xlat	ss:[bx]			; get the mapping index
	stosb
        dec     cx                      ; one more pel done
	jcxz	copy_4_bp_done

copy_4_bp_full_loop:
	call	get_byte		; process first byte separately
	mov	ah,al			; save the byte
	shiftr	al,4			; get the high nibble into low pos
	xlat	ss:[bx]			; get the mapping index
	stosb
        mov     al,ah                   ; get back pel
	loop	copy_4_bp_full_lownib	; process all the remaining pels

copy_4_bp_done:
	ret

copy_4_bp_full	endp
;----------------------------------------------------------------------------;
;                       copy_8_bp_full                                       ;
;                       --------------                                       ;
;  handles the blt of a scan or a part of it for 8 bits per pixel case       ;
;                                                                            ;
;  Entry:                                                                    ;
;               DS:SI     --      current byte in the map                    ;
;               ES:DI     --      current byte in the screen                 ;
;                  BX     --      address of color_xlate table on stack      ;
;                  CX     --      number of pels to convert                  ;
;                                                                            ;
;  Returns:                                                                  ;
;               DS:SI,ES:DI --    next bytes in respective areas             ;
;                     BX,DX --    unchanged                                  ;
;                     AL,CX --    destroyed                                  ;
;----------------------------------------------------------------------------;

copy_8_bp_full	proc	near

; here every source byte contributes one pel to the destination

        call    get_byte                ; get the next source byte
	xlat	ss:[bx]			; get the mapping index value
	mov	es:[di],al		; xfer it to the screen
	inc	di			; the next screen byte
	loop	copy_8_bp_full		; repeat till all bytes processed

	ret

copy_8_bp_full	endp
;----------------------------------------------------------------------------;
;                       copy_24_bp_full                                      ;
;                       ---------------                                      ;
;  handles the blt of a scan or a part of it for 24 bits per pixel case      ;
;                                                                            ;
;  Entry:                                                                    ;
;               DS:SI     --      current byte in the map                    ;
;               ES:DI     --      current byte in the screen                 ;
;                  CX     --      number of pels to convert                  ;
;                                                                            ;
;  Returns:                                                                  ;
;               DS:SI,ES:DI --    next bytes in respective areas             ;
;                        DX --    unchanged                                  ;
;                  AL,BX,CX --    destroyed                                  ;
;----------------------------------------------------------------------------;

c24slower:
        call    get_byte_with_test      ; get the blue pel
	mov	dl,al			; have blue in dl
        call    get_byte_with_test      ; get green
	mov	ah,al			; have it in ah
	call	get_byte_with_test	; get red in al
        jmp     c24_compare

public	copy_24_bp_full
copy_24_bp_full proc    near

c24_full_loop:

        push    cx
        push    es
	push	di

	cmp	si,0fffdh
	jae	c24slower
	lodsw
	mov	dl	,BYTE PTR ds:[si]
	inc	si

c24_compare:
	mov	cx	,ss
	mov	es	,cx
	lea	bx	,LastRGBh
	lea	di	,LastRGBl
	sub	bx	,di

	mov	cx	,FoundRGB
	shl	cx	,1
	add	di	,cx
	mov	cx	,512
	sub	cx	,FoundRGB

c24_firstlook:
        repne   scasw
	jne	c24_trysecondpart
	cmp	dl	,BYTE PTR es:[di + bx - 2]
	jne	c24_firstlook
	sub	cx	,511
	neg	cx
	mov	FoundRGB,cx
        mov     al      ,BYTE PTR es:[di + bx - 1]
	jmp	c24noconvert

c24_trysecondpart:
	lea	di	,LastRGBl
	mov	cx	,FoundRGB

c24_secondlook:
        repne   scasw
	jne	c24convertit
	cmp	dl	,BYTE PTR es:[di + bx - 2]
	jne	c24_secondlook
	sub	cx	,FoundRGB
	not	cx
	mov	FoundRGB,cx
        mov     al      ,BYTE PTR es:[di + bx - 1]
	jmp	c24noconvert

c24convertit:
	mov	di	,AllocRGB
	dec	di
	and	di	,1FFH
	mov	AllocRGB,di
	mov	FoundRGB,di
        shl     di      ,1
	mov	word ptr [di + LastRGBl] ,ax
	mov	byte ptr [di + LastRGBh] ,dl

	xor	dh	,dh
        push    dx
	push	ax
	push	wptr lpColorInfo[2]
	push	wptr lpColorInfo
	call	DeviceColorMatch
;
	mov	byte ptr [di + LastRGBh + 1] ,al
;
c24noconvert:
	pop	di
	pop	es
	pop	cx
        stosb
	dec	cx
	je	copy_24_full_exit
	jmp	c24_full_loop
;
copy_24_full_exit:
        ret

copy_24_bp_full	endp



;----------------------------------------------------------------------------;
;  The following two routines will be used to fetch the next source byte     ;
;  The first one is to be used when no segment crossing is to be tested and  ;
;  the next one when the scan at some point will cross a segment. The address;
;  of one of these two routinbes will be put in 'get_byte'                   ;
;----------------------------------------------------------------------------;

get_byte_wo_test	proc	near
	lodsb				; get the next byte
	ret
get_byte_wo_test	endp


get_byte_with_test	proc	near
	lodsb				; get the current byte
	or	si,si			; si wraps to 0 ?
	jnz	get_byte_with_test_ret	; no, we are in same segment
	push	dx			; save
	mov	dx,ds			; get current segment
	add	dx,__NEXTSEG		; go to the next segment
	mov	ds,dx			; update segment

; here we assume that the rest of the current scan will not cross a segment
; boundary again, so will use the shorter get byte proc

	mov	get_byte,DIMapSegOFFSET get_byte_wo_test
	pop	dx			; restore
get_byte_with_test_ret:
	ret				; get back

get_byte_with_test	endp
;----------------------------------------------------------------------------;
; finally reset the EGA/VGA registers to the their original state and return ;
; to caller wih success/failure.                                             ;
;----------------------------------------------------------------------------;

reset_registers:

	assumes	es,nothing
ifdef	EXCLUSION
	call	unexclude_far		; re-draw the cursor
endif

;
; return back with success code
;
	mov	ax,1			; success code
DIScreenBlt_Ret:

cEnd

public	rle_to_screen
rle_to_screen	proc	near

	call	rlescr_decode_bitmap
	jmp	reset_registers

rle_to_screen	endp



public	rlescr_decode_bitmap
rlescr_decode_bitmap  proc    near

	les	bx	,lp_bi
	mov	ax	,WORD PTR es:[bx].biCompression

        mov     si      ,0
	mov	cx	,16			;number of color in 4bits
	cmp	ax	,BI_RLE4
	je	rlescr_external_type_found

        add     si      ,2
	mov	cx	,256			;number of colors in 8bits
	cmp	ax	,BI_RLE8
	je	rlescr_external_type_found
	jmp	rlescr_decode_end

rlescr_external_type_found:

	mov	ax, decode_rle_table[si]
	mov	decode_rle	,ax
	mov	ax	,decode_absolute_table[si]
	mov	decode_absolute	,ax

	lds	si	,lp_bi
	add	si	,wptr es:[bx].biSize
	mov	ax, ss			; get stack segment into es
	mov	es, ax
	lea	di, color_xlate
	rep	movsw			; word indices

ifdef EXCLUSION

	mov	cx,left_of_dib		; X1
	mov	dx,top_of_dib		; Y1
	mov	si,right_of_dib 	; X2
	mov	di,bottom_of_dib	; Y2
	call	exclude_far		; exclude cursor from blt area

endif
        lds     si      ,lp_bits
	les	bx	,lp_bi
        mov     ax      ,rle_yorg
	add	ax	,WORD PTR es:[bx].biHeight

	dec	ax
	push	ax
	mul	WORD PTR next_screen_scan
	les	di	,lp_screen
	cmp	rle_xorg,0
	jns	rlescr_pos_coord
	add	ax	,rle_xorg
	cmc
	sbb	dx	,0
	jmp	short rlescr_neg_coord

rlescr_pos_coord:
	add	ax	,rle_xorg

rlescr_neg_coord:
        mov     di      ,ax
	mov	bank_select,dl
        call    far_set_bank_select
	pop	dx			;ax was pushed earlier with startscan
	mov	bx	,rle_xorg
	mov	bank_wrap_flag ,0
        push    di

public	rlescr_decode_next
rlescr_decode_next:

	call	getword

	cmp	al	,0
	jne	rlescr_decode_rle

	cmp	ah	,0
	jne	rlescr_decode_next_0
	jmp	rlescr_decode_end_of_line

rlescr_decode_next_0:
        cmp     ah      ,1
	jne	rlescr_decode_next_1
	jmp	rlescr_decode_end

rlescr_decode_next_1:
	cmp	ah	,2
	jne	rlescr_decode_next_2
        jmp     rlescr_decode_skip

rlescr_decode_next_2:
	jmp	rlescr_decode_absolute




public	rlescr_decode_rle
rlescr_decode_rle:

	mov	cl	,al
	sub	ch	,ch
	cmp	dx	,top_of_dib
	jl	rlescr_decode_rle_0
	cmp	dx	,bottom_of_dib
	jge	rlescr_decode_rle_0
	cmp	bx	,right_of_dib
	jge	rlescr_decode_rle_0
	add	bx	,cx
	cmp	bx	,left_of_dib
	jle	rlescr_decode_rle_1
	sub	bx	,cx

	push	dx
        push    cx
	push	bx
	push	ax

	mov	dx	,right_of_dib	;compute amt of run clipped off right
	sub	dx	,bx
	sub	dx	,cx
	sbb	ax	,ax
	and	dx	,ax
	neg	dx

	cmp	bx	,left_of_dib	;compute amt of run clipped off left
	jge	no_left_clip_on_rle
	sub	bx	,left_of_dib
	neg	bx
	jmp	short left_clip_on_rle

no_left_clip_on_rle:
	sub	bx	,bx

left_clip_on_rle:
	add	di	,bx		;advance ptr past clipped left edge
	jnc	neg_coord_hurt0 	;might wrap a bank if left edge of
	mov	ax	,dx		;  image is off the left edge of the
	inc	bank_select		;  screen
	mov	dl	,bank_select
	call	far_set_bank_select
	mov	dx	,ax
	mov	bank_wrap_flag ,1

neg_coord_hurt0:
        sub     cx      ,bx
        sub     cx      ,dx             ;sub clipped runs from total run length

        shr     bx      ,1              ;if bx is odd, then for external 4 bit
					;  we will begin with the low nibble
					;  pixel
        pop     ax
	mov	al	,ah
        lea     bx      ,color_xlate
        call    decode_rle
	add	di	,dx		;update dest past right clipped area
	pop	bx
	pop	cx
	pop	dx
	add	bx	,cx
	jmp	rlescr_decode_next

rlescr_decode_rle_0:
	add	bx	,cx

rlescr_decode_rle_1:
        add     di      ,cx             ;update dest past clipped area
	jnc	neg_coord_hurt1 	;might wrap a bank if left edge of
	mov	ax	,dx		;  image is off the left edge of the
	inc	bank_select		;  screen
	mov	dl	,bank_select
	call	far_set_bank_select
	mov	dx	,ax
	mov	bank_wrap_flag ,1

neg_coord_hurt1:
        jmp     rlescr_decode_next



public	rlescr_decode_absolute
rlescr_decode_absolute:

	mov	cl	,ah
	sub	ch	,ch
        cmp     dx      ,top_of_dib
	jge	rlescr_decode_abs_skip

rlescr_decode_abs_relay:
	jmp	rlescr_decode_abs_2

rlescr_decode_abs_relay1:
	jmp	rlescr_decode_abs_3

rlescr_decode_abs_skip:
	cmp	dx	,bottom_of_dib
	jge	rlescr_decode_abs_relay
	cmp	bx	,right_of_dib
	jge	rlescr_decode_abs_relay
	add	bx	,cx
	cmp	bx	,left_of_dib
	jle	rlescr_decode_abs_relay1
	sub	bx	,cx

	push	dx
        push    cx
	push	bx

	mov	dx	,right_of_dib	;compute amt of run clipped off right
	sub	dx	,bx
	sub	dx	,cx
	sbb	ax	,ax
	and	dx	,ax
	neg	dx

	cmp	bx	,left_of_dib	;compute amt of run clipped off left
	jge	no_left_clip_on_abs
	sub	bx	,left_of_dib
	neg	bx
	jmp	short left_clip_on_abs

no_left_clip_on_abs:
	sub	bx	,bx

left_clip_on_abs:
	add	di	,bx		;advance ptr past clipped left edge
	jnc	neg_coord_hurt2 	;might wrap a bank if left edge of
	mov	ax	,dx		;  image is off the left edge of the
	inc	bank_select		;  screen
	mov	dl	,bank_select
	call	far_set_bank_select
	mov	dx	,ax
	mov	bank_wrap_flag ,1

neg_coord_hurt2:
        sub     cx      ,bx
        sub     cx      ,dx             ;sub clipped runs from total run length
	mov	ax	,bx

	cmp	rle_type ,BI_RLE4	;if the external format is 4 bit pixels
	jne	rlescr_decode_abs_not4	;  the number of BYTES clipped off the
	shr	ax	,1		;  left is half the number of PIXELS

rlescr_decode_abs_not4:
	add	si	,ax
	jnc	rlescr_decode_abs_0
	mov	ax	,ds
	add	ax	,__NEXTSEG
	mov	ds	,ax
rlescr_decode_abs_0:

	shr	bx	,1		;if bx is odd, then for external 4 bit
					;  we will begin with the low nibble
					;  pixel
        lea     bx      ,color_xlate
	call	decode_absolute
	mov	ax	,dx
	adc	ax	,0		;carry flag returned by decode_absolute

	cmp	rle_type ,BI_RLE4	;if the external format is 4 bit pixels
	jne	rlescr_decode_abs_aint4 ;  the number of BYTES clipped off the
	shr	ax	,1		;  right is half the number of PIXELS

rlescr_decode_abs_aint4:
	inc	ax
	add	si	,ax
	jnc	rlescr_decode_abs_1
	mov	ax	,ds
	add	ax	,__NEXTSEG
	mov	ds	,ax
rlescr_decode_abs_1:

	and	si	,0FFFEH
	add	di	,dx		;update dest past right clipped area
        pop     bx
	pop	cx
	pop	dx
	add	bx	,cx
	jmp	rlescr_decode_next

rlescr_decode_abs_2:
	add	bx	,cx

rlescr_decode_abs_3:
	add	di	,cx
	jnc	neg_coord_hurt3 	;might wrap a bank if left edge of
	mov	ax	,dx		;  image is off the left edge of the
	inc	bank_select		;  screen
	mov	dl	,bank_select
	call	far_set_bank_select
	mov	dx	,ax
	mov	bank_wrap_flag ,1

neg_coord_hurt3:
	cmp	rle_type ,BI_RLE4	;if the external format is 4 bit pixels
	jne	rlescr_decode_abs_darn4 ;  the number of BYTES clipped
        inc     cx
	shr	cx	,1		;  is half the number of PIXELS

rlescr_decode_abs_darn4:
        inc     cx
        add     si      ,cx
	jnc	rlescr_decode_abs_4
	mov	ax	,ds
	add	ax	,__NEXTSEG
	mov	ds	,ax
rlescr_decode_abs_4:
	and	si	,0FFFEH
        jmp     rlescr_decode_next

public rlescr_decode_end_of_line
rlescr_decode_end_of_line:

	pop	di
	sub	di	,next_screen_scan
	jc	rlescr_decode_end_of_line_0
	cmp	bank_wrap_flag ,0
	je	rlescr_decode_end_of_line_1

rlescr_decode_end_of_line_0:
	dec	bank_select
	mov	ax	,dx
	mov	dl	,bank_select
	call	far_set_bank_select
	mov	dx	,ax

rlescr_decode_end_of_line_1:
	dec	dx
	mov	bx	,rle_xorg
	mov	bank_wrap_flag ,0
        push    di
        jmp     rlescr_decode_next

public	rlescr_decode_skip
rlescr_decode_skip:

	call	getbyte
	xor	ah,ah
	add	bx, ax
	call	getbyte
	xor	ah,ah
	sub	dx, ax

	; ax contains the delta y value
	; update start of scan value on stack
	pop	di
	push	dx

	mul	word PTR next_screen_scan   ; modulo arithmetic to compute
	sub	di,ax			    ;  new start of scan, note
					    ;  the clip rectangle must be
	pop	dx			    ;  considered here, so it is
	push	di			    ;  best to make it a relative calc

	; compute new address based on skip
	push	bx
	push	dx
	mov	ax	,dx

	mul	WORD PTR next_screen_scan
	les	di	,lp_screen

	add	ax	,bx
	adc	dl	,dh
	add	di	,ax
	adc	dl	,dh

	mov	bank_select,dl
        call    far_set_bank_select
	mov	bank_wrap_flag ,0

	pop	dx
	pop	bx
	jmp	rlescr_decode_next

public	rlescr_decode_end
rlescr_decode_end:

	pop	di
        ret

rlescr_decode_bitmap  endp


public	getbyte
getbyte proc	near

	lodsb
	or	si  ,si
	jne	getbyte_0
	mov	si   ,ds
	add	si  ,__NEXTSEG
	mov	ds  ,si
	sub	si  ,si

getbyte_0:
	ret

getbyte endp


public	getword
getword proc	near

	cmp	si	,0FFFEH
	ja	getword_0
	lodsw
	je	getword_1
	ret

getword_0:
	lodsb
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si
	mov	ah	,ds:[si]
	inc	si
	ret

getword_1:
	mov	si	,ds
	add	si	,__NEXTSEG
	mov	ds	,si
	sub	si	,si
	ret

getword endp

sEnd	DIMapSeg

END










sEnd DIMapSeg

	end
