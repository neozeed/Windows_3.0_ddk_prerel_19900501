        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	OUTPUT.ASM
;
;   This module contains the dispatch routine for the Output function.
;
; Created: 22-Feb-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	Output
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
;   Those functions of output which are supported by this driver
;   are dispatched to.
;
; Restrictions:
;
;-----------------------------------------------------------------------;


	??_out	output

incOutput	= 1			;Include control for gdidefs.inc

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include macros.mac
	.list

	externFP do_scanlines
	externFP do_polylines


sBegin	Code
assumes cs,Code
page

;--------------------------Exported-Routine-----------------------------;
; Output
;
;   Output is the entry point for output functions such as lines,
;   scanlines, arcs, etc.  Those functions which are supported
;   will be dispatched to.  If the function is not supported, an
;   error code will be returned.
;
; Entry:
;	None
; Return:
;	Per sub-function
; Error Returns:
;	Per sub-function
;	AX = 0 if sub-function not supported
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;	AX,BX,CX,DX,ES,FLAGS
; Calls:
;	scanlines
;	lines
; History:
;	Wed 04-Mar-1987 12:25:32 -by-  Walt Moore [waltm]
;	Created.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


	assumes ds,Data
	assumes es,nothing


cProc	Output,<FAR,PUBLIC,WIN,PASCAL>,<si,di>

	parmD	lp_dst_dev		;--> to the destination
	parmW	style			;Output operation
	parmW	count			;# of points
	parmD	lp_points		;--> to a set of points
	parmD	lp_phys_pen		;--> to physical pen
	parmD	lp_phys_brush		;--> to physical brush
	parmD	lp_draw_mode		;--> to a Drawing mode
	parmD	lp_clip_rect		;--> to a clipping rectange if <> 0

cBegin	<nogen>

	mov	bx,sp
	mov	ax,wptr ss:[bx][26]	;Get the style parameter
	cmp	ax,OS_POLYLINE		;Is this a polyline
	je	dispatch_lines		;  Yes
	cmp	ax,OS_SCANLINES 	;Is this a scanline ?
	jne	output_return_error	;  No, return an error

dispatch_scanlines:
	jmp	do_scanlines

dispatch_lines:
	jmp	do_polylines

output_return_error:
	xor	ax,ax			;Show error
	ret	28

cEnd	<nogen>

sEnd	Code

ifdef	PUBDEFS
	include output.pub
endif

end

