	TITLE	VDD - Virtual Display Device for CGA   version 2.30  1/88
;******************************************************************************
;
;CGASVC - Virtual Display Device Services
;
;   Author: MDW
;
;   (C) Copyright MICROSOFT Corp. 1986, 1987, 1988, 1989
;
;   January, 1988
;
;DESCRIPTION:
;
;COMPILE:
;..\..\tools\masm5 -t -Mx -DEGA -DDEBUG -I..\..\INCLUDE -p vddctl.asm;
;
;******************************************************************************

	.386p

	INCLUDE VMM.INC
	INCLUDE VDD.INC
	INCLUDE CGA.INC
        INCLUDE SHELL.INC
	INCLUDE DEBUG.INC

;******************************************************************************
; EXTRN routines
;
VxD_CODE_SEG
	EXTRN	VDD_RestMsgStt:NEAR
	EXTRN	VDD_State_CtlrChgs:NEAR
	EXTRN	VDD_State_MemChgs:NEAR
	EXTRN	VDD_State_GetStt:NEAR
	EXTRN	VDD_State_Update:NEAR
	EXTRN	VDD_Mem_Chg:NEAR
	EXTRN	VDD_Mem_Virtual:NEAR
	EXTRN	VDD_Mem_Update:NEAR
	EXTRN	VDD_Mem_Physical:NEAR
	EXTRN	VDD_Mem_VMSetTyp:NEAR
	EXTRN	VDD_Grab:NEAR
	EXTRN	VDD_Detach:NEAR
	EXTRN	VDD_Restore:NEAR
	EXTRN	VDD_Cancel_Restore:NEAR
	EXTRN	VDD_Mem_DCopy:NEAR
        EXTRN   VDD_VMDA_Update:NEAR
	EXTRN	VDD_IO_SetTrap:NEAR
	EXTRN	VDD_Clr_VM_Time_Out:NEAR
	EXTRN	VDD_Set_VM_Time_Out:NEAR
VxD_CODE_ENDS

;******************************************************************************
; EXTRN data
;
VxD_IDATA_SEG
	EXTRN VDD_Str_CheckVidPgs:BYTE
	EXTRN VDD_Str_BadDevice:BYTE
	EXTRN VDD_Time_Ini:BYTE
	EXTRN VDD_NoSnow_Ini:BYTE
VxD_IDATA_ENDS

VxD_DATA_SEG
	EXTRN	vgVDD:BYTE
	EXTRN	VDD_CB_Off:DWORD
	EXTRN	VDD_Focus_VM:DWORD
	EXTRN	VDD_PhysB8000:DWORD

	EXTRN VMB_Str_NoMainMem:BYTE
	EXTRN VMB_Str_NoCopyMem:BYTE

; Message mode processing data
PUBLIC VDD_Msg_VM
VDD_Msg_VM	DD  0				; Msg VM handle
VDD_Msg_Attr	DB  0				; Msg text mode attribute
VDD_Scrn_Attr	DB  0				; Screen clear attribute

; Save for instance data during System Message processing
VDD_Msg_SaveBuf DB  CGA_I_Len1 DUP (?)
		DB  3 -(CGA_I_Len1 MOD 4) ; Padding


VxD_DATA_ENDS

VxD_CODE_SEG

BeginDoc
;******************************************************************************
;VDD_Get_Version
;
;DESCRIPTION:
;	Returns version number and device ID.
;
;ENTRY: none
;
;EXIT:	ESI = ptr to 8 byte ID string
;	AH = major version
;	AL = minor version
;
;USES:	flags, AX, ESI
;
;==============================================================================
EndDoc

BeginProc VDD_Get_Version, SERVICE
BeginProc VDD_Get_GrbVersion
.ERRE	VDD_VerNum EQ Grabber_VerNum

	mov	ax,VDD_VerNum			; Pass version number to VMDOSAPP
	mov	esi,OFFSET32 TYPESTRNG1         ; Assume CGA
	cmp	[vgVDD.Vid_Type],Vid_Type_CGA	; Q: CGA?
        je      SHORT VV_Exit                   ;  Y: Done
	mov	esi,OFFSET32 TYPESTRNG2         ;  N: return IDC string
VV_Exit:
	ret

TYPESTRNG1    DB 'VIDEOCGA'
TYPESTRNG2    DB 'VIDEOIDC'

EndProc VDD_Get_GrbVersion
EndProc VDD_Get_Version


;******************************************************************************
;
; VDD_Begin_Msg_Mode
;
;DESCRIPTION:
;	This routine is called when a Begin_Message_Mode device control is
;	received. It is followed by a call to the VDD_Msg_Init routine, which
;	initializes (or sets up an event to initialize) the screen. The
;	Init call is followed by calls to set the background and forground
;	color and to output text. When the display is finished, an
;	End_Message_Mode device control is made torestore the focus VM's
;	display.
;
;ENTRY: none
;
;EXIT:	none
;
;USES:	flags, EAX, EDX
;
;==============================================================================
EndDoc
BeginProc VDD_Begin_Msg_Mode,PUBLIC

IFDEF	DEBUG
	test	[vgVDD.Vid_Flags],fVid_Msg	; Q: Message mode?
	jz	SHORT VSMM_D00
Debug_Out "VDD:Begin_Message_Mode when already in message mode"
VSMM_D00:
ENDIF
	or	[vgVDD.Vid_Flags],fVid_Msg	; Message mode
;	clc
	ret
EndProc VDD_Begin_Msg_Mode

;******************************************************************************
;
; VDD_End_Msg_Mode
;
;DESCRIPTION:
;	This routine is called after a sysmodal message box is complete. It
;	restore's the VM's video state.
;
;ENTRY: EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================

BeginProc VDD_End_Msg_Mode, PUBLIC

	test	[vgVDD.Vid_Flags],fVid_Msg
IFDEF	DEBUG
	jnz	SHORT VEMM_D00
Debug_Out "VDD:End_Msg_Mode not in Msg mode"
VEMM_D00:
ENDIF
	and	[vgVDD.Vid_Flags],NOT fVid_Msg+fVid_MsgA
	pushad
        
; Restore video memory and controller state for Focus VM
        push    ebx
	mov	ebx,[VDD_Focus_VM]
	mov	edi,ebx
	add	edi,[VDD_CB_Off]		; EDI = VM's VDD CB ptr
	call	VDD_Restore			; Restore focus VM to video
        pop     ebx

; Restore trapping for Msg Mode VM
        mov     edi,ebx
	add	edi,[VDD_CB_Off]		; EDI = VM's VDD CB ptr
	and	[vgVDD.Vid_Flags],NOT fVid_MsgI ; Clear message mode Init
	call	VDD_IO_SetTrap			; Restore I/O traps
	popad
	clc
	ret
EndProc VDD_End_Msg_Mode


BeginDoc
;******************************************************************************
;
; VDD_Msg_ClrScrn
;
;DESCRIPTION:
;	This routine is called to initialize the screen for putting up
;	messages.  It is a service called by the shell.  If the focus VM is
;	the current VM it will clear the screen immediately.  Otherwise,
;	the screen will be initialized when the focus changes.	A
;	Begin_Message_Mode device control must be issued before this
;	service is utilized.
;
;ENTRY: EBX = VM handle
;	EAX = background attribute
;
;EXIT:	EAX = Width in columns
;	EDX = Height in rows
;
;USES:	flags, EAX, EDX
;
;==============================================================================
EndDoc

BeginProc VDD_Msg_ClrScrn, SERVICE

	pushad
IFDEF	DEBUG
	test	[vgVDD.Vid_Flags],fVid_Msg	; Special Init for Message mode
	jnz	SHORT VMI_D00
Debug_Out "VDD:Scrn_Init not in SYSMSG mode"
VMI_D00:
ENDIF
	and	al,07h
	shl	eax,4
	or	al,7
	mov	[VDD_Msg_Attr],al
	mov	[VDD_Scrn_Attr],al
	cmp	ebx,[VDD_Focus_VM]
	jnz	SHORT VMI_Exit
	call	VDD_Detach			; Detach everything
	mov	edi,ebx
	add	edi,[VDD_CB_Off]		; EDI = VM's VDD CB ptr
	call	VDD_Cancel_Restore		; Make sure no restore pending
	call	VDD_Msg_InitScreen		; Initialize the screen
VMI_Exit:
	popad
	mov	eax,VDD_Msg_ScrnWid
	mov	edx,VDD_Msg_ScrnHgt
	ret
EndProc VDD_Msg_ClrScrn

;******************************************************************************
; VDD_Msg_InitScreen
;
;DESCRIPTION:
;	This routine is called to initialize the screen for putting up
;	messages.  The VM must be the focus VM.  If the VM is not the current
;	VM, an event is queued to initialize the screen.
;
;ENTRY: EBX = VM handle
;	EDI = VDD CB ptr
;
;EXIT:	CF = 0
;
;USES:	flags, EAX, ECX, EDX, ESI, EDI
;
;==============================================================================
BeginProc VDD_Msg_InitScreen

IFDEF	DEBUG
	cmp	ebx, [VDD_Focus_VM]
	jz	SHORT VMIS_D00
Debug_Out "VDD:Msg Screen clear not focus VM"
VMIS_D00:
ENDIF
	test	[vgVDD.Vid_Flags],fVid_MsgA	; Q: Already init screen?
	jnz	SHORT VMIS_Exit 		;   Y: All done
	mov	edx,edi
	jmp	SHORT VDD_Msg_InitScrn_Event
VMIS_Exit:
	clc
	ret


VDD_Msg_InitScrn_Event:

	pushad
	mov	edi,edx
Assert_VDD_ptrs ebx,edi
IFDEF	DEBUG
	cmp	ebx, [VDD_Focus_VM]
	jz	SHORT VMISE_D01
Debug_Out "VDD:Msg Screen clear event not focus VM"
VMISE_D01:
ENDIF
        cld
	or	[vgVDD.Vid_Flags],fVid_MsgI+fVid_MsgA ; Set message mode Init
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov	[VDD_Msg_VM],ebx		; This is the message VM
        test    [vgVDD.Vid_Flags],fVid_MsgSttSaved    ; Q: saved stt exists?
        jnz     VMISE_HaveSavedStt                    ;  Y: 
; Save instanced video BIOS data
	push	edi
	mov	eax,[EBX.CB_High_Linear]
	mov	edi,OFFSET32 VDD_Msg_SaveBuf
	mov	esi,CGA_I_Addr1
	add	esi,eax
	mov	ecx,CGA_I_Len1
	rep movsb
	pop	edi

	call	VDD_IO_SetTrap			; Remove I/O traps
	call	VDD_Mem_Physical		; Map the video adaptor
	mov	ebp, [ebx.CB_Client_Pointer]	; EBP -> Client frame

; Set video adapter into MODE 3
	Push_Client_State
	VMMcall Begin_Nest_Exec 		; Get ready for software ints

	mov	[ebp.Client_AX], 0003h
	mov	eax,10h
	VMMcall Exec_Int			; Set mode 3

	VMMcall End_Nest_Exec			; All done with software ints
	Pop_Client_State

; Restore VM state - but now runs detached
	and	[vgVDD.Vid_Flags],NOT fVid_MsgI ; Clear message mode Init
	call	VDD_IO_SetTrap			; Restore I/O traps
	call	VDD_Mem_Virtual
	mov	eax,[EBX.CB_High_Linear]
	mov	esi,OFFSET32 VDD_Msg_SaveBuf
	mov	edi,CGA_I_Addr1
	add	edi,eax
	mov	ecx,CGA_I_Len1
	rep movsb

; Initialize memory
	mov	edi,[VDD_PhysB8000]
	mov	ah,[VDD_Scrn_Attr]
	mov	al,' '
	mov	ecx,25*80
	rep stosw
        jmp     SHORT VMISE_Exit
VMISE_HaveSavedStt:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        call    VDD_RestMsgStt

VMISE_Exit:
	popad
	clc
	ret
EndProc VDD_Msg_InitScreen


BeginDoc
;******************************************************************************
;
; VDD_Msg_ForColor
;
;DESCRIPTION:
;	After calling Msg_Init, this service sets up the forground attribute.
;
;ENTRY: EAX = Color (for EGA/VGA driver, a text mode attribute)
;	EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Msg_ForColor, SERVICE
	push	eax
	and	al,0Fh
	mov	ah,[VDD_Msg_Attr]
	and	ah,0F0h
	or	al,ah
	mov	[VDD_Msg_Attr],al
	pop	eax
	ret
EndProc VDD_Msg_ForColor

BeginDoc
;******************************************************************************
;
; VDD_Msg_BakColor
;
;DESCRIPTION:
;	After calling Msg_Init, this service sets up the background attribute.
;
;ENTRY: EAX = Color (for EGA/VGA driver, a text mode attribute)
;	EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Msg_BakColor, SERVICE
	push	eax
	and	al,07h
	shl	eax,4
	mov	ah,[VDD_Msg_Attr]
	and	ah,0Fh
	or	al,ah
	mov	[VDD_Msg_Attr],al
	pop	eax
	ret
EndProc VDD_Msg_BakColor


BeginDoc
;******************************************************************************
;
; VDD_Msg_TextOut
;
;DESCRIPTION:
;	After calling Msg_Init and setting up the forground and background
;	colors, this service puts character on the screen.
;
;ENTRY: ESI = address of string
;	ECX = length of string
;	EAX = Row start
;	EDX = Column start
;	EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Msg_TextOut, SERVICE

	jecxz	SHORT VMTO_Exit

	pushad
	push	edx
	mov	edx,160
	mul	dl
	pop	edx
	add	eax,edx
	add	eax,edx
	mov	edi,[VDD_PhysB8000]
	add	edi,eax
	mov	ah,[VDD_Msg_Attr]
        cld
VMTO_00:
	lodsb
	stosw
	loopd	VMTO_00
	popad

VMTO_Exit:
	ret
EndProc VDD_Msg_TextOut


BeginDoc
;******************************************************************************
;
; VDD_Msg_SetCursPos
;
;DESCRIPTION:
;	After calling Msg_Init this routine sets the cursor position.
;
;ENTRY: EAX = Row
;	EDX = Column
;	EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Msg_SetCursPos, SERVICE

	push	eax
	push	edx
	cmp	eax,VDD_Msg_ScrnWid
	jae	SHORT VMSCP_Off
	push	edx
	mov	edx,80
	mul	dl
	pop	edx
	add	eax,edx
VMSCP_00:
	mov	edx,3d4h
	push	eax
	mov	al,0Fh
	out	dx,al
	pop	eax
	IO_Delay
	inc	edx
	out	dx,al
	dec	edx
	mov	al,0Eh
	out	dx,al
	IO_Delay
	inc	edx
	mov	al,ah
	out	dx,al
	pop	edx
	pop	eax
	ret
VMSCP_Off:
	mov	eax,-1
	jmp	SHORT VMSCP_00
EndProc VDD_Msg_SetCursPos



BeginDoc
;******************************************************************************
;
;   VDD_PIF_State
;
;DESCRIPTION:
;	Informs VDD about PIF bits for VM just created.
;
;ENTRY: EBX = VM handle
;	AX = PIF bits
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc   VDD_PIF_State, SERVICE
	push	edi
	mov	edi,ebx
	add	edi,[VDD_CB_Off]
	mov	[edi.VDD_PIF],ax
	call	VDD_IO_SetTrap
	pop	edi
	ret
EndProc     VDD_PIF_State

BeginDoc
;******************************************************************************
;
;   VDD_Set_HCurTrk
;
;DESCRIPTION:
;	Sets flag passed to VMDOSAPP indicating that VMDOSAPP should
;	maintain cursor position within display window for this application.
;	This is called by the Keyboard driver when a keyboard interrupt
;	is simulated into a VM.
;
;ENTRY: EBX = VM handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc   VDD_Set_HCurTrk,SERVICE
	push	ebx
	add	ebx,[VDD_CB_Off]
	or	[ebx.VDD_Flags],fVDD_HCurTrk
	pop	ebx
	ret
EndProc     VDD_Set_HCurTrk

BeginDoc
;******************************************************************************
;
;   VDD_Get_GrabRtn
;
;DESCRIPTION:
;	Returns address of video grab routine. The grab routine is called
;	by the SHELL device when the appropriate hot key is pressed by the
;	user. It makes a copy of the visible screen and controller state
;	of the current VM. That copy is then accessible via VDD_Get_GrbState
;	and VDD_Get_GrbMem services.
;
;ENTRY: none
;
;EXIT:	ESI = address of grab routine
;
;USES:	flags, ESI
;
;==============================================================================
EndDoc

BeginProc   VDD_Get_GrabRtn, SERVICE
	mov	esi,OFFSET32 VDD_Grab
	ret
EndProc     VDD_Get_GrabRtn

BeginDoc
;******************************************************************************
;VDD_Hide_Cursor    Hide/Show cursor in a window
;
;DESCRIPTION:
;	If EAX is non zero, sets a hide cursor flag else clears the flag.
;	This is so that, if the mouse is using a hardware cursor, it can
;	turn off that cursor while the VM is windowed(since the VM will
;	no longer own the mouse).
;
;ENTRY: EAX = 0 if cursor SHOULD be displayed in a window
;	   != 0 if cursor SHOULD NOT be displayed in a window
;	EBX = control block pointer
;
;EXIT:	none
;
;USES:	Flags
;
;==============================================================================
EndDoc
BeginProc VDD_Hide_Cursor, SERVICE

	push	edi
	mov	edi,ebx
	add	edi,[VDD_CB_Off]
Assert_VDD_ptrs ebx,edi
	and	[edi.VDD_Flags],NOT fVDD_Hide   ; assume show cursor
	or	eax,eax                         ; Q: Assumption correct?
	jz	SHORT VHC_Exit                  ;   Y:
	or	[edi.VDD_Flags],fVDD_Hide       ;   N: Hide the cursor
; Force cursor change on the next window update event
	mov     WORD PTR [edi.VDD_SttCopy.V_CAddrH], 0FFFFh 
VHC_Exit:
	pop	edi
	ret
EndProc VDD_Hide_Cursor

BeginDoc
;******************************************************************************
;VDD_Query_Access
;
;DESCRIPTION:
;	If the VM were to access its video memory now, would the VDD be
;	able to handle it?  Some devices, such as the Mouse device would
;	like to call software in the VM that will access the video.  However,
;	if this will cause a video display error, it would be better to
;	let the side affect occur rather than generate the error (in the
;	case of the mouse, the mouse cursor will be left on when the
;	VM is displayed in a window).  This call should only be done
;	just before returning to the VM in question (i.e. in a VM Event)
;	since subsequent setfocus calls may make the result incorrect.
;	For CGA, VMs can always access the video.
;
;ENTRY: EBX = control block pointer
;
;EXIT:	CF = 0 if video access is OK.
;
;USES:	Flags
;
;==============================================================================
EndDoc
BeginProc VDD_Query_Access, SERVICE

	clc
	ret
EndProc VDD_Query_Access

BeginDoc
;******************************************************************************
;Displaying VM's video memory in a window
;
;   Parameter passing
;	There are several services supplied in order to efficiently render
;	a VM's video memory into a window. These routines are called by the
;	grabber. Since the grabber runs in a virtual machine, parameters
;	are passed in the Client registers and in VM memory pointed to by
;	the Client registers. Other services available use protect mode
;	registers.
;
;   Updating windowed VMs
;	The first step is for the SHELL to call Set_VMState with a parameter
;	indicating that the VM is to be windowed. When the VM is no longer
;	windowed, Set_VMState is called again. When the VMState is not
;	windowed, the Get_Mod call will always return no changes and the
;	video update message will never be generated.
;
;	VMDOSAPP has to be assured that the call to get the video memory
;	is consistent with the call to get the video state; displaying a
;	mode 3 VM in mode 10 does not look good. To this end, the VM will
;	be made not to run between the calls Get_Mem and Free_Mem. This means
;	that the rest of the pertinent calls that the grabber makes:
;	Get_State, Get_Mod, Clear_Mod should all be bracketed by the
;	Get_Mem and Free_Mem calls in order to assure that the VM's state
;	will not change during the process of window updating.
;
;	Note that when a VM's video state changes, including controller state
;	changes such as cursor movement and memory modification, the VDD will
;	send VMDOSAPP a display update message. All the changes made to the
;	video state will accumulate and be reported by Get_Mod until a
;	Clear_Mod call is made. There will only be one display update
;	message per Clear_Mod call.
;==============================================================================
EndDoc

BeginDoc
;******************************************************************************
;
;   VDD_Set_VMType
;
;DESCRIPTION:
;	This service is used to inform the VDD of a VM's type. The parameter
;	explicitly passed is the Windowed flag. Implicitly passed are the
;	VM status flags, Exclusive and Background. This should be called
;	prior to running the VM and each time thereafter that any of the
;	VM paramaters are modified. Note that for a system critical Set_Focus
;	this routine may not be called before the Set_Focus. In that case,
;	the VDD is responsible for doing an implied Set_VMType(not windowed).
;
;ENTRY: EAX = state flag (= non zero if changing to windowed VM)
;	EBX = VM handle whose state is to change
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Set_VMType, SERVICE

; Set Windowed flag, set up VM for new type
	pushad
	mov	edi,ebx
	add	edi,[VDD_CB_Off]		; EDI = VM's VDD CB ptr
	and	[edi.VDD_Flags], NOT fVDD_Win	; Clear windowed flag
	or	eax,eax 			; Q: Windowed?
	jz	SHORT VST_00			;   N: Assumption correct
	or	[edi.VDD_Flags], fVDD_Win	;   Y: Set windowed flag
VST_00:
	call	VDD_Clr_VM_Time_Out		; Cancel previous timeout
	call	VDD_Mem_VMSetTyp		; Adjust memory
	test	[edi.VDD_Flags],fVDD_Win	; Q: Windowed?
	jnz	SHORT VST_Window

; Full screen VM
	cmp	ebx,[VDD_Focus_VM]		; Q: This VM have focus?
	jnz	SHORT VST_Ex			;   N: Don't restore
	bts	[edi.VDD_Flags],fVDD_RestBit	; Q: Restore pending?
	jc	SHORT VST_Ex			;   Y: All done
	mov	edx,edi
	mov	esi,OFFSET32 VDD_Restore	;   N: Restore before running
	VMMCall Call_VM_Event
VST_Ex:
	popad
	ret

; Windowed VM
VST_Window:
	call	VDD_Set_VM_Time_Out		; Set new timeout
    ;
    ; When windowed from FS VM, force cursor tracking in the window by 
    ; setting up an update event(which has the cursor change).
    ;
	or	[edi.VDD_Flags],fVDD_HCurTrk    ; Force Hor. cursor trk 
	mov     WORD PTR [edi.VDD_SttCopy.V_CAddrH], 0FFFFh ; Force cursor change
        call    VDD_VMDA_Update                 ; update event to track cursor
	popad
	ret

EndProc VDD_Set_VMType

BeginDoc
;******************************************************************************
;
;   VDD_Get_ModTime
;
;DESCRIPTION:
;	This routine is used to determine if any video activity has occured.
;	The poll device uses it to determine if the VM is idle.
;
;ENTRY: EBX = VM handle
;
;EXIT:	EAX = System Timer at last video modification
;
;USES:	Flags, EAX
;
;==============================================================================
EndDoc

BeginProc VDD_Get_ModTime, SERVICE
	push	ecx
	push	edx
	push	edi
	mov	edi,ebx
	add	edi,[VDD_CB_Off]
	test	[edi.VDD_Flags],fVDD_ModeSet	; Q: In middle of mode set?
	jz	SHORT VGMT_00			;   N: Check last mem access
	VMMCall Get_System_Time 		;   Y: Video busy right now
	jmp	SHORT VGMT_Ex
VGMT_00:
	call	VDD_Mem_Chg			; If dirty pages set new time
	mov	eax,[edi.VDD_ModTime]		; EAX = last time video mod'd
VGMT_Ex:
	pop	edi
	pop	edx
	pop	ecx
	ret
EndProc VDD_Get_ModTime

;******************************************************************************
;
;   VDD_PM_API
;
;DESCRIPTION:
;	This routine dispatches to the various service routines required
;	by the grabber.  It is called only by the grabber in the SYS VM.
;
;ENTRY: EBX = Current VM handle = SYS VM handle
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle (except for function 0, get version)
;	Client_AX = Device function
;
;EXIT:	dispatch to API service routines
;
;USES:	flags
;
;==============================================================================

BeginProc VDD_PM_API,PUBLIC
	VMMCall Test_Sys_VM_Handle
IFDEF	DEBUG
	jz	SHORT VPA_D00
Debug_Out   "VDD:API call made by VM #EBX, non system VM"
VPA_D00:
ENDIF
	jnz	SHORT VPA_Exit
	movzx	ecx,[ebp.Client_AX]
	cmp	cl,GRB_Unlock_APP
	ja	SHORT VPA_Exit
; Get VM pointers for VM
	mov	edx,ebx 			; EDX = SYS VM handle
	or	ecx,ecx
	jz	SHORT VPA_00
	mov	ebx,[ebp.Client_EBX]		; EBX = other VM's handle
	mov	edi,ebx
	add	edi,[VDD_CB_Off]		; EDI = VM's VDD CB ptr
	VMMCall Validate_VM_Handle
VPA_00:
	jmp	DWORD PTR Grb_JumpTable[ecx*4]
VPA_Exit:
	ret

;VDD virtual mode services for the grabber
Grb_JumpTable	LABEL DWORD
.ERRE		GRB_Get_Version*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Grb_Version ; Get version number and ID string ptr
.ERRE		GRB_Get_Mem*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Get_Mem     ; Get selector and map to video memory
.ERRE		GRB_Get_State*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Get_State   ; Get video state
.ERRE		GRB_Get_Mod*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Get_Mod     ; Get state changes
.ERRE		GRB_Clear_Mod*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Clear_Mod   ; Clear state change accumulation
.ERRE		GRB_Free_Mem*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Free_Mem    ; Free memory
.ERRE		GRB_Get_GrbMem*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Get_GrbMem  ; Get selector and map to video grab mem
.ERRE		GRB_Free_Grab*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Free_Grab   ; Free grab memory and controller state
.ERRE		GRB_Get_GrbState*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Get_GrbState; Get grab video state
.ERRE		GRB_Unlock_APP*4 EQ $-Grb_JumpTable
	DD  OFFSET32 VDD_Unlock_APP     ; Unlock the APP 

EndProc VDD_PM_API,PUBLIC

BeginProc VDD_Grb_Version
	call	VDD_Get_GrbVersion
	mov	[ebp.Client_EAX],eax	; Pass version number to grabber
	movzx	edi,[ebp.Client_ES]	; Get create structure segment
	push	edx
	VMMCall _SelectorMapFlat,<edx,edi,0>
	pop	edx
	inc	eax
IFDEF DEBUG
	jnz	short VGV_D00
Debug_Out "SelectorMapFlat failed VDD_Grb_DeviceVersion"
VGV_D00:
ENDIF
	jz	SHORT VGV_Exit
	dec	eax
	mov	edi,eax
	add	edi,[ebp.Client_EDI]	; EDI -> Target Buffer for version string
	cld
	movsd				; Copy 8 byte name
	movsd
VGV_Exit:
	ret
EndProc VDD_Grb_Version

BeginDoc
;******************************************************************************
;
;   VDD_Get_State, VDD_Get_GrbState
;
;DESCRIPTION:
;	These routines are called by the grabber. Since the grabber is a
;	virtual machine piece of code, all parameters exist on the stack
;	in the Client registers. These routines return a video state
;	structure (see VDD.INC). The Get_GrbState version of the
;	routine returns the state of the video for the VM specified at the
;	point when the GrabRtn was called. The Get_State version of the
;	routine returns the current state of the VM. The Get_State routine
;	should be called after the Get_Mem call, before the Free_Mem call.
;
;	Note that a state structure size of zero, returned in CX,
;	indicates an error.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_ES:Client_EDI = SELECTOR:OFFSET of buffer to store state structure
;	Client_EBX = VM Handle
;	Client_CX = size of buffer to hold state structure (DEBUG ONLY!)
;
;EXIT:	Client_SI = size of state structure returned (DEBUG ONLY!)
;	State structure saved in buffer indicated
;
;USES:	flags, Client_CX
;
;==============================================================================
EndDoc

BeginProc VDD_Get_GrbState
	jc	SHORT VGS_Err0
	clc
	jmp	SHORT VGS_Start
EndProc VDD_Get_GrbState

BeginProc VDD_Get_State

	jc	SHORT VGS_Err0
	stc
VGS_Start:
	pushfd
;*******
; Verify parameters
IFDEF	DEBUG
	cmp	[ebp.Client_CX],SIZE VDA_CGA_State ; Q: State buffer big enough?
	jnc	SHORT VGS_D02
	mov	ax,[ebp.Client_CX]
Debug_Out   "VDD:Get_State buffer too small, is #AX"
VGS_D02:
	jb	SHORT VGS_Error 		;   N: return no changes
	mov	[ebp.Client_CX],SIZE VDA_CGA_State ; Set size of state returned
ENDIF

;*******
; Get address of buffer to return state information
	movzx	esi,[ebp.Client_ES]		; Get state structure selector
	push	edx
	VMMCall _SelectorMapFlat,<edx,esi,0>
	pop	edx
	inc	eax
IFDEF DEBUG
	jnz	short VGS_D03
	debug_out "VDD: SelectorMapFlat failed for Get_State"
VGS_D03:
ENDIF
	jz	SHORT VGS_Error
	dec	eax
	add	eax,[ebp.Client_EDI]
	mov	esi,eax 			; ESI -> State VM structure

;*******
; Set the state structure(windowed VM's main state, grabbed VM's copy state)
	popfd					; CF = 1 indicates main state
	call	VDD_State_GetStt		; Build state structure at ESI
	ret
;*******
; Set the state structure to zero length on error
VGS_Err0:
	pushfd
VGS_Error:
	xor	eax,eax
	mov	[ebp.Client_ECX],eax
	popfd
	ret
EndProc VDD_Get_State


BeginDoc
;******************************************************************************
;
;   VDD_Get_Mod
;
;DESCRIPTION:
;	This routine is called to get changes in a VM's video state.
;	The changes are passed to the grabber in a buffer that includes
;	a flag indicating what kind of changes occured and what type of
;	a memory change list follows. The flag is followed by a count of
;	memory change list entries and the change list itself. Note that
;	a change structure size of zero, returned in CX, indicates an error.
;
;	This routine will return cumulative changes until Clear_Mod is called.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	Client_ES:Client_EDI = SELECTOR:OFFSET of buffer to store chgs structure
;	Client_EBX = VM Handle
;	Client_CX = size of buffer to hold changes structure (DEBUG ONLY!)
;
;EXIT:	Client_CX = size of change structure returned (DEBUG ONLY!)
;	Changes structure saved in buffer indicated
;
;USES:	flags, Client_CX
;
;==============================================================================
EndDoc

BeginProc VDD_Get_Mod
	mov	ecx,0
	jc	VGM_Error
IFDEF RECTDEBUG
Trace_Out "VDD: Get_Mod"
ENDIF
IFDEF	DEBUG
	cmp	[ebp.Client_CX],SIZE VDD_Mod_State
	jnc	SHORT VGM_D01
	mov	ax,SIZE VDD_Mod_State
	mov	cx,[ebp.Client_CX]
Debug_Out   "VDD:Get_Mod buffer too small, is #CX, need #AX"
	xor	ecx,ecx
	jmp	SHORT VGM_Error
VGM_D01:
ENDIF
	xor	ecx,ecx

; Get address of buffer to return state information
	movzx	esi,[ebp.Client_ES]		; Get state structure selector
	push	ecx
	push	edx
	VMMCall _SelectorMapFlat,<edx,esi,0>
	pop	edx
	pop	ecx
	inc	eax
IFDEF DEBUG
	jnz	short VGM_D02
	debug_out "VDD: SelectorMapFlat failed for Get_Mod"
VGM_D02:
ENDIF
	jz	SHORT VGM_Error
	dec	eax
	add	eax,[ebp.Client_EDI]
	mov	esi,eax 			; ESI -> State VM structure

        call    VDD_Suspend_NormSch             ; Lock the APP on Get_Mod call
;
; check for special cases.
;
	test	[edi.VDD_EFlags],fVDE_NoMain    ; No main mem
        jnz     SHORT VGM_Error
	test	[edi.VDD_Flags],fVDD_MInit	; Q: Memory initialized?
	jz	SHORT VGM_Error                 ;   N: report screen off
; Force an update(grabber has old state) if first update after VMInit
;
        btr     [edi.VDD_Flags],fVDD_ForcedUpdBit  ; Q: Forced Update?
        jc      SHORT VGM_ForcedUpdate             ;   Y: 
; Get mode, save video state if necessary
	mov	[esi.VDD_Mod_Flag],0            ; clear all flags
	call	VDD_State_CtlrChgs		; Build controller changes struc
%out Return appropriate value from State_CtlrChgs for debug code
;IFDEF	DEBUG
;	jnc	SHORT VGM_D03
;Debug_Out   "VDD: Get_Mod while VM #EBX is in undisplayable state"
;VGM_D03:
;ENDIF
%out what is going on here?
        jz	SHORT VGM_NoChg		        ; VM not displayable just now
	call	VDD_State_MemChgs		; Build memory changes struc
        mov     cx,SIZE VDD_Mod_State
VGM_Ex:
	mov	[ebp.Client_CX],cx		; Return length of change buf
	ret
VGM_NoChg:
	xor	eax,eax
	jmp	SHORT VGM_NoList
VGM_Error:
	mov	eax,fVDD_M_Err
VGM_NoList:
	mov	[esi.VDD_Mod_Flag],ax		; Save state change flags
	mov	[esi.VDD_Mod_Count],cx
	mov	cl,4
	jmp	VGM_Ex
VGM_ForcedUpdate:
	mov	[esi.VDD_Mod_Flag],fVDD_M_Ctlr+fVDD_M_Curs ; Set ctrlr chg flag	       
	mov	[esi.VDD_Mod_Count],0
	mov	[ebp.Client_CX],4		; Return length of change buf
        ret

EndProc VDD_Get_Mod

BeginDoc
;******************************************************************************
;
;   VDD_Clear_Mod
;
;DESCRIPTION:
;	This routine is called to clear the change state of a VM.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle
;
;EXIT:	none
;
;USES:	flags
;
;==============================================================================
EndDoc

BeginProc VDD_Clear_Mod
	jc	SHORT VCM_Exit
IFDEF RECTDEBUG
Trace_Out "VDD: Clear_Mod"
ENDIF
	call	VDD_State_Update		; Update controller chg state
	call	VDD_Mem_Update			; Update memory change state
	test	[edi.VDD_Flags],fVDD_Win
	jz	SHORT VCM_Exit
	call	VDD_Set_VM_Time_Out		; Set new timeout
VCM_Exit:
	ret
EndProc VDD_Clear_Mod

BeginDoc
;******************************************************************************
;
;   VDD_Get_Mem, VDD_Get_GrbMem
;
;DESCRIPTION:
;	Return flat address and allocation bit map for VM's video memory.
;
;	The main difference between these two routines is that Get_Mem
;	returns the main video save memory and Get_GrbMem a copy of the
;	memory that was made by VDD_Grab. An addional important difference
;	is that Get_Mem boosts the scheduling priority of the system VM, as
;	mentioned below.
;
;	In order to assure that the memory does not change from the point
;	that Get_Mem is called until Free_Mem is called, we will
;	boost the system VM's priority by a Low_Priority_Device_Boost.
;	This will inhibit normal scheduling but will not hamper scheduling
;	of VM's to handle events such as interrupts.
;
;	THE ALLOCATION OF MEMORY TO THE BIT MAP IS DYNAMIC. The grabber
;	must make this call each time that it wants to access the memory.
;
;	Get_Mem call MUST BE FOLLOWED by a Free_Mem call AS SOON AS POSSIBLE.
;	No other VM's will be scheduled until the Free_Mem call is made.
;
;	If the client buffer is not big enough or the selector is bad or if
;	GrbMem is called when no grab is active, an error return will be done,
;	that is, Client SI = 0.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle
;	Client_ES:Client_EDI = SELECTOR:OFFSET of buffer to store VDA_Mem_State
;	Client_CX = size of buffer to hold VDA_Mem_State (DEBUG ONLY!)
;
;EXIT:	Mem structure modified for current state of memory
;	Client_CX = size of structure (=0 if error)
;
;USES:	Flags
;
;==============================================================================
EndDoc

BeginProc VDD_Get_Mem
	jc	SHORT VGMm_Error
IFDEF RECTDEBUG
Trace_Out "VDD: Get_Mem"
ENDIF
        inc     [edi.VDD_Get_Mem_Count]         ; one more call to Get_Mem
	lea	esi,[edi.VDD_Pg.VPH_MState]	; ESI = VM's Mem state ptr
	call	VGMm_Set_Struc			; Pass the structure
	jc	SHORT VGMm_Exit
        call    VDD_Suspend_NormSch
VGMm_Exit:
	ret
VGMm_Error:
	mov	[ebp.Client_CX],0		; Return length of change buf
	ret
EndProc VDD_Get_Mem

; ENTRY  EDX = SYS VM handle, EDI = CB ptr for VM being Locked
;
; USES : EAX
BeginProc VDD_Suspend_NormSch, PUBLIC

        push    ebx
        push    edi

	xchg	edx, ebx                       
	mov	edi, ebx
	add	edi, [VDD_CB_Off]
	bts	[edi.VDD_Flags], fVDD_BoostBit  ; Q: SYS VM Boosted already?
	jc	SHORT VNS_Exit                  ;   Y: 

	VMMcall Get_VM_Exec_Time                ; Get Exec time for Sys VM
        pop     edi
        push    edi
	mov	[edi.VDD_Upd_Start], eax        ; Store as start time for 
                                                ; window update
	mov	eax, VDD_Pri_Device_Boost	; Boost SYS VM
	VMMCall Adjust_Exec_Priority
VNS_Exit:
        pop     edi
        pop     ebx
	ret

EndProc VDD_Suspend_NormSch

BeginProc VDD_Get_GrbMem

	jc	SHORT VGMm_Error
	lea	esi,[edi.VDD_CPg.VPH_MState]	; ESI = VM's copy Mem state ptr
	jmp	SHORT VGMm_Set_Struc
EndProc VDD_Get_GrbMem

;******************************************************************************
;
;   VGMm_Set_Struc
;
;DESCRIPTION: Copy memory state structure to client buffer
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	ESI = state structure ptr
;	Client_EBX = VM Handle
;	Client_CX = size of buffer to hold mem structure
;	Client_ES:Client_EDI = SELECTOR:OFFSET of buffer to store mem structure
;
;EXIT:	Mem structure copied to client buffer
;	Client_ECX = size of structure copied (=0 and CF=1 if error)
;
;USES:	Flags, EAX, ESI
;
;==============================================================================
BeginProc VGMm_Set_Struc

	push	edi
IFDEF	DEBUG
	cmp	[ebp.Client_CX],SIZE VDA_Mem_State
	jnc	SHORT VGMm_D01
	mov	ax,SIZE VDA_Mem_State
	mov	cx,[ebp.Client_CX]
Debug_Out   "VDD:Get_Mem buffer too small, is #CX, need #AX"
VGMm_D01:
	jc	VGM_Error
ENDIF
	mov	[ebp.Client_CX],SIZE VDA_Mem_State ; Set size of buff returned
	movzx	eax,[ebp.Client_ES]		; Get state structure selector
	push	edx
	VMMCall _SelectorMapFlat,<edx,eax,0>
	pop	edx
	inc	eax
IFDEF DEBUG
	jnz	short VGMm_D02
	debug_out "VDD: SelectorMapFlat failed for Get_Mem"
VGMm_D02:
ENDIF
	jz	SHORT VGMmS_Error
	dec	eax
	add	eax,[ebp.Client_EDI]

.ERRE	((SIZE VDA_Mem_State) MOD 4) EQ 0
	mov	edi,eax 			; EDI = Mem struc ptr
	mov	ecx,SIZE VDA_Mem_State/4
        cld
	rep movsd
	pop	edi
	clc
	ret

VGMmS_Error:
	xor	eax,eax
	mov	[ebp.Client_CX],ax		; Save size of buffer returned
	pop	edi
	stc
	ret
EndProc VGMm_Set_Struc

BeginDoc
;******************************************************************************
;
;   VDD_Free_Grab
;
;DESCRIPTION:
;	This routine is used to release the copy of the video memory that
;	was allocated when a screen grab was done.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle to free grab for
;
;EXIT:	none
;
;USES:	Flags
;
;==============================================================================
EndDoc

BeginProc VDD_Free_Grab
	test	[edi.VDD_Flags],fVDD_Win	; Q: Windowed?
	jz	VDD_Mem_DCopy                   ;  N: Deallocate copy mem
        ret                                     ;  Y: Keep copy memory
EndProc VDD_Free_Grab

BeginDoc
;******************************************************************************
;
;   VDD_Free_Mem
;
;DESCRIPTION:
;	This routine is used to release the scheduling freeze that occurred
;	when the Get_Mem call was made. See Get_Mem documentation.
;
;ENTRY: EDX = Sys VM handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle to free mem for
;
;EXIT:	none
;
;USES:	Flags
;
;==============================================================================
EndDoc

BeginProc VDD_Free_Mem
	jc	SHORT VFM_Exit
        dec     [edi.VDD_Get_Mem_Count]       ; one less call to Free_Mem
IFDEF RECTDEBUG
Trace_Out "VDD: Free_Mem"
ENDIF
        clc                               ; safety
        call    VDD_Unlock_APP
VFM_Exit:
	ret
EndProc VDD_Free_Mem

BeginDoc
;******************************************************************************
;
;   VDD_Unlock_APP
;
;DESCRIPTION:
;	This routine is used to Unlock the Windowed OLDAPP from the grabber
;       Called from VDD_Free_Mem also
;
;ENTRY: EDX = Sys VM handle
;	IF CF = 1, call was made with invalid VM Handle
;	EBX = VM handle of VM for whom call is made
;	EDI = VDD CB ptr
;	EBP = Client stack frame ptr
;	Client_EBX = VM Handle to free mem for
;
;EXIT:	none
;
;USES:	EAX,Flags
;
;==============================================================================
EndDoc
BeginProc VDD_Unlock_APP

	jc	SHORT VRN_Exit
        mov     eax, [edi.VDD_Get_Mem_Count]
        or      eax,eax
        jnz     SHORT VRN_Exit

	xchg	edx, ebx
        push    edi
        mov     edi, ebx
	add	edi,[VDD_CB_Off]
	btr	[edi.VDD_Flags], fVDD_BoostBit
        pop     edi
	jnc	SHORT VRN_Exit
	VMMcall Get_VM_Exec_Time
	sub	eax, [edi.VDD_Upd_Start]
	VMMcall Adjust_Execution_Time
	xchg	edx, ebx
	neg	eax
	VMMcall Adjust_Execution_Time

	xchg	edx, ebx
	mov	eax,- VDD_Pri_Device_Boost
	VMMCall Adjust_Exec_Priority
VRN_Exit:
	ret

EndProc VDD_Unlock_APP


;******************************************************************************
;VDD_SysModalMsg
;
;DESCRIPTION:
;
;ENTRY: EBX = VM for whom message is generated
;	EAX = message box flags
;	ECX = ptr to ASCIIZ message text
;
;EXIT:	EAX = message response code
;
;USES: Flags, EAX
;
;***************
BeginProc VDD_SysModalMsg,PUBLIC

	push	edi
	xor	edi,edi 			; Use VM name for caption
	or	eax, MB_SYSTEMMODAL		; Always system modal
	VxDCall SHELL_SYSMODAL_Message		; Get user response in EAX
	pop	edi
	ret
EndProc VDD_SysModalMsg

;******************************************************************************
;VDD_Message
;
;DESCRIPTION:
;
;ENTRY: EBX = VM for whom message is generated
;	EAX = message box flags
;	ECX = ptr to ASCIIZ message text
;	EDI = VM's VDD CB ptr
;
;EXIT:	none
;
;USES:	Flags, EAX, EDX
;
;***************
BeginProc VDD_Message,PUBLIC

	push	edi
	push	eax
	xor	edi,edi 			; Use VM name for caption
	xor	esi,esi 			; No call back
	VxDCall SHELL_Message			; send message to Windows
	pop	eax
	pop	edi				; Q: Successful?
	jc	VDD_SysModalMsg 		;   N: Do sysmodal msg NOW
	ret
VDD_Msg_CallBack:

EndProc VDD_Message

;******************************************************************************
;VDD_Msg_NoMainMem
;
;DESCRIPTION:
;
;ENTRY: EBX = VM for whom message is generated
;	EDI = VM's VDD CB ptr
;
;EXIT:	none
;
;USES:	Flags, EAX, ECX, EDX, ESI
;
;***************
BeginProc VDD_Msg_NoMainMem,PUBLIC

VMB_Typ_NoMainMem   EQU MB_OK+MB_ICONEXCLAMATION+MB_ASAP
	mov	eax,VMB_Typ_NoMainMem
	mov	ecx,OFFSET32 VMB_Str_NoMainMem
	bts	[edi.VDD_EFlags],fVDE_NoMMsgBit
	jnc	VDD_Message
	ret
EndProc VDD_Msg_NoMainMem

;******************************************************************************
;VDD_Msg_NoCopyMem
;
;DESCRIPTION:
;
;ENTRY: EBX = VM for whom message is generated
;	EDI = VM's VDD CB ptr
;
;EXIT:	none
;
;USES:	Flags, EAX, ECX, EDX, ESI
;
;***************
BeginProc VDD_Msg_NoCopyMem,PUBLIC

VMB_Typ_NoCopyMem   EQU MB_OK+MB_ICONASTERISK
	mov	eax,VMB_Typ_NoCopyMem
	mov	ecx,OFFSET32 VMB_Str_NoCopyMem
	bts	[edi.VDD_EFlags],fVDE_NoCMsgBit
	jnc	VDD_Message
	ret
EndProc VDD_Msg_NoCopyMem

VxD_CODE_ENDS

	END

