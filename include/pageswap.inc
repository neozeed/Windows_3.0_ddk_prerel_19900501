;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988-1990
;
;   Title:	PageSwap.Inc - Demand Paging Swap Device Services
;
;   Version:	1.00
;
;   Date:	18-Oct-1988
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   18-Oct-1988 RAL Original
;
;==============================================================================


Begin_Service_Table PageSwap

PageSwap_Service PageSwap_Get_Version, LOCAL
PageSwap_Service PageSwap_Test_Create, LOCAL
PageSwap_Service PageSwap_Create, LOCAL
PageSwap_Service PageSwap_Destroy, LOCAL
PageSwap_Service PageSwap_In, LOCAL
PageSwap_Service PageSwap_Out, LOCAL
PageSwap_Service PageSwap_Test_IO_Valid, LOCAL

End_Service_Table PageSwap

PS_Zero_Init_Mask	EQU	0001h
PS_Zero_Init_Bit	EQU	00h
PS_Fixed_Page_Mask	EQU	0008h
PS_Fixed_Page_Bit	EQU	03h
PS_First_Page_In_Mask	EQU	0020h
PS_First_Page_In_Bit	EQU	05h
PS_Dirty_Mask		EQU	0040h
PS_Dirty_Bit		EQU	06h
PS_Ever_Dirty_Mask	EQU	8000h
PS_Ever_Dirty_Bit	EQU	0Fh
