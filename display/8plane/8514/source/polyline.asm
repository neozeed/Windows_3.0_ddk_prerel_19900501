page            ,132
title           Polylines Support for the IBM 8514
.286c

.xlist
include         CMACROS.INC
include 	8514.INC
include 	gdidefs.inc
.list

sBegin          Data
externB 	Rop2TranslateTable
externB 	WriteEnable
sEnd            Data

sBegin          Code
assumes         cs,Code
externFP	CursorExclude		;in ROUTINES.ASM
externFP	SetScreenClipFar	;in OUTPUT.ASM
sEnd            Code


subttl          Code Area
page +
createSeg       _OUTPUT,OutputSeg,word,public,CODE
sBegin          OutputSeg
assumes         cs,OutputSeg
assumes         ds,Data

externW 	DrawModeTbl		;in DRAWMODE.ASM
externB 	rot_bit_tbl		;in PIXEL.ASM


subttl          Data For Polyline Support
page +


subttl          Code for Polyline Support
page +
cProc           OutputDummy,<FAR>

include 	OUTPUT.INC		;contains stack definitions

cBegin          <nogen>     

;This routine exists so that we set up a stack frame which is correct for 
;our common Output stack frame.  It's never called but allows us to make
;near calls to Output routines.

cEnd            <nogen>


page +
cProc           Polylines,<NEAR,PUBLIC>,<ds>

cBegin

;First, set the cursor exclusion.  We want to use the clipping rectangle
;as our cursor exclusion area.  If there is no clip rectangle, then
;unconditionaly remove the cursor from the screen while the drawing is going on.
;Then, set the scissor clip.

	mov	ax,ds			;make ES = DS
	mov	es,ax
	assumes es, Data
	cmp	BandFlags,0ffh		;is this the outline of a
					;filled polygon or rectangle?
	je	PLGetPen		;yes, no need to set the clip
	cmp	seg_lpClipRect,0	;any clipping rectangle?
	jne	PLGetCursorExcludeArea	;nope, get rid of the cursor
	push	0fffeh			;set unconditional exclusion
	sub	sp,6			;correct stack for 3 parameters
	cCall	CursorExclude
	jmp	short PLGetPen		;go get the pen
public  PLGetCursorExcludeArea
PLGetCursorExcludeArea:
	lds	si,lpClipRect		;get clip rectangle into ES:DI
	lodsw
	mov	bx,ax			;get starting X
	lodsw
	mov	cx,ax			;get starting Y
	lodsw
	mov	dx,ax			;get ending X
	lodsw				;get ending Y
	arg	bx			;send these to CursorExclude
	arg	cx
	arg	dx
	arg	ax
	cCall	CursorExclude		;go exclude the cursor
	lds	si,lpClipRect		;reestablish clip rectangle
	call	SetScreenClipFar	;go set the clipping rect to
					;lpClipRect
	jnc	PLGetPen		;if valid clip rect, continue

PLLeave:
	jmp	PLEnd			;otherwise, unexclude cursor
					;and leave now
public  PLGetPen
PLGetPen:

;Next, get and set our pen (style will always be solid since the 8514 doesn't
;know how to do styled lines).

	lds	si,lpPPen		;get the pen into DS:SI
	lodsw				;now AH has the pen colour
	cmp	al,5			;is this a null pen?
	je	PLLeave 		;yes, get out now!
	mov	cx,ax			;save pen in CX
public  PLSetRop2
PLSetRop2:

;Now, set the foreground colour and pattern:

CheckFIFOSpace  THREE_WORDS
	lds	si,lpDrawMode		;get the drawmode into ES:DI
	lodsw				;get the ROP into AX
	dec	ax			;make it offset from 0
	mov	bx,DataOFFSET Rop2TranslateTable
	xlat	es:Rop2TranslateTable	;now AL has the proper function

;At this point:
;       AL has our foreground function.

	or	al,20h			;look at foreground colour
	mov	dx,FUNCTION_1_PORT	;set foreground function
	out	dx,ax
	mov	al,ch			;get colour into AX
	and	al, es:[WriteEnable]	; use only 16 colors
	mov	dx,COLOUR_1_PORT
	out	dx,ax			;set foreground colour

;Now set up the mode:

	mov	dx,MODE_PORT
	mov	ax,0a000h		;set to "no pattern" mode
	out	dx,ax
public  PLSetupLoop
PLSetupLoop:

;OK, now it's time to set up our polyline loop:

	mov	cx,Count		;make CX a point loop counter
	dec	cx			;but correct for extra point
					;(since points are connected)
	jle	PLEnd			;if no points, get out now
	lds	si,lpPoints		;DS:SI points at our point array
public  PLLoop
PLLoop:

;The 8514 line command requires a coded direction flag.  We set ourselves up
;with a default command in BX and then adjust it as needed by the direction
;and major axis of the line:

	CheckFIFOSpace	SEVEN_WORDS
	push	cx			;save our loop counter
	xor	cx,cx			;this is either 0 or -1
	mov	bx,2017h		;get default cmd into BX
	lodsw				;get starting X
	mov	dx,Srcx_PORT		;get it onto board
	out	dx,ax
	mov	di,ax			;save it in DI
	lodsw				;get starting Y
	cmp	di,1536 		;is starting X too big?
	jae	PLEndLoop		;yes, skip this line
	cmp	ax,1536 		;is starting Y too big?
	jae	PLEndLoop		;yes, skip this line
	mov	dx,Srcy_PORT		;get it onto board
	out	dx,ax
	sub	di,[si] 		;now DI has X-length
	jns	PLL_1			;if not negative, go on
	or	bl,20h			;if negative, change direction
	neg	di			;negate the X-length
	dec	cx			;make CX = -1
PLL_1:                                           
	sub	ax,[si+2]		;subtract off ending Y
	jns	PLL_2			;if not negative, go on
	or	bl,80h			;if negative, change direction
	neg	ax			;negate the Y-length
PLL_2:

;At this point, 
;               DI has the X-length
;               AX has the Y-length
;               BX has the command to this point
;               CX contains either 0 or -1

	cmp	di,ax			;see which axis is major
	ja	PLL_3			;if X is major, go on
	or	bl,40h			;set Y-major flag in command
	xchg	ax,di			;and exchange the lengths
PLL_3:
	shl	ax,1			;send out minor axis to K1
	mov	dx,K1_PORT
	out	dx,ax

;Now calculate the error term:

	sub	ax,di			;subtract minor from major axis
	xchg	ax,cx			;get error term
	add	ax,cx
	mov	dx,ERROR_TERM_PORT	;and put it out thru port
	out	dx,ax
	xchg	ax,cx			;get back AX
	sub	ax,di			;subtract off major extent
	mov	dx,K2_PORT		;and send it out as K2
	out	dx,ax
	mov	dx,RECT_WIDTH_PORT	;send out X-extent
	mov	ax,di
	out	dx,ax
	mov	ax,bx			;get completed command
	mov	dx,COMMAND_FLAG_PORT
	out	dx,ax			;and send it on out
PLEndLoop:
	pop	cx			;restore saved loop counter
	loop	PLLoop			;loop for all lines
PLEnd:
cEnd

if 0
cProc	GetXForY, <NEAR, PUBLIC>
cBegin
	sub	ax, Y2
	imul	bx
	mov	di, ax
	mov	ax, X2
	imul	cx
	add	ax, di
	mov	di, cx			;now add 1/2 of CX to dividend
	sar	di, 1
	add	ax, di
	cwd				;sign extention into DX
	idiv	cx			;now Y is in AX
cEnd

cProc	GetYForX, <NEAR, PUBLIC>
cBegin
	sub	ax, X2
	imul	cx
	mov	di, ax
	mov	ax, Y2
	imul	bx
	add	ax, di
	mov	di, bx			;now add 1/2 of CX to dividend
	sar	di, 1
	add	ax, di
	cwd				;sign extention into DX
	idiv	bx			;now Y is in AX
cEnd

ClipFunction	label	word
	dw	ClipTop
	dw	ClipBottom
	dw	ClipNOP
	dw	ClipRight
	dw	ClipTopRight
	dw	ClipBottomRight
	dw	ClipNOP
	dw	ClipLeft
	dw	ClipTopLeft
	dw	ClipBottomLeft

cProc	ClipNOP, <NEAR>
cBegin
	int	3			;this function should NEVER get called!
	sub	ax, ax			;return 0
	cwd
cEnd

cProc	ClipTopLeft, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.top	;Y coordinate we need to get X for
	cCall	GetXForY
	mov	dx, RectPoints.top	;anticipate success (early exit)
	cmp	ax, RectPoints.left
	jge	CTLDone
	mov	ax, RectPoints.left
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.left
CTLDone:
cEnd

cProc	ClipBottomLeft, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.bottom
	cCall	GetXForY
	mov	dx, RectPoints.bottom
	cmp	ax, RectPoints.left
	jge	CBLDone
	mov	ax, RectPoints.left
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.left
CBLDone:
cEnd

cProc	ClipTopRight, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.top	;Y coordinate we need to get X for
	cCall	GetXForY
	mov	dx, RectPoints.top	;anticipate success (early exit)
	cmp	ax, RectPoints.right	;is X intercept acceptable?
	jle	CTRDone 		;yes! Get out
	mov	ax, RectPoints.right
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.right
CTRDone:
cEnd

cProc	ClipBottomRight, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.bottom
	cCall	GetXForY
	mov	dx, RectPoints.bottom
	cmp	ax, RectPoints.right
	jle	CBRDone
	mov	ax, RectPoints.right
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.right
CBRDone:
cEnd

cProc	ClipTop, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.top	;Y coordinate we need to get X for
	cCall	GetXForY
	mov	dx, RectPoints.top
cEnd

cProc	ClipBottom, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.bottom
	cCall	GetXForY
	mov	dx, RectPoints.bottom
cEnd

cProc	ClipRight, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.right
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.right
cEnd

cProc	ClipLeft, <NEAR, PUBLIC>
cBegin
	mov	ax, RectPoints.left
	cCall	GetYForX
	mov	dx, ax
	mov	ax, RectPoints.left
cEnd
if 0
cProc	ComputeAbs, <NEAR, PUBLIC>
cBegin
	mov	ax, cx			;CX: 2*delta Y
	cwd				;compute absolute value of delta X
	xor	ax, dx
	sub	ax, dx
	mov	di, ax
	mov	ax, bx			;BX: 2*delta X
	cwd				;now compute absolute value of delta Y
	xor	ax, dx
	sub	ax, dx
	cmp	ax, di
cEnd
endif
cProc	ClipLinePoints, <FAR, PUBLIC>	;will trash SI and DI
cBegin	nogen				;it is assumed that most points will
	sub	si, si			;  not need any clipping at al and will
	sub	di, di			;  not be rejected.
	cmp	dx, RectPoints.left	;is X of 1st pt inside clip rect (GE)?
	jl	ClipLinePoints_110	;DX: X1
ClipLinePoints_10:
	cmp	dx, RectPoints.right	;is X of 1st pt inside clip rect (LE)?
	jg	ClipLinePoints_120
ClipLinePoints_20:
	cmp	cx, RectPoints.bottom	;CX: Y1
	jg	ClipLinePoints_130
ClipLinePoints_30:
	cmp	cx, RectPoints.top
	jl	ClipLinePoints_140
ClipLinePoints_40:
	cmp	bx, RectPoints.left	;BX: X2
	jl	ClipLinePoints_150
ClipLinePoints_50:
	cmp	bx, RectPoints.right
	jg	ClipLinePoints_160
ClipLinePoints_60:
	cmp	ax, RectPoints.bottom	;AX: Y2
	jg	ClipLinePoints_170
ClipLinePoints_70:
	cmp	ax, RectPoints.top
	jl	ClipLinePoints_180
ClipLinePoints_80:
	test	si, di			;is the line completely outside the
	jnz	CLPRejectLine		; clip rect?  If NZ, then reject line.

	cmp	dx, bx			;now check whether the end points for
	je	CLPTestIfIdenticalInY	; the line are identical.  If so, we
CLPLineVisible: 			; can reject the line, too.

	sub	ax, cx			;AX: delta Y
	sub	bx, dx			;BX: delta X
	mov	cx, si
	or	cx, di
	jz	CLPNoClipping
	mov	cx, ax
	shl	cx, 1			;CX: 2*(Y2-Y1) (2*delta Y)
	shl	bx, 1			;BX: 2*(X2-X1) (2*delta X)
	dec	di			;DI==0 then don't clip 1st point (saves
	js	CLPFirstPointOkay	; one entry in dispatch table)
	shl	di, 1			;DI: offset into clip function table
	cCall	cs:[di].ClipFunction	;AX: clipped X, DX: clipped Y
	mov	X1, ax			;save the clipped 1st point.
	mov	Y1, dx
CLPFirstPointOkay:
	dec	si			;now do the same with the second point
	js	CLPNoClipping
	shl	si, 1			;SI: offset into clip function table
	cCall	cs:[si].ClipFunction
	mov	X2, ax
	mov	Y2, dx

CLPNoClipping:
	clc
	retf

ClipLinePoints_110:
	add	di, 8
	jmp	short ClipLinePoints_10
ClipLinePoints_120:
	add	di, 4
	jmp	short ClipLinePoints_20
ClipLinePoints_130:
	inc	di
	inc	di
	jmp	short ClipLinePoints_30
ClipLinePoints_140:
	inc	di
	jmp	short ClipLinePoints_40
ClipLinePoints_150:
	add	si, 8
	jmp	short ClipLinePoints_50
ClipLinePoints_160:
	add	si, 4
	jmp	short ClipLinePoints_60
ClipLinePoints_170:
	inc	si
	inc	si
	jmp	short ClipLinePoints_70
ClipLinePoints_180:
	inc	si
	jmp	short ClipLinePoints_80

CLPTestIfIdenticalInY:			;if the line is just a single point,
	cmp	ax, cx			;then don't draw it!
	jne	CLPLineVisible
CLPRejectLine:
	stc
	retf
cEnd	nogen
endif

sEnd            OutputSeg
end
