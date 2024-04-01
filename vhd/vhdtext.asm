PAGE 58,132
;******************************************************************************
TITLE vhdtext.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:	vhdtext.asm -
;
;   Version:	1.00
;
;   Date:	08-Jan-1990
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   08-Jan-1990 RAL
;
;==============================================================================

	.386p

	PUBLIC	VHD_Win_Ini_Key_String

	INCLUDE VMM.Inc

VxD_IDATA_SEG

BeginMsg
;
;   This system.ini flag is used to turn off the IRQ virtualization of the
;   VHD.  It is on by default for all non-MCA machines.  Some hard disks
;   are not comptaible with the feature and so this switch was put in to
;   allow it to be turned off.
;
VHD_Win_Ini_Key_String LABEL BYTE
	db	"VirtualHDIRQ", 0
EndMsg

VxD_IDATA_ENDS

	END
