;----------------------------------------------------------------------------;
; Copyright (C) Microsoft Corporation 1985-1990. All Rights Reserved.        ;
;----------------------------------------------------------------------------;

        page    ,132

;-----------------------------Module-Header-----------------------------;
; Module Name:	STRBLT.INC
;
;   This module contains the definition of strblt's frame, flag
;   definitions, and anything else common to all the seperate
;   modules which make up the strblt function.
;
; Created: 15-Apr-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1984-1987 Microsoft Corporation
;
; Exported Functions:	None
;
; Public Functions:	none
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;-----------------------------------------------------------------------;


PROTECTEDMODE = 1


;	Definitions for the "accel" flag

IS_OPAQUE	equ	00000001b	;Background mode is opaque
HAVE_CHAR_EXTRA equ	00000010b	;Non-zero character extra given
DDA_NEEDED	equ	00000100b	;Justification DDA must be run
HAVE_BRK_EXTRA	equ	00001000b	;Some justification each break char
NEG_SPACING	equ	00010000b	;CharExtra or TBreakExtra was negative
HAVE_WIDTH_VECT equ	00100000b	;Width vector was given
FIXED_PITCH	equ	01000000b	;Font is fixed pitch
WIDTH_IS_8	equ	10000000b	;Font is 8 pixels per character
WEIRD_SPACING	equ	00111110b	;Any of these need special attention



;	Definitions for the "excel" flag
OPAQUE_RECT	equ	00000001b	;Opaquing rectangle is present
TEXT_VISIBLE	equ	00000010b	;Some part of the text is visible
BOUNDED_IN_Y	equ	00000100b	;Text fits within opaque rectangle in Y
IS_DEVICE	equ	00001000b	;Destination is our device
FIRST_IN_PREV	equ	00010000b	;First char starts in previous byte
RES_EGA_INNER	equ	00100000b	;Restore EGA for opaque inner loop
CLIPPED_RIGHT	equ	01000000b	;Overhanging right side of clip rect
CLIPPED_LEFT	equ	10000000b	;Overhanging left  side of clip rect



;	Definitions for the "wc_flags"
WC_OLD_OPAQUE	equ	00000001b	;Shadow of IS_OPAQUE
WC_SET_LR	equ	00000010b	;Set left/right into text bbox
;		equ	00000100b
STEPPED_BACK	equ	00001000b	;Stepped backwards at somepoint
;		equ	00010000b
;		equ	00100000b
;		equ	01000000b
;		equ	10000000b



MOST_POS_INT	equ	7FFFh		;Maximum positive integer
MOST_NEG_INT	equ	8000h		;Maximum negative integer


PROP_WIDTH	equ	0		;Width entry index for prop fonts
PROP_OFFSET	equ	2		;Offset entry index for prop fonts


STACK_SLOP	equ	200		;# of bytes of stack space we cannot
					;  use when building up a string


;	Definitions for the eto_options parameters

;		equ	00000001b	;Be thankful we removed this bit!
ETO_OPAQUE_FILL equ	00000010b	;Fill opaque rect with background color
ETO_OPAQUE_CLIP equ	00000100b	;Clip to opaque rectangle
;		equ	00001000b
;		equ	00010000b
;		equ	00100000b
;		equ	01000000b
;		equ	10000000b



;	Index for foreground and background colors stored
;	in the frame variable "colors"

FOREGROUND	equ	0
BACKGROUND	equ	1



;	frame_data is the definition of the data which is pushed
;	into the stack for each character (where a character
;	should be defined as a pattern of 8 bits or less).

frame_data	struc
fd_width	db	?		;Width of the character
fd_phase	db	?		;X location mod 8

ifdef PROTECTEDMODE

fd_bits_offset	dd	?		;Offset to the bits

else

fd_bits_offset	dw	?		;Offset to the bits

endif

frame_data	ends



;	The local font structure has information copied from the
;	font structure passed to strblt.  Having the data in a
;	structure makes it a little more efficient when copying
;	the data to the frame.

local_font_def	struc
font_width	dw	0		;Width field for the font
font_height	dw	0		;Height field for the font
max_width	dw	0		;Maximum character width
first_char	db	0		;First character in the font
last_char	db	0		;Last character in the font (mapped)
default_char	db	0		;Default character for out of range
break_char	db	0		;Character to define word breaks
lp_font_bits	dd	0		;Pointer to actual bits of font
local_font_def	ends



;	Certain locations of the stack frame will be used to store
;	variables for the actual drawing routines.  No call can be
;	outside the driver, or to another segment which could be
;	non-resident while these locations are altered!
;
;	Note well: only 16 bytes are available!

;----------------------------------------------------------------------------;
; The first paragraph in any windows data segment is used by windows for the ;
; local heap. Every windows application is supposed to reserve one paragraph ;
; at the start of the data segment for windows use. The Textout output routi-;
; -nes use all registers that are available to speed up the output, they even;
; use BP and thus cannot access any of the local variables in the stack by   ;
; name. But they still do need other variables to be accessed from memory. To;
; solve this problem, the first paragraph of the data (same as stack segment);
; is saved and the output routines are allowed to use these 16 bytes for loc-;
; -al storage. This are is restore after the output routines are done.       ;
;									     ;
; When the dispplay drivers do bolding, the output routines need a flag bit  ;
; to tell whether bolding is to be done or not. But the lowparagraph of the  ;
; stack segment has already been assigned and there is no space to keep this ;
; flag. The location 'ss_num_chars' is used only in the n-char output routine;
; We have decided to use this location for two purposes:		     ;
;									     ;
;		1. The lsb of this location will have the bold flag for the  ;
;		   duration of the build string and output routines.	     ;
;		2. The n-char output routines will use bits 1 through 7 of   ;
;		   the same location for keeping the value of n. (Note n can ;
;		   never excedd 16 as we do output 16 bits at a time - so    ;
;		   atbest 16 characters would fit in the 16 bits).	     ;
;								    	     ;
; The 1 through 5 character output routines should clear out all but bit 0 of;
; this location (since the n_char routine saves n here), and they refer to   ;
; this location as 'ss_boldflag'. The n_char routine zero out bits 1-7, left ;
; shift the value of n and save it. While retrieving n, the value fetched    ;
; from the location should be right_shifted by 1.			     ;
;									     ;
; (we keep the bold flag in the lsb, because some of the output routines use ;
; the bold flag to specify the amount of shift (0=> no shift, 1=> shift by 1 ;
; and or).								     ;
;									     ;
; This location is thus referred to as either 'ss_boldflag' or as 'ss_num_ch-;
; -ars' depending on the context.					     ;
;									     ;
; CAUTION: REMEMBER, THE STRUCTURE DEFINED BELOW CAN ONLY BE 16 BYTES LONG.  ;
;          YOU CANNOT ADD ANY MORE FIELDS HERE. IF YOU NEED EXTRA MEMORY     ;
;          FOR TEMPORARY STORAGE, CONSIDER SAVING SOME LOCATION IN THE HEADER;
;          PART OF THE FONT SEGMENT AND USING THEM (THE OUTPUT ROUTINES HAVE ;
;          DS LOADED WITH THE FONT SEGMENT SELECTOR.)			     ;
;----------------------------------------------------------------------------;


StrStuff	segment at 0
ss_height	dw	0		;# of scans to output
ss_next_scan	dw	?		;Delta to next scan of a bitmap
ss_draw 	dw	?		;Function to do output
ss_draw_clipped dw	?		;Function to do clipped output
ss_phases	dw	?		;Phases for 4 character case
ss_clip_mask	db	?		;Clipping mask
ss_num_chars	db	?		;Number characters for n char case
ss_boldflag	equ	ss_num_chars	;redefines prev byte
ss_colors	dw	?		;Foreground/background colors
ss_p_frame	dw	?		;Link to stack frame
StrStuff	ends




;---------------------------------Macro---------------------------------;
; define_frame
;
; Macro for defining the frame for strblt functions
;
; This file allows the frame definiton to be shared by all
; the modules whoch make up the strblt function.
;
; Calls:
;	none
; History:
;	Wed 15-Apr-1987 17:06:26 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;


define_frame macro  name
cProc	name,<FAR,PUBLIC,WIN,PASCAL>

	parmD	lp_device		;Destination device
	parmW	x			;Left origin of string
	parmW	y			;Top  origin of string
	parmD	lp_clip_rect		;Clipping rectangle
	parmD	lp_string		;The string itself
	parmW	count			;Number of characters in the string
	parmD	lp_font 		;Font to use
	parmD	lp_draw_mode		;Drawmode structure to use
	parmD	lp_xform		;Current text transform
	parmD	lp_dx			;Widths for the characters
	parmD	lp_opaque_rect		;Opaquing rectangle
	parmW	eto_options		;ExtTextOut options

	localB	local_enabled_flag	;local copy of the enabled_flag
	localB	smart_flags
	localB	special_bm_opaque_color ;Used for opauqe mode bitmaps
	localB	left_clip_mask		;lhs clip mask (00 if no partial left)
	localB	right_clip_mask 	;rhs clip mask (00 if no partial byte)
	localB	accel			;Accelerator flags
	localB	excel			;Accelerator flags
	localB	num_planes		;Number of planes in a bitmap
	localB	wc_flags		;Flags for worst case text
	localW	colors			;Text/background colors
	localW	fontweight		;normal/bold indicator

	localW	char_xtra		;# extra pixels to add each char
	localW	brk_extra		;# extra pixels to add each break char
	localW	brk_err 		;Justification DDA error term
	localW	brk_count		;DDA - # breaks into which BrkRem
	localW	brk_rem 		;  extra pixels are distributed
	localW	tot_brk_extra		;Total break extra

	localW	next_scan		;Index to get to next scan line
	localW	next_plane		;Index to get to next plane
	localW	opaque_routine		;Routine which does actual opaquing
	localW	opaque_height		;Height of opaque box (sometimes)
	localD	lp_surface		;Pointer to first byte of destination
	localW	temp_off_lp_bits	;Intermediate pointer to dev surface
	localW	scan_start		;Delta from start of scan

	localW	clipped_font_height	;Actual height of bits to be drawn
	localW	amt_clipped_on_top	;# of font scans clipped off top

	localW	cell_adjust		;Delta to get from one column to next
	localW	buffer			;Offset of buffer on the stack

ifdef PROTECTEDMODE

	localD	null_char_offset	;Offset of the special null character

else

	localW	null_char_offset	;Offset of the special null character

endif

	localW	num_null_pixels 	;Number of pixels to add
	localW	fix_total_bytes 	;Total bytes of a fixed pitch char

	localW	clear_stack		;SP value used to clean up stack
	localW	min_stack		;SP where we run out of stack
	localW	build_proc		;Procedure which builds stack data

	localW	phase
	localW	x2
	localW	inner_loop

	localW	clipped_table		;Dispatch table for clipped characters
	localW	non_clipped_table	;Dispatch table for non-clipped chars
	localW	inner_byte_count	;Number of whole dest bytes altered

	localW	current_lhs		;LHS of current sub-string
	localW	current_rhs		;RHS of current sub-string
	localW	wc_opaque_lhs		;Left  side opaqued by worst case code
	localW	wc_opaque_rhs		;Right side opaqued by worst case code

	localW	string_start_bias	;# chars skipped at start of string
	localW	ret_addr		;Fixed pitch draw routine return addr
	localW	unrolled_entry_point	;Start addr for unrolled loops

	localW	vect_one_in_first	;Dispatch vectors for fixed pitch code
	localW	vect_two_in_first
	localW	vect_one_in_middle
	localW	vect_two_in_middle
	localW	vect_one_in_last
	localW	vect_two_in_last

	localV	lfd,%(size local_font_def)
	localV	clip,%(size RECT)
	localV	text_bbox,%(size RECT)
	localV	o_rect,%(size RECT)

	localV	ss_save_area,16
	localW	ret_addr_2
	localW	ega_oc_saved_data

	endm
