	page	,132
;
;-----------------------------Module-Header-----------------------------;
; Module Name:	SMARTPRO.ASM
;
;   This module contains the routines for outputting proportion
;   and fixed pitch characters.
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Created: 24-Jul-1987
; Author:  Walt Moore [waltm]
;
; Modified for 256 color and protected mode support: 20-April-89
; 	Doug Cody, Video 7, Inc
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
;
; PROLOGUE:
;   With the advent of BIG FONTS, it becomes self evident that all
; processors are not created equal... ...some are created with the
; inalienable limitations only the powerful New can overcome.
;   To this end, the 386 set of registers will be used to process 32
; bit font table offsets. BIG FONTS, named for their intra segment
; (64k segment that is...) size, used in protected mode only, require
; the use of a 32 bit pointer. This creates a dilema for the bi-modal
; nature of this code since the same code must be used in 16 bit and 
; and 32 bit mode. The macro-ized code generation which follows is going
; to be maintained, but modified (like, the same, but different). Due
; to speed requirements, the code will be generated twice, once for 16
; bit operation, then again for 32 bit operation. The routine, "PRESET_
; PRO_TEXT" will make a determination at runtime to which code to
; execute.
;
;-----------------------------------------------------------------------;
;
	.xlist
	include cmacros.inc
	include gdidefs.inc
	include fontseg.inc
	include	display.inc
	include macros.mac
	include strblt.inc
	include	rt.mac
;
	.list
;
	externA SCREEN_W_BYTES
	externA ScreenSelector
	externA __NEXTSEG
;	externNP set_ega_opaque_mode
;
;
; Special cases
;
SPECIAL_CASE_BM_OBBNC = 0	;Bitmap, opaque, black on black, non-clipped
SPECIAL_CASE_BM_OBWNC = 1	;Bitmap, opaque, black on white, non-clipped
SPECIAL_CASE_BM_OWBNC = 0	;Bitmap, opaque, white on black, non_clipped
SPECIAL_CASE_BM_OWWNC = 0	;Bitmap, opaque, white on white, non-clipped
SPECIAL_CASE_BM_TBNC  = 0	;Bitmap, transparent black, non-clipped
SPECIAL_CASE_BM_TWNC  = 0	;Bitmap, transparent white, non-clipped
;
SPECIAL_CASE_DEV_ONC  = 1	;Device, opaque, non-clipped
SPECIAL_CASE_DEV_TNC  = 1	;Device, transparent, non-clipped
;SPECIAL_CASE_DEV_OC  = 0	;Device, opaque, clipped (see notes below)
SPECIAL_CASE_DEV_TC   = 0	;Device, transparent, clipped
;
;
;-----------------------------------------------------------------------;
;	The following equates are used to index into the buffer
;	of character widths, phases (x location), and offsets
;	of the bits within the font.
;-----------------------------------------------------------------------;
;
base	equ	0

wwidth	equ	byte ptr 0
pphase	equ	byte ptr 1
cchar	equ	dword ptr -2

wwidth1 equ	byte ptr base - 0
pphase1 equ	byte ptr wwidth1 + 1
cchar1	equ	dword ptr wwidth1 - 2

wwidth2 equ	byte ptr base - 6
pphase2 equ	byte ptr wwidth2 + 1
cchar2	equ	dword ptr wwidth2 - 2

wwidth3 equ	byte ptr base - 12
pphase3 equ	byte ptr wwidth3 + 1
cchar3	equ	dword ptr wwidth3 - 2

wwidth4 equ	byte ptr base - 18
pphase4 equ	byte ptr wwidth4 + 1
cchar4	equ	dword ptr wwidth4 - 2
;
;
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
;
upd_bm	macro	ll
if DRAW_ADJUST				;;If last logic operation defined
	dec	di			;;  did a stosb, adjust for it
endif
	add	di,ss_next_scan
	ll
	endm
;
;
;--------------------------------Macro----------------------------------;
; upd_dev
;
;	upd_dev is the macro used for generating the destination update
;	code for the physical device
;
;	Usage:
;		upd_dev ll
;	Where
;		ll	is the macro to invoke for the looping logic
;-----------------------------------------------------------------------;
;
upd_dev	macro	ll
	add	di,SCREEN_W_BYTES-DRAW_ADJUST
	ll
	endm
;
;
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
;
n_char	macro	name,output,update,setup
	local	genflag
;
	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff
;
;-----------------------------------------------------------------------;
; n character looping logic macro
;-----------------------------------------------------------------------;
;
loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	ss_height		;;Loop until all chars output
	jnz	name&_n_char_outer_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; n character long looping logic macro
;-----------------------------------------------------------------------;
;
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
;
;-----------------------------------------------------------------------;
; n character setup logic
;-----------------------------------------------------------------------;
;
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
;
;-----------------------------------------------------------------------;
; n character compilation logic
;-----------------------------------------------------------------------;
;
name&_n_char_outer_loop:
	xor	si,si			;;SI = char index
	mov	ch,ss_num_chars 	;;Get # of characters - 1
if	a386
	.386
	xchg	edx,edi			;;Index to next font scan in DI
	.8086
else
	xchg	dx,di			;;Index to next font scan in DI
endif

name&_n_char_inner_loop:
if	a386
	.386
	mov	ebx,[bp][si].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	.8086
else
	mov	bx,wptr [bp][si].cchar	;;BX = offset of bits
	mov	al,[bx][di]
endif
	mov	cl,[bp][si].wwidth	;;CL = width
	shl	ax,cl
	sub	si,6			;;--> next char
	errnz	<(size frame_data) - 6>
	dec	ch
	jnz	name&_n_char_inner_loop
if	a386
	.386
	mov	ebx,[bp][si].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	.8086
else
	mov	bx,wptr [bp][si].cchar	;;BX = offset of bits
	mov	al,[bx][di]
endif
	mov	cl,[bp][si].pphase	;;CL = phase
	shr	ax,cl
if	a386
	.386
	xchg	edx,edi			;;DI = dest ptr
	.8086
else
	xchg	dx,di			;;DI = dest ptr
endif

	output	<update>		;;Macro to do whatever for outputting
name&_n_char  endp
	endm
;
;
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
;
four_char macro name,output,update,setup
	local	genflag
;
	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff
;
;-----------------------------------------------------------------------;
; four character looping logic macro
;-----------------------------------------------------------------------;
;
loop_logic	&Macro
	dec	ss_height
	jnz	name&_four_char_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; four character long looping logic macro
;-----------------------------------------------------------------------;
;
long_loop_logic	&Macro
	dec	ss_height
	jz	name&_four_char_exit
	jmp	name&_four_char_loop
name&_four_char_exit:
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; four character setup logic
;-----------------------------------------------------------------------;
;
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
if	a386
	.386
	mov	esi,[bp].cchar4		;;
	mov	ebx,[bp].cchar1		;;
	mov	edx,[bp].cchar3		;;
	mov	ebp,[bp].cchar2		;;
	sub	ebx,esi			;;  compute deltas
	sub	ebp,esi			;;
	sub	edx,esi			;;
	.8086
else
	mov	si,[bp].cchar4		;;
	mov	bx,[bp].cchar1		;;
	mov	dx,[bp].cchar3		;;
	mov	bp,[bp].cchar2		;;
	sub	bx,si			;;  compute deltas
	sub	bp,si			;;
	sub	dx,si			;;
endif
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
;
;-----------------------------------------------------------------------;
; four character compilation logic
;-----------------------------------------------------------------------;
;
name&_four_char_loop:
if	a386
	.386
	mov	al,[esi][ebx]
	.8086
else
	mov	al,[si][bx]
endif
	shl	ax,cl
if	a386
	.386
	mov	al,ds:[esi][ebp]
	.8086
else
	mov	al,ds:[si][bp]
endif
	xchg	cl,ch
	shl	ax,cl
	xchg	cl,ch
	xchg	cx,ss_phases
if	a386
	.386
	xchg	ebx,edx
	mov	al,[esi][ebx]
	xchg	ebx,edx
	.8086
else
	xchg	bx,dx
	mov	al,[si][bx]
	xchg	bx,dx
endif
	shl	ax,cl
	lodsb				; 8086/386 safe
	xchg	cl,ch
	shr	ax,cl
	xchg	cl,ch
	xchg	cx,ss_phases
	output	<update>		;;Macro to do whatever for outputting
name&_four_char endp
	endm
;
;
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
;
three_char macro name,output,update,setup
	local	genflag
;
	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff
;
;-----------------------------------------------------------------------;
; three character looping logic macro
;-----------------------------------------------------------------------;
;
loop_logic	&macro
	dec	ss_height
	jnz	name&_three_char_loop
	pop	bp
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; three character long looping logic macro
;-----------------------------------------------------------------------;
;
long_loop_logic	&macro
	dec	ss_height
	jz	name&_three_char_exit
	jmp	name&_three_char_loop
name&_three_char_exit:
	pop	bp
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; three character setup logic
;-----------------------------------------------------------------------;
;
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
if	a386
	.386
	mov	esi,[bp].cchar3		;;
	mov	ebx,[bp].cchar2		;;
	mov	ebp,[bp].cchar1		;;
	sub	ebx,esi			;;
	sub	ebp,esi			;;
	.8086
else
	mov	si,[bp].cchar3		;;
	mov	bx,[bp].cchar2		;;
	mov	bp,[bp].cchar1		;;
	sub	bx,si			;;
	sub	bp,si			;;
endif
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
;
;-----------------------------------------------------------------------;
; three character compilation logic
;-----------------------------------------------------------------------;
;
name&_three_char_loop:
if	a386
	.386
	mov	al,ds:[esi][ebp]
	.8086
else
	mov	al,ds:[si][bp]
endif
	mov	cl,dl
	shl	ax,cl
if	a386
	.386
	mov	al,[esi][ebx]
	.8086
else
	mov	al,[si][bx]
endif
	mov	cl,dh
	shl	ax,cl
	lodsb			; 8086/386 safe
	mov	cl,ch
	shr	ax,cl
	output	<update>		;;Macro to do whatever for outputting
name&_three_char endp
	endm
;
;
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
;
two_char macro name,output,update,setup
	local	genflag
;
	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff
;
;-----------------------------------------------------------------------;
; two character looping logic macro
;-----------------------------------------------------------------------;
;
loop_logic	&macro
	dec	dx
	jnz	name&_two_char_loop
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; two character long looping logic macro
;-----------------------------------------------------------------------;
;
long_loop_logic	&macro
	dec	dx
	jz	name&_two_char_exit
	jmp	name&_two_char_loop
name&_two_char_exit:
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; two character setup logic
;-----------------------------------------------------------------------;
;
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
if	a386
	.386
	mov	ebx,[bp].cchar1		;;
	mov	esi,[bp].cchar2		;;
	sub	ebx,esi			;;  delta between the characters
	.8086
else
	mov	bx,[bp].cchar1		;;
	mov	si,[bp].cchar2		;;
	sub	bx,si			;;  delta between the characters
endif
	mov	bp,dx			;;  restore frame pointer
	xchg	ax,dx			;;  set DX = font height
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
;
;-----------------------------------------------------------------------;
; two character compilation logic
;-----------------------------------------------------------------------;
;
name&_two_char_loop:
if	a386
	.386
	mov	al,[ebx][esi]
	.8086
else
	mov	al,[bx][si]
endif
	shl	ax,cl
	xchg	cl,ch
	lodsb			; 8086/386 safe
	shr	ax,cl
	xchg	cl,ch
	output	<update>		;;Macro to do whatever for outputting
name&_two_char endp
	endm
;
;
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
;
one_char macro name,output,update,setup
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff
;
;-----------------------------------------------------------------------;
; one character looping logic macro
;-----------------------------------------------------------------------;
;
loop_logic	&macro
	dec	dx
	jnz	name&_one_char_loop
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; one character long looping logic macro
;-----------------------------------------------------------------------;
;
long_loop_logic	&macro
	dec	dx
	jz	name&_one_char_exit
	jmp	name&_one_char_loop
name&_one_char_exit:
	sub	di,cell_adjust
	ret
	&endm
;
;-----------------------------------------------------------------------;
; one character setup logic
;-----------------------------------------------------------------------;
;
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
if	a386
	.386
	mov	esi,[bp].cchar1		;;  DS:SI = char1
	.8086
else
	mov	si,[bp].cchar1		;;  DS:SI = char1
endif
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
;
;-----------------------------------------------------------------------;
; one character compilation logic
;-----------------------------------------------------------------------;
;
name&_one_char_loop:
	lodsb		; 8086/386 safe	;;char1
	shr	al,cl
	output	<update>
name&_one_char endp
	endm
;
;
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
;
clipped_output macro x
	call	ss_draw_clipped
	x
	endm
;
;
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
;
non_clipped_output macro x
	call	ss_draw
	x
	endm
;
;
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
;
owwc	macro	update
	mov	al,ss_clip_mask
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
twc	macro	update
	and	al,ss_clip_mask
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
twnc	macro	update
	or	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
obbc	macro	update
	mov	al,ss_clip_mask
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
tbc	macro	update
	and	al,ss_clip_mask
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
tbnc	macro	update
	not	al
	and	es:[di],al
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
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
;
;
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
;
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
;
;
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
;
obwnc	macro	update
	not	al
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm
;
;
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
;
owbnc	macro	update
	stosb
DRAW_ADJUST	= 1			;;STOSB is used
	update
	endm
;
;
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
;
obbnc	macro	update
	mov	byte ptr es:[di],0
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
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
;
owwnc	macro	update
	mov	byte ptr es:[di],0FFh
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
;
;
;--------------------------------Macro----------------------------------;
; GetColor
;
;	Loads the proper color into AL.
;
;	Assumes:
;		BL holds bit mask. 1=use forground, 0=background
;		DX holds colors. DL=Bg, DH=Fg
;
;-----------------------------------------------------------------------;
;
GetColor	macro
	rol	bl,1			;; send next Fg/Bg bit into carry
	sbb	ax,ax			;; ffff or 0000
	not	ah
	and	ax,dx
	or	al,ah			;; either dl or dh
	endm
;
;
sBegin	Code
assumes cs,Code
;
;
;-----------------------------------------------------------------------;
;
;	The following tables are used to dispatch the various
;	combinations of drawing required for foreground/background,
;	opaque/transparent, device/bitmap, clipped/non-clipped text
;
;-----------------------------------------------------------------------;
;
;
special_case_clip_tables	label	word
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
	dw	CodeOFFSET gen_cl
;
special_case_non_clip_tables	label	word
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET bm_obwnc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
	dw	CodeOFFSET gen_nc
;
gen_nc	label	word
	dw	CodeOFFSET gen_nc_one_char
	dw	CodeOFFSET gen_nc_two_char
	dw	CodeOFFSET gen_nc_three_char
	dw	CodeOFFSET gen_nc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
;
	public	ega_oc
ega_oc label word
gen_cl	label	word
	dw	CodeOFFSET gen_cl_one_char
	dw	CodeOFFSET gen_cl_two_char
	dw	CodeOFFSET gen_cl_three_char
	dw	CodeOFFSET gen_cl_four_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char
	dw	CodeOFFSET gen_cl_n_char
;
bm_obwnc label word
	dw	CodeOFFSET bm_obwnc_one_char
	dw	CodeOFFSET bm_obwnc_two_char
	dw	CodeOFFSET bm_obwnc_three_char
	dw	CodeOFFSET bm_obwnc_four_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
	dw	CodeOFFSET gen_nc_n_char
;
special386_case_clip_tables	label	word
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
	dw	CodeOFFSET gen_cl_386
;
special386_case_non_clip_tables	label	word
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET bm_obwnc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
	dw	CodeOFFSET gen_nc_386
;
gen_nc_386	label	word
	dw	CodeOFFSET gen386_nc_one_char
	dw	CodeOFFSET gen386_nc_two_char
	dw	CodeOFFSET gen386_nc_three_char
	dw	CodeOFFSET gen386_nc_four_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
;
gen_cl_386	label	word
	dw	CodeOFFSET gen386_cl_one_char
	dw	CodeOFFSET gen386_cl_two_char
	dw	CodeOFFSET gen386_cl_three_char
	dw	CodeOFFSET gen386_cl_four_char
	dw	CodeOFFSET gen386_cl_n_char
	dw	CodeOFFSET gen386_cl_n_char
	dw	CodeOFFSET gen386_cl_n_char
	dw	CodeOFFSET gen386_cl_n_char
;
bm_obwnc_386 label word
	dw	CodeOFFSET bm386_obwnc_one_char
	dw	CodeOFFSET bm386_obwnc_two_char
	dw	CodeOFFSET bm386_obwnc_three_char
	dw	CodeOFFSET bm386_obwnc_four_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
	dw	CodeOFFSET gen386_nc_n_char
;
;
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
;
;-----------------------------------------------------------------------;
;
MASK_DEVBM	equ	00010000b
MASK_OT		equ	00001000b
MASK_FG		equ	00000100b
MASK_BK		equ	00000010b
;
MASK_DEV_ONC	equ	MASK_DEVBM or MASK_OT	; MASK_BK, MASK_FG wildcards
MASK_DEV_TNC	equ	MASK_DEVBM		; MASK_BK, MASK_FG wildcards
MASK_DEV_OC	equ	MASK_DEVBM or MASK_OT	; MASK_BK, MASK_FG wildcards
MASK_DEV_TC	equ	MASK_DEVBM		; MASK_BK, MASK_FG wildcards
;
MASK_BM_OBWNC	equ	MASK_OT or MASK_BK
MASK_BM_OWBNC	equ	MASK_OT or MASK_FG
MASK_BM_TBNC	equ	0			; MASK_BK is a wildcard
MASK_BM_TWNC	equ	MASK_FG			; MASK_BK is a wildcard
;
;
clipped_drawing_functions label word
non_clipped_drawing_functions label word
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_trans_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
	dw	CodeOFFSET  bm_opaque_color
;
;
mono_non_clipped_drawing_functions	label word
	dw	CodeOFFSET  bm_trans_black
	dw	CodeOFFSET  bm_trans_black
	dw	CodeOFFSET  bm_trans_white
	dw	CodeOFFSET  bm_trans_white
	dw	CodeOFFSET  bm_opaque_black_on_black
	dw	CodeOFFSET  bm_opaque_black_on_white
	dw	CodeOFFSET  bm_opaque_white_on_black
	dw	CodeOFFSET  bm_opaque_white_on_white
;
;
mono_clipped_drawing_functions	label word
	dw	CodeOFFSET  bm_trans_black_clip
	dw	CodeOFFSET  bm_trans_black_clip
	dw	CodeOFFSET  bm_trans_white_clip
	dw	CodeOFFSET  bm_trans_white_clip
	dw	CodeOFFSET  bm_opaque_black_on_black_clip
	dw	CodeOFFSET  bm_opaque_black_on_white_clip
	dw	CodeOFFSET  bm_opaque_white_on_black_clip
	dw	CodeOFFSET  bm_opaque_white_on_white_clip
;
;
define_frame	SmartPro
cBegin	nogen
cEnd	nogen
;
;
??PROTECTEDMODE	DW    __NEXTSEG	; This variable tells us if we're in Pmode.
RealModeSeg	equ   1000h	; ??PROTECTEDMODE will equal this equate if
;				; we're in Pmode.
;
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
;	Wed 22-Jul-1987 17:54:46 -by-  Walt Moore [waltm]
;	Wrote it.
;-----------------------------------------------------------------------;
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
bm_opaque_white_on_white_clip:
	mov	al,0FFh
;
bm_trans_white_clip:
	and	al,ss_clip_mask
;
bm_trans_white:
	or	es:[di],al
;
DRAW_ADJUST	= 0			; STOSB isn't used
	upd_bm	<ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
bm_opaque_black_on_black_clip:
	mov	al,0FFh
;
bm_trans_black_clip:
	and	al,ss_clip_mask
;
bm_trans_black:
	not	al
	and	es:[di],al
;
DRAW_ADJUST	= 0			; STOSB isn't used
	upd_bm	<ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
bm_opaque_black_on_white:
	not	al
;
bm_opaque_white_on_black:
	stosb
;
DRAW_ADJUST	= 1			; STOSB is used
	upd_bm	<ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
bm_opaque_black_on_white_clip:
	not	al
;
bm_opaque_white_on_black_clip:
	mov	ah,ss_clip_mask
	and	al,ah
	not	ah
	and	ah,es:[di]
	or	al,ah
	stosb
;
DRAW_ADJUST	= 1			; STOSB is used
	upd_bm	<ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
bm_opaque_white_on_white:
	mov	al,0FFh
	jmpnext
;
bm_opaque_black_on_black:
	xor	al,al
	jmpnext stop
	stosb
;
DRAW_ADJUST	= 1			; STOSB is used
	upd_bm	<ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
	public	bm_trans_color_fix
	public	bm_trans_color
;
bm_trans_color_fix:
	push	dx			; put return address on stack
;
bm_trans_color:
	pushem	ax,bx,cx,dx,di
	xchg	bp,ss_p_frame		; fetch ss_p_frame
	mov	bl,al			; get bits to be changed
	mov	cx,8			; check all 8 pixels
	mov	dx,ss_colors		; dl=Fg, dh=Bg
;
bmtrco_loop:
	mov	dh,es:[di]		; dh=old Bg
	GetColor			; calc the new color
	stosb				; save the new color
	loop	bmtrco_loop
;
	xchg	bp,ss_p_frame		; restore bp and ss_p_frame
	popem	ax,bx,cx,dx,di		; restore other registers
;
DRAW_ADJUST = 0
	upd_bm <ret>			; Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
	public	bm_opaque_color_fix
	public	bm_opaque_color
;
bm_opaque_color_fix:
	push	dx			;put return address on stack
;
bm_opaque_color:
	pushem	ax,bx,cx,dx,di
	xchg	bp,ss_p_frame		; fetch ss_p_frame
	mov	bl,al			; get bits to be changed
	mov	cx,8			; check all 8 pixels
	mov	dx,ss_colors		; dl=Fg, dh=Bg
;
bmopco_loop:
	GetColor			; calc the new color
	stosb				; save the new color
	loop	bmopco_loop
;
	xchg	bp,ss_p_frame		; restore bp and ss_p_frame
	popem	ax,bx,cx,dx,di		; restore other registers
;
DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
;
;
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
;
;
;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;
;
	assumes ds,nothing
	assumes es,nothing
	assumes ss,StrStuff
;
	public	preset_pro_text
;
preset_pro_text proc	near
	mov	ss_p_frame,bp
	mov	cx,next_scan
	mov	ss_next_scan,cx
	mov	cx,colors
	mov	ss_colors,cx
;
; Accumulate foreground/background colors, opaque/transparent mode,
; and device/bitmap to determine which drawing functions will be used.
;
	and	bx,IS_OPAQUE		;BL = 0000000 O/T
	errnz	IS_OPAQUE-00000001
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
;
; Now that we have an index into our tables of drawing functions,
; find out which table to use.  We use special tables for the
; nonclipped cases of device and monochrome bitmaps.  For color
; bitmaps, and all clipped cases, we'll get the drawing function
; from the general case table.
;
	mov	ax,non_clipped_drawing_functions[bx]	; load color routines
	mov	cx,clipped_drawing_functions[bx]
	mov	si,ax					; si | di will not be
	mov	di,cx					; used by color.
;
	test	bl,MASK_DEVBM			; is this color?
	errnz	<MASK_DEVBM and 0FF00h>
	jnz	preset_pro_text_lookup_func	; yes, skip
;
	mov	ax,mono_non_clipped_drawing_functions[bx]
	mov	cx,mono_clipped_drawing_functions[bx]
;
rtIFNEQU PROTECTEDMODE,RealModeSeg
; 32 bit table lookups
	mov	si,special386_case_non_clip_tables[bx]
	mov	di,special386_case_clip_tables[bx]
rtELSE
; 16 bit table lookups
	mov	si,special_case_non_clip_tables[bx]
	mov	di,special_case_clip_tables[bx]
rtENDIF
;
preset_pro_text_lookup_func:
	mov	non_clipped_table,si
	mov	clipped_table,di
	mov	ss_draw,ax
	mov	ss_draw_clipped,cx
	ret

preset_pro_text endp
;
;
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
;
; The following equate directs the control of macro code expansion
; for 16 bit or 32 bit operation.
;
; 32 bit code
;
	a386 = 1
;
	one_char	<gen386_nc>,<non_clipped_output>,<loop_logic>,<sub>
	two_char	<gen386_nc>,<non_clipped_output>,<loop_logic>,<sub>
	three_char	<gen386_nc>,<non_clipped_output>,<loop_logic>,<sub>
	four_char	<gen386_nc>,<non_clipped_output>,<loop_logic>,<sub>
	n_char		<gen386_nc>,<non_clipped_output>,<loop_logic>,<sub>
;
	one_char	<gen386_cl>,<clipped_output>,<loop_logic>,<sub>
	two_char	<gen386_cl>,<clipped_output>,<loop_logic>,<sub>
	three_char	<gen386_cl>,<clipped_output>,<loop_logic>,<sub>
	four_char	<gen386_cl>,<clipped_output>,<loop_logic>,<sub>
	n_char		<gen386_cl>,<clipped_output>,<loop_logic>,<sub>
;
if SPECIAL_CASE_BM_OBWNC eq 1
	one_char	<bm386_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm386_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm386_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm386_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
endif
;
; 16 bit code
;
	a386 = 0
;
	one_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	two_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	three_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	four_char	<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
	n_char		<gen_nc>,<non_clipped_output>,<loop_logic>,<sub>
;
	one_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	two_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	three_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	four_char	<gen_cl>,<clipped_output>,<loop_logic>,<sub>
	n_char		<gen_cl>,<clipped_output>,<loop_logic>,<sub>
;
if SPECIAL_CASE_BM_OBWNC eq 1
	one_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	two_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	three_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
	four_char	<bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>
endif
;
;-----------------------=================================----------------------
;-----------------------====< color output routines >====----------------------
;-----------------------=================================----------------------
;
;
; C O L O R _ O P A Q U E _ O U T P U T
;
; Assumed Entry Conditions:
;	   dl holds the background color
;	   dh holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   cl holds the character width
;	   bp holds the character length
;
; Exit Conditions:
;	CH = 0, CL holds the width
;	BP = 0
;	AX,BX,DX,SI,DI modified
;
coopou_xx1:
	add	di,ss_next_scan		; skip to next scan line
;
	public	color_opaque_output
color_opaque_output	label near
	lodsb		;8086/386 safe	; fetch the pattern byte
	mov	bl,al			; place byte in bl
	mov	ch,cl			; load width counter (inner loop)
	mov	cl,ss_clip_mask
	shl	bl,cl			; adjust for clipping
	mov	cl,ch
;
; optimize for zero/ones case
;
	or	bl,bl			; is it all zeros
	jz	coopou_zeros		; yes, handle as a special case
	cmp	bl,0ffh			; is it all ones?
	jz	coopou_ones		; yes, handle as a special case
;
;------ZERO CASE------------------------
;
coopou_xx2:
	shl	bl,1			; push out next pel to carry
	jc	coopou_xx5		; go to ONE CASE if set
;
coopou_xx3:
	mov	al,dh			; save the new background
	stosb				;  color
	dec	ch			; dec inner loop control
	jnz	coopou_xx2		; go for more...
	dec	bp			; dec outer loop control
	jnz	coopou_xx1		; not done, go for more...
	jmp	short coopou_xx7	; all done with this scan line
;
;------ONES CASE-----------------------
;
coopou_xx4:
	shl	bl,1			; push out next pel to carry
	jnc	coopou_xx3		; go to ZERO CASE if not set
;
coopou_xx5:
	mov	al,dl			; load the foreground
	stosb				; save the new color
	dec	ch			; dec inner loop counter
	jnz	coopou_xx4		; go for more...
;
coopou_xx6:
	dec	bp			; dec outer loop counter
	jnz	coopou_xx1		; go for more...
;
coopou_xx7:
	ret
;
coopou_zeros	label	near	; all zeros so output all background color
	mov	al,dh			; load the new background...
	mov	ah,cl
	sub	ch,ch			; clear CH for the big move
	rep	stosb			; splat!!!
	mov	cl,ah
	jmp	coopou_xx6		; go check for end of loop
;
coopou_ones	label	near	; all ones so output all foreground color
	mov	al,dl			; load the new foreground...
	mov	ah,cl
	sub	ch,ch			; clear CH for the big move
	rep	stosb			; splat!!!
	mov	cl,ah
	jmp	coopou_xx6		; go check for end of loop
;
;
;---------------------------------------
; T R A N S P _ L O O P _ C O N T R O L
;
; Assumed Entry Conditions:
;	   dh holds the background color
;	   dl holds the foreground color
;	es:di ==> destination
;	ds:si ==> font data
;	   cl holds the character width
;	   bp holds the character length
;
; Exit Conditions:
;	CH = 0, CL holds the width
;	BP = 0
;	AX,BX,DX,SI,DI modified
;
cotrou_xx1:
	add	di,ss_next_scan
;
	public	color_transp_output
color_transp_output label near		; entrypoint starts here...
	lodsb		;8086/386 safe	; fetch the pattern byte
	xchg	al,bl			; place byte in bl
	mov	ch,cl			; load width counter (inner loop)
	mov	cl,ss_clip_mask
	shl	bl,cl			; adjust for clipping
	mov	cl,ch
;
; optimize for zeros and ones
;
	mov	al,dl			; AL will alway hold the foreground
	or	bl,bl			; all zeros?
	jz	cotrou_zeros		; yes, do special case
	cmp	bl,0ffh			; all ones?
	jz	cotrou_zeros		; yes, do special case
;
;--------------ZEROS CASE---------------;
;
cotrou_xx3:
	shl	bl,1			; carry clear = old color
	jc	cotrou_xx6		; transition to ONES CASE
;
cotrou_xx4:
	inc	di			; skip this move 
	dec	ch			; decrement inner loop count
	jnz	cotrou_xx3		; not done, continue inner loop
	dec	bp			; dec outer loop
	jnz	cotrou_xx1		; done with outer, skip to next raster
	jmp	short cotrou_xx8	; all done, exit out
;
;---------------ONES CASE---------------;
;
cotrou_xx5:
	shl	bl,1			; carry set = new color
	jnc	cotrou_xx4		; transition to ZEROS CASE
;
cotrou_xx6:
	stosb				; save the new color
	dec	ch			; decrement inner loop count
	jnz	cotrou_xx5		; not done, continue inner loop
;
cotrou_xx7:
	dec	bp			; dec outer loop
	jnz	cotrou_xx1		; done with outer, skip to next raster
;
cotrou_xx8:
	ret				; exit home...
;
cotrou_zeros	label	near	; all zeros so just advance the pointer
	sub	ch,ch
	add	di,cx			; advance the pointer to the next char
	jmp	short cotrou_xx7	; go for more...

cotrou_ones	label	near	; all ones so splat
	mov	ah,cl
	sub	ch,ch
	rep	stosb			; splat!!!
	mov	cl,ah
	jmp	short cotrou_xx7	; go for more...
;
; TTTThats All folks !!!!
;
sEnd	Code
	end
