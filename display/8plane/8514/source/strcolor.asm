page	,132
title           Colour Memory StrBlt Routines
.286c

REALFIX 	equ	1

.xlist
include CMACROS.INC
include gdidefs.inc
include drvpal.inc
.list

externFP	BoardBltFar		;in BOARDBLT.ASM
externA 	SMALLFONTLIMIT

sBegin          Data
externB 	ShadowMemoryTrashed	;in SAVESCRN.ASM
externB 	PaletteFlags
externB 	BitsPixel		;in Data.asm
externB 	WriteEnable		;in Data.asm
sEnd            Data

sBegin	Code
assumes cs, Code

externW _cstods

cProc           StrBltDummy,<FAR>

include 	STRBLT.INC		;contains stack definitions

cBegin	<nogen>

;This routine exists so that we set up a stack frame which is correct for 
;our common StrBlt stack frame.  It's never called but allows us to make
;near calls to StrBlt routines.

cEnd	<nogen>

BltFunction label   word
	dw	FillRect4BitsPixel
	dw	FillRect8BitsPixel

cProc	DrawOpaqueRect, <FAR, PUBLIC>

cBegin	<nogen> 			;don't mess up the stack frame
	mov	cx, Options
	jcxz	DORDoneNode		;no opaquing rect??
	test	cl, 04h
	jnz	DORDoneNode
	test	cl, 02h
	jnz	DORDrawOpaqueRect
DORDoneNode:
	jmp	short DORDone

DORDrawOpaqueRect:
	lds	si, lpDstDev
	assumes ds, nothing
	push	ss
	pop	es
	lea	di, MinimumClipRect
	sub	ax, ax
	stosw
	.errnz	left
	stosw
	.errnz	top-left-2
	mov	ax, [si].bmWidth
	stosw
	.errnz	right-top-2
	mov	ax, [si].bmHeight
	stosw
	.errnz	bottom-right-2
	sub	di, size RECT
	lds	si, lpOpaqueRect
	cCall	IntersectRect		;ES:DI-->Minimum clip rect
	jc	DORDoneNode		;if null intersect, exit now
	cmp	seg_lpClipRect, 0
	je	DORGetBltType
	lds	si, lpClipRect
	cCall	IntersectRect		;ES:DI-->Minimum clip rect
	jc	DORDoneNode
DORGetBltType:
	lds	si, lpDrawMode		;CL: background color out of draw mode
	mov	cl, [si][4]		;CL: byte ptr [si].bkColor
	mov	si, di			;SS:SI-->dest clip rect
	mov	ds, _cstods
	assumes ds, Data
	les	di, lpDstDev
	mov	al, [BitsPixel]
	cmp	al, es:[di].bmBitsPixel
	jne	DORDoneNode
	mov	ax, es:[di].bmSegmentIndex
	mov	SegmentIndex, ax
	mov	ax, es:[di].bmWidthBytes
	mov	WidthBytes, ax
	mov	bl, [WriteEnable]
	and	cl, bl			;strip off accelerator bytes if any
	mov	dl, bl
	not	dl
	mov	WritePlane, dl
	shr	bl, 3
	and	bl, 02h
	sub	bh, bh
	call	cs:BltFunction[bx]

DORDone:
	retf
cEnd	<nogen>

cProc	GetBitmapPointer, <NEAR, PUBLIC>

cBegin
	mov	ax, MinimumClipRect.top
	mul	WidthBytes
	mov	bx, ax
	mov	ax, dx
	mov	dx, SegmentIndex
	mul	dx
	les	di, es:[di].bmBits
	add	di, bx
	mov	dx, es
	add	dx, ax
	mov	es, dx			;ES:DI-->destination
cEnd

cProc	FillRect4BitsPixel, <NEAR, PUBLIC>

cBegin
	cCall	GetBitmapPointer
	mov	ax, MinimumClipRect.left
	shr	ax, 1
	jnc	FR4ByteAlligned
	not	WritePlane
FR4ByteAlligned:
	add	di, ax			;ES:DI-->1st pixel to draw
	mov	al, cl			;put color index in low+high nibble
	shl	al, 4
	or	al, cl
	mov	ah, WritePlane		;phase mask in AH
	mov	cx, MinimumClipRect.bottom
	sub	cx, MinimumClipRect.top ;CX: inner loop count
	jcxz	FR4Done
	mov	si, MinimumClipRect.right
	sub	si, MinimumClipRect.left;SI: outer loop count
FR4Loop:
	xchg	cx, si
	jcxz	FR4Done
	push	cx
	mov	bx, di
	or	ah, ah
	js	FR4EvenStart
	mov	dh, al
	and	dh, ah
	not	ah
	mov	dl, es:[di]
	and	dl, ah
	not	ah
	or	dl, dh
	mov	es:[di], dl
	inc	di
	dec	cx
FR4EvenStart:
	shr	cx, 1
	jcxz	FR4DoRemainder
rep	stosb
FR4DoRemainder:
	jnc	FR4BottomOfLoop
	mov	dl, al
	and	dl, 0f0h
	mov	dh, es:[di]
	and	dh, 0fh
	or	dh, dl
	mov	es:[di], dh
FR4BottomOfLoop:
	pop	cx			;restore inner loop count
	xchg	cx, si
	add	bx, WidthBytes
	jc	FR4HugeInc
	mov	di, bx
	loop	FR4Loop
	jmp	short FR4Done

FR4HugeInc:
	dec	cx
	jcxz	FR4Done
	mov	di, es
	add	di, SegmentIndex
	mov	es, di
	sub	di, di
	jmp	short FR4Loop

FR4Done:
cEnd

cProc	FillRect8BitsPixel, <NEAR, PUBLIC>

cBegin
	cCall	GetBitmapPointer
	add	di, ss:[si].left
	mov	al, cl			;AL: color index to fill rect with
	mov	cx, ss:[si].bottom	;CX: outer loop ocunt
	sub	cx, ss:[si].top
	jcxz	FR8Done
	mov	dx, ss:[si].right	;DX: inner loop count
	sub	dx, ss:[si].left
FR8Loop:
	xchg	cx, dx			;CX:inner loop count, DX: outer loop ct
	jcxz	FR8Done
	mov	si, cx
	mov	bx, di
rep	stosb
	mov	cx, si
	xchg	cx, dx			;CX: outer loop cnt, DX: inner loop cnt
	add	bx, WidthBytes
	jc	FR8HugeInc
	mov	di, bx
	loop	FR8Loop
	jmp	short FR8Done

FR8HugeInc:
	dec	cx
	jcxz	FR8Done
	mov	di, es
	add	di, SegmentIndex
	mov	es, di
	sub	di, di
	jmp	short FR8Loop

FR8Done:
cEnd

cProc	IntersectRect, <NEAR, PUBLIC>

cBegin
	mov	ax, es:[di].left
	mov	bx, es:[di].right
	mov	cx, [si].left
	mov	dx, [si].right
	cCall	IntersectLines
	jc	IRDone
	mov	es:[di].left, cx
	mov	es:[di].right, dx

	mov	ax, es:[di].top
	mov	bx, es:[di].bottom
	mov	cx, [si].top
	mov	dx, [si].bottom
	cCall	IntersectLines
	jc	IRDone
	mov	es:[di].top, cx
	mov	es:[di].bottom, dx
IRDone:
cEnd

cProc	IntersectLines, <NEAR, PUBLIC>

cBegin
	cmp	ax, cx
	je	ILCheckHiLimit
	jb	ILCheckLoLimit
	cmp	ax, dx
	jae	ILErrorExit
	mov	cx, ax			;clip src rhs to dst rhs
ILCheckHiLimit:
	cmp	bx, dx
	jae	ILDone			;i.e., NC
	mov	dx, bx
	clc
	jmp	short ILDone

ILCheckLoLimit:
	cmp	bx, cx
	jbe	ILErrorExit
	jmp	short ILCheckHiLimit

ILErrorExit:
	stc
ILDone:
cEnd

sEnd	Code

createSeg   _REAL, rCode, word, public, CODE
sBegin	    rCode
assumes     cs,rCode
assumes     ds,Data

externNP	RBoardStrBlt		;in STRBLT.ASM

cProc	RColourMemoryStrBlt, <NEAR, PUBLIC>

cBegin

;We take a very slow and complex strategy to perform this StrBlt.  This is
;because we hardly ever perform this routine (EXCEL drawing into a chart is
;the only place I've ever seen this thing needed).  The strategy that's used
;is the following:
;
;     1) If the BLT is to be transparent, call BoardBLT to BLT the destination
;        from main memory to the board's off screen workspace.
;
;     2) Play around with the clipping rectangle so that the string will be
;        clipped properly when BLTed to off-screen memory.
;
;     3) Call BoardStrBlt to BLT the string into invisible memory.
;
;     4) Call BoardBLT to read the string back into main memory.
;
;
;First, set up some common parameters for the two BoardBLTs:

	cCall	DrawOpaqueRect		;need to draw opaque rect? Do it now!!
	mov	cx, Count		;CX: number of characters to process
	or	cx, cx			;We're done if there are no characters
	jnz	NeedToDoSomeWork	;to be drawn
	jmp	CMSExit
NeedToDoSomeWork:
	mov	ShadowMemoryTrashed, 0	;make sure save area is trashed
	mov	BoardDstDev,2000h	;this is so BoardBlt will BLT
					;onto the board
	les	di,lpFont		;get FONTINFO into ES:DI
	mov	bx,es:[di+22]		;get font height
	mov	FontHeight,bx		;save this for later

;       Under normal circumstances, we would use the lowest Y-coordinate in
;off-screen memory as the place that we're going to BLT to.  This value
;is contained in the variable FreeSpaceyOrg.  However, it's possible that
;BoardStrBlt could change this variable's value due to the LoadFont 
;process.  Therefore, we use the highest Y-coordinate in off-screen memory
;to do the StrBlt to.  This is in fact, 1023-FontHeight-1.

	mov	ax,1022
	sub	ax,bx			;BX has the font height
	mov	StrColouryCoordinate,ax
	mov	ax,es:[di+27]		;get width of widest character
	inc	ax			;in the font
	inc	ax			;account for embolding as well
	mul	cx			;now AX has the worst-case
					;xExt of this BLT
	mov	xExt,ax 		;save this

public  CMSBAdjustCoordinates
CMSBAdjustCoordinates:

;It is unfortunately possible for StrBlts to have negative X & Y origins.
;Unfortunately, although our StrBlt code can handle this, BoardBLT cannot.
;Thus, we have to do input clipping on the origins.  We also must check to
;make sure our calculated extents do not exceed the size of the bitmap.  If
;they do, just use the ones from the BITMAP data structure:

	les	di,lpDstDev		;get the BITMAP data structure
	mov	bx,es:[di+2]		;get the size of the bitmap
	sub	bx,DstxOrg		;correct it for xOrg <> 0
	jle	CMSBACExit		;if negative or 0, there's
					;nothing to draw
	cmp	ax,bx			;do we run off the right side
					;of the bitmap?
	jb	CMSBAC1 		;nope, we're OK
	mov	xExt,bx 		;otherwise, replace our xExt

CMSBAC1:
	mov	ax,FontHeight
	mov	bx,es:[di+4]		;get the size of the bitmap
	sub	bx,DstyOrg		;correct it for yOrg <> 0
	jle	CMSBACExit		;if negative or 0, there's
					;nothing to draw
	cmp	ax,bx			;do we run off the bottom of
					;the bitmap?
	jb	CMSBAC2 		;nope, we're OK
	mov	FontHeight,bx		;otherwise, replace our yExt

CMSBAC2:
	xor	ax,ax			;AX will be our "input clip"
	mov	di,DstxOrg		;get our xOrg
	or	di,di			;is it negative?
	jns	CMSBAC3 		;nope, we're OK
	sub	ax,di			;now AX has the amount to clip
	xor	di,di			;and clip DI off to 0

CMSBAC3:
	sub	xExt,ax 		;subtract off amount clipped
	jle	CMSBACExit		;if negative, or zero there's
					;nothing to do
	mov	CMSDstxOrg,di
	xor	ax,ax			;AX will be our "input clip"
	mov	di,DstyOrg		;get our yOrg
	or	di,di			;is it negative?
	jns	CMSBAC4 		;nope, we're OK
	sub	ax,di			;now AX has the amount to clip
	xor	di,di			;and clip DI off to 0

CMSBAC4:
	mov	ClippedY,ax		;save the amount that we've
					;clipped in Y
	sub	FontHeight,ax		;subtract off amount clipped
	jle	CMSBACExit		;if negative or zero, there's
					;nothing to do!
	mov	CMSDstyOrg,di
	jmp	short CMSBBLTDownDest	;continue

public  CMSBACExit
CMSBACExit:
	jmp	CMSExit 		;get on out

public  CMSBBLTDownDest
CMSBBLTDownDest:

;Now call BoardBLT to get the destination into off-screen memory:

	lea	di,ss:BoardDstDev		;get the address as our
						;fake lpDstDev for the BLT
	mov	ax,StrColouryCoordinate 	;make FontHeight an extent
	add	ax,ClippedY			;adjust the BLT for negative
						;Y-coordinates
	and	PaletteFlags, (not NOMEMSTRBLT) ; signal `no color translation'
	arg	ss				;this is lpDstDev
	arg	di
	arg	CMSDstxOrg			;this is DstxOrg
	arg	ax				;this is DstyOrg
	arg	lpDstDev			;this is lpSrcDev
	arg	CMSDstxOrg			;this is SrcxOrg
	arg	CMSDstyOrg			;this is SrcyOrg
	arg	xExt				;this is xExt
	arg	FontHeight			;this is yExt
	arg	0cch				;this is Rop3 for SrcCopy
	arg	20h
	arg	0				;send down a dummy for lpPBrush
	arg	0
	arg	lpDrawMode			;this is lpDrawMode
	cCall	<far ptr BoardBltFar>		;get the destination onto the
                                                ;off-screen area of the board

public  CMSBGetClipRect
CMSBGetClipRect:

;Now comes the hard part.  We must take the original clipping rectangle passed
;to us and create a new clipping rectangle which will allow us to draw the
;correctly clipped string into invisible memory.  Clipping in X is no problem
;since we have the full width in X in off-screen memory.  The clipping problem
;exists in Y since we won't be at the same coordinates in terms of Y.  The
;way that we can superimpose our clipping rectangle on top of off-screen
;memory is according to the following equation:
;
;       AdderToClipRect = StrColouryCoordinate - DstyOrg
;       NewClipRectStartY = OriginalClipRectStartY + AdderToClipRect
;       NewClipRectEndY = OriginalClipRectEndY + AdderToClipRect
;
;By the way, we must also do this for the opaqueing rectangle if it exists.

	mov	cx,StrColouryCoordinate
	mov	dx,cx			;get it for later use too
	sub	cx,DstyOrg		;get the AdderToClipRect
	cmp	seg_lpClipRect,0	;any clipping rectangle?
	je	CMSBGetOpaquingRect	;nope, go do the opaquing rect
	les	di,lpClipRect
	mov	ax,es:[di+2]		;get the starting Y
	push	ax			;save the starting Y
	mov	bx,es:[di+6]		;get the ending Y
	push	bx			;save the ending Y
	add	ax,cx			;this is NewClipRectStartY
	mov	es:[di+2],ax
	add	bx,cx			;this is NewClipRectEndY
	mov	es:[di+6],bx		;now we have our new clip rect

public  CMSBGetOpaquingRect
CMSBGetOpaquingRect:
	cmp	seg_lpOpaqueRect,0	;any opaquing rectangle?
	je	CMSBDoStrBLT		;nope, continue
	les	di,lpOpaqueRect
	mov	ax,es:[di+2]		;get the starting Y
	push	ax			;save the starting Y
	mov	bx,es:[di+6]		;get the ending Y
	push	bx			;save the ending Y
	add	ax,cx			;this is NewClipRectStartY
	mov	es:[di+2],ax
	add	bx,cx			;this is NewClipRectEndY
	mov	es:[di+6],bx		;now we have our new clip rect

public  CMSBDoStrBLT
CMSBDoStrBLT:

;Now we must adjust the DstyOrg:

	push	DstyOrg 		;save our original DstyOrg
	mov	DstyOrg,dx		;get StrColouryCoord as our
					;DstyOrg
	push	xExt			;these may be destroyed by call
	push	FontHeight
	cCall	RBoardStrBlt		;go do the StrBlt to invisible
					;memory
	pop	FontHeight		;get back correct sizes for
	pop	xExt			;the bitmap readback
	pop	DstyOrg 		;get back original DstyOrg
	cmp	seg_lpOpaqueRect,0	;any lpOpaqueRect?
	je	CMSBRestoreClipRect	;nope, continue
	les	di,lpOpaqueRect 	;reload the opaqueing rectangle
	pop	es:[di+6]		;get back the original rect
	pop	es:[di+2]

public  CMSBRestoreClipRect
CMSBRestoreClipRect:
	cmp	seg_lpClipRect,0	;any lpClipRect?
	je	CMSBReadBack		;nope, continue
	les	di,lpClipRect		;reload the clip rectangle
	pop	es:[di+6]		;get back the original rect
	pop	es:[di+2]

public  CMSBReadBack
CMSBReadBack:

;Lastly, call BoardBLT to read the string back into the main memory bitmap.

	mov	bx,StrColouryCoordinate
	add	bx,ClippedY			;add on amount that we clipped
	lea	di,ss:BoardDstDev		;make a pointer to the phony
						;board PDevice structure
	and	PaletteFlags, (not NOMEMSTRBLT) ; signal `no color translation'
	arg	lpDstDev
	arg	CMSDstxOrg
	arg	CMSDstyOrg
	arg	ss				;this is lpSrcDev
	arg	di
	arg	CMSDstxOrg			;this is SrcxOrg
	arg	bx				;this is SrcyOrg
	arg	xExt				;this is xExt
	arg	FontHeight			;this is yExt
	arg	0cch				;this is Rop2 code for SrcCopy
	arg	20h
	arg	0				;send down a dummy lpPBrush
	arg	0
	arg	lpDrawMode
	cCall	<far ptr BoardBltFar>

public  CMSExit
CMSExit:
cEnd

sEnd	rCode

if REALFIX
createSeg   _PROTECT, pCode, word, public, CODE
sBegin	    pCode

assumes cs, pCode
assumes ds, Data
assumes es, nothing

externNP    PBoardStrBlt

cProc	PColourMemoryStrBlt, <NEAR, PUBLIC>

cBegin

;We take a very slow and complex strategy to perform this StrBlt.  This is
;because we hardly ever perform this routine (EXCEL drawing into a chart is
;the only place I've ever seen this thing needed).  The strategy that's used
;is the following:
;
;     1) If the BLT is to be transparent, call BoardBLT to BLT the destination
;        from main memory to the board's off screen workspace.
;
;     2) Play around with the clipping rectangle so that the string will be
;        clipped properly when BLTed to off-screen memory.
;
;     3) Call BoardStrBlt to BLT the string into invisible memory.
;
;     4) Call BoardBLT to read the string back into main memory.
;
;
;First, set up some common parameters for the two BoardBLTs:

	cCall	DrawOpaqueRect		;need to draw opaque rect? Do it now!!
	mov	cx, Count
	or	cx, cx			;We're done if there are no characters
	jnz	PNeedToDoSomeWork	;to be drawn
	jmp	PCMSExit
PNeedToDoSomeWork:
	mov	ShadowMemoryTrashed, 0	;make sure save area is trashed
	mov	BoardDstDev,2000h	;this is so BoardBlt will BLT
					;onto the board
	les	di,lpFont		;get FONTINFO into ES:DI
	mov	bx,es:[di+22]		;get font height
	mov	FontHeight,bx		;save this for later

;       Under normal circumstances, we would use the lowest Y-coordinate in
;off-screen memory as the place that we're going to BLT to.  This value
;is contained in the variable FreeSpaceyOrg.  However, it's possible that
;BoardStrBlt could change this variable's value due to the LoadFont 
;process.  Therefore, we use the highest Y-coordinate in off-screen memory
;to do the StrBlt to.  This is in fact, 1023-FontHeight-1.

	mov	ax,1022
	sub	ax,bx			;BX has the font height
	mov	StrColouryCoordinate,ax
	mov	ax,es:[di+27]		;get width of widest character
	inc	ax			;in the font
	inc	ax			;account for possible embolding
	mul	cx			;now AX has the worst-case
					;xExt of this BLT
	mov	xExt,ax 		;save this

public	PCMSBAdjustCoordinates
PCMSBAdjustCoordinates:

;It is unfortunately possible for StrBlts to have negative X & Y origins.
;Unfortunately, although our StrBlt code can handle this, BoardBLT cannot.
;Thus, we have to do input clipping on the origins.  We also must check to
;make sure our calculated extents do not exceed the size of the bitmap.  If
;they do, just use the ones from the BITMAP data structure:

	les	di,lpDstDev		;get the BITMAP data structure
	mov	bx,es:[di+2]		;get the size of the bitmap
	sub	bx,DstxOrg		;correct it for xOrg <> 0
	jle	PCMSBACExit		;if negative or 0, there's
					;nothing to draw
	cmp	ax,bx			;do we run off the right side
					;of the bitmap?
	jb	PCMSBAC1		;nope, we're OK
	mov	xExt,bx 		;otherwise, replace our xExt

PCMSBAC1:
	mov	ax,FontHeight
	mov	bx,es:[di+4]		;get the size of the bitmap
	sub	bx,DstyOrg		;correct it for yOrg <> 0
	jle	PCMSBACExit		;if negative or 0, there's
					;nothing to draw
	cmp	ax,bx			;do we run off the bottom of
					;the bitmap?
	jb	PCMSBAC2		;nope, we're OK
	mov	FontHeight,bx		;otherwise, replace our yExt

PCMSBAC2:
	xor	ax,ax			;AX will be our "input clip"
	mov	di,DstxOrg		;get our xOrg
	or	di,di			;is it negative?
	jns	PCMSBAC3		;nope, we're OK
	sub	ax,di			;now AX has the amount to clip
	xor	di,di			;and clip DI off to 0

PCMSBAC3:
	sub	xExt,ax 		;subtract off amount clipped
	jle	PCMSBACExit		;if negative, or zero there's
					;nothing to do
	mov	CMSDstxOrg,di
	xor	ax,ax			;AX will be our "input clip"
	mov	di,DstyOrg		;get our yOrg
	or	di,di			;is it negative?
	jns	PCMSBAC4		;nope, we're OK
	sub	ax,di			;now AX has the amount to clip
	xor	di,di			;and clip DI off to 0

PCMSBAC4:
	mov	ClippedY,ax		;save the amount that we've
					;clipped in Y
	sub	FontHeight,ax		;subtract off amount clipped
	jle	PCMSBACExit		;if negative or zero, there's
					;nothing to do!
	mov	CMSDstyOrg,di
	jmp	short PCMSBBLTDownDest	;continue

public	PCMSBACExit
PCMSBACExit:
	jmp	PCMSExit		;get on out

public	PCMSBBLTDownDest
PCMSBBLTDownDest:

;Now call BoardBLT to get the destination into off-screen memory:

	lea	di,ss:BoardDstDev		;get the address as our
						;fake lpDstDev for the BLT
	mov	ax,StrColouryCoordinate 	;make FontHeight an extent
	add	ax,ClippedY			;adjust the BLT for negative
						;Y-coordinates
	and	PaletteFlags, (not NOMEMSTRBLT) ; signal `no color translation'
	arg	ss				;this is lpDstDev
	arg	di
	arg	CMSDstxOrg			;this is DstxOrg
	arg	ax				;this is DstyOrg
	arg	lpDstDev			;this is lpSrcDev
	arg	CMSDstxOrg			;this is SrcxOrg
	arg	CMSDstyOrg			;this is SrcyOrg
	arg	xExt				;this is xExt
	arg	FontHeight			;this is yExt
	arg	0cch				;this is Rop3 for SrcCopy
	arg	20h
	arg	0				;send down a dummy for lpPBrush
	arg	0
	arg	lpDrawMode			;this is lpDrawMode
	cCall	<far ptr BoardBltFar>		;get the destination onto the
                                                ;off-screen area of the board

public	PCMSBGetClipRect
PCMSBGetClipRect:

;Now comes the hard part.  We must take the original clipping rectangle passed
;to us and create a new clipping rectangle which will allow us to draw the
;correctly clipped string into invisible memory.  Clipping in X is no problem
;since we have the full width in X in off-screen memory.  The clipping problem
;exists in Y since we won't be at the same coordinates in terms of Y.  The
;way that we can superimpose our clipping rectangle on top of off-screen
;memory is according to the following equation:
;
;       AdderToClipRect = StrColouryCoordinate - DstyOrg
;       NewClipRectStartY = OriginalClipRectStartY + AdderToClipRect
;       NewClipRectEndY = OriginalClipRectEndY + AdderToClipRect
;
;By the way, we must also do this for the opaqueing rectangle if it exists.

	mov	cx,StrColouryCoordinate
	mov	dx,cx			;get it for later use too
	sub	cx,DstyOrg		;get the AdderToClipRect
	cmp	seg_lpClipRect,0	;any clipping rectangle?
	je	PCMSBGetOpaquingRect	;nope, go do the opaquing rect
	les	di,lpClipRect
	mov	ax,es:[di+2]		;get the starting Y
	push	ax			;save the starting Y
	mov	bx,es:[di+6]		;get the ending Y
	push	bx			;save the ending Y
	add	ax,cx			;this is NewClipRectStartY
	mov	es:[di+2],ax
	add	bx,cx			;this is NewClipRectEndY
	mov	es:[di+6],bx		;now we have our new clip rect

public	PCMSBGetOpaquingRect
PCMSBGetOpaquingRect:
	cmp	seg_lpOpaqueRect,0	;any opaquing rectangle?
	je	PCMSBDoStrBLT		;nope, continue
	les	di,lpOpaqueRect
	mov	ax,es:[di+2]		;get the starting Y
	push	ax			;save the starting Y
	mov	bx,es:[di+6]		;get the ending Y
	push	bx			;save the ending Y
	add	ax,cx			;this is NewClipRectStartY
	mov	es:[di+2],ax
	add	bx,cx			;this is NewClipRectEndY
	mov	es:[di+6],bx		;now we have our new clip rect

public	PCMSBDoStrBLT
PCMSBDoStrBLT:

;Now we must adjust the DstyOrg:

	push	DstyOrg 		;save our original DstyOrg
	mov	DstyOrg,dx		;get StrColouryCoord as our
					;DstyOrg
	push	xExt			;these may be destroyed by call
	push	FontHeight
	cCall	PBoardStrBlt		;go do the StrBlt to invisible
					;memory
	pop	FontHeight		;get back correct sizes for
	pop	xExt			;the bitmap readback
	pop	DstyOrg 		;get back original DstyOrg
	cmp	seg_lpOpaqueRect,0	;any lpOpaqueRect?
	je	PCMSBRestoreClipRect	;nope, continue
	les	di,lpOpaqueRect 	;reload the opaqueing rectangle
	pop	es:[di+6]		;get back the original rect
	pop	es:[di+2]

public	PCMSBRestoreClipRect
PCMSBRestoreClipRect:
	cmp	seg_lpClipRect,0	;any lpClipRect?
	je	PCMSBReadBack		;nope, continue
	les	di,lpClipRect		;reload the clip rectangle
	pop	es:[di+6]		;get back the original rect
	pop	es:[di+2]

public	PCMSBReadBack
PCMSBReadBack:

;Lastly, call BoardBLT to read the string back into the main memory bitmap.

	mov	bx,StrColouryCoordinate
	add	bx,ClippedY			;add on amount that we clipped
	lea	di,ss:BoardDstDev		;make a pointer to the phony
						;board PDevice structure
	and	PaletteFlags, (not NOMEMSTRBLT) ; signal `no color translation'
	arg	lpDstDev
	arg	CMSDstxOrg
	arg	CMSDstyOrg
	arg	ss				;this is lpSrcDev
	arg	di
	arg	CMSDstxOrg			;this is SrcxOrg
	arg	bx				;this is SrcyOrg
	arg	xExt				;this is xExt
	arg	FontHeight			;this is yExt
	arg	0cch				;this is Rop2 code for SrcCopy
	arg	20h
	arg	0				;send down a dummy lpPBrush
	arg	0
	arg	lpDrawMode
	cCall	<far ptr BoardBltFar>

public	PCMSExit
PCMSExit:
cEnd
sEnd	    pCode
endif
end
