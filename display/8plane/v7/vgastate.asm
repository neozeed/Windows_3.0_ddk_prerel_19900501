        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:	VGASTATE.ASM
;
; This file contains the pointer shape support routines required to be
; able to save and restore the EGA registers when a pointer shape needs
; to be drawn at interrupt time.
;
; Created: 06-Jan-1987
; Author:  Walt Moore [waltm]
;
; Copyright (c) 1987 Microsoft Corporation
;
; Exported Functions:	none
;
; Public Functions:	save_hw_regs
;			res_hw_regs
;			init_hw_regs
;
; Public Data:		none
;
; General Description:
;
;	SAVE_HW_REGS is called by the pointer shape routine to save
;	the state of those EGA's registers which must be used to draw
;	the pointer shape.
;
;	RES_HW_REGS is called by the pointer shape routine to restore
;	the state of those EGA registers saved by SAVE_HW_REGS, and
;	to prepare for the next call to SAVE_HW_REGS.
;
;	INIT_HW_REGS is called immediately after the EGA is placed
;	into graphics mode to initialize locations in EGA memory that
;	are required by SAVE_HW_REGS.
;
; Restrictions:
;
;	These routines are intended to be executed while protected
;	with some form of a semephore.	The contents of both the
;	Graphics Controller Address Register and the Sequencer
;	Address Register are assumed to belong to these pieces of
;	code while executing, and assumed not to change unless done
;	so by these routines.
;
;	Win386 will not preempt us while we are in the driver.	They
;	will restore the EGA to the default state as defined in EGA.INC,
;	and save and restore the last 16 bytes of whichever 16K segment
;	the display memory ends on (3FF0 or 7FF0).
;
	page
;	INIT_HW_REGS must be called before any call to either
;	SAVE_HW_REGS or RES_HW_REGS.
;
;	RES_HW_REGS must be called prior to the next call to
;	SAVE_HW_REGS or the detection code will fail.
;-----------------------------------------------------------------------;
;	Copyright February, 1990  HEADLAND TECHNOLOGY, INC.


        .xlist
        include cmacros.inc
        include windefs.inc
	include macros.mac
        include cursor.inc
        .list


	??_out	vgastate		;;Identify if not in quiet mode


	public	save_hw_regs
	public	res_hw_regs
	public	init_hw_regs

;
;   BANK dependent code.
;
        include bank.inc


GC_SET_RESET		equ	001H
GC_DATA_ROTATE		equ	003H
GC_WRITE_MODE		equ	005H
GC_BITMASK		equ	008H

SC_MAP_MASK             equ     002H
SC_MODE 		equ	004H
SC_BACKLATCH0		equ	0A0H
SC_BACKLATCH1		equ	0A1H
SC_BACKLATCH2		equ	0A2H
SC_BACKLATCH3		equ	0A3H
SC_FORELATCH0		equ	0ECH
SC_FORELATCH1		equ	0EDH
SC_FORELATCH2		equ	0EEH
SC_FORELATCH3		equ	0EFH
SC_MASKED_WRITE_MASK    equ     0F3H
SC_BANK_SELECT		equ	0F6H
SC_EXTENDED_PAGE	equ	0F9H
SC_COMPATIBILITY	equ	0FCH
SC_FOREBACK_CONTROL	equ	0FEH

MISC_INPUT              equ     3CCH
MISC_OUTPUT		equ	3C2H

SET_RESET_MODE		equ	000H
WRITE_MODE_0            equ     040H
CHAIN4_MODE		equ	00EH
CHAIN_TYPE		equ	06CH


sBegin  Data

hw_state    db	  22 dup(0)

sEnd    Data

        page
sBegin  Code
	assumes cs,Code
;-----------------------------Public-Routine----------------------------;
; save_hw_regs
;
; Save Hardware Video Registers
;
; This routine is called by the pointer shape drawing code whenever
; the state of the EGA registers must be save.  The contents of the
; following registers are saved:
;
; The pointer shape drawing routine must call RES_HW_REGS to restore
; the registers and prepare the internal work areas for the next call
; to this routine.
;
; Entry:
;       DS              = Data segment selector
;       ES              = EGA  memory  selector
; Returns:
;
; Error Returns:
;       none
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
;	GRAF_ADDR					(EGA register)
; Registers Preserved:
;       SI,DI,BP,DS,ES
;	SEQ_ADDR					(EGA register)
; Calls:
;       none
; History:
;
;	Mon 23-Jan-1989 13:00:00 -by-  David D. Miller
;	  The VRAM VGA's 256 color modes do not use any of the
;	  registers previously saved here. So don't do it
;	Wed 21-Dec-1988 09:25:30 -by-  Amit Chatterjee [amitc]
;       The VGA registers can directly be read, the save_hw_regs routine
;       now takes advantage of this.
;
;	Tue 06-Jan-1987 12:42:56 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;

	page
	assumes ds,Data
	assumes es,nothing


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;  Code for V7 VRAM card
;
;
ifdef V7VGA

%out Driver for Video 7 VRAM board

save_hw_regs    proc    near
        push    dx
        push    ax

        EnterCrit

	mov	 dx	,GC_INDEX
	in	 al	,dx
	mov	 hw_state[20] ,al

	mov	 dx	,GC_INDEX
	mov	 al	,GC_SET_RESET
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[0] ,al
	mov	 al	,0
	out	 dx	,al

	dec	 dx
	mov	 al	,GC_DATA_ROTATE
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[1] ,al
	mov	 al	,0
	out	 dx	,al

        dec      dx
	mov	 al	,GC_WRITE_MODE
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[2] ,al
ifdef VRAM768
	mov	 al	,00h
else
	mov	 al	,40H
endif
	out	 dx	,al

	dec	 dx
	mov	 al	,GC_BITMASK
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[3] ,al
	mov	 al	,0FFH
	out	 dx	,al

	mov	 dx	,SC_INDEX
	in	 al	,dx
	mov	 hw_state[21] ,al

	mov	 al	,SC_MAP_MASK
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[4] ,al
	mov	 al	,0FH
	out	 dx	,al

	dec	 dx
	mov	 al	,SC_MODE
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[5] ,al
	mov	 al	,0EH
	out	 dx	,al

	dec	 dx
        mov      al     ,SC_BACKLATCH0
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[6] ,al

	dec	 dx
        mov      al     ,SC_BACKLATCH1
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[7] ,al

	dec	 dx
        mov      al     ,SC_BACKLATCH2
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[8] ,al

	dec	 dx
        mov      al     ,SC_BACKLATCH3
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[9] ,al

	dec	 dx
	mov	 al	,SC_FORELATCH0
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[10] ,al

	dec	 dx
	mov	 al	,SC_FORELATCH1
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[11] ,al

	dec	 dx
	mov	 al	,SC_FORELATCH2
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[12] ,al

	dec	 dx
	mov	 al	,SC_FORELATCH3
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[13] ,al

	dec	 dx
        mov      al     ,SC_MASKED_WRITE_MASK
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[14] ,al
	mov	 al	,0
	out	 dx	,al

	dec	 dx
        mov      al     ,SC_BANK_SELECT
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[15] ,al

	dec	 dx
        mov      al     ,SC_EXTENDED_PAGE
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[16] ,al

	dec	 dx
        mov      al     ,SC_COMPATIBILITY
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[17] ,al
	mov	 al	,6CH
	out	 dx	,al

	dec	 dx
        mov      al     ,SC_FOREBACK_CONTROL
	out	 dx	,al
	inc	 dx
	in	 al	,dx
	mov	 hw_state[18] ,al
	mov	 al	,0
	out	 dx	,al

	mov	 dx	,MISC_INPUT
	in	 al	,dx
	mov	 hw_state[19] ,al

        LeaveCrit a

        pop     ax
        pop     dx
        ret

save_hw_regs   endp
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;  Code for GENERIC VGA card.
;
;

ifdef IBMVGA

save_hw_regs     proc    near
        ret
save_hw_regs     endp

endif

	page
;-----------------------------Public-Routine----------------------------;
; res_hw_regs
;
; Restore Hardware Video Registers
;
; This routine is called by the pointer shape drawing code whenever
; the state of the EGA registers is to be restored.
;
; Entry:
;       DS              = Data segment selector
;       ES              = EGA  memory  selector
; Returns:
;
; Error Returns:
;       none
; Registers Destroyed:
;       AX,BX,DX,FLAGS
; Registers Preserved:
;       SI,DI,BP,DS,ES
;	SEQ_ADDR					(EGA register)
; Calls:
;       set_proc_locs
; History:
;	Tue 06-Jan-1987 12:42:56 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;  Code for V7 VRAM card
;
;
ifdef V7VGA

%out Driver for Video 7 VRAM board

res_hw_regs     proc    near
        push    dx
        push    ax

        EnterCrit

	mov	 dx	,GC_INDEX
	mov	 al	,GC_SET_RESET
	mov	 ah	,hw_state[0]
	out	 dx	,ax

	mov	 al	,GC_DATA_ROTATE
	mov	 ah	,hw_state[1]
	out	 dx	,ax

	mov	 al	,GC_WRITE_MODE
	mov	 ah	,hw_state[2]
	out	 dx	,ax

	mov	 al	,GC_BITMASK
	mov	 ah	,hw_state[3]
	out	 dx	,ax

	mov	 dx	,SC_INDEX
        mov      al     ,SC_MAP_MASK
	out	 dx	,al
	inc	 dx
	mov	 al	,hw_state[4]
	out	 dx	,al

	dec	 dx
	mov	 al	,SC_MODE
	mov	 ah	,hw_state[5]
	out	 dx	,ax

        mov      al     ,SC_BACKLATCH0
	mov	 ah	,hw_state[6]
	out	 dx	,ax

        mov      al     ,SC_BACKLATCH1
	mov	 ah	,hw_state[7]
        out      dx     ,ax

        mov      al     ,SC_BACKLATCH2
	mov	 ah	,hw_state[8]
        out      dx     ,ax

        mov      al     ,SC_BACKLATCH3
	mov	 ah	,hw_state[9]
        out      dx     ,ax

	mov	 al	,SC_FORELATCH0
	mov	 ah	,hw_state[10]
	out	 dx	,ax

	mov	 al	,SC_FORELATCH1
	mov	 ah	,hw_state[11]
        out      dx     ,ax

	mov	 al	,SC_FORELATCH2
	mov	 ah	,hw_state[12]
        out      dx     ,ax

	mov	 al	,SC_FORELATCH3
	mov	 ah	,hw_state[13]
        out      dx     ,ax

        mov      al     ,SC_MASKED_WRITE_MASK
	mov	 ah	,hw_state[14]
	out	 dx	,ax

        mov      al     ,SC_BANK_SELECT
	mov	 ah	,hw_state[15]
        out      dx     ,ax

        mov      al     ,SC_EXTENDED_PAGE
	mov	 ah	,hw_state[16]
        out      dx     ,ax

        mov      al     ,SC_COMPATIBILITY
	mov	 ah	,hw_state[17]
        out      dx     ,ax

        mov      al     ,SC_FOREBACK_CONTROL
	mov	 ah	,hw_state[18]
        out      dx     ,ax

	mov	 dx	,MISC_OUTPUT
	mov	 al	,hw_state[19]
	out	 dx	,al

	mov	 dx	,GC_INDEX
	mov	 al	,hw_state[20]
	out	 dx	,al

	mov	 dx	,SC_INDEX
	mov	 al	,hw_state[21]
	out	 dx	,al

        LeaveCrit   a

        pop     ax
        pop     dx
        ret

res_hw_regs     endp
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;  Code for GENERIC VGA card.
;
;

ifdef IBMVGA

res_hw_regs     proc    near
        ret
res_hw_regs     endp

endif

	page
;-----------------------------Public-Routine----------------------------;
; init_hw_regs
;
; Initialize Hardware Video Registers
;
; This routine is called at display initialization time to initialize
; the state required to save and restore the EGA's registers and
; processor latches.
;
; The default EGA state assumed by the rest of the display driver
; code is also initialized.
;
; This code is intended to be called immediately after the EGA has
; been programmed for graphics mode and the palette registers set.
;
;
; Error Returns:
;       none
; Registers Destroyed:
;       AX,BX,DX,FLAGS
; Registers Preserved:
;       SI,DI,BP,DS,ES
; Calls:
;       none
; History:
;	Tue 06-Jan-1987 12:42:56 -by-  Walt Moore [waltm]
;	Initial version
;-----------------------------------------------------------------------;

	assumes ds,Data
	assumes es,nothing

init_hw_regs    proc    far

;
;   Initialize the banking code, set bank 0
;
        mov     bank_select,0FFh
        xor     dx,dx
        call    far_set_bank_select
        ret

init_hw_regs	endp


sEnd    Code

        end
