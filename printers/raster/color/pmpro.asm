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
;       AS THIS FILE IS BASED ON SMARTPRO.ASM IN THE DISP30 TREE OF 4 PLANE 
;	EGA/VGA DRIVERS, ANY BUG FIX THAT YOU DO IN THIS FILE WOULD PROBABLY
;       BE HAVE TO DONE FOR THE DISP30 TREE EGA/VGA DISPLAY DRIVERS TOO.
;       ********************************************************************
;
;       Thu 06-Apr-1989 10:20:10 -by-  Amit Chatterjee [amitc]
;	Modified.
;          This TextOut function has 80386 soecific code in it, so
;          will non run on 8086 or 80286 protected mode. So now all
;          the text function is being put in a separate fixed segment
;          with the 8086 TextOut appearing in another fixed segment and
;          one of these two segments chosen at enable time.	
;		. Moved code to _PROTECT segment
;		. prefixed a 'p_' to all public labels
;
;	Fri 27-Jan-1989 14:46:50 -by-  Amit Chatterjee [amitc]
;       Modified code to support >64k fonts. 
;                . The header in the font segment now has 6 byte entries
;                  per character, with the last 4 bytes being a 32 bit 
;                  pointer to the bits in the same segment.	
;		
;		 . At this point 16 bit code and 16 bit data is still being
;                  used, however where ever necessary we have used the 
;		   extended register set and the address override to take
;		   advantage of 32 bit code and data capabilities.
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

PROTECTEDMODE  = 1

	.xlist
	include macros.mac
	include cmacros.inc
	include gdidefs.inc
	include pmfntseg.inc
	include pmstrblt.inc
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
cchar	equ	dword ptr -4

wwidth1 equ	byte ptr base - 0
pphase1 equ	byte ptr wwidth1 + 1
cchar1	equ	dword ptr wwidth1 - 4

wwidth2 equ	byte ptr base - 6
pphase2 equ	byte ptr wwidth2 + 1
cchar2	equ	dword ptr wwidth2 - 4

wwidth3 equ	byte ptr base - 12
pphase3 equ	byte ptr wwidth3 + 1
cchar3	equ	dword ptr wwidth3 - 4

wwidth4 equ	byte ptr base - 18
pphase4 equ	byte ptr wwidth4 + 1
cchar4	equ	dword ptr wwidth4 - 4

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
	sub	di,DRAW_ADJUST		;;extra adjust for stosb or stosw
	add	di,ss_next_scan
	ll
	endm


;----------------------------------------------------------------------------;

page

;----------------------------------------------------------------------------;
; The bit gathering code generating macros are defined next. These logic of  ;
; the code is quite general in the sense that it can be used to gather 8 or  ;
; 16 bits (actually can gather upto 24 bits). The gathered bits will either  ;
; be in AL (for 8 bit gathering) or in AX (for 16 bit gathering). However the;
; 'UPDATE' will mostly be for 16 bit output. Now clipped case would always   ;
; output 8 bytes. However for non-clipped cases if we have an odd number of  ;
; inner bytes we will have to gather 8 bits for the last byte, but we will   ;
; force this into the clip case with a clip mask of 0ffh. As the update      ;
; will normally convert 16 bits at a time, CELL_ADJUST will have the appropr-;
; -ate decrement. But for clipped cases CELL_ADJUST needs to be 1 more. To   ;
; take care of this we have added one more parameter to the macro,namely,    ;
; DIDEC. DIDEC will be set to non_blank only for the macros for the clipped_ ;
; -cases. If it is non blank, we will force a DEC DI instruction.	     ;
;----------------------------------------------------------------------------;

;--------------------------------Macro----------------------------------;
; n_char
;
;	n_char is a macro for generating the code required to process
;	a destination character consisting of 5,6,7 or 8 source bytes
;
;	Usage:
;		n_char	name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

n_char	macro	name,output,update,setup,didec
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
	jnz	short name&_n_char_outer_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; n character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	ss_height		;;Loop until all chars output
	jz	short name&_n_char_exit
	jmp	name&_n_char_outer_loop
name&_n_char_exit:
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
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
  ifndef p_n_char_setup			;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_n_char_setup			;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif 				;;
endif					;;
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_n_char_setup			;;    make public for debugging
p_n_char_setup proc near	    	;;    define start of setup proc
  endif 				;;  endif
	mov	ss_height,ax		;;  save # scans in character
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
  	mov	ch,ss_boldflag		;;get the bold flag
	and	ch,1			;;isolate just bold flag
	shl	cl,1			;;make room for bold flag
	or	cl,ch			;;n can't be > 127, or in bold flag 
	mov	ss_num_chars,cl 	;;  save # of characters - 1
	push	bp			;;  save frame pointer
	mov	bp,dx			;;  set buffer pointer
	xor	dx,dx			;;  index into font scan
	movzx	edi,di
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
p_n_char_setup endp			;;    terminate the procedure
name&_n_char proc near			;;    define actual procedure
	call	p_n_char_setup		;;    call setup code
  endif 				;;  endif
else					;;else
name&_n_char proc near			;;    define actual procedure
	call	p_n_char_setup		;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; n character compilation logic
;-----------------------------------------------------------------------;

name&_n_char_outer_loop:
	xor	eax,eax
	xor	si,si			;;SI = char index
	mov	ch,ss_num_chars 	;;Get # of characters - 1
	shr	ch,1			;;throw bold flag out, get count
	xchg	dx,di			;;Index to next font scan in DI
name&_n_char_inner_loop:
	mov	ebx,[bp][si].cchar	;;EBX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][si].wwidth	;;CL = width
	shl	eax,cl
	sub	si,6			;;--> next char
	dec	ch
	jnz	name&_n_char_inner_loop
	mov	ebx,[bp][si].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	test	ss_boldflag,1		;;test bolding. here flag is MSB
	jnz	short name&_n_char_bold_it    ;;yes
name&_n_char_output:
	mov	cl,[bp][si].pphase	;;CL = phase
	shr	eax,cl
	xchg	dx,di			;;DI = dest ptr
	output	<update>		;;Macro to do whatever for outputting
ifnb	<didec>
	dec	di			;;for 8 bit update
endif
	ret
name&_n_char_bold_it:
	mov	ebx,eax
	shr	ebx,1
	or	eax,ebx
	jmp	short name&_n_char_output

name&_n_char  endp
	endm


;--------------------------------Macro----------------------------------;
; five_char
;
;	five_char is a macro for generating the code required to process
;	a destination character consisting of 5,6,7 or 8 source bytes
;
;	Usage:
;		five_char	name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

five_char	macro	name,output,update,setup,didec
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; 5 character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	si			;;Loop until all chars output
	jnz	short name&_five_char_outer_loop
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; 5 character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	inc	dx			;;Next scan of font
	dec	si			;;Loop until all chars output
	jz	short name&_five_char_exit
	jmp	name&_five_char_outer_loop
name&_five_char_exit:
	pop	bp			;;BP = frame pointer
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; 5 character setup logic
;-----------------------------------------------------------------------;
public	name&_five_char			;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_five_char proc near			;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef p_five_char_setup		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_five_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif 				;;
endif					;;
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_five_char_setup  		;;    make public for debugging
p_five_char_setup proc near  		;;    define start of setup proc
  endif 				;;  endif
	mov	si,ax			;;  save # scans in character
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	mov	ch,ss_boldflag		;;get the bold flag
	and	ch,01			;;isolate bold flag
	push	bp			;;  save frame pointer
	mov	bp,dx			;;  set buffer pointer
	xor	dx,dx			;;  index into font scan
	movzx	edi,di			;;clear out high word
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
p_five_char_setup endp			;;    terminate the procedure
name&_five_char proc near	      	;;    define actual procedure
	call	p_five_char_setup     	;;    call setup code
  endif 				;;  endif
else					;;else
name&_five_char proc near		;;    define actual procedure
	call	p_five_char_setup      	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; 5 character compilation logic
;-----------------------------------------------------------------------;

name&_five_char_outer_loop:
	xor	eax,eax
	xchg	dx,di			;;Index to next font scan in DI

	mov	ebx,[bp][0].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][0].wwidth	;;CL = width
	shl	eax,cl

	mov	ebx,[bp][-6].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][-6].wwidth	;;CL = width
	shl	eax,cl

	mov	ebx,[bp][-12].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][-12].wwidth	;;CL = width
	shl	eax,cl

	mov	ebx,[bp][-18].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][-18].wwidth	;;CL = width
	shl	eax,cl

	mov	ebx,[bp][-24].cchar	;;BX = offset of bits
	mov	al,[ebx][edi]
	mov	cl,[bp][-24].pphase	;;CL = phase
	mov	ebx,eax
	xchg	cl,ch			;;get the bold flag 1 for bold text
	shr	ebx,cl			;;bold flag is 0 for normal
	or	eax,ebx
	xchg	cl,ch
name&_five_char_output:
	shr	eax,cl
	xchg	dx,di			;;DI = dest ptr
	output	<update>		;;Macro to do whatever for outputting
ifnb	<didec>
	dec	di			;;for 8 bit update
endif
	ret

name&_five_char  endp
	endm




page

;--------------------------------Macro----------------------------------;
; four_char
;
;	four_char is a macro for generating the code required to
;	process a destination character consisting of 4 source bytes
;
;	Usage:
;		four_char  name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

four_char macro name,output,update,setup,didec
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; four character looping logic macro
;
; the no-of-scans-to-converted has been cached in HIWORD of EDI
;-----------------------------------------------------------------------;

loop_logic	&Macro
	sub	edi,10000h		;;subtract from the HIWORD
	jnc	short name&_four_char_loop    ;;still more scans to convert
	pop	ebp			;;EBP = frame pointer
	sub	di,cell_adjust		;;adjust di for the next char
	&endm

;-----------------------------------------------------------------------;
; four character long looping logic macro
;
; the no-of-scans-to-converted has been cached in HIWORD of EDI
;-----------------------------------------------------------------------;

long_loop_logic	&Macro
	sub	edi,10000h		;;subtract 1 from the HIWORD
	jc	short name&_four_char_exit
	jmp	name&_four_char_loop
name&_four_char_exit:
	pop	ebp			;;EBP = frame pointer
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; four character setup logic
;
; we will use the HIWORD of EDI to cache the number of scans to convert
; and the HIWORD of ECX to cache some of the phases
;-----------------------------------------------------------------------;

public	name&_four_char 		;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_four_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef p_four_char_setup		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_four_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_four_char_setup 		;;    make public for debugging
p_four_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
  	and	ss_boldflag,1		;;isolate bold flag
	rol	edi,16			;; save DI in HIWORD
	mov	di,ax			;; load # scans to convert
	dec	di			;;we will decrement till cary in loop
	rol	edi,16			;; cache away # and restore DI
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	push	ebp			;;  save frame pointer
	mov	bp,dx			;;  --> buffer
	mov	cl,[bp].wwidth3 	;;
	mov	ch,[bp].pphase4 	;;
	rol	ecx,16
	mov	cl,[bp].wwidth1		;;
	mov	ch,[bp].wwidth2
	xor	esi,esi			;; will use [esi][edx] address mode
	mov	esi,[bp].cchar4		;;  4th character so we can lodsb
	mov	ebx,[bp].cchar1		;;
	xor	edx,edx			;; will use [edx] address mode
	mov	edx,[bp].cchar3		;;
	mov	ebp,[bp].cchar2		;;
	sub	ebx,esi			;;  compute deltas
	sub	ebp,esi			;;
	sub	edx,esi			;;  for [esi][edx] addressing
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
p_four_char_setup endp			;;    terminate the procedure
name&_four_char proc near		;;    define actual procedure
	call	p_four_char_setup 	;;    call setup code
  endif 				;;  endif
else					;;else
name&_four_char proc near		;;    define actual procedure
	call	p_four_char_setup 	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; four character compilation logic
;-----------------------------------------------------------------------;

name&_four_char_loop:
	xor	eax,eax
	mov	al,[esi][ebx]
	shl	eax,cl
	mov	al,ds:[esi][ebp]
	xchg	cl,ch
	shl	eax,cl
	xchg	cl,ch
	rol	ecx,16			;; get the cached phases in CX
	mov	al,[esi][edx]
	shl	eax,cl
	lods	byte ptr ds:[esi]
	test	ss_boldflag,1
	jnz	short name&_4_char_boldit
name&_4_char_output:
	xchg	cl,ch
	shr	eax,cl
	xchg	cl,ch
	rol	ecx,16			;; cache back the phases
	output	<update>		;;Macro to do whatever for outputting
ifnb	<didec>
	dec	di			;;extra adjust for 8 bit code
endif
	ret
name&_4_char_boldit:
	push	ebx 
	mov	ebx,eax
	shr	ebx,1
	or	eax,ebx
	pop	ebx 
	jmp	short name&_4_char_output

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
;		three_char  name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

three_char macro name,output,update,setup,didec
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; three character looping logic macro
;
; the number of scans to convert is cached in the HIWORD of ECX
;-----------------------------------------------------------------------;

loop_logic	&macro
	sub	ecx,10000h		;;subtract 1 from HIWORD
	jnc	short name&_three_char_loop   ;;still more to convert
	pop	ebp			;;was pushed by setup macro
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; three character long looping logic macro
;
; the number of scans to convert is cached in the HIWORD of ECX
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	sub	ecx,10000h		;;subtract 1 from the HIWORD
	jc	short name&_three_char_exit
	jmp	name&_three_char_loop
name&_three_char_exit:
	pop	ebp
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; three character setup logic
; the number of scans to convert will be cached into hiword of ECX
;-----------------------------------------------------------------------;

public	name&_three_char		;;make public
genflag=0				;;assume out_of_line, already generated
ifidn <setup>,<inline>			;;if in_line
name&_three_char proc near		;;  define procedure
  genflag = 01b 			;;  show code must be generated
else					;;else
  ifndef p_three_char_setup		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_three_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_three_char_setup		;;    make public for debugging
p_three_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
  	and	ss_boldflag,01		;;isolate bold flag
	mov	cx,ax
	dec	cx			;;in loop we test for carry not 0
	rol	ecx,16			;; cache scan count into hiword ECX
  if genflag and 10b			;;  if generating setup procedure
	pop	ax			;;    get return address
  endif 				;;  endif
	push	ebp			;;loop_logic macro pops EBP on exit
	mov	bp,dx			;;  BP = buffer
	mov	dl,[bp].wwidth1 	;;
	mov	dh,[bp].wwidth2 	;;
	mov	ch,[bp].pphase3 	;;
	mov	esi,[bp].cchar3		;;
	mov	ebx,[bp].cchar2		;;
	mov	ebp,[bp].cchar1		;;
	sub	ebx,esi			;;
	sub	ebp,esi			;;
  if genflag and 10b			;;  if generating setup procedure
	jmp	ax			;;    dispatch to caller
p_three_char_setup endp			;;    terminate the procedure
name&_three_char proc near		;;    define actual procedure
	call	p_three_char_setup	;;    call setup code
  endif 				;;  endif
else					;;else
name&_three_char proc near		;;    define actual procedure
	call	p_three_char_setup	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; three character compilation logic
;-----------------------------------------------------------------------;

name&_three_char_loop:
	xor	eax,eax
	mov	al,ds:[esi][ebp]
	mov	cl,dl
	shl	eax,cl
	mov	al,[esi][ebx]
	mov	cl,dh
	shl	eax,cl
	lods	byte ptr ds:[esi]
	test	ss_boldflag,1
	jnz	short name&_3_char_boldit
name&_3_char_output:
	mov	cl,ch
	shr	eax,cl
	output	<update>		;;Macro to do whatever for outputting
ifnb	<didec>
	dec	di			;;extra adjust for 8 bit gathering
endif
	ret
name&_3_char_boldit:
	push	ebx 
	mov	ebx,eax
	shr	ebx,1
	or	eax,ebx
	pop	ebx
	jmp	short name&_3_char_output

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
;		two_char  name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

two_char macro name,output,update,setup,didec
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; two character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	dec	dx
	jnz	short name&_two_char_loop
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; two character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	dec	dx
	jz	short name&_two_char_exit
	jmp	name&_two_char_loop
name&_two_char_exit:
	sub	di,cell_adjust
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
  ifndef p_two_char_setup 		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_two_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_two_char_setup		;;    make public for debugging
p_two_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
  	and	ss_boldflag,01		;;isolate bold flag
	xchg	bp,dx			;;  BP = buffer, DX = font height
	mov	cl,[bp].wwidth1 	;;
	mov	ch,[bp].pphase2 	;;
	mov	ebx,[bp].cchar1		;;
	mov	esi,[bp].cchar2		;;
	mov	bp,dx			;;  restore frame pointer
	xchg	ax,dx			;;  set DX = font height
	sub	ebx,esi			;;  delta between the characters
  if genflag and 10b			;;  if generating setup procedure
	ret				;;    dispatch to caller
p_two_char_setup endp			;;    terminate the procedure
name&_two_char proc near		;;    define actual procedure
	call	p_two_char_setup	;;    call setup code
  endif 				;;  endif
else					;;else
name&_two_char proc near		;;    define actual procedure
	call	p_two_char_setup	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; two character compilation logic
;-----------------------------------------------------------------------;

name&_two_char_loop:
	xor	eax,eax
	mov	al,[ebx][esi]
	shl	eax,cl
	xchg	cl,ch
	lods	byte ptr ds:[esi]
	test	ss_boldflag,1
	jnz	short name&_2_char_boldit
name&_2_char_output:
	shr	eax,cl
	xchg	cl,ch
	output	<update>		;;Macro to do whatever for outputting
ifnb	<didec>
	dec	di			;;extra update for 8 bit gathering code
endif
	ret
name&_2_char_boldit:
	push	ebx
	mov	ebx,eax
	shr	ebx,1
	or	eax,ebx
	pop	ebx
	jmp	short name&_2_char_output

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
;		one_char  name,output,update,setup,didec
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
;		didec	if not blank, it expands into dec di, which
;			is needed for 8 bit update code
;
;	The macro will generate the loop_logic macro which is the
;	macro used by all update macros for generating looping logic.
;	It is defined as a macro so that devices which are interleaved
;	can make multiple copies of it, possibly removing a jump.
;-----------------------------------------------------------------------;

one_char macro name,output,update,setup,didec
	local	genflag

	assumes ds,nothing		;;Set the assumptions now
	assumes es,nothing
	assumes ss,StrStuff

;-----------------------------------------------------------------------;
; one character looping logic macro
;-----------------------------------------------------------------------;

loop_logic	&macro
	dec	dx
	jnz	short name&_one_char_loop
	sub	di,cell_adjust
	&endm

;-----------------------------------------------------------------------;
; one character long looping logic macro
;-----------------------------------------------------------------------;

long_loop_logic	&macro
	dec	dx
	jz	short name&_one_char_exit
	jmp	name&_one_char_loop
name&_one_char_exit:
	sub	di,cell_adjust
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
  ifndef p_one_char_setup 		;;  if procedure not defined
    genflag = 11b			;;    show proc must also be generated
  else					;;  else
    ife $-p_one_char_setup		;;    a hack since it is defined on
      genflag = 11b			;;    pass 2 regardless of if we have
    endif				;;    generated it
  endif
endif
if genflag				;;if to generate setup
  if genflag and 10b			;;  if generating procedural version
public	p_one_char_setup      		;;    make public for debugging
p_one_char_setup proc near		;;    define start of setup proc
  endif 				;;  endif
  	and	ss_boldflag,01		;;isolate bold flag
	xchg	dx,bp			;;  BP --> character buffer
	mov	esi,[bp].cchar1		;;  DS:SI = char1
	mov	cl,[bp].pphase1 	;;
	xchg	dx,bp			;;  BP --> frame
	xchg	ax,dx			;;  DX = clipped_font_height
  if genflag and 10b			;;  if generating setup procedure
	ret				;;    dispatch to caller
p_one_char_setup endp			;;    terminate the procedure
name&_one_char proc near		;;    define actual procedure
	call	p_one_char_setup	;;    call setup code
  endif 				;;  endif
else					;;else
name&_one_char proc near		;;    define actual procedure
	call	p_one_char_setup     	;;    call setup code
endif					;;endif ;setup code

;-----------------------------------------------------------------------;
; one character compilation logic
;-----------------------------------------------------------------------;

name&_one_char_loop:
	lods	byte ptr ds:[esi]	;;char1
	test	ss_boldflag,1
	jnz	short name&_1_char_boldit
name&_1_char_output:
	shr	al,cl
	output	<update>
ifnb	<didec>
	dec	di			;;extra update for 8 bit gathering code
endif
	ret

name&_1_char_boldit:
	mov	bl,al
	shr	bl,1
	or	al,bl
	jmp	short name&_1_char_output


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
	or	es:[di],ah
	or	es:[di+1],al
DRAW_ADJUST	= 0			;;DI has not been inc'ed
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
	not	ax
	and	es:[di],ah
	and	es:[di+1],al
DRAW_ADJUST	= 0			;;DI has not been inced
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
	not	ax
	xchg	ah,al
	stosw
DRAW_ADJUST	= 2			;;STOSW is used
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
	xchg	ah,al
	stosw
DRAW_ADJUST	= 2			;;STOSW is used
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
	mov	word ptr es:[di],0
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
	mov	word ptr es:[di],0FFFFh
DRAW_ADJUST	= 0			;;STOSB isn't used
	update
	endm
page

;--------------------------------Macro----------------------------------;
; mapc
;
;	mapc is a macro for generating the character drawing
;	logic onto plane memory basically to synthesize italic font for
;       the particular string.
;
;	Usage:
;		mapc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

mapc	macro	update
	and	al,ss_clip_mask			;;combine with mask
	stosb					;;write it to map
DRAW_ADJUST = 1					;;stosb used
	update					;;update DI
	endm

;--------------------------------Macro----------------------------------;
; mapnc
;
;	mapnc is a macro for generating the character drawing
;	logic onto plane memory basically to synthesize italic font for
;       the particular string. This is for nonclipped bytes
;
;	Usage:
;		mapnc update
;	Where
;		update	the macro which will generate the required
;			destination update logic.
;-----------------------------------------------------------------------;

mapnc	macro	update
	xchg	al,ah			;;first byte was in ah
	stosw				;;write synthesized byte
DRAW_ADJUST = 2				;;stosw used
	update				;;update destination pointer
	endm

;-----------------------------------------------------------------------;
sBegin	Code
assumes cs,Code

	.386p

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
	dw	CodeOFFSET p_gen_nc_one_char
	dw	CodeOFFSET p_gen_nc_two_char
	dw	CodeOFFSET p_gen_nc_three_char
	dw	CodeOFFSET p_gen_nc_four_char
	dw	CodeOFFSET p_gen_nc_five_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char

gen_cl	label	word
	dw	CodeOFFSET p_gen_cl_one_char
	dw	CodeOFFSET p_gen_cl_two_char
	dw	CodeOFFSET p_gen_cl_three_char
	dw	CodeOFFSET p_gen_cl_four_char
	dw	CodeOFFSET p_gen_cl_five_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char
	dw	CodeOFFSET p_gen_cl_n_char


if SPECIAL_CASE_BM_OBWNC eq 1
bm_obwnc label word
	dw	CodeOFFSET p_bm_obwnc_one_char
	dw	CodeOFFSET p_bm_obwnc_two_char
	dw	CodeOFFSET p_bm_obwnc_three_char
	dw	CodeOFFSET p_bm_obwnc_four_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
endif

if SPECIAL_CASE_BM_OWBNC eq 1
bm_owbnc label word
	dw	CodeOFFSET p_bm_owbnc_one_char
	dw	CodeOFFSET p_bm_owbnc_two_char
	dw	CodeOFFSET p_bm_owbnc_three_char
	dw	CodeOFFSET p_bm_owbnc_four_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
endif

if SPECIAL_CASE_BM_TBNC eq 1
bm_tbnc label word
	dw	CodeOFFSET p_bm_tbnc_one_char
	dw	CodeOFFSET p_bm_tbnc_two_char
	dw	CodeOFFSET p_bm_tbnc_three_char
	dw	CodeOFFSET p_bm_tbnc_four_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
endif

if SPECIAL_CASE_BM_TWNC eq 1
bm_twnc label word
	dw	CodeOFFSET p_bm_twnc_one_char
	dw	CodeOFFSET p_bm_twnc_two_char
	dw	CodeOFFSET p_bm_twnc_three_char
	dw	CodeOFFSET p_bm_twnc_four_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
	dw	CodeOFFSET p_gen_nc_n_char
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

	dw	CodeOFFSET  p_bm_trans_color
	dw	CodeOFFSET  p_bm_trans_color
	dw	CodeOFFSET  p_bm_trans_color
	dw	CodeOFFSET  p_bm_trans_color
	dw	CodeOFFSET  p_bm_opaque_color
	dw	CodeOFFSET  p_bm_opaque_color
	dw	CodeOFFSET  p_bm_opaque_color
	dw	CodeOFFSET  p_bm_opaque_color


non_clipped_drawing_functions label word

	dw	CodeOFFSET  p_bm_trans_color_nc
	dw	CodeOFFSET  p_bm_trans_color_nc
	dw	CodeOFFSET  p_bm_trans_color_nc
	dw	CodeOFFSET  p_bm_trans_color_nc
	dw	CodeOFFSET  p_bm_opaque_color_nc
	dw	CodeOFFSET  p_bm_opaque_color_nc
	dw	CodeOFFSET  p_bm_opaque_color_nc
	dw	CodeOFFSET  p_bm_opaque_color_nc

mono_non_clipped_drawing_functions	label word

	dw	CodeOFFSET	p_bm_trans_black
	dw	CodeOFFSET	p_bm_trans_black
	dw	CodeOFFSET	p_bm_trans_white
	dw	CodeOFFSET	p_bm_trans_white
	dw	CodeOFFSET	p_bm_opaque_black_on_black
	dw	CodeOFFSET	p_bm_opaque_black_on_white
	dw	CodeOFFSET	p_bm_opaque_white_on_black
	dw	CodeOFFSET	p_bm_opaque_white_on_white
page

define_frame	PSmartPro
cBegin	nogen
cEnd	nogen
page

;--------------------------Private-Routine------------------------------;
; p_bm_opaque_white_on_white_clip
; p_bm_trans_white_clip
; p_bm_trans_white
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
;	Wed 18-Jan-1989 15:06:12 -by-  Amit Chatterjee [amitc]
;	included 16 bit gathering from the stack and introduced bolding
;       also made use of extended register set to speed up things a bit
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

p_bm_opaque_white_on_white_clip:
	mov	al,0FFh

p_bm_trans_white_clip:
	and	al,ss_clip_mask
	or	es:[di],al

DRAW_ADJUST	= 0			;STOSB isn't used
	upd_bm	<ret>			;Will generate the ret
page

p_bm_trans_white:
	xchg	ah,al			;first byte was in ah
	or	es:[di],ax

DRAW_ADJUST	= 0			;di has not been updated
	upd_bm	<ret>

page
;--------------------------Private-Routine------------------------------;
; p_bm_opaque_black_on_black_clip
; p_bm_trans_black_clip
; p_bm_trans_black
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

p_bm_opaque_black_on_black_clip:
	mov	al,0FFh

p_bm_trans_black_clip:
	and	al,ss_clip_mask
	not	al
	and	es:[di],al

DRAW_ADJUST	= 0			;STOSB isn't used
	upd_bm	<ret>			;Will generate the ret
page

p_bm_trans_black:
	not	ax
	xchg	ah,al			;ah had the first byte
	and	es:[di],ax		;output it
DRAW_ADJUST 	= 0			;di has not been updates
	upd_bm	<ret>

page
;--------------------------Private-Routine------------------------------;
; p_bm_opaque_black_on_white
; p_bm_opaque_white_on_black
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

p_bm_opaque_black_on_white:
	not	ax

p_bm_opaque_white_on_black:
	xchg	al,ah			;ah had the first byte
	stosw

DRAW_ADJUST	= 2			;STOSW is used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; p_bm_opaque_black_on_white_clip
; p_bm_opaque_white_on_black_clip
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

p_bm_opaque_black_on_white_clip:
	not	al

p_bm_opaque_white_on_black_clip:
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
; p_bm_opaque_white_on_white
; p_bm_opaque_black_on_black
;
;   Standard bitmap drawing functions for:
;
;	opaque, white on white, non-clipped
;	opaque, black on black, non-clipped
;
; Entry:
;	AH,AL = characters to output
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

p_bm_opaque_white_on_white:
	mov	ax,0FFFFh
	jmpnext

p_bm_opaque_black_on_black:
	xor	ax,ax
	jmpnext stop
	stosw

DRAW_ADJUST	= 2			;STOSW is used
	upd_bm	<ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; p_bm_trans_color_fix
; p_bm_trans_color
;
;   Standard bitmap drawing function for:
;   (now used only for clipped cases and 8 bit output)
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

	public	p_bm_trans_color_fix
	public	p_bm_trans_color

p_bm_trans_color_fix:
	push	dx			;put return address on stack

p_bm_trans_color:
	push	ax
	push	cx
	push	di

	xchg	bp,ss_p_frame		;get frame pointer

	mov	ch,ss_clip_mask
	or	ch,ch
	mov	ah,al
	jz	p_bm_trans_color_no_clip

	mov	ah,ch
	and	ah,al
p_bm_trans_color_no_clip:
	not	ah
	mov	cl,num_planes
	mov	ch,byte ptr ss_colors[FOREGROUND]

p_bm_trans_color_loop:
	and	es:[di],ah
	ror	ch,1			;see if this color plane is used
	jnc	p_bm_trans_color_set_byte
	or	es:[di],al

p_bm_trans_color_set_byte:
	add	di,next_plane
	dec	cl
	jnz	p_bm_trans_color_loop

	xchg	bp,ss_p_frame		;restore bp and ss_p_frame

	pop	di
	pop	cx
	pop	ax

DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page

;--------------------------Private-Routine------------------------------;
; p_bm_trans_color_fix_nc
; p_bm_trans_color_nc
;
;   Standard bitmap drawing function for:
;   (now used only for non-clipped 16 bit output)
;
;	transparent, color
;
;  Entry points are for the fixed width code and proportional width code,
;  respectively.
;
; Entry (fixed):
;	AH,AL = characters to output
;	DX = return address
;	ES:DI --> destination byte
; Entry (prop):
;	AH,AL = characters to output
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

	public	p_bm_trans_color_fix_nc
	public	p_bm_trans_color_nc

p_bm_trans_color_fix_nc:
	push	dx			;put return address on stack

p_bm_trans_color_nc:
	push	ax
	push	cx
	push	bx
	push	di
  	xchg	ah,al			;get the first byte in al,2nd in ah
	xchg	bp,ss_p_frame		;get frame pointer

	mov	bx,ax			;bx has the characters
	not	ax			;ax has their inverses

	mov	cl,num_planes
	mov	ch,byte ptr ss_colors[FOREGROUND]

p_bm_trans_color_nc_loop:
	and	word ptr es:[di],ax
	ror	ch,1			;see if this color plane is used
	jnc	p_bm_trans_color_set_byte_nc
	or	word ptr es:[di],bx

p_bm_trans_color_set_byte_nc:
	add	di,next_plane
	dec	cl
	jnz	p_bm_trans_color_nc_loop

	xchg	bp,ss_p_frame		;restore bp and frame pointers

	pop	di
	pop	bx
	pop	cx
	pop	ax

DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page
;--------------------------Private-Routine------------------------------;
; p_bm_opaque_color_fix
; p_bm_opaque_color
;
;   Standard bitmap drawing function for:
;   (now used only for clipped 8 bit output)
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

	public	p_bm_opaque_color_fix
	public	p_bm_opaque_color
p_bm_opaque_color_fix:
	push	dx			;put return address on stack

p_bm_opaque_color:

	push	ax
	push	cx
	push	dx
	push	di

	xchg	bp,ss_p_frame		;get frame pointer

	mov	ch,ss_clip_mask
	mov	dh,num_planes
	mov	dl,special_bm_opaque_color

p_bm_opaque_color_partial:
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
	jnz	p_bm_opaque_color_partial	;handle next color plane

	xchg	bp,ss_p_frame		;restore bp and ss_p_frame

	pop	di
	pop	dx
	pop	cx
	pop	ax
DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page
;--------------------------Private-Routine------------------------------;
; p_bm_opaque_color_fix_nc
; p_bm_opaque_color_nc
;
;   Standard bitmap drawing function for:
;   (now used only for non-clipped 16 bit output)
;
;	opaque, color
;
; Entry:
;	AH,AL = characters to output
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

	public	p_bm_opaque_color_fix_nc
	public	p_bm_opaque_color_nc
p_bm_opaque_color_fix_nc:
	push	dx			;put return address on stack

p_bm_opaque_color_nc:

	push	ax
	push	bx
	push	cx
	push	dx
	push	di

	xchg	bp,ss_p_frame		;get frame pointer

	mov	dh,num_planes
	mov	dl,special_bm_opaque_color

p_bm_opaque_color_partial_nc:
	shr	dl,1			;set C to inversion mask
	sbb	bx,bx
	and	bx,ax			;BH,BL=1 where we want NOT background
	shr	dl,1			;set C to background color
	sbb	cx,cx			
	xor	cx,bx			;AX=destination bytes
	mov	bx,word ptr es:[di]	; get the first byte
	xor	bl,ch			; CH has first processed src byte
	xor	bh,cl			; CL has second processed src byte
	xor	word ptr es:[di],bx	;first byte done
	add	di,next_plane		;point to next color plane
	dec	dh
	jnz	p_bm_opaque_color_partial_nc;handle next color plane

	xchg	bp,ss_p_frame		;restore bp and ss_p_frame

	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax

DRAW_ADJUST = 0
	upd_bm <ret>			;Will generate the ret
page

page

;--------------------------Private-Routine------------------------------;
; p_preset_pro_text
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

	public	p_preset_pro_text
p_preset_pro_text proc	near

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
	jnz	p_preset_pro_text_lookup_func
	cmp	num_planes,1
	jne	p_preset_pro_text_standard_func

	mov	ax,mono_non_clipped_drawing_functions[bx]

p_preset_pro_text_lookup_func:
	mov	si,special_case_non_clip_tables[bx]
	mov	di,special_case_clip_tables[bx]

p_preset_pro_text_have_func:
	mov	non_clipped_table,si
	mov	clipped_table,di
	mov	ss_draw,ax

	mov	ax,clipped_drawing_functions[bx]
	mov	ss_draw_clipped,ax

	ret


p_preset_pro_text_standard_func:
	mov	si,CodeOFFSET gen_nc
	mov	di,CodeOFFSET gen_cl
	jmp	short p_preset_pro_text_have_func

p_preset_pro_text endp
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
	one_char	<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>
	two_char	<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>
	three_char	<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>
	four_char	<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>
	five_char	<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>
	n_char		<p_gen_nc>,<non_clipped_output>,<loop_logic>,<sub>,<>

	one_char	<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>
	two_char	<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>
	three_char	<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>
	four_char	<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>
	five_char	<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>
	n_char		<p_gen_cl>,<clipped_output>,<loop_logic>,<sub>,<didec>


if SPECIAL_CASE_BM_OBWNC eq 1
	one_char	<p_bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>,<>
	two_char	<p_bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>,<>
	three_char	<p_bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>,<>
	four_char	<p_bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>,<>
;	n_char		<p_bm_obwnc>,<obwnc>,<upd_bm loop_logic>,<sub>,<>
endif

if SPECIAL_CASE_BM_OWBNC eq 1
	one_char	<p_bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>,<>
	two_char	<p_bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>,<>
	three_char	<p_bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>,<>
	four_char	<p_bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>,<>
;	n_char		<p_bm_owbnc>,<owbnc>,<upd_bm loop_logic>,<sub>,<>
endif

if SPECIAL_CASE_BM_TBNC eq 1
	one_char	<p_bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>,<>
	two_char	<p_bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>,<>
	three_char	<p_bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>,<>
	four_char	<p_bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>,<>
;	n_char		<p_bm_tbnc>,<tbnc>,<upd_bm loop_logic>,<sub>,<>
endif

if SPECIAL_CASE_BM_TWNC eq 1
	one_char	<p_bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>,<>
	two_char	<p_bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>,<>
	three_char	<p_bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>,<>
	four_char	<p_bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>,<>
;	n_char		<p_bm_twnc>,<twnc>,<upd_bm loop_logic>,<sub>,<>
endif

page
sEnd	Code
	end


