PAGE 58,132
;******************************************************************************
TITLE VHD.ASM - Virtual Hard Disk Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp. 1986, 1987, 1988, 1989
;
;   Module:   VHD.ASM - Virtual Hard Disk Controller Driver
;
;   Version:  3.00
;
;   Date:     On the edge
;
;   Author:   Ralph Lipe
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   04-Jan-1988 RAL Converted from 2.1 Win386 interface
;   26-Jul-1988 RAL New System_Control interface
;   03-Aug-1988 RAL Uses new Win.Ini stuff
;   11-Oct-1988 RAL Removed silly timer int disable stuff
;   26-Apr-1989 RAL VHD now provides services
;   26-Jun-1989 RAP check for INT 13's for hard disk drives
;
;==============================================================================
	.386p


;******************************************************************************
;
;   This Device hooks INT 13's and checks for the high bit to be set in the
;   drive specification (DL).  If the bit is set, then Begin_Critical_Section
;   is called and timer port trapping is disabled.  When the bios INT 13 handler
;   IRET's, then timer port trapping is re-enabled and End_Critical_Section
;   is called.	INT 13's that don't have the high bit of DL set are simply
;   reflected so that VFD can handle them.
;
;******************************************************************************



;******************************************************************************
;			L O C A L   C O N S T A N T S
;******************************************************************************

	.XLIST
	INCLUDE VMM.Inc
	INCLUDE VPICD.INC
	INCLUDE VTD.Inc
	INCLUDE Debug.Inc
	INCLUDE SYSINFO.INC
	INCLUDE VDMAD.INC
	INCLUDE SmartDrv.Inc
	.LIST

	Create_VHD_Service_Table EQU True

	INCLUDE VHD.Inc


;******************************************************************************
;		V I R T U A L	D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VHD, 3, 0, VHD_Control, VHD_Device_ID, VHD_Init_Order


;******************************************************************************
;		     E X T E R N A L   R E F E R E N C E S
;******************************************************************************


VxD_IDATA_SEG
VHD_IRQ_Desc VPICD_IRQ_Descriptor <0Eh,,OFFSET32 VHD_INT>

EXTRN VHD_Win_Ini_Key_String:BYTE

VxD_IDATA_ENDS


VxD_LOCKED_DATA_SEG

Dest_Ptr    dd	    ?

VHD_IRQ_Handle	dd  ?

Sector_Count dw     0

VHD_Orig_V86_Int_15 dd	0

VxD_LOCKED_DATA_ENDS


VxD_ICODE_SEG


;******************************************************************************
;
;   VHD_V86_Int_15_Hook
;
;   DESCRIPTION:
;	This code will be copied to V86 mode during Sys_Critical_Init to
;	disable the Int 15h disk multi-tasking hooks.  This is necessary
;	because Compaq BIOSs do pushf/calls to simulate the Int 15h's.
;	It is important to catch all multi-tasking signals so that software
;	that uses them will never see a begin without an end (or an end
;	without a begin).
;
;   ENTRY:
;	In V86 mode from Int 15h.
;
;   EXIT:
;	If AH=90h or AH=91h then interrupt is eaten and appropriate parameters
;	are returned, otherwise chain to next Int 15h handler.
;
;   USES:
;
;==============================================================================

VHD_V86_Int_15_Hook LABEL NEAR
	cmp	ah, 90h
	jb	SHORT VI15_Chain
	je	SHORT VI15_Fix_Flags
	cmp	ah, 91h
	ja	SHORT VI15_Chain
	xor	ah, ah
	iretd
VI15_Fix_Flags:
	xor	ah, ah
	clc
	sti
	retf 2

VI15_Chain:
	db	0EAh				; Far JMP instruction
VI15_Chain_Addr dd  ?				; This will be patched


VHD_V86_Hook_Size EQU $-VHD_V86_Int_15_Hook





;******************************************************************************
;
;   VHD_Sys_Critical_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = System VM Handle
;	EDX = Reference data from real mode
;		Zero if double buffering disabled
;		Non-Zero if double buffering enabled
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Sys_Critical_Init

	mov	eax, 13h
	mov	esi, OFFSET32 VHD_Int_13
	VMMcall Hook_V86_Int_Chain

;
;   Depending on the machine type / disk type we may want to request a 64K
;   DMA buffer.  Note that if SmartDrv is double buffering I/O then we never
;   need to request a large DMA buffer.
;
	push	edx
	VMMcall Get_Machine_Info
	pop	edx
	test	edx, edx
	jnz	SHORT VHD_Init_Test_VHIRQ

	jecxz	SHORT VHD_Init_Test_VHIRQ
	mov	al, BYTE PTR [ecx.SD_feature1]
	test	al, SF1_FD_uses_DMA3 + SF1_MicroChnPresent
						; Q: DMA disk or Micro Channel?
	jz	SHORT VHD_Init_Test_VHIRQ	;    N: Test for IRQ Virt
						; request a 64Kb buffer
	mov	eax, 16
	xor	ecx, ecx
	VxDCall VDMAD_Reserve_Buffer_Space
	jmp	VHD_Init_Exit

;
;   Check for an AT model byte.
;
VHD_Init_Test_VHIRQ:
	cmp	bl, 0Fch
	jne	VHD_Init_Off

	xor	esi, esi
	mov	edi, OFFSET32 VHD_Win_Ini_Key_String
	mov	eax, True			; Default = ON
	VMMcall Get_Profile_Boolean		; Get Win.Ini entry (if any)
	test	eax, eax			; Q: VHIRQ ON?
	jz	DEBFAR VHD_Init_Exit		;    N: Don't vitrualize IRQ
						;    Y: OK -- Continue



;
;   Turn VHIRQD on.  Note that we do not need to store the IRQ handle anywhere
;   since it is not needed.
;
	mov	edi, OFFSET32 VHD_IRQ_Desc	; EDI -> IRQ Descriptor
	VxDCall VPICD_Virtualize_IRQ		; Get ready for ints
IFDEF DEBUG
	jnc	SHORT VHD_Init_OK
	Debug_Out "ERROR: VHD could not virtualize IRQ"
	jmp	SHORT VHD_Init_Abort
VHD_Init_OK:
ELSE
	jc	DEBFAR VHD_Init_Abort
ENDIF

;
;   Hook Int 15h to intercept and EAT the disk multi-tasking interrupts.  Note
;   that we hook Int 15h in V86 mode also since the Compaq BIOS does PUSHF/CALLs
;   for some Int 15h's.
;
	mov	eax, 15h			; Software interrupt #
	mov	esi, OFFSET32 VHD_Int15 	; Procedure
	VMMcall Hook_V86_Int_Chain		; Hook the interrupt

	VMMcall Get_V86_Int_Vector
	shl	ecx, 16
	mov	cx, dx
	mov	[VI15_Chain_Addr], ecx
	mov	[VHD_Orig_V86_Int_15], ecx

	VMMcall _Allocate_Global_V86_Data_Area, <VHD_V86_Hook_Size, 0>
	test	eax, eax
	jz	SHORT VHD_Init_Abort

	mov	edi, eax
	shl	eax, 12
	shr	ax, 12
	movzx	edx, ax
	shr	eax, 16
	mov	ecx, eax
	mov	eax, 15h
	VMMcall Set_V86_Int_Vector

	mov	esi, OFFSET32 VHD_V86_Int_15_Hook
	cld
	mov	ecx, VHD_V86_Hook_Size
	rep movsb


VHD_Init_Off:
VHD_Init_Exit:
VHD_Init_Abort:
	clc
	ret

EndProc VHD_Sys_Critical_Init


;******************************************************************************
;
;   VHD_Device_Init
;
;   DESCRIPTION:
;	Do IOCTL to smart drive to get correct interrupt vector.
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Device_Init

	clc
	ret

EndProc VHD_Device_Init


VxD_ICODE_ENDS


VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VHD_Control
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Control

	Control_Dispatch Sys_Critical_Init, VHD_Sys_Critical_Init
	Control_Dispatch Device_Init, VHD_Device_init
	Control_Dispatch Sys_Critical_Exit, <SHORT VHD_Sys_Critical_Exit>
	clc
	ret

EndProc VHD_Control


;******************************************************************************
;
;   VHD_Sys_Critical_Exit
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Sys_Critical_Exit

	mov	ecx, [VHD_Orig_V86_Int_15]
	jecxz	SHORT VHD_SCE_Exit
	movzx	edx, cx
	shr	ecx, 16
	mov	eax, 15h
	VMMcall Set_V86_Int_Vector
VHD_SCE_Exit:
	clc
	ret

EndProc VHD_Sys_Critical_Exit


;******************************************************************************
;			     S E R V I C E S
;******************************************************************************


BeginDoc
;******************************************************************************
;
;   VHD_Get_Version
;
;   DESCRIPTION:
;
;   ENTRY:
;	None
;
;   EXIT:
;	AH = Major version number, AL = Minor version number
;	CL = Number of drives supported (0 based)
;	EDX = Flags
;	      Bit 0 = 1 if supports direct to hardware read/write
;	Carry flag clear
;
;   USES:
;	EAX, Flags
;
;==============================================================================
EndDoc

BeginProc VHD_Get_Version, Service

	mov	eax, 300h
	xor	ecx, ecx
	clc
	ret

EndProc VHD_Get_Version


;******************************************************************************
;
;   VHD_Get_Drive_Info
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Get_Drive_Info, Service

EndProc VHD_Get_Drive_Info


;******************************************************************************
;
;   VHD_Allocate_Handle
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Allocate_Handle, Service

	Debug_Out "STRANGE!  Someone called a VHD service!"
	Fatal_Error

EndProc VHD_Allocate_Handle


;******************************************************************************
;
;   VHD_Read
;
;   DESCRIPTION:
;
;   ENTRY:
;	EAX = VHD handle
;	EBX = Starting sector
;	 CL = Sector count (0 = 256 sectors)
;	 CH = Drive number
;	EDX = Flags
;	      RWF_Buffer = Use disk cache for read
;	ESI = Address of buffer (must be locked)
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Read, Service
	Debug_Out "STRANGE!  Someone called a VHD service!"
	Fatal_Error

EndProc VHD_Read



;******************************************************************************
;
;   VHD_Write
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Write, Service
	Debug_Out "STRANGE!  Someone called a VHD service!"
	Fatal_Error

EndProc VHD_Write


;******************************************************************************
;
;   VHD_Get_Status
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Get_Status, Service
	Debug_Out "STRANGE!  Someone called a VHD service!"
	Fatal_Error

EndProc VHD_Get_Status



;******************************************************************************
;
;   VHD_INT
;
;   DESCRIPTION:
;	This code will only be executed if Virtual HD IRQ is on.
;
;   ENTRY:
;	EAX = IRQ Handle
;	EBX = Current VM handle
;
;   EXIT:
;
;------------------------------------------------------------------------------

BeginProc VHD_INT, High_Freq

	VxDcall VPICD_Phys_EOI
	mov	esi, OFFSET32 VHD_Event
	VMMjmp	Call_Global_Event

EndProc VHD_INT

BeginProc VHD_Event, High_Freq

	mov	BYTE PTR ds:[48Eh], 0FFh ; Set Int occurance
	test	[ebx.CB_VM_Status], VMStat_Idle
	jz	SHORT VHD_E_Exit
	VMMjmp	Wake_Up_VM
VHD_E_Exit:
	ret

EndProc VHD_Event


;******************************************************************************
;
;   VHD_Int15
;
;   DESCRIPTION:
;   In order not to blow disk revs and slow the had disk down by up to 7x
;   we short-circut the hard disk muti-tasking hooks.  Note that this will
;   break any application that is trying to use these hooks.
;
;   This int hook will only be installed if Virtual HD IRQ is on.
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Int15

	mov	ah, [ebp.Client_AH]		; Get client's AH
	cmp	ah, 90h
	jb	SHORT SI15_Not_Disk_Mult_Task
	je	SHORT SI15_Fix_Flags
	cmp	ah, 91h
	ja	SHORT SI15_Not_Disk_Mult_Task
	mov	[ebp.Client_AH], 0		; Does this in the ROM
	clc					; DON'T chain to next handler
	ret

SI15_Fix_Flags:
	and	[ebp.Client_Flags], NOT CF_Mask ; Clear client carry flag
	VMMcall Enable_VM_Ints			; Return with ints enabled
	mov	[ebp.Client_AH], 0		; Does this in the ROM
	clc					; DON'T chain to next handler
	ret

SI15_Not_Disk_Mult_Task:
	stc					; Chain to next int handler
	ret

EndProc VHD_Int15


;******************************************************************************
;
;   VHD_Int_13
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VHD_Int_13, High_Freq

	test	[ebp.Client_DL], 80h		;Q: high bit set?
	jz	short INT13_exit		;   Y: not for hard drive!

	mov	ecx, (Block_Svc_Ints OR Block_Enable_Ints)
	VMMCall Begin_Critical_Section
	VxDCall VTD_Disable_Trapping
	xor	eax, eax
	mov	esi, OFFSET32 VHD_I13_Iret
	VMMCall Call_When_VM_Returns

INT13_exit:
	stc					; Reflect to next handler
	ret

EndProc VHD_Int_13


;------------------------------------------------------------------------------


BeginProc VHD_I13_Iret, High_Freq

	VxDcall VTD_Enable_Trapping
	VMMjmp	End_Critical_Section

EndProc VHD_I13_Iret

VxD_LOCKED_CODE_ENDS




;******************************************************************************
;		  R E A L   M O D E   I N I T	C O D E
;******************************************************************************

;******************************************************************************
;
;   VHD_Real_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

VxD_REAL_INIT_SEG

Smart_Drv_Name db SD_DEV_NAME
Smart_Drv_Read_Buff db SIZE SD_IOCTL_Read dup (?)

BeginProc VHD_Real_Init

;
;   If another netbios is loaded then don't load -- Just abort our load
;
	test	bx, Duplicate_From_INT2F OR Duplicate_Device_ID
	jnz	SHORT VHD_RI_Abort_Load


;
;   No other VHD is loaded.  Get SmartDrv info about double buffering.
;
	mov	ax, 3D00h		    ; Open file DOS function
	mov	dx, OFFSET Smart_Drv_Name
	int	21h
	jc	SHORT VHD_RI_No_SmartDrv

	mov	bx, ax			    ; BX = File handle
	mov	ax, 4400h		    ; Get Device Data IOCTL
	int	21h
	jc	SHORT VHD_RI_Failed
	test	dx, 80h 		    ; Q: Is it a device?
	jz	SHORT VHD_RI_Failed	     ;	  N: Not Smart Drive
					    ;	 Y: Read control strings
	mov	dx, OFFSET Smart_Drv_Read_Buff
	mov	cx, SIZE SD_IOCTL_Read
	mov	ax, 4402h
	int	21h
	jc	SHORT VHD_RI_Failed

	mov	ah, 3Eh
	int	21h
	xor	edx, edx		    ; Assume no double buffering
	cmp	[Smart_Drv_Read_Buff.SD_IR_Minor_Ver], 3
	jb	SHORT VHD_RI_Exit	    ; Must be ver 3 or above to dbl buff
	mov	dl, [Smart_Drv_Read_Buff.SD_IR_Double_Buffer]
	jmp	SHORT VHD_RI_Exit

VHD_RI_Failed:
	mov	ah, 3Eh 		    ; Close file handle
	int	21h			    ; And abort operation

VHD_RI_No_SmartDrv:
	xor	edx, edx

VHD_RI_Exit:
	xor	bx, bx
	xor	si, si
	mov	ax, Device_Load_Ok
	ret

VHD_RI_Abort_Load:
	xor	bx, bx
	xor	si, si
	mov	ax, Abort_Device_Load + No_Fail_Message
	ret

EndProc VHD_Real_Init

VxD_REAL_INIT_ENDS




	END VHD_Real_Init
