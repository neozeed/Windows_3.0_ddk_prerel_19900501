;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989-1990
;
;   Title:	VHD.INC - Virtual Hard Disk Device Services
;
;   Version:	3.00
;
;   Date:	26-Apr-1989
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   26-Apr-1989 RAL Original
;
;==============================================================================


Begin_Service_Table VHD

VHD_Service	VHD_Get_Version, LOCAL
VHD_Service	VHD_Allocate_Handle, LOCAL
VHD_Service	VHD_Read, LOCAL
VHD_Service	VHD_Write, LOCAL
VHD_Service	VHD_Get_Status, LOCAL

End_Service_Table VHD


VHD_Stat_Busy		EQU	00000001b
VHD_Stat_Busy_Bit	EQU	0
VHD_Stat_In_Progress	EQU	00000010b
VHD_Stat_In_Progress_Bit EQU	1
