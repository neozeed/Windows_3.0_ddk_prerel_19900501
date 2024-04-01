;----------------------------------------------------------------------------;
; Copyright (C) Microsoft Corporation 1985-1990. All Rights Reserved.        ;
;----------------------------------------------------------------------------;

	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	SMARTPRO.ASM
;
;   This module contains the routines for outputting proportion
;   and fixed pitch characters.
;
; Created: 24-Jul-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	None
;
; Public Functions:
;	gen_nc_one_char
;	gen_nc_two_char
;	gen_nc_three_char
;	gen_nc_four_char
;	gen_nc_n_char
;	gen_nc_n_char
;	gen_nc_n_char
;	gen_nc_n_char
;	gen_cl_one_char
;	gen_cl_two_char
;	gen_cl_three_char
;	gen_cl_four_char
;	gen_cl_n_char
;	gen_cl_n_char
;	gen_cl_n_char
;	gen_cl_n_char
;
; if SPECIAL_CASE_BM_OBWNC
;	bm_obwnc_one_char
;	bm_obwnc_two_char
;	bm_obwnc_three_char
;	bm_obwnc_four_char
; endif
;
; if SPECIAL_CASE_BM_OWBNC
;	bm_owbnc_one_char
;	bm_owbnc_two_char
;	bm_owbnc_three_char
;	bm_owbnc_four_char
; endif
;
; if SPECIAL_CASE_BM_TBNC
;	bm_tbnc_one_char
;	bm_tbnc_two_char
;	bm_tbnc_three_char
;	bm_tbnc_four_char
; endif
;
; if SPECIAL_CASE_BM_TWNC
;	bm_twnc_one_char
;	bm_twnc_two_char
;	bm_twnc_three_char
;	bm_twnc_four_char
; endif
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;

	.xlist
	include cmacros.inc
	include gdidefs.inc
	include rlfntseg.inc
	include rlstrblt.inc
	include macros.mac
	.list



;	Special cases

SPECIAL_CASE_BM_OBBNC = 0	;Bitmap, opaque, black on black, non-clipped
SPECIAL_CASE_BM_OBWNC = 1	;Bitmap, opaque, black on white, non-clipped
SPECIAL_CASE_BM_OWBNC = 0	;Bitmap, opaque, white on black, non_clipped
SPECIAL_CASE_BM_OWWNC = 0	;Bitmap, opaque, white on white, non-clipped
SPECIAL_CASE_BM_TBNC  = 0	;Bitmap, transparent black, non-clipped
SPECIAL_CASE_BM_TWNC  = 0	;Bitmap, transparent white, non-clipped

;-----------------------------------------------------------------------;
;	The following equates are used to index into the buffer
;	of character widths, phases (x location), and offsets
;	of the bits within the font.
;-----------------------------------------------------------------------;

base	equ	0

wwidth	equ	byte ptr 0
pphase	equ	byte ptr 1
cchar	equ	word ptr -2

wwidth1 equ	byte ptr base - 0
pphase1 equ	byte ptr wwidth1 + 1
cchar1	equ	word ptr wwidth1 - 2

wwidth2 equ	byte ptr base - 4
pphase2 equ	byte ptr wwidth2 + 1
cchar2	equ	word ptr wwidth2 - 2

wwidth3 equ	byte ptr base - 8
pphase3 equ	byte ptr wwidth3 + 1
cchar3	equ	word ptr wwidth3 - 2

wwidth4 equ	byte ptr base - 12
pphase4 equ	byte ptr wwidth4 + 1
cchar4	equ	word ptr wwidth4 - 2
page

;--------------------------------Macro----------------------------------;
; upd_bm
;
;	upd_bm is the macro used for generating the destination update
;	code for a bitmap.
;
;	Usage:
;		upd_bm	ll
;	Where
;		ll	is the macro to invoke for the looping logic
;-----------------------------------------------------------------------;

upd_bm	macro	ll
if DRAW_ADJUST				;;If last logic operation defined
	dec	di			;;  did a stosb, adjust for it
endif
	add	di,ss_next_scan
	ll
	endm


;--------------------------------Macro----------------------------------;
; n_char
;
;	n_char is a macro for generating the code required to process
;	a destination character consisting of 5,6,7 or 8 source bytes
;
;	Usage:
;		n_char	name,output,update,setup
;	Where
;
;		name	the name of the procedure
;		output	the macro which will generate the required
;			output logic
;		update	the macro which will generate the required
;			destination update logic
;		setup	if this parameter is given as "inline", then
;			setup code will be generated inline, else a
;			subroutine will be called to perform the
;			initialization.  This subroutine will be
;			created if not defined.
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

n_char	macro	name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; n character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	ss_height		;;Loop until all chars output
	jnz	name&_n_char_outer_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; n character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	ss_height		;;Loop until all chars output
	jz	name&_n_char_exit
	jmp	name&_n_char_outer_loop
name&_n_char_exit:
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; n character setup logic
;-----------------------------------------------------------------------;
public	name&_n_char			;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_n_char proc near			;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef n_char_setup			;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-n_char_setup			;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif 				;;
endif					;;
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	n_char_setup			;;    make public for debugging
n_char_setup proc near			;;    define start of setup proc
  endif 				;;  endif
	mov	ss_height,ax		;;  save # scans in character
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	mov	ss_num_chars,cl 	;;  save # of characters - 1
	push	bp			;;  save frame pointer
	mov	bp,dx			;;  set buffer pointer
	xor	dx,dx			;;  index into font scan
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
n_char_setup endp			;;    terminate the procedure
name&_n_char proc near			;;    define actual procedure
	call	n_char_setup		;;    call setup code
  endif 				;;  endif
else					;;else
name&_n_char proc near			;;    define actual procedure
	call	n_char_setup		;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; n character compilation logic
;-----------------------------------------------------------------------;

name&_n_char_outer_loop:
	xor	si,si			;;SI = char index
	mov	ch,ss_num_chars 	;;Get # of characters - 1
	xchg	dx,di			;;Index to next font scan in DI

name&_n_char_inner_loop:
	mov	bx,[bp][si].cchar	;;BX = offset of bits
	mov	al,[bx][di]
	mov	cl,[bp][si].wwidth	;;CL = width
	shl	ax,cl
	sub	si,4			;;--> next char
	dec	ch
	jnz	name&_n_char_inner_loop
	mov	bx,[bp][si].cchar	;;BX = offset of bits
	mov	al,[bx][di]
	mov	cl,[bp][si].pphase	;;CL = phase
	shr	ax,cl
	xchg	dx,di			;;DI = dest ptr
	output	<update>		;;Macro to do whatever for outputting
name&_n_char  endp
	endm
page

;--------------------------------Macro----------------------------------;
; four_char
;
;	four_char is a macro for generating the code required to
;	process a destination character consisting of 4 source bytes
;
;	Usage:
;		four_char  name,output,update,setup
;	Where
;
;		name	the name of the procedure
;		output	the macro which will generate the required
;			output logic
;		update	the macro which will generate the required
;			destination update logic
;		setup	if this parameter is given as "inline", then
;			setup code will be generated inline, else a
;			subroutine will be called to perform the
;			initialization.  This subroutine will be
;			created if not defined.
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

four_char macro name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; four character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&Macro
	dec	ss_height
	jnz	name&_four_char_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; four character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&Macro
	dec	ss_height
	jz	name&_four_char_exit
	jmp	name&_four_char_loop
name&_four_char_exit:
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; four character setup logic
;-----------------------------------------------------------------------;

public	name&_four_char 		;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_four_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef four_char_setup		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-four_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	four_char_setup 		;;    make public for debugging
four_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
	mov	ss_height,ax		;;  save # scans in character
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	push	bp			;;  save frame pointer
	mov	bp,dx			;;  --> buffer
	mov	dl,[bp].wwidth3 	;;
	mov	dh,[bp].pphase4 	;;
	mov	ss_phases,dx		;;
	mov	cl,[bp].wwidth1 	;;
	mov	ch,[bp].wwidth2 	;;
	mov	si,[bp].cchar4		;;  4th character so we can lodsb
	mov	bx,[bp].cchar1		;;
	mov	dx,[bp].cchar3		;;
	mov	bp,[bp].cchar2		;;
	sub	bx,si			;;  compute deltas
	sub	bp,si			;;
	sub	dx,si			;;
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
four_char_setup endp			;;    terminate the procedure
name&_four_char proc near		;;    define actual procedure
	call	four_char_setup 	;;    call setup code
  endif 				;;  endif
else					;;else
name&_four_char proc near		;;    define actual procedure
	call	four_char_setup 	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; four character compilation logic
;-----------------------------------------------------------------------;

name&_four_char_loop:
	mov	al,[si][bx]
	shl	ax,cl
	mov	al,ds:[si][bp]
	xchg	cl,ch
	shl	ax,cl
	xchg	cl,ch
	xchg	cx,ss_phases
	xchg	bx,dx
	mov	al,[si][bx]
	xchg	bx,dx
	shl	ax,cl
	lodsb
	xchg	cl,ch
	shr	ax,cl
	xchg	cl,ch
	xchg	cx,ss_phases
	output	<update>		;;Macro to do whatever for outputting
name&_four_char endp
	endm
page

;--------------------------------Macro----------------------------------;
; three_char
;
;	three_char is a macro for generating the code required to
;	process a destination character consisting of 3 source bytes
;
;	Usage:
;		three_char  name,output,update,setup
;	Where
;
;		name	the name of the procedure
;		output	the macro which will generate the required
;			output logic
;		update	the macro which will generate the required
;			destination update logic
;		setup	if this parameter is given as "inline", then
;			setup code will be generated inline, else a
;			subroutine will be called to perform the
;			initialization.  This subroutine will be
;			created if not defined.
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

three_char macro name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; three character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	dec	ss_height
	jnz	name&_three_char_loop
	pop	bp
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; three character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	dec	ss_height
	jz	name&_three_char_exit
	jmp	name&_three_char_loop
name&_three_char_exit:
	pop	bp
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; three character setup logic
;-----------------------------------------------------------------------;

public	name&_three_char		;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_three_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef three_char_setup		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-three_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	three_char_setup		;;    make public for debugging
three_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
	mov	ss_height,ax		;;  save # scans in character
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	push	bp
	mov	bp,dx			;;  BP = buffer
	mov	dl,[bp].wwidth1 	;;
	mov	dh,[bp].wwidth2 	;;
	mov	ch,[bp].pphase3 	;;
	mov	si,[bp].cchar3		;;
	mov	bx,[bp].cchar2		;;
	mov	bp,[bp].cchar1		;;
	sub	bx,si			;;
	sub	bp,si			;;
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
three_char_setup endp			;;    terminate the procedure
name&_three_char proc near		;;    define actual procedure
	call	three_char_setup	;;    call setup code
  endif 				;;  endif
else					;;else
name&_three_char proc near		;;    define actual procedure
	call	three_char_setup	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; three character compilation logic
;-----------------------------------------------------------------------;

name&_three_char_loop:
	mov	al,ds:[si][bp]
	mov	cl,dl
	shl	ax,cl
	mov	al,[si][bx]
	mov	cl,dh
	shl	ax,cl
	lodsb
	mov	cl,ch
	shr	ax,cl
	output	<update>		;;Macro to do whatever for outputting
name&_three_char endp
	endm
page

;--------------------------------Macro----------------------------------;
; two_char
;
;	two_char is a macro for generating the code required to
;	process a destination character consisting of 2 source bytes
;
;	Usage:
;		two_char  name,output,update,setup
;	Where
;
;		name	the name of the procedure
;		output	the macro which will generate the required
;			output logic
;		update	the macro which will generate the required
;			destination update logic
;		setup	if this parameter is given as "inline", then
;			setup code will be generated inline, else a
;			subroutine will be called to perform the
;			initialization.  This subroutine will be
;			created if not defined.
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

two_char macro name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; two character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	dec	dx
	jnz	name&_two_char_loop
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; two character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	dec	dx
	jz	name&_two_char_exit
	jmp	name&_two_char_loop
name&_two_char_exit:
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; two character setup logic
;-----------------------------------------------------------------------;

public	name&_two_char			;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_two_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef two_char_setup 		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-two_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	two_char_setup			;;    make public for debugging
two_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
	xchg	bp,dx			;;  BP = buffer, DX = font height
	mov	cl,[bp].wwidth1 	;;
	mov	ch,[bp].pphase2 	;;
	mov	bx,[bp].cchar1		;;
	mov	si,[bp].cchar2		;;
	mov	bp,dx			;;  restore frame pointer
	xchg	ax,dx			;;  set DX = font height
	sub	bx,si			;;  delta between the characters
  if genflag and 10b			;;  if generating setup procedure
	ret				;;    dispatch to caller
two_char_setup endp			;;    terminate the procedure
name&_two_char proc near		;;    define actual procedure
	call	two_char_setup		;;    call setup code
  endif 				;;  endif
else					;;else
name&_two_char proc near		;;    define actual procedure
	call	two_char_setup		;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; two character compilation logic
;-----------------------------------------------------------------------;

name&_two_char_loop:
	mov	al,[bx][si]
	shl	ax,cl
	xchg	cl,ch
	lodsb
	shr	ax,cl
	xchg	cl,ch
	output	<update>		;;Macro to do whatever for outputting
name&_two_char endp
	endm
page

;--------------------------------Macro----------------------------------;
; one_char
;
;	one_char is a macro for generating the code required to
;	process a destination character consisting of 1 source byte
;
;	Usage:
;		one_char  name,output,update,setup
;	Where
;
;		name	the name of the procedure
;		output	the macro which will generate the required
;			output logic
;		update	the macro which will generate the required
;			destination update logic.
;		setup	if this parameter is given as "inline", then
;			setup code will be generated inline, else a
;			subroutine will be called to perform the
;			initialization.  This subroutine will be
;			created if not defined.
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

one_char macro name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; one character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	dec	dx
	jnz	name&_one_char_loop
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; one character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	dec	dx
	jz	name&_one_char_exit
	jmp	name&_one_char_loop
name&_one_char_exit:
	sub	di,cell_adjust
	ret
	&endm

;-----------------------------------------------------------------------;
; one character setup logic
;-----------------------------------------------------------------------;

public	name&_one_char			;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_one_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef one_char_setup 		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-one_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	one_char_setup			;;    make public for debugging
one_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
	xchg	dx,bp			;;  BP --> character buffer
	mov	si,[bp].cchar1		;;  DS:SI = char1
	mov	cl,[bp].pphase1 	;;
	xchg	dx,bp			;;  BP --> frame
	xchg	ax,dx			;;  DX = clipped_font_height
  if genflag and 10b			;;  if generating setup procedure
	ret				;;    dispatch to caller
one_char_setup endp			;;    terminate the procedure
name&_one_char proc near		;;    define actual procedure
	call	one_char_setup		;;    call setup code
  endif 				;;  endif
else					;;else
name&_one_char proc near		;;    define actual procedure
	call	one_char_setup		;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; one character compilation logic
;-----------------------------------------------------------------------;

name&_one_char_loop:
	lodsb				;;char1
	shr	al,cl
	output	<update>
name&_one_char endp
	endm
page

;--------------------------------Macro----------------------------------;
; clipped_output
;
;	clipped_output is the macro passed to the "x" character macros
;	when the default subroutines are to be called for outputing a
;	clipped character
;
;	Usage:
;		clipped_output ll
;	Where
;		ll	is the macro to be invoked for generating
;			the looping logic.
;-----------------------------------------------------------------------;

clipped_output macro x
	call	ss_draw_clipped
	x
	endm


;--------------------------------Macro----------------------------------;
; non_clipped_output
;
;	non_clipped_output is the macro passed to the "x" character
;	macros when the default subroutines are to be called for
;	outputing a non-clipped character
;
;	Usage:
;		non_clipped_output ll
;	Where
;		ll	is the macro to be invoked for generating
;			the looping logic.
;-----------------------------------------------------------------------;

non_clipped_output macro x
	call	ss_draw
	x
	endm

;--------------------------------Macro----------------------------------;
; owwc
;
;	owwc  is a macro for generating the character drawing
;	logic for opaque mode, white text, white background, clipped.
;
;	Usage:
;		owwc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

owwc	macro	update
	mov	al,ss_clip_mask
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; twc
;
;	twc is a macro for generating the character drawing
;	logic for transparent mode, white text, clipped.
;
;	Usage:
;		twc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

twc	macro	update
	and	al,ss_clip_mask
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; twnc
;
;	twnc is a macro for generating the character drawing
;	logic for transparent mode, white text, non-clipped.
;
;	Usage:
;		twnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

twnc	macro	update
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; obbc
;
;	obbc is a macro for generating the character drawing
;	logic for opaque mode, black text, black background, clipped.
;
;	Usage:
;		obbc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

obbc	macro	update
	mov	al,ss_clip_mask
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; tbc
;
;	tbc is a macro for generating the character drawing
;	logic for transparent mode, black text, clipped.
;
;	Usage:
;		tbc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

tbc	macro	update
	and	al,ss_clip_mask
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; tbnc
;
;	tbnc is a macro for generating the character drawing
;	logic for transparent mode, black text, non-clipped.
;
;	Usage:
;		tbnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

tbnc	macro	update
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; obwc
;
;	obwc is a macro for generating the character drawing
;	logic for opaque mode, black text, white background, clipped.
;
;	Usage:
;		obwc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

obwc	macro	update
	not	al
	mov	ah,ss_clip_mask
	and	al,ah
	not	ah
	and	ah,es:[di]
	or	al,ah
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm


;--------------------------------Macro----------------------------------;
; owbc
;
;	owbc is a macro for generating the character drawing
;	logic for opaque mode, white text, black background, clipped.
;
;	Usage:
;		owbc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

owbc	macro	update
	mov	ah,ss_clip_mask
	and	al,ah
	not	ah
	and	ah,es:[di]
	or	al,ah
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm


;--------------------------------Macro----------------------------------;
; obwnc
;
;	obwnc is a macro for generating the character drawing
;	logic for opaque mode, black text, white background, non-clipped.
;
;	Usage:
;		obwnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

obwnc	macro	update
	not	al
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm


;--------------------------------Macro----------------------------------;
; owbnc
;
;	owbnc is a macro for generating the character drawing
;	logic for opaque mode, white text, black background, non-clipped.
;
;	Usage:
;		owbnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

owbnc	macro	update
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm


;--------------------------------Macro----------------------------------;
; obbnc
;
;	obbnc is a macro for generating the character drawing
;	logic for opaque mode, black text, black background, non-clipped.
;
;	Usage:
;		obbnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

obbnc	macro	update
	mov	byte ptr es:[di],0
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm


;--------------------------------Macro----------------------------------;
; owwnc
;
;	owwnc is a macro for generating the character drawing
;	logic for opaque mode, white text, white background, non-clipped.
;
;	Usage:
;		owwnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

owwnc	macro	update
	mov	byte ptr es:[di],0FFh
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
page

sBegin	Code
assumes cs,Code

;-----------------------------------------------------------------------;
;
;	The following tables are used to dispatch the various
;	combinations of drawing required for foreground/background,
;	opaque/transparent, device/bitmap, clipped/non-clipped text
;
;-----------------------------------------------------------------------;


special_case_clip_tables	label	word
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl

special_case_non_clip_tables	label	word
if SPECIAL_CASE_BM_TBNC eq 1
	dw	CodeOFFSET bm_tbnc
	dw	CodeOFFSET bm_tbnc
else
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
endif
if SPECIAL_CASE_BM_TWNC eq 1
	dw	CodeOFFSET bm_twnc
	dw	CodeOFFSET bm_twnc
else
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
endif
if SPECIAL_CASE_BM_OBBNC eq 1
	dw	CodeOFFSET bm_obbnc
else
	dw	CodeOFFSET gen_nc
endif
if SPECIAL_CASE_BM_OBWNC eq 1
	dw	CodeOFFSET bm_obwnc
else
	dw	CodeOFFSET gen_nc
endif
if SPECIAL_CASE_BM_OWBNC eq 1
	dw	CodeOFFSET bm_owbnc
else
	dw	CodeOFFSET gen_nc
endif
if SPECIAL_CASE_BM_OWWNC eq 1
	dw	CodeOFFSET bm_owwnc
else
	dw	CodeOFFSET gen_nc
endif

gen_nc	label	word
	dw	CodeOFFSET gen_nc_one_char
	dw	CodeOFFSET gen_nc_two_char
	dw	CodeOFFSET gen_nc_three_char
	dw	CodeOFFSET gen_nc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char

gen_cl	label	word
	dw	CodeOFFSET gen_cl_one_char
	dw	CodeOFFSET gen_cl_two_char
	dw	CodeOFFSET gen_cl_three_char
	dw	CodeOFFSET gen_cl_four_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char

if SPECIAL_CASE_BM_OBWNC eq 1
bm_obwnc label word
	dw	CodeOFFSET bm_obwnc_one_char
	dw	CodeOFFSET bm_obwnc_two_char
	dw	CodeOFFSET bm_obwnc_three_char
	dw	CodeOFFSET bm_obwnc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
endif

if SPECIAL_CASE_BM_OWBNC eq 1
bm_owbnc label word
	dw	CodeOFFSET bm_owbnc_one_char
	dw	CodeOFFSET bm_owbnc_two_char
	dw	CodeOFFSET bm_owbnc_three_char
	dw	CodeOFFSET bm_owbnc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
endif

if SPECIAL_CASE_BM_TBNC eq 1
bm_tbnc label word
	dw	CodeOFFSET bm_tbnc_one_char
	dw	CodeOFFSET bm_tbnc_two_char
	dw	CodeOFFSET bm_tbnc_three_char
	dw	CodeOFFSET bm_tbnc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
endif

if SPECIAL_CASE_BM_TWNC eq 1
bm_twnc label word
	dw	CodeOFFSET bm_twnc_one_char
	dw	CodeOFFSET bm_twnc_two_char
	dw	CodeOFFSET bm_twnc_three_char
	dw	CodeOFFSET bm_twnc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
endif

;-----------------------------------------------------------------------;
;
;	Define the drawing logic tables.  These tables are used to
;	fetch the address of the output function for the combination
;	of foreground/background colors, opaque/transparent mode,
;	and device/bitmap.
;
;	The tables are indexed as follows:
;
;	  DEV/BM O/T FG BK  0;
;	    |	  |   |  |
;	    |	  |   |   ----------  background color (0/1)
;	    |	  |   |
;	    |	  |    -------------  foreground color (0/1)
;	    |	  |
;	    |	   -----------------  0 if transparent, 1 if opaque
;	    |
;	     ------------------------ 0 if a bitmap, 1 if the device
;-----------------------------------------------------------------------;

MASK_DEVBM	equ	00010000b
MASK_OT		equ	00001000b
MASK_FG		equ	00000100b
MASK_BK		equ	00000010b

MASK_DEV_ONC	equ	MASK_DEVBM or MASK_OT	;MASK_BK, MASK_FG wildcards
MASK_DEV_TNC	equ	MASK_DEVBM		;MASK_BK, MASK_FG wildcards
MASK_DEV_OC	equ	MASK_DEVBM or MASK_OT	;MASK_BK, MASK_FG wildcards
MASK_DEV_TC	equ	MASK_DEVBM		;MASK_BK, MASK_FG wildcards

MASK_BM_OBWNC	equ	MASK_OT or MASK_BK
MASK_BM_OWBNC	equ	MASK_OT or MASK_FG
MASK_BM_TBNC	equ	0			;MASK_BK is a wildcard
MASK_BM_TWNC	equ	MASK_FG			;MASK_BK is a wildcard

clipped_drawing_functions label word

	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color


non_clipped_drawing_functions label word

	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color


mono_non_clipped_drawing_functions	label word

	dw	CodeOFFSET  bm_trans_black
	dw	CodeOFFSET  bm_trans_black
	dw	CodeOFFSET  bm_trans_white
	dw	CodeOFFSET  bm_trans_white
	dw	CodeOFFSET  bm_opaque_black_on_black
	dw	CodeOFFSET  bm_opaque_black_on_white
	dw	CodeOFFSET  bm_opaque_white_on_black
	dw	CodeOFFSET  bm_opaque_white_on_white
page

define_frame	SmartPro
cBegin	nogen
cEnd	nogen

;--------------------------Private-Routine------------------------------;
; bm_opaque_white_on_white_clip
; bm_trans_white_clip
; bm_trans_white
;
;   Standard bitmap drawing functions for:
;
;	opaque, white on white, clipped
;	transparent, white, clipped
;	transparent, white, non-clipped
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;
;       Mon 04-Dec-1989 17-32-00 -by-  Amit Chatterjee [amitc]
;       Ported this routine to work with IBMCOLOR printers. To do this a few
;       changes had to be done:
;	     (a) Adjusted to work with 3 color planes rather than 4
;            (b) Deals only with bitmaps, so all the device specific code
;	         has been yanked out.
;	     (c) The format for small color bitmaps was changed back to be in
;         	 the old style, that is, a red plane followed by a green plane 
;		 and followed by a blue plane.
;            (d) The was the physical color structure is interpreted was  
;		 changed to be appropriate for the printer driver.
;	Note:
;	     Support for huge bitmaps and ExtTextOut is still in although they
;	     will not currently be utilized.
;	*********************************************************************
;       AS THIS FILE IS BASED ON REALPRO.ASM IN THE DISP30 TREE OF 4 PLANE 
;	EGA/VGA DRIVERS, ANY BUG FIX THAT YOU DO IN THIS FILE WOULD PROBABLY
;       BE HAVE TO DONE FOR THE DISP30 TREE EGA/VGA DISPLAY DRIVERS TOO.
;       ********************************************************************
;
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

bm_opaque_white_on_white_clip:
	mov	al,0FFh

bm_trans_white_clip:
	and	al,ss_clip_mask

bm_trans_white:
	or	es:[di],al

DRAW_ADJUST	= 0			;STOSB isn't used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_opaque_black_on_black_clip
; bm_trans_black_clip
; bm_trans_black
;
;   Standard bitmap drawing functions for:
;
;	opaque, black on black, clipped
;	transparent, black, clipped
;	transparent, black, non-clipped
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

bm_opaque_black_on_black_clip:
	mov	al,0FFh

bm_trans_black_clip:
	and	al,ss_clip_mask

bm_trans_black:
	not	al
	and	es:[di],al

DRAW_ADJUST	= 0			;STOSB isn't used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_opaque_black_on_white
; bm_opaque_white_on_black
;
;   Standard bitmap drawing functions for:
;
;	opaque, black on white, non-clipped
;	opaque, white on black, non-clipped
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

bm_opaque_black_on_white:
	not	al

bm_opaque_white_on_black:
	stosb

DRAW_ADJUST	= 1			;STOSB is used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_opaque_black_on_white_clip
; bm_opaque_white_on_black_clip
;
;   Standard bitmap drawing functions for:
;
;	opaque, black on white, clipped
;	opaque, white on black, clipped
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

bm_opaque_black_on_white_clip:
	not	al

bm_opaque_white_on_black_clip:
	mov	ah,ss_clip_mask
	and	al,ah
	not	ah
	and	ah,es:[di]
	or	al,ah
	stosb

DRAW_ADJUST	= 1			;STOSB is used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_opaque_white_on_white
; bm_opaque_black_on_black
;
;   Standard bitmap drawing functions for:
;
;	opaque, white on white, non-clipped
;	opaque, black on black, non-clipped
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

bm_opaque_white_on_white:
	mov	al,0FFh
	jmpnext

bm_opaque_black_on_black:
	xor	al,al
	jmpnext stop
	stosb

DRAW_ADJUST	= 1			;STOSB is used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_trans_color_fix
; bm_trans_color
;
;   Standard bitmap drawing function for:
;
;	transparent, color
;
;  Entry points are for the fixed width code and proportional width code,
;  respectively.
;
; Entry (fixed):
;	AL = character to output
;	DX = return address
;	ES:DI --> destination byte
; Entry (prop):
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Tue 25-Aug-1987 17:00:00 -by-  Bob Grudem [bobgru]
;	Added fixed pitch entry point to consolidate it with the
;	proportional width case.
;	Late spring, 1987        -by-  Tony Pisculli [tonyp]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

	public	bm_trans_color_fix
	public	bm_trans_color

bm_trans_color_fix:
	push	dx			;put return address on stack

bm_trans_color:
	push	ax
	push	cx
	push	di

	xchg	bp,ss_p_frame		;get frame pointer

	mov	ch,ss_clip_mask
	or	ch,ch
	mov	ah,al
	jz	bm_trans_color_no_clip

	mov	ah,ch
	and	ah,al

bm_trans_color_no_clip:
	not	ah
	mov	cl,num_planes
	mov	ch,byte ptr ss_colors[FOREGROUND]

bm_trans_color_loop:
	and	es:[di],ah
	ror	ch,1			;see if this color plane is used
	jnc	bm_trans_color_set_byte
	or	es:[di],al

bm_trans_color_set_byte:
	add	di,next_plane
	dec	cl
	jnz	bm_trans_color_loop

	xchg	bp,ss_p_frame		;restore bp and ss_p_frame

	pop	di
	pop	cx
	pop	ax

DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; bm_opaque_color_fix
; bm_opaque_color
;
;   Standard bitmap drawing function for:
;
;	opaque, color
;
; Entry:
;	AL = character to output
;	ES:DI --> destination byte
; Returns:
;	ES:DI --> same byte, next scan
; Error Returns:
;	None
; Registers Preserved:
;	BX,CX,DX,SI,DI,DS,ES,BP
; Registers Destroyed:
;	AX
; Calls:
;	None
; History:
;	Tue 25-Aug-1987 17:00:00 -by-  Bob Grudem [bobgru]
;	Added fixed pitch entry point to consolidate it with the
;	proportional width case.
;	Late spring, 1987        -by-  Tony Pisculli [tonyp]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

	public	bm_opaque_color_fix
	public	bm_opaque_color
bm_opaque_color_fix:
	push	dx			;put return address on stack

bm_opaque_color:

	push	ax
	push	cx
	push	dx
	push	di

	xchg	bp,ss_p_frame		;get frame pointer

	mov	ch,ss_clip_mask
	mov	dh,num_planes
	mov	dl,special_bm_opaque_color

bm_opaque_color_partial:
	shr	dl,1			;set C to inversion mask
	sbb	ah,ah
	and	ah,al			;AH = 1 where we want NOT background
	shr	dl,1			;set C to background color
	sbb	cl,cl			;AH = background color (00 or FF)
	xor	cl,ah			;AH = destination byte
	mov	ah,es:[di]
	xor	ah,cl
	and	ah,ch
	xor	es:[di],ah		;output byte to color plane
	add	di,next_plane		;point to next color plane
	dec	dh
	jnz	bm_opaque_color_partial	;handle next color plane

	xchg	bp,ss_p_frame		;restore bp and ss_p_frame

	pop	di
	pop	dx
	pop	cx
	pop	ax

DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; preset_pro_text
;
;  Set any frame variables and stack locations (in the StrStuff segment)
;  required to output text with the current attributes.
;
; Entry:
;	BL = accel
;	AH = excel
;	SS = StrStuff
; Returns:
;	None
; Error Returns:
;	None
; Registers Preserved:
;	DX,BP,DS,ES
; Registers Destroyed:
;	AX,BX,CX,SI,DI,FLAGS
; Calls:
;	None
; History:
;	Tue 25-Aug-1987 17:00:00 -by-  Bob Grudem [bobgru]
;	Rewrote assignment of bit-gathering and drawing functions.
;	Fri 24-Jul-1987 13:21:18 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;


;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff

	public	preset_pro_text
preset_pro_text proc	near

	mov	ss_p_frame,bp
	mov	cx,next_scan
	mov	ss_next_scan,cx
	mov	cx,colors
	mov	ss_colors,cx

;	Accumulate foreground/background colors, opaque/transparent mode,
;	and device/bitmap to determine which drawing functions will be
;	used.

	and	bx,IS_OPAQUE		;BL = 0000000 O/T
	errnz	IS_OPAQUE-00000001
;	mov	cx,colors
	shr	cl,1
	rcl	bl,1			;BL = 000000 O/T FG
	errnz	FOREGROUND
	shr	ch,1			;BL = 00000 O/T FG BK
	rcl	bl,1
	errnz	BACKGROUND-FOREGROUND-1
	and	ah,IS_DEVICE
	or	bl,ah			;BL = 0000 DEV/BM O/T FG BK
	errnz	IS_DEVICE-00001000b
	shl	bx,1			;BX = 00000000 000 DEV/BM O/T FG BK 0


;	Now that we have an index into our tables of drawing functions,
;	find out which table to use.  We use special tables for the
;	nonclipped cases of device and monochrome bitmaps.  For color
;	bitmaps, and all clipped cases, we'll get the drawing function
;	from the general case table.

	mov	ax,non_clipped_drawing_functions[bx]

	test	bl,MASK_DEVBM
	errnz	<MASK_DEVBM and 0FF00h>
	jnz	preset_pro_text_lookup_func
	cmp	num_planes,1
	jne	preset_pro_text_standard_func

	mov	ax,mono_non_clipped_drawing_functions[bx]

preset_pro_text_lookup_func:
	mov	si,special_case_non_clip_tables[bx]
	mov	di,special_case_clip_tables[bx]

preset_pro_text_have_func:
	mov	non_clipped_table,si
	mov	clipped_table,di
	mov	ss_draw,ax

	mov	ax,clipped_drawing_functions[bx]
	mov	ss_draw_clipped,ax

	ret


preset_pro_text_standard_func:
	mov	si,CodeOFFSET gen_nc
	mov	di,CodeOFFSET gen_cl
	jmp	short preset_pro_text_have_func

preset_pro_text endp
	page

;-----------------------------------------------------------------------;
;
;	The following cases are the general purpose routines for
;	outputting the text.  They will call through the stack
;	locations ss_draw and ss_draw_clipped to perform the
;	actual output operations.
;
;	Following the general purpose routines are any special
;	case routines.
;
;-----------------------------------------------------------------------;

	one_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	two_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	three_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	four_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	n_char		<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>

	one_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	two_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	three_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	four_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	n_char		<gen_cl>,<clipped_output>,<loop_logic>,<sub>

if SPECIAL_CASE_BM_OBWNC eq 1
	one_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
;	n_char		<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
endif

if SPECIAL_CASE_BM_OWBNC eq 1
	one_char	<bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>
;	n_char		<bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>
endif

if SPECIAL_CASE_BM_TBNC eq 1
	one_char	<bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>
;	n_char		<bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>
endif

if SPECIAL_CASE_BM_TWNC eq 1
	one_char	<bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>
;	n_char		<bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>
endif
page
sEnd	Code
	end


