PAGE 58,132
;******************************************************************************
TITLE vdmadmsg.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989
;
;   Title:	vdmadmsg.asm -
;
;   Version:	1.00
;
;   Date:	16-Aug-1989
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   16-Aug-1989 RAP
;
;==============================================================================

	.386p

.XLIST
	include VMM.INC
.LIST

VxD_DATA_SEG

BeginMsg
	PUBLIC Buffer_Too_Small, Buffer_Too_Small_value

Buffer_Too_Small    db	'DMA buffer too small.  Set DMABUFFERSIZE entry in SYSTEM.INI to '
Buffer_Too_Small_value db   'ddd', 13, 10, 0
;
; this error occurs when a requested DMA transfer that needs to be buffered
; exceeds the allocated size of the DMA buffer.  The error message should
; state that the user increase the DMABUFFERSIZE entry in SYSTEM.INI

ENDMsg

BeginMsg
;
; INI entry strings for DMA buffer allocation:
;

	PUBLIC VDMAD_Ini_Buffer_Size

VDMAD_Ini_Buffer_Size	db  'DMABUFFERSIZE', 0

; This entry is used to specify the amount of memory, in Kilobytes, to
; reserve for buffered DMA

	PUBLIC VDMAD_Ini_XT_Buffer

VDMAD_Ini_XT_Buffer	db  'DMABUFFERIN1MB', 0

; This TRUE or FALSE entry specifies that the DMA buffer memory should be
; in the first 1Mb of memory to be compatible with 8-bit bus master cards,
; if set to TRUE.

	PUBLIC VDMAD_EISA_Size_Ini

VDMAD_EISA_Size_Ini	db  'EISADMA', 0

; EISA MACHINES ONLY:
;
; The first occurance can be used to disable the auto-detection of an EISA
; machine.  VDMAD first checks for a boolean value, if it is set to FALSE, then
; VDMAD will treat the machine as non-EISA.  There is no way to force VDMAD
; to think that the machine is EISA, if the ROM doesn't identify it as EISA!
;
; If the machine has been identified as an EISA machine, then this switch can
; be used to specify the default transfer size for a DMA channel.  The EISA
; hardware doesn't allow for reading registers to determine the current mode,
; and VDMAD needs the information to correctly virtualize DMA.
;
; Set with channel #, size.  e.g.  EISADMA=1,8	EISADMA=5,16
; Channel can be 0,1,2,3,5,6, or 7.  Size can be 8, 16, 32, or 16w.  8, 16 and
; 32 specify transfer size in bits and specify that the transfer count will be
; programmed with number of bytes to transfer.	16w specifies that the channel
; is in ISA compatible mode with a transfer size of 16-bits and transfer count
; will be programmed with number of words to transfer.
;
; Defaults are:
;
;	EISADMA=0,8
;	EISADMA=1,8
;	EISADMA=2,8
;	EISADMA=3,8
;	EISADMA=5,16w
;	EISADMA=6,16w
;	EISADMA=7,16w

ENDMsg

VxD_DATA_ENDS

END
