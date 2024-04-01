	TITLE	VDD - Virtual Display Device for EGA/VGA  vers 3.0a  2/89
;******************************************************************************
;
;VDDMSG.ASM	Messages for VDD
;
;   Author: MDW
;
;   (C) Copyright MICROSOFT Corp. 1986, 1987, 1988, 1989
;
;   February, 1989
;
;DESCRIPTION:
;
;******************************************************************************

	.386p

	INCLUDE VMM.INC

VxD_IDATA_SEG

BeginMsg
;***************
; Load time error messages
;
; The following is a message that results from incorrect installation of
;   device drivers. Another device driver is conflicting with the VDD.
PUBLIC VDD_Str_CheckVidPgs
VDD_Str_CheckVidPgs DB	"Video pages reserved by another device",0

; The following is a message that results from incorrect initialization of
;   of the VDD, probably due to insufficient memory, incorrect display
;   adapter or bad files. User should verify that enough system memory
;   is available and that SETUP was completed properly.
PUBLIC VDD_Str_BadDevice
VDD_Str_BadDevice   DB	"Video initialization failed",0
EndMsg

;******************************************************************************
;
; Win.Ini entry for window update time.
;
PUBLIC VDD_Time_Ini
VDD_Time_Ini db "WindowUpdateTime", 0

; Win.Ini entry for CGA no snow option.
;
PUBLIC VDD_NoSnow_Ini
VDD_NoSnow_Ini db "CGANoSnow", 0

VxD_IDATA_ENDS


VxD_DATA_SEG

BeginMsg
;***************
; Video Message Box (VMB) definitions: Message text followed by Msg Box type
;	The caption of the message box is always the VM name. Note that
;	the messages that indicate that the application cannot run in the
;	background may be transitory. Many applications will change their
;	screen mode in the course of program execution.
;
;
;	The "Check the PIF" message is appended to each of the messages.
;	By using the PIF files correctly(see PIFEDITOR documentation) these
;	messages can be avoided in almost all cases.

; The following is a message that appears when the virtual display device
;   attempts to save the state of the video adapter and runs out of memory
;   The display for the app is probably corrupted. The user should free up
;   some memory by closing another application or getting another application
;   to free up some of its data and then get the application to redraw its
;   display. In some cases, the user will need to exit the application and
;   start it up again in order to redraw the display.
PUBLIC VMB_Str_NoMainMem
VMB_Str_NoMainMem   DB	"This application does not have enough memory for its "
		    DB	"display. Some of its display may have been lost. "
		    DB	"You may have to restart it again to fix it. "
		    DB	"Check the PIF settings to make sure they are correct.",0

; The following is a message that appears when the virtual display device
;   runs out of memory that it uses to update the display in a window. If the
;   user wants to continue to run the application in a window, they should
;   free up some memory by closing an application or get another application
;   to release some of its working data.
PUBLIC VMB_Str_NoCopyMem
VMB_Str_NoCopyMem   DB	"This application does not have enough memory for its "
		    DB	"display. This may cause it to be updated incorrectly. "
		    DB	"Exiting some other applications should fix the "
		    DB	"problem. "
		    DB	"Check the PIF settings to make sure they are correct.",0

VxD_DATA_ENDS

	END
