PAGE 58,132
;******************************************************************************
TITLE PSTEXT.ASM - Text entries for page swap device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989
;
;   Title:	PSTEXT.ASM - Text entries for page swap device
;
;   Version:	1.00
;
;   Date:	07-Feb-1989
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   07-Feb-1989 RAL Contains system initialization file entries
;
;==============================================================================


	.386p

	PUBLIC PS_Enable_Ini
	PUBLIC PS_Swap_Drive_Ini
	PUBLIC PS_Min_Free_Ini
	PUBLIC PS_Max_Size_Ini

	PUBLIC PS_Invalid_Part_Msg
	PUBLIC PS_Caption_Title_Msg

	INCLUDE VMM.Inc

BeginDoc
;******************************************************************************
;
;   The PageSwap VxD has three system initialization file entries.
;
;	Paging= On or Off to enable or disable demand paging
;		Default value: ON
;	PagingDrive= Drive letter to place paging file on. The paging file
;		     will be placed in the ROOT of this drive.
;		Default= same drive and directory as SYSTEM.INI is in
;		NOTE THAT YOU CANNOT CHANGE THE DIRECTORY, only the drive.
;	MinUserDiskSpace= # of K bytes to reserve for user files on swap disk
;		Default value: 500
;	MaxPagingFileSize= MAX # of K bytes for paging file
;		Default value: as large as allowed by MinUserDiskSpace
;
;==============================================================================
EndDoc


VxD_IDATA_SEG

PS_Enable_Ini	  db "Paging", 0
PS_Swap_Drive_Ini db "PagingDrive", 0
PS_Min_Free_Ini   db "MinUserDiskSpace", 0
PS_Max_Size_Ini   db "MaxPagingFileSize", 0


PS_Invalid_Part_Msg LABEL BYTE
db 'Your swap file is corrupt.  See Chapter 13, "Optimizing Windows," in the '
db "Microsoft Windows User's Guide for instructions about recreating the file.", 0

PS_Caption_Title_Msg db "Corrupt Swap File Warning", 0


VxD_IDATA_ENDS


	END
