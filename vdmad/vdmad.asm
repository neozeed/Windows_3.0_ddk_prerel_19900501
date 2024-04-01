PAGE 58,132
;******************************************************************************
TITLE vdmad.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988
;
;   Title:	vdmad.asm -
;
;   Version:	2.00
;
;   Date:	15-Nov-1988
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Nov-1988 RAP Started complete rewrite
;
;==============================================================================

	.386p

;==============================================================================
;;    rpMemTst = 1	  ; this equate conditionally generates code which
			; registers a VKD hot key "Ctrl+Alt+M" to force
			; page fragmentation by allocating a bunch of pages,
			; then deallocating every other one.  This was used to
			; better test buffered DMA.
;==============================================================================

BeginDoc
;******************************************************************************
;
;   VDMAD Virtual DMA Device
;
;   VDMAD virtualizes DMA I/O for standard DMA channels for all VM's.  By
;   default it handles all programmed I/O for the DMA controllers and
;   arbitrates I/O to the physical DMA ports so that more than one VM can
;   be using the same DMA channels at the same time.  In some cases the
;   default handling of DMA channels is not desirable.	To handle these cases,
;   VDMAD provides a number of services to allow another Windows/386 device
;   to take control of the virtualization of specific DMA channels.
;
;   VDMAD also provides some services which can be used by Bus Master devices
;   which have their own DMA controllers.  These devices still need to be able
;   to lock and unlock DMA regions in memory and determine the physical
;   addresses of these regions.  Bus Master devices can also make use of the
;   buffer services, if they can't otherwise scatter gather a linear region
;   which isn't physically contiguous.
;
;   VDMAD services available for Bus Master use are:
;
;		VDMAD_Create_Handle
;		VDMAD_Lock_DMA_Region
;		VDMAD_Unlock_DMA_Region
;		VDMAD_Scatter_Lock
;		VDMAD_Scatter_Unlock
;		VDMAD_Request_Buffer
;		VDMAD_Release_Buffer
;		VDMAD_Copy_To_Buffer
;		VDMAD_Copy_From_Buffer
;
;******************************************************************************
EndDoc



;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

.XLIST
	include VMM.INC
	include VPICD.INC

	Create_VDMAD_Service_Table EQU 1      ; VDMAD service table created
	include VDMAD.INC
	include DMASYS.INC

	include Debug.INC

IFDEF rpMemTst
%OUT including VKD.INC to declare a test hot key - used to fragment memory
	include VKD.INC
ENDIF

	include sysinfo.INC

.LIST

;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VDMAD, 2, 0, VDMAD_Control, VDMAD_Device_ID, VDMAD_Init_Order


;******************************************************************************
;******************************************************************************
; Initialization data


VxD_IDATA_SEG

EXTRN VDMAD_IO_Table:BYTE
EXTRN VDMAD_EISA_Size_Ini:BYTE

VDMAD_SysCrit_Done  db	0

VxD_IDATA_ENDS

VxD_CODE_SEG

EXTRN VDMAD_NoCheck:NEAR
EXTRN VDMAD_Check_TC1:NEAR
EXTRN VDMAD_Check_TC2:NEAR

VxD_CODE_ENDS


;******************************************************************************
;******************************************************************************
; Global data

VxD_DATA_SEG

EXTRN VDMAD_Ini_Buffer_Size:BYTE
EXTRN Buffer_Too_Small:BYTE
EXTRN Buffer_Too_Small_value:BYTE
EXTRN DMA_EISA_Ext_Modes:BYTE

VxD_DATA_ENDS


VxD_LOCKED_DATA_SEG

	ALIGN	4

	PUBLIC VDMAD_CB_Offset
VDMAD_CB_Offset     dd	0

EXTRN page_ports:BYTE
EXTRN base_ports:BYTE
EXTRN count_ports:BYTE

	PUBLIC DMA_Channels
DMA_Channels	    label dword
    DMA_Channel_Data <0, OFFSET32 VDMAD_Call_Def>  ; Channel 0
    DMA_Channel_Data <1, OFFSET32 VDMAD_Call_Def>  ; Channel 1
    DMA_Channel_Data <2, OFFSET32 VDMAD_Call_Def>  ; Channel 2
    DMA_Channel_Data <3, OFFSET32 VDMAD_Call_Def>  ; Channel 3
DMA_Channels_2	    label dword
    DMA_Channel_Data <4, 0>				  ; Channel 4
    DMA_Channel_Data <5, OFFSET32 VDMAD_Call_Def>  ; Channel 5
    DMA_Channel_Data <6, OFFSET32 VDMAD_Call_Def>  ; Channel 6
    DMA_Channel_Data <7, OFFSET32 VDMAD_Call_Def>  ; Channel 7


DMA_Channels_Available	equ 8


	PUBLIC DMA_consts
DMA_consts label byte
DMA1 Controller_Const <OFFSET32 DMA_Channels, \
		       DMA1_Mode, DMA1_SingleMask, DMA1_SoftReq, DMA1_CLR_FF, \
		       OFFSET32 VDMAD_Check_TC1, OFFSET32 VDMAD_HW_Check_TC1>
IFNDEF PCXT
DMA2 Controller_Const <OFFSET32 DMA_Channels_2, \
		       DMA2_Mode, DMA2_SingleMask, DMA2_SoftReq, DMA2_CLR_FF, \
		       OFFSET32 VDMAD_Check_TC2, OFFSET32 VDMAD_HW_Check_TC2>
ENDIF

	PUBLIC	DMA_Buffer_Size
MinBufferSize	    equ 16	    ; default buffer of 16Kb (4 pages)
DMA_Buffer_Size     dd	MinBufferSize
DMA_Buffer_Phys     dd	0	    ; first page of DMA buffer
DMA_Buffer_Linr     dd	0	    ; linear address
DMA_Buffer_State    dd	0
DMA_Buffer_Timeout  dd	0	    ; timeout handle

%OUT THIS IS INCORRECT NEEDS TO BE DRIVEN BY INIT SERVICE NOT A CONDITIONAL
IFDEF PCXT
DMA_Max_Physical    dd	0FFFFFh     ; 1Mb
ELSE
DMA_Max_Physical    dd	0FFFFFFFFh  ; no max
ENDIF


Buffer_In_Use	      equ 1
Buffer_Needs_Copying  equ 2

	PUBLIC VDMAD_Check_TC
VDMAD_Check_TC	    dd	0
VDMAD_Hw_Int_Next   dd	0	    ; address of next routine to receive the
				    ; VPICD Hardware Int notification; 0 if none
VDMAD_Hw_Int_Filter dd	0	    ; address of DMA check routine, if DMA in
				    ; progress, else same as VDMAD_Hw_Int_Next

	PUBLIC VDMAD_DMA1_status
	PUBLIC VDMAD_DMA2_status

VDMAD_DMA1_status   db	0
VDMAD_DMA2_status   db	0

PUBLIC VDMAD_Machine_Type
VDMAD_Machine_Type  db	ISA_Machine

VxD_LOCKED_DATA_ENDS


;******************************************************************************
;******************************************************************************
; Initialization code

VxD_ICODE_SEG

EXTRN VDMAD_PS2_Device_Init:NEAR
EXTRN VDMAD_API_Device_Init:NEAR
EXTRN VDMAD_EISA_Init:NEAR

;******************************************************************************
;
;   VDMAD_Sys_Crit_Init
;
;   DESCRIPTION:    Check INI file for a VDMAD buffer size specification
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Sys_Crit_Init

	mov	[VDMAD_SysCrit_Done], True
	mov	eax, [DMA_Buffer_Size]
	mov	edx, eax
	xor	esi, esi
	mov	edi, OFFSET32 VDMAD_Ini_Buffer_Size
	VMMCall Get_Profile_Decimal_Int
	cmp	eax, edx		    ;Q: user value > current size?
	jbe	SHORT no_change 	    ;	N:
	mov	edx, eax		    ;	Y: record new value
no_change:

;
; Allocate DMA buffer
;
	mov	edi, OFFSET32 DMA_Buffer_Phys

	add	edx, 3			    ; round up to get
	shr	edx, 2			    ; # of pages
	mov	[DMA_Buffer_Size], edx	    ; convert DMA_Buffer_Size to # of pages
	mov	eax, edx		    ; (examples:       3    7	  11
					    ;		      12K  28K	  44K
	dec	eax			    ; # pages - 1     10b  111b  1011b
	bsr	cx, ax			    ; max power of 2   1    2	   3
	inc	cl			    ; shift cnt        2    3	   4
	mov	eax, 1
	shl	eax, cl 		    ; mask + 1	     100b 1000b 10000b
	dec	eax			    ; mask	      11b  111b  1111b
					    ; alignment       16K  32K	  64K
	VMMCall _PageAllocate <edx, PG_SYS, 0, eax, 0, DMA_Max_Physical, edi, PageUseAlign+PageContig+PageFixed>
	test	eax, eax
	jnz	SHORT DMAD_B_OK
	Debug_Out "VDMAD ERROR:  Could not alloc DMA buffer"
	VMMcall Fatal_Memory_Error

DMAD_B_OK:
	mov	[DMA_Buffer_Linr], edx

	push	ebx
	VMMCall Get_Machine_Info

	test	ebx, GMIF_MCA		    ; Q: Micro channel?
	jz	SHORT not_PS2DMA	    ;	N:
	mov	[VDMAD_Machine_Type], MCA_Machine

; channel 4 is available on PS2's, so assign default handler proc
;
	mov	[DMA_Channels_2.call_back], OFFSET32 VDMAD_Call_Def
	call	VDMAD_PS2_Device_Init

not_PS2DMA:
	test	ebx, GMIF_EISA		    ; Q: EISA machine?
	jz	SHORT not_EISADMA	    ;	N:

	mov	eax, TRUE
	xor	esi, esi
	mov	edi, OFFSET32 VDMAD_EISA_Size_Ini
	VMMCall Get_Profile_Boolean
	or	eax, eax
	jz	short not_EISADMA	    ; jump if forced to non-EISA

	mov	[VDMAD_Machine_Type], EISA_Machine
	call	VDMAD_EISA_Init

not_EISADMA:
	call	VDMAD_API_Device_Init

;
; Allocate portion of each VM's control block for DMA information
;
	push	ebx
	VMMCall _Allocate_Device_CB_Area, <<SIZE DMA_CB_DATA>, 0>
	test	eax, eax
	jnz	SHORT DMAD_CB_OK
	Debug_Out "VDMAD ERROR:  Could not alloc control block data area space"
	VMMcall Fatal_Memory_Error

DMAD_CB_OK:
	mov	[VDMAD_CB_Offset], eax
	pop	ebx

;
; hook DMA I/O ports
;
	mov	edi, OFFSET32 VDMAD_IO_Table  ; Table of ports
	VMMCall Install_Mult_IO_Handlers    ; Install the port traps

;
; register VPICD Hardware Int filter proc

	pushfd
	cli
	mov	esi, OFFSET32 VDMAD_jmp_filter
	VxDCall VPICD_Call_When_Hw_Int
	mov	[VDMAD_Hw_Int_Next], esi    ; save procedure to chain to
	mov	[VDMAD_Hw_Int_Filter], esi  ; no filter so far
	mov	[VDMAD_Check_TC], OFFSET32 VDMAD_NoCheck
	popfd

	pop	ebx
	jmp	VDMAD_Init_VM_CB

EndProc VDMAD_Sys_Crit_Init


IFDEF rpMemTst
;******************************************************************************
;
;   VDMAD_Device_Init
;
;   DESCRIPTION:
;
;   ENTRY:	    EBX is SYS VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Device_Init

%OUT special setup of Ctrl+Alt+M is memory fragmenter hot key
	mov	ax, 32h 		    ; 'M' key
	ShiftState <SS_Either_Ctrl + SS_Either_Alt + SS_Toggle_mask>, <SS_Ctrl + SS_Alt>
	mov	cl, CallOnPress
	mov	esi, OFFSET32 Fragment_Memory ; call-back routine
	xor	edx, edx		    ; no reference data
	VxDCall VKD_Define_Hot_Key
	Trace_Out "Ctrl+Alt+M is memory fragmenter hot key"
	clc
	ret

EndProc VDMAD_Device_Init
ENDIF


BeginDoc
;******************************************************************************
;
;   VDMAD_Reserve_Buffer_Space
;
;   DESCRIPTION:
;	This service allows other devices that are going to handle DMA to
;	make sure that VDMAD allocates a buffer large enough for any transfers
;	that they might require.  It also allows a device to specify a maximum
;	physical address that would be valid for the device's DMA requests
;	(such as 1Mb for an XT.)  During the Init_Complete phase of
;	initialization, VDMAD will allocate the DMA buffer using all of the
;	contraints specified by other devices.	i.e. the buffer will be at
;	least as big as the largest size specified by the calls to this
;	service, and it will be allocate below the lowest maximum physical
;	addresses specified.
;
;	This service is only available before Init_Complete.
;
;   ENTRY:	    EAX = # of pages requested
;		    ECX = maximum physical address that can be included in a
;			  DMA transfer; 0, if no limit.
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Reserve_Buffer_Space, SERVICE

	cmp	[VDMAD_SysCrit_Done], True
	je	SHORT vres_bad
	pushad
	shl	eax, 2			; convert from pages to Kb
	cmp	eax, [DMA_Buffer_Size]	;Q: already allow for this request?
	jbe	short req_done		;   Y:
	mov	[DMA_Buffer_Size], eax	;   N: store new requested size
req_done:
	cmp	cl, 0FFh		;Q: adjusted as max usable?
	je	short reasonable_max	;   Y: probably
	dec	ecx			;   N: convert 100000h to 0FFFFFh
					;	       1Mb to 1Mb-1 (last usable)
reasonable_max:
	cmp	ecx, [DMA_Max_Physical]
	jae	short no_max_change
	mov	[DMA_Max_Physical], ecx
no_max_change:
	popad
	ret

vres_bad:
	Debug_Out "ERROR:  VDMAD_Reserve_Buffer_Space called after Sys_Critical_Init"
	Fatal_Error
EndProc VDMAD_Reserve_Buffer_Space

VxD_ICODE_ENDS


;******************************************************************************
;******************************************************************************

IFDEF rpMemTst
VxD_DATA_SEG
	fragmentList dd 100 DUP(0)
VxD_DATA_ENDS
ENDIF

VxD_CODE_SEG

EXTRN VDMAD_API_System_Exit:NEAR
EXTRN VDMAD_PS2_Init_VM_CB:NEAR

IFDEF rpMemTst
BeginProc Fragment_Memory

	pushfd
	pushad
	mov	ecx, 100
	mov	edi, OFFSET32 fragmentList
	cmp	dword ptr [edi], 0	    ;Q: already called once?
	je	short do_allocs 	    ;	N:

	Trace_Out "freeing fragmentation pages"
free_pages:
	mov	eax, [edi]
	or	eax, eax		    ;Q: valid handle?
	jz	short no_page		    ;	N:
	push	ecx
	VMMCall _PageFree <eax, 0>
	pop	ecx
	xor	eax, eax
no_page:
	cld
	stosd
	loop	free_pages
	jmp	short fm_exit

do_allocs:
	push	ecx
	VMMCall _PageAllocate <1, PG_SYS, 0, 0, 0, 0, 0, PageLocked>
	cld
	stosd				    ; save handle
	pop	ecx
	loop	do_allocs

	mov	ecx, 50
	mov	edi, OFFSET32 fragmentList
free_odd_pages:
	add	edi, 4
	mov	eax, [edi]
	push	ecx
	VMMCall _PageFree <eax, 0>
	pop	ecx
	xor	eax, eax
	cld
	stosd				    ; clear handle
	loop	free_odd_pages
	Trace_Out "allocated fragmentation pages"

fm_exit:
	popad
	popfd
	ret

EndProc Fragment_Memory
ENDIF


;******************************************************************************
;
;   VDMAD_Control
;
;   DESCRIPTION:    dispatch control messages to the correct handlers
;
;   ENTRY:
;
;   EXIT:	    Carry clear if no error
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Control

	Control_Dispatch Sys_Critical_Init,  VDMAD_Sys_Crit_Init
IFDEF rpMemTst
	Control_Dispatch Device_Init,	     VDMAD_Device_Init
ENDIF
	Control_Dispatch Create_VM,	     <SHORT VDMAD_Init_VM_CB>
IFDEF rpMemTst
	Control_Dispatch VM_Terminate,	     VDMAD_Terminate
ENDIF
	Control_Dispatch VM_Not_Executeable, <DEBFAR VDMAD_VM_Not_Executeable>
	Control_Dispatch System_Exit,	     VDMAD_API_System_Exit
IFDEF DEBUG
	Control_Dispatch Debug_Query,	     VDMAD_Query
ENDIF
	clc
	ret

EndProc VDMAD_Control


;******************************************************************************
;
;   VDMAD_Init_VM_CB
;
;   DESCRIPTION:    Initialize virtual DMA state for new VM.
;			mask all channels
;			set default modes
;
;   ENTRY:	    EBX = Handle of VM being initialized
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_Init_VM_CB

	Assert_VM_Handle ebx		; Debugging
	mov	esi, ebx		; set esi to point to DMA data in
	add	esi, [VDMAD_CB_Offset]	; the VM's control block

	mov	[esi.ctl1.DMA_flags], 0
	mov	[esi.ctl2.DMA_flags], DMA_2nd_Ctlr OR DMA_Word_Ctlr

	mov	al, 1111b		; mask all channels of both controllers
	mov	[esi.ctl1.DMA_mask], al
	mov	[esi.ctl2.DMA_mask], al

IFDEF allow_partial_virtualization
	mov	[esi.ctl1.DMA_mode_set], 1111b	; no mode set in controller 1
	mov	[esi.ctl2.DMA_mode_set], 1111b	; no mode set in controller 1

ENDIF

	out	DMA1_CLR_FF, al 	; clear flop flop
	out	DMA2_CLR_FF, al 	; clear flop flop

	IF_MCA_JMP  <SHORT read_ps2_chnls>

	mov	ecx, 4
	xor	edi, edi

init_chnls:

IFDEF allow_partial_virtualization
	movzx	edx, page_ports[edi]
	in	al, dx
	movzx	eax, al
	mov	word ptr [edi*4][esi.ctl1.DMA_adrs+2], ax
	movzx	edx, base_ports[edi]
	in	al, dx
	mov	byte ptr [edi*4][esi.ctl1.DMA_adrs], al
	in	al, dx
	mov	byte ptr [edi*4][esi.ctl1.DMA_adrs+1], al
	movzx	edx, count_ports[edi]
	xor	eax, eax
	in	al, dx
	mov	ah, al
	in	al, dx
	xchg	al, ah
	mov	[edi*4][esi.ctl1.DMA_counts], eax
	IF_NOT_EISA_JMP <SHORT skip_EISA_ext_ports_1>
%OUT read initial EISA extended ports

skip_EISA_ext_ports_1:
	movzx	edx, page_ports[edi+4]
	in	al, dx
	movzx	eax, al
	mov	word ptr [edi*4][esi.ctl2.DMA_adrs+2], ax
	movzx	edx, base_ports[edi+4]
	in	al, dx
	mov	byte ptr [edi*4][esi.ctl2.DMA_adrs], al
	in	al, dx
	mov	byte ptr [edi*4][esi.ctl2.DMA_adrs+1], al
	movzx	edx, count_ports[edi+4]
	xor	eax, eax
	in	al, dx
	mov	ah, al
	in	al, dx
	xchg	al, ah
	mov	[edi*4][esi.ctl2.DMA_counts], eax
	IF_NOT_EISA_JMP <SHORT skip_EISA_ext_ports_2>
%OUT read initial EISA extended ports

skip_EISA_ext_ports_2:
	shl	[edi*4][esi.ctl2.DMA_adrs], 1
ENDIF

	mov	[edi*4][esi.ctl1.DMA_counts], 0000FFFFh

	mov	[edi][esi.ctl1.DMA_modes], DMA_single_mode
	mov	[edi][esi.ctl2.DMA_modes], DMA_single_mode
	mov	[edi][esi.ctl1.DMA_Ext_Modes], 0	     ; assume 1st 4 are byte ports
	mov	[edi][esi.ctl2.DMA_Ext_Modes], _16_bit_xfer  ; assume 2nd 4 are word ports
	inc	edi
	loop	init_chnls
	clc
	ret

read_ps2_chnls:
	callret VDMAD_PS2_Init_VM_CB


EndProc VDMAD_Init_VM_CB


;******************************************************************************
;
;   VDMAD_VM_Not_Executeable
;
;   DESCRIPTION:    Abort any DMA transfers that are in progress for this VM,
;		    since it is no longer running.
;
;   ENTRY:	    EBX is VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_VM_Not_Executeable

	Assert_VM_Handle ebx		; Debugging
%OUT abort any DMA transfers that are active for this VM
	clc
	ret

EndProc VDMAD_VM_Not_Executeable


;******************************************************************************
;
;   VDMAD_Get_DMA_Handle
;
;   DESCRIPTION:    Get a DMA handle for a given channel
;
;   ENTRY:	    EAX is channel #
;
;   EXIT:	    Carry clear
;			EAX is DMA handle
;		    Carry set
;			invalid channel #
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Get_DMA_Handle

	cmp	eax, DMA_Channels_Available
	jae	SHORT bad_channel
	imul	eax, SIZE DMA_Channel_Data
	add	eax, OFFSET32 DMA_Channels    ; eax is DMA handle
	stc

bad_channel:
	cmc
	ret

EndProc VDMAD_Get_DMA_Handle



;******************************************************************************
; SERVICES
;******************************************************************************

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Version
;
;   DESCRIPTION:
;	Returns the version of the Virtual DMA Device
;
;   ENTRY:
;	None
;
;   EXIT:
;	AH = Major version number
;	AL = Minor version number
;	ECX = Buffer size in bytes (0, if not allocated; a buffer will always
;		be allocated, but it doesn't happen until Init_Complete)
;	Carry flag clear
;
;   USES:
;	EAX, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Version, SERVICE

	mov	eax, 200h
	clc
	ret

EndProc VDMAD_Get_Version

BeginDoc
;******************************************************************************
;
;   VDMAD_Virtualize_Channel
;
;   DESCRIPTION:
;	This service allows another Windows/386 device to claim ownership
;	of a standard DMA channel.  The new owner registers a call-back
;	routine that will be called whenever the virtual state of the channel
;	is changed as a result of I/O done in a VM.  In some cases a device
;	doesn't want to allow a VM to perform DMA to a channel at all (they
;	will handle programming based on a private API, etc. instead of
;	virtualized hardware I/O), so it is possible to pass a 0 to specify
;	a null call-back routine.  VDMAD will continue to trap the I/O for
;	the channel, but won't ever change the physical state of the channel
;	as a result of any VM I/O.
;
;   ENTRY:	    EAX is Channel #
;		    ESI is I/O Call-back procedure (0 = none)
;
;   EXIT:	    Carry set if channel is already owned
;		    ELSE
;		      EAX is DMA handle
;
;   USES:	    flags
;
;   CALL BACK:	    ENTRY:
;			EAX = DMA handle
;			EBX = VM handle
;			Proc can modify EAX, EBX, ECX, EDX, ESI, EDI, and flags
;
;		    EXIT
;			none
;
;==============================================================================
EndDoc
BeginProc VDMAD_Virtualize_Channel, SERVICE

IFDEF DEBUG
	push	ebx
	mov	ebx, eax
ENDIF
	call	VDMAD_Get_DMA_Handle
IFDEF DEBUG
	jnc	SHORT channel_valid
	cmp	[eax.channel_num], ebx	;Q: valid channel?
	je	SHORT channel_valid	;   Y:
	Debug_Out "VDMAD:  invalid channel requested #EBX"
	Fatal_Error

channel_valid:
	pop	ebx
ENDIF

	cmp	[eax.call_back], OFFSET32 VDMAD_Call_Def
	je	short not_virtualized	; jump if not virtualized already
	Debug_Out "VDMAD:  channel #EBX already virtualized"
	stc
	jmp	short virt_exit

not_virtualized:
	mov	[eax.call_back], esi	; assign new call-back proc, or 0
	clc

virt_exit:
	ret

EndProc VDMAD_Virtualize_Channel


BeginDoc
;******************************************************************************
;
;   VDMAD_Disable_Translation
;
;   DESCRIPTION:    This service disables the automatic translation done for
;		    the standard DMA channels.	It is necessary, if a V86 app
;		    or driver, or a PM app uses the DMA services thru INT 4Bh
;		    to determine actual physical addresses for DMA transfers.
;		    A disable count is maintained, so a matching call to
;		    VDMAD_Enable_Translation is required for each call to this
;		    service to re-enable translation.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    Carry clear
;			automatic translation is disable for the channel
;		    Carry set
;			the disable count overflowed
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Disable_Translation, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	push	ebx
	push	ecx
	add	ebx, [VDMAD_CB_Offset]	; point to controller 1 data in CB
	mov	ecx, [eax.channel_num]
	bts	[ebx.DMA_disabled], ecx ; set disabled flag
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short dt_ctrl_1 	;   N:
	sub	cl, 4			;   Y:
	add	ebx, SIZE DMA_Controller_State	; point to controller 2 data
dt_ctrl_1:
	lea	ebx, [ecx*4][ebx.DMA_disable_cnt]
	mov	ecx, [ebx]
	add	ecx, 1
	jc	SHORT disable_exit
	mov	[ebx], ecx
disable_exit:
	pop	ecx
	pop	ebx
	ret

EndProc VDMAD_Disable_Translation


BeginDoc
;******************************************************************************
;
;   VDMAD_Enable_Translation
;
;   DESCRIPTION:    This decrements the disable count associated with a
;		    standard DMA channel.  If the disable count goes to 0, then
;		    automatic translation is re-enabled.
;		    See VDMAD_Disable_Translation for further information.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    Carry clear
;			service completed successfully
;			Z-flag clear, if automatic translation is re-enabled
;		    Carry set
;			attempt to enable when translation already enabled
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Enable_Translation, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	pushad
	add	ebx, [VDMAD_CB_Offset]	; point to controller 1 data in CB
	mov	esi, ebx
	mov	ecx, [eax.channel_num]
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short et_ctrl_1 	;   N:
	sub	cl, 4			;   Y:
	add	ebx, SIZE DMA_Controller_State	; point to controller 2 data
et_ctrl_1:
	lea	ebx, [ecx*4][ebx.DMA_disable_cnt]
	mov	ecx, [ebx]
	sub	ecx, 1
	jc	SHORT enable_exit
	mov	[ebx], ecx
	jnz	SHORT enable_exit	; jump if still disabled
	mov	ecx, [eax.channel_num]
	btr	[esi.DMA_disabled], ecx ; clear disabled bit
	clc
enable_exit:
	popad
	ret

EndProc VDMAD_Enable_Translation

BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Region_Info
;
;   DESCRIPTION:    Get information about the current region assigned to a DMA
;		    handle.  This information can be used by a handler to
;		    call following services:
;
;			VDMAD_Unlock_DMA_Region
;			VDMAD_Release_Buffer
;			VDMAD_Copy_To_Buffer
;			VDMAD_Copy_From_Buffer
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    BL	= buffer id
;		    BH	= pages locked (0 = FALSE, else TRUE)
;		    ESI = region linear
;		    ECX = size in bytes
;
;   USES:	    EBX, ECX, ESI
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Region_Info, SERVICE

	Validate_DMA_Handle
	movzx	ebx, [eax.buffer_id]
	mov	bh, [eax.locked_pages]
	mov	esi, [eax.region_base]
	mov	ecx, [eax.region_size]
	ret

EndProc VDMAD_Get_Region_Info


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Region_Info
;
;   DESCRIPTION:    Set information about the current region assigned to a DMA
;		    handle.  This service must be called before calling
;		    VDMAD_Set_Phys_State.
;
;   ENTRY:	    EAX = DMA handle
;		    BL	= buffer id
;		    BH	= pages locked (0 = FALSE, else TRUE)
;		    ESI = region linear
;		    ECX = size in bytes
;		    EDX = physical address for transfer
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_Region_Info, SERVICE

	Validate_DMA_Handle
	mov	[eax.buffer_id], bl
	mov	[eax.locked_pages], bh
	mov	[eax.region_base], esi
	mov	[eax.region_size], ecx

IFDEF DEBUG
; record largest transfer request
	cmp	ecx, [eax.largest_request]
	jbe	SHORT setr_D00
	mov	[eax.largest_request], ecx
setr_D00:
ENDIF

	push	edx
; IF PS2, then extended command is used to program the address in 3 bytes
; (address lines A0-A23), else we have to adjust for AT word DMA scheme
;
	IF_MCA_JMP  <SHORT skip_buf_word_adjust>

	cmp	[eax.channel_num], 4	;Q: 2nd controller?
	jb	short rb_not_2nd	;   N: page = address lines A16-A23
					;      base = address lines A0-A15
	shr	edx, 1			;   Y: page = address lines A17-A23
					;     edx=  <0>A31..A17|A16..A1
	ror	edx, 16 		;		A16..A1|<0>A31..A17
	shl	dx, 1			;		A16..A1|A31..A17<0>
	ror	edx, 16 		;	    A31..A17<0>|A16..A1
					;      base = address lines A1-A16
rb_not_2nd:
skip_buf_word_adjust:

	mov	[eax.xfer_base], dx
	shr	edx, 16
	mov	[eax.xfer_page], dx
	pop	edx
	ret

EndProc VDMAD_Set_Region_Info


BeginDoc
;******************************************************************************
;
;   VDMAD_Get_Virt_State
;
;   DESCRIPTION:
;	This service allows a channel owner to determine the current virtual
;	state of the channel.  The virtual state consists of all the
;	information necessary to physically program the DMA channel for a
;	DMA transfer (linear address of target region, byte length of region,
;	mode of transfer, and state of mask bit and software request bit)
;	This state information reflects how the VM thinks the hardware is
;	currently programmed.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;
;   EXIT:	    If translation is enabled
;		      ESI = high linear address of the user's DMA region
;			  (high linear is used so that the DMA can proceed
;			   even if a different VM is actually running at the
;			   time of the transfer)
;		    Else
;		      ESI = physical byte address programmed (shifted left 1,
;			    for word ports)
;		    ECX = count in bytes
;		    DL	= mode (same as 8042 mode byte with channel # removed
;				and DMA_masked & DMA_requested set as
;				appropriate:
;				    DMA_masked	    channel masked and not ready
;						    for a transfer
;				    DMA_requested   software request flag set)
;		    DH	= extended mode (ignored on non-PS2 machines that don't
;					 have extended DMA capabilities)
;
;   USES:	    ESI, ECX, EDX, flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_Virt_State, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	mov	esi, ebx
	add	esi, [VDMAD_CB_Offset]	; point to controller 1 data in CB
	mov	ecx, [eax.channel_num]
	push	eax
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short gvs_ctrl_1	;   N:
	sub	cl, 4			;   Y:
	add	esi, SIZE DMA_Controller_State	; point to controller 2 data
gvs_ctrl_1:
	movzx	edx, [ecx][esi.DMA_modes]
	and	dl, NOT DMA_chan_sel

	mov	dh, [ecx][esi.DMA_Ext_Modes]

IFDEF allow_partial_virtualization
	bt	[esi.DMA_mode_set], cx	;Q: mode been set?
	jnc	short gvs_mode_set	;   Y:
	or	edx, 80000000h		;   N: set high bit in edx
gvs_mode_set:
ENDIF

	movzx	eax, [esi.DMA_mask]
	bt	eax, ecx		;Q: channel masked?
	jnc	short gvx_not_masked	;   N:
	or	dl, DMA_masked		;   Y: set flag bit
gvx_not_masked:
	movzx	eax, [esi.DMA_request]
	bt	eax, ecx		;Q: channel requested?
	jnc	short gvx_not_requested ;   N:
	or	dl, DMA_requested	;   Y: set flag bit
gvx_not_requested:
	mov	eax, [ecx*4][esi.DMA_adrs]
	cmp	[ecx*4][esi.DMA_disable_cnt], 0 ;Q: translation disabled?
	mov	ecx, [ecx*4][esi.DMA_counts]
	mov	esi, eax
	jne	short gvx_ret_phys	;   Y: return phys address (esi)

	add	esi, [ebx.CB_High_Linear]

gvx_ret_phys:
	test	dh, _16_bit_xfer	;Q: word transfer on this channel?
	jz	short gvx_byte_cnt	;   N:
	shl	ecx, 1			;   Y: convert word cnt to byte cnt
	inc	ecx
gvx_byte_cnt:
	inc	ecx			; return actual # of bytes

	pop	eax

	; dump these in opposite order, because .S displays them backwards
	DMA_Q_OUT "      cnt=#eax  mode=#bx", ecx, edx
	DMA_Q_OUT "Get Virt State chn=#al  adr=#ebx", [eax.channel_num], esi
	ret

EndProc VDMAD_Get_Virt_State


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Virt_State
;
;   DESCRIPTION:
;	Modify the virtual state of a DMA channel.  This is service is used
;	when a channel owner wants to change the virtual state of a channel
;	from how the VM programmed it.	This might be used to split a DMA
;	request into smaller pieces, etc.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;		    If translation is enabled
;		      ESI = high linear address of the user's DMA region
;			  (high linear is used so that the DMA can proceed
;			   even if a different VM is actually running at the
;			   time of the transfer)
;		    Else
;		      ESI = physical byte address programmed (shifted left 1,
;			    for word ports)
;		    ECX = count in bytes
;		    DL	= mode (same as 8042 mode byte with channel # removed
;				and DMA_masked & DMA_requested set as
;				appropriate:
;				    DMA_masked	    channel masked and not ready
;						    for a transfer
;				    DMA_requested   software request flag set)
;		    DH	= extended mode (ignored on non-PS2 machines that don't
;					 have extended DMA capabilities)
;
;   EXIT:	    NOTHING
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_Virt_State, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	pushad
	; dump these in opposite order, because .S displays them backwards
	DMA_Q_OUT "      cnt=#eax  mode=#bx", ecx, edx
	DMA_Q_OUT "Set Virt State chn=#al  adr=#ebx", [eax.channel_num], esi
	mov	edi, ebx
	add	edi, [VDMAD_CB_Offset]	; point to controller 1 data in CB
	mov	ecx, [eax.channel_num]
	cmp	cl, 4			;Q: channel of 2nd controller?
	jb	short svs_ctrl_1	;   N:
	sub	cl, 4			;   Y:
	add	edi, SIZE DMA_Controller_State	; point to controller 2 data
svs_ctrl_1:

;
; update mask flag
;
	movzx	eax, [edi.DMA_mask]
	test	dl, DMA_masked		;Q: channel masked?
	jz	short svs_not_masked	;   N:
	bts	eax, ecx
	jmp	short svs_upd_mask
svs_not_masked:
	btr	eax, ecx
svs_upd_mask:
	mov	[edi.DMA_mask], al

;
; update requested flag
;
	movzx	eax, [edi.DMA_request]
	test	dl, DMA_requested	;Q: channel requested?
	jz	short svs_not_reqd	;   N:
	bts	eax, ecx
	jmp	short svs_upd_req
svs_not_reqd:
	btr	eax, ecx
svs_upd_req:
	mov	[edi.DMA_request], al

;
; update mode bytes
;

IFDEF allow_partial_virtualization
	test	edx, 80000000h		;Q: set mode?
	jnz	short skip_virt_mode	;   N:
	btr	[edi.DMA_mode_set], cx	; indicate that mode has been set
ENDIF

	and	dl, NOT DMA_chan_sel
	mov	[ecx][edi.DMA_modes], dl
	mov	[ecx][edi.DMA_Ext_Modes], dh
skip_virt_mode:

;
; update address
;
	mov	eax, [esp.Pushad_ESI]	; high linear address
	cmp	[ecx*4][edi.DMA_disable_cnt], 0 ;Q: translation disabled?
	jne	short svx_set_phys	;   Y: high linear bias not included
	sub	eax, [ebx.CB_High_Linear] ; N: remove high linear bias
svx_set_phys:
	mov	[ecx*4][edi.DMA_adrs], eax

;
; update count
;
	mov	eax, [esp.Pushad_ECX]	; byte length
	dec	eax
	test	dh, _16_bit_xfer	;Q: word transfer on this channel?
	jz	short svx_byte_cnt	;   N:
	shl	eax, 1			;   Y: convert byte cnt to word cnt
svx_byte_cnt:
	mov	[ecx*4][edi.DMA_counts], eax

	popad
	ret

EndProc VDMAD_Set_Virt_State


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_Phys_State
;
;   DESCRIPTION:
;	This service programs the DMA controller state for a channel.  All
;	that it needs to know is the desired mode.  The location and size
;	of the buffer is taken from the information passed to the service
;	VDMAD_Set_Region_Info which must be called previously.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;		    DL	= mode
;		    DH	= extended mode
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_Phys_State, SERVICE

	pushad

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

IFDEF DEBUG
	; dump these in opposite order, because .S displays them backwards
	mov	ebx, [eax.region_size]
	dec	ebx
	DMA_Q_OUT "      cnt=#eax  mode=#bl", ebx, edx
	movzx	ebx, [eax.xfer_page]
	shl	ebx, 16
	mov	bx, [eax.xfer_base]
	DMA_Q_OUT "Set Phys State chn=#al  adr=#ebx", [eax.channel_num]
ENDIF
	mov	edi, eax		; EDI is now the DMA handle
	mov	ebx, edx		; BL = new mode & flags, BH = extended mode

IFDEF DEBUG
	cmp	[edi.locked_pages], 0	;Q: region locked?
	jne	short region_ok 	;   Y:
	cmp	[edi.buffer_id], 0	;Q: buffer assigned?
	jne	short region_ok 	;   Y:
	Debug_Out "VDMAD: Attempted to start DMA without locking a region"
	Fatal_Error

region_ok:
ENDIF

	movzx	eax, [edi.xfer_page]
	mov	edx, [edi.channel_num]
	movzx	edx, [edx.page_ports]
	out	dx, al			; write new page value

	IF_NOT_EISA_JMP <SHORT no_high_page>
	add	dx, DMA_E_Hoff
	mov	al, ah
	out	dx, al			; write EISA hi page byte
no_high_page:

	movzx	eax, [edi.xfer_base]
	mov	edx, [edi.channel_num]
	movzx	edx, [edx.base_ports]
	call	Out_DMA_Word		; write new base value

	mov	eax, [edi.region_size]
	dec	eax
	mov	edx, [edi.channel_num]
	cmp	edx, 4			;Q: controller 2?
	jb	short sp_byte_xfer	;   N: use # of bytes
	shr	eax, 1			;   Y: convert to # of words
sp_byte_xfer:
	movzx	edx, [edx.count_ports]
	call	Out_DMA_Word		; write new count value

	IF_NOT_EISA_JMP <SHORT no_high_cnt>
	add	dx, DMA_E_Hoff
	shr	eax, 16
	out	dx, al			; write EISA hi count byte
no_high_cnt:

IFDEF allow_partial_virtualization
	test	ebx, 80000000h		;Q: set mode?
	jnz	short skip_phys_mode	;   N:
ENDIF

	mov	eax, [edi.channel_num]
	mov	edx, eax
	and	al, DMA_chan_sel
	and	bl, NOT DMA_chan_sel
	or	al, bl			; add channel # to MODE command
	Cntrl_Const_Offset edx
	movzx	edx, [edx+DMA_consts.DMA_mode_port]
	out	dx, al			; write new mode

skip_phys_mode:
	popad
	ret

EndProc VDMAD_Set_Phys_State


BeginDoc
;******************************************************************************
;
;   VDMAD_Mask_Channel
;
;   DESCRIPTION:
;	This service physically masks a channel so that it will not attempt
;	any further DMA transfers.
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Mask_Channel, SERVICE


	Validate_DMA_Handle
	pushad

	pushfd
	cli
;
; reset filter proc
;
	mov	ebx, [eax.owner_VM]
	or	ebx, ebx
	jz	short filter_already_reset

	Assert_VM_Handle ebx		; Debugging
	push	eax
	mov	eax, [VDMAD_Hw_Int_Next]
	mov	[VDMAD_Hw_Int_Filter], eax
	pop	eax
	mov	[VDMAD_Check_TC], OFFSET32 VDMAD_NoCheck


	add	ebx, [VDMAD_CB_Offset]
	xor	esi, esi
	xchg	[ebx.DMA_TC_event], esi
	VMMCall Cancel_Global_Event
filter_already_reset:
	popfd

	Validate_DMA_Handle
	DMA_Q_OUT 'Mask channel #al', [eax.channel_num]

	mov	[eax.owner_VM], 0
	mov	eax, [eax.channel_num]
	mov	edx, eax
	Cntrl_Const_Offset edx
	and	al, DMA_chan_sel
	or	al, 100b
	movzx	edx, [edx+DMA_consts.DMA_single_mask_p]
	out	dx, al			; write mask command
	IO_Delay

	popad
	ret

EndProc VDMAD_Mask_Channel


BeginDoc
;******************************************************************************
;
;   VDMAD_UnMask_Channel
;
;   DESCRIPTION:
;	This service physically unmasks a channel so that DMA transfers can
;	proceed.
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;
;   EXIT:	    nothing
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_UnMask_Channel, SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	pushad
	DMA_Q_OUT 'Unmask channel #al', [eax.channel_num]
	mov	edx, eax
	in	al, DMA1_Status 	; clear the status bytes
	in	al, DMA2_Status
	xor	al, al
	mov	[VDMAD_DMA1_status], al
	mov	[VDMAD_DMA2_status], al

	mov	[edx.owner_VM], ebx
	mov	eax, [edx.channel_num]
	mov	edx, eax
	Cntrl_Const_Offset edx
	and	al, DMA_chan_sel
	push	edx
	movzx	edx, [edx+DMA_consts.DMA_single_mask_p]
	out	dx, al			; write mask command
	IO_Delay
	pop	edx

;
; assign filter proc
;
	mov	esi, [edx+DMA_consts.DMA_HW_TC_checker]
	mov	[VDMAD_Hw_Int_Filter], esi
	DMA_Q_OUT "assign filter proc ?eax", esi

	test	[DMA_Buffer_State], Buffer_In_Use   ;Q: buffer in use?
	jz	short no_buffer_no_TC_chk	    ;	N:
	mov	esi, [edx+DMA_consts.DMA_TC_checker];	Y: check TC on DMA I/O
	mov	[VDMAD_Check_TC], esi
no_buffer_no_TC_chk:

	popad
	ret

EndProc VDMAD_UnMask_Channel


;******************************************************************************
;
;   Out_DMA_Word
;
;   DESCRIPTION:    Output a word to a DMA port as 2 bytes, low then high.
;		    First the routine resets the DMA controller's flipflop
;		    so that the controller will be ready to receive the low
;		    byte first.
;
;   ENTRY:	    AX is word to send
;		    DX is port address
;		    EDI = DMA handle
;
;   EXIT:
;
;   USES:	    flags
;
;==============================================================================
BeginProc Out_DMA_Word

	push	eax
	push	edx

	mov	eax, [edi.channel_num]
	Cntrl_Const_Offset eax
	movzx	edx, [eax+DMA_consts.DMA_flipflop_port]
	out	dx, al			; clear the flip flop to write low byte
	IO_Delay

	pop	edx
	pop	eax

	out	dx, al			; low byte first
	IO_Delay
	xchg	al, ah
	out	dx, al			; now high byte
	xchg	al, ah			; restore AX

	ret

EndProc Out_DMA_Word


BeginDoc
;******************************************************************************
;
;   VDMAD_Lock_DMA_Region
;
;   DESCRIPTION:
;	This service attempts to lock a region of memory for a DMA transfer.
;	It is called before a DMA transfer is started (before the physical
;	state is set for a channel and before it is unmasked.)
;
;	It first verifies that the region is mapped to contiguous pages of
;	physical memory.
;
;	Then it determines whether the region will result in a DMA bank (page)
;	wrap
;	    On AT class machines each channel has a base address register
;	    and a page address register.  The base address register is
;	    incremented after each byte or word transfered.  If the
;	    increment of this 16 bit register results in the roll over
;	    from FFFFh to 0, then the transfer wraps to the start of
;	    the DMA bank because the page register is not updated.
;	    Normally DOS watches for this condition and adjusts INT 13h
;	    parameters to split transfers to avoid this wrap, but DOS
;	    doesn't know anything about the difference between linear
;	    and physical addresses under Windows/386, so VDMAD checks
;	    again to prevent wrap from occurring undesirably.
;
;	If all of these checks are okay, then the service calls the memory
;	manager to lock the physical pages.
;
;   NOTE:
;	This routine does not check to see if the region is within some
;	physical maximum constraint.  If the region is lockable, then it locks
;	the memory, and it is up to the caller to check to see if the physical
;	region is acceptable.  If the region is not acceptable, then the caller
;	should unlock the region and perform a buffered DMA transfer.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;		    DL	= 1b, if region must be aligned on 64K page boundary
;			= 10b, if region must be aligned on 128K page boundary
;
;   EXIT:	    Carry set, if lock failed
;			ECX = # of bytes that are lockable in the region
;			      (starting from ESI)
;			AL = 1 (DMA_Not_Contiguous), region not contiguous
;			   = 2 (DMA_Not_Aligned), region crossed physical
;			       alignment boundary
;			   = 3 (DMA_Lock_Failed), unable to lock pages
;		    ELSE
;			EDX = physical address of the DMA region
;			the region has been locked
;
;   USES:	    EAX, ECX, EDX, Flags
;
;
;
;==============================================================================
EndDoc
BeginProc VDMAD_Lock_DMA_Region, SERVICE

	pushad
	mov	ebp, esp
	mov	byte ptr [ebp.Pushad_ESP], DMA_Not_Contiguous
					; use for error code save
	mov	ebx, ecx
IFDEF DEBUG
	test	edx, 11b		;Q: alignment constraint?
	jz	short xfer_ok		;   N:
	mov	edx, esi
	movzx	edx, dx
	add	edx, ecx
	test	[ebp.Pushad_EDX], 10b
	jz	short xfer_not_128K
	shr	edx, 1			; check wrap in 128Kb
xfer_not_128K:
	dec	edx
	cmp	edx, 10000h		;Q: transfer will result in a wrap?
	jb	short xfer_ok		;   N:
	Debug_Out "VDMAD:  DMA transfer will result in a wrap"
xfer_ok:
ENDIF
	dec	ebx
	add	ebx, esi
	shr	esi, 12 		; start page # (linear)
	shr	ebx, 12 		; last page #
	sub	ebx, esi
	inc	ebx			; # of pages in region
	mov	edi, esp
	mov	ecx, ebx		; # of dwords for page table copy
	shl	ecx, 2			; # of bytes for page table copy
	sub	esp, ecx
	push	edi			; save original esp
	sub	edi, ecx		; edi -> page table copy area
	push	esi			; save starting page
	push	ebx			; and page count
	VMMCall _CopyPageTable <esi, ebx, edi, 0>
	xor	ecx, ecx		; # of lockable bytes
	dec	ebx
	jz	short single_page

	mov	esi, edi
	mov	edx, [ebp.Pushad_ESI]
	and	edx, 0FFFh
	mov	ecx, 1000h
	sub	ecx, edx		; bytes lockable in the first page
	cld
	lodsd
	shr	eax, 12 		; convert to page #
	mov	edx, eax		; EDX = last page temp variable

	or	edi, -1
	test	[ebp.Pushad_EDX], 11b	;Q: alignment constraint?
	jz	short skip_wrap_mask_bld ;  N:
;
; Setup DMA page overflow mask
;	byte transfers of controller 1 put address lines A16-A23 in the DMA
;	page register, so if the bottom 4 bits of a page # are clear, then
;	the region requires the DMA page to increment, so the lock fails!
;	word transfers of controller 2 put address lines A17-A23 in the DMA
;	page register, so if the bottom 5 bits of a page # are clear, then
;	the lock fails.  EDI is set as the mask 0Fh for byte channels and
;	1Fh for word channels.
;
	test	[ebp.Pushad_EDX], 10b	;Q: 128K alignment?
	mov	edi, 0Fh		;   N: DMA page overflow mask = 4 bits
	jz	short align_64K
	add	edi, 10h		;   Y: DMA page overflow mask = 5 bits
align_64K:
skip_wrap_mask_bld:

chk_pages:
	lodsd
	shr	eax, 12

	inc	edx
	cmp	eax, edx		;Q: next page after last?
	jne	short lk_failed_1	;   N:

	test	eax, edi		;Q: overflowed DMA page?
	jz	short lk_failed_2	;   Y: can't DMA into this region
skip_wrap_check:
	add	ecx, 1000h		;   N: increment lockable bytes count
	dec	ebx
	jnz	chk_pages

single_page:
	pop	ebx			; retrieve the saved page count
	pop	edi			; retrieve the saved linear page #
	VMMCall _LinPageLock <edi, ebx, 0>  ; lock with linear page #
	or	eax, eax
	jz	short lk_failed_3

	pop	edi			; get original esp
	pop	edx			; get first page table entry
	mov	esp, edi		; restore esp
	and	edx, NOT (P_SIZE-1)
	mov	esi, [ebp.Pushad_ESI]
	and	esi, P_SIZE - 1 	; offset of start within first page
	or	edx, esi		; physical address of start of region

	mov	[ebp.Pushad_EDX], edx
	popad
	clc				; lock successful
	ret

lk_failed_2:
.errnz DMA_Not_Aligned - DMA_Not_Contiguous - 1
	inc	byte ptr [ebp.Pushad_ESP]	; error code = 2
lk_failed_1:					; error code = 1
	add	esp, 8			; discard saved pg cnt & linear pg #
	jmp	short lk_failed

lk_failed_3:
	mov	byte ptr [ebp.Pushad_ESP], DMA_Lock_Failed
	xor	ecx, ecx			; say that no bytes were lockable
						; since we don't know!
lk_failed:
	pop	esp			; restore stack
	movzx	eax, byte ptr [ebp.Pushad_ESP]
	DMA_Q_OUT "DMA lock failed #al"
	mov	[ebp.Pushad_ECX], ecx	; save lockable bytes for return
	mov	[ebp.Pushad_EAX], eax	; return error code
	popad
	stc
	ret

EndProc VDMAD_Lock_DMA_Region


BeginDoc
;******************************************************************************
;
;   VDMAD_Unlock_DMA_Region
;
;   DESCRIPTION:
;	This service unlocks the DMA region previously locked to a channel.
;	It is called after a DMA transfer is complete and the channel has
;	been masked.  So that the controller will not attempt any further
;	transfers to the programmed address.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear
;			memory unlocked
;		    Carry set
;			error
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Unlock_DMA_Region, SERVICE

	pushad
	dec	ecx
	add	ecx, esi
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region
	VMMCall _LinPageUnLock <esi, ecx, 0>
	sub	eax, 1			; set carry, if error
	popad
	ret

EndProc VDMAD_Unlock_DMA_Region



;------------------------------------------------------------------------------
IFDEF rpMemTst

VxD_DATA_SEG
Test_DDS Extended_DDS_Struc <28023h, 40335h, 0, 0, 28, 0>
	dd	32 DUP(?)
VxD_DATA_ENDS

BeginProc VDMAD_Terminate
	int 1
	xor	eax, eax
	mov	edi, OFFSET32 Test_DDS
	VxDCall VDMAD_Scatter_Lock
	jc	short rp_lock_failed
	VxDCall VDMAD_Scatter_Unlock
rp_lock_failed:
	clc
	ret
EndProc VDMAD_Terminate
ENDIF
;------------------------------------------------------------------------------


VxD_DATA_SEG
Scatter_Lock_Proc   dd	OFFSET32 VDMAD_Scatter_Page_Lock
VxD_DATA_ENDS

;******************************************************************************
;
;   VDMAD_Scatter_Page_Lock
;
;   DESCRIPTION:    Used to process list of page table entries.  It attempts
;		    to lock a page.
;
;   ENTRY:	    EAX -> entry in DDS table for current page
;		    EBX = VM Handle
;		    ESI = linear page #
;
;   EXIT:	    Carry set, if lock failed
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Scatter_Page_Lock

	Assert_VM_Handle ebx		; Debugging
	push	eax
	push	ecx
	VMMCall _LinPageLock <esi, 1, 0> ; lock page
	sub	eax, 1
	pop	ecx
	pop	eax
	ret

EndProc VDMAD_Scatter_Page_Lock

;******************************************************************************
;
;   VDMAD_Scatter_Page_Lock_w_holes
;
;   DESCRIPTION:    Used to process list of page table entries.  It first
;		    checks to see if the page is currently present, if not
;		    then it zeros the page entry in the DDS table, else it
;		    attempts to lock a page.
;
;   ENTRY:	    EAX -> entry in DDS table for current page
;		    EBX = VM Handle
;		    ESI = linear page #
;
;   EXIT:	    Carry set, if lock failed
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Scatter_Page_Lock_w_holes

	Assert_VM_Handle ebx		; Debugging
	test	byte ptr [eax], P_PRES	;Q: page present?
	jz	short spl_not_pres	;   N:
	push	eax			;   Y: attempt to lock it
	push	ecx
	VMMCall _LinPageLock <esi, 1, 0> ; lock page
	sub	eax, 1
	pop	ecx
	pop	eax
	ret

spl_not_pres:
	mov	dword ptr [eax], 0
	clc
	ret

EndProc VDMAD_Scatter_Page_Lock_w_holes


BeginDoc
;******************************************************************************
;
;   VDMAD_Scatter_Lock
;
;   DESCRIPTION:    This service attempts to lock all pages mapped to a DMA
;		    region and return the actual physical addresses of the
;		    pages.
;
;   ENTRY:	    EBX = VM Handle
;		    AL = 0, if the DDS table should be filled with physical
;			    addresses and sizes of the physical regions that
;			    make up the DMA region
;		    AL = 1, if the DDS table should be filled with the actual
;			    page table entries
;		    AL = 3, if the DDS table should be filled with the actual
;			    page table entries and not present pages should not
;			    be locked
;		    EDI -> extended DDS (DMA Descriptor Structure)
;
;   EXIT:	    Carry clear
;			Z-flag set
;			    whole region was locked successfully
;			Z-flag clear
;			    partial region locked
;		    Carry set
;			nothing locked
;
;		    EDX = # of table entries needed to describe whole region
;		    DDS_size = # of bytes locked
;		    DDS table has been updated
;		    if request was for page table copy (AL=1 OR 3), then
;			ESI = offset into first page for start of the region
;
;   USES:	    EDX, ESI, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Scatter_Lock, SERVICE

	Assert_VM_Handle ebx		; Debugging

	pushad
	movzx	eax, [edi.DDS_sel]
	or	eax, eax
	jz	short no_selector
	VMMcall _SelectorMapFlat, <ebx, eax, 0>

no_selector:
	mov	esi, [edi.DDS_linear]
	add	esi, eax
	mov	edx, esi		; edx = base linear address
	mov	ecx, [edi.DDS_size]
	dec	ecx
	add	ecx, esi
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region

	cmp	byte ptr [esp.Pushad_EAX], 0 ;Q: copy page table request?
	je	phys_region_list	;	  N:

	mov	[Scatter_Lock_Proc], OFFSET32 VDMAD_Scatter_Page_Lock
	cmp	byte ptr [esp.Pushad_EAX], 3 ;Q: allow np pages?
	jne	short all_present_req	;	N:
	mov	[Scatter_Lock_Proc], OFFSET32 VDMAD_Scatter_Page_Lock_w_holes
all_present_req:
	mov	[esp.Pushad_EDX], ecx	; save # of entries needed
	cmp	cx, [edi.DDS_avail]	;Q: enough table entries?
	jbe	short copy_full_region	;   Y:
	movzx	ecx, [edi.DDS_avail]	;   N: copy available entries
copy_full_region:
	lea	eax, [edi+(SIZE Extended_DDS_Struc)]
	push	ecx			; and page count
	VMMCall _CopyPageTable <esi, ecx, eax, 0>
	pop	ecx

	mov	[edi.DDS_used], cx
	lea	eax, [edi+(SIZE Extended_DDS_Struc)]

lock_pages:
	and	word ptr [eax], 01111000000000001b  ; clear bits 1-11
	call	[Scatter_Lock_Proc]
	jc	short single_page_lock_failed
	inc	esi
	add	eax, 4			; point to next entry
	loop	lock_pages
single_page_lock_failed:
	sub	[edi.DDS_used], cx	; record actual # used
	jz	short nothing_locked
	mov	esi, [edi.DDS_linear]
	and	esi, P_SIZE - 1
	mov	[esp.Pushad_ESI], esi
	movzx	ecx, [edi.DDS_used]	; # of pages locked
	shl	ecx, 12 		; # of bytes locked
	sub	ecx, esi		; minus bytes before start of region
	xor	al, al			;
	cmp	ecx, [edi.DDS_size]	;Q: locked whole region?
	jae	short scatter_locked_all;   Y: return C clear & Z set
	mov	[edi.DDS_size], ecx	;   N: return size of locked region
	or	al, 1			;	      Z & C clear
scatter_locked_all:
	or	al, al
	jmp	scatter_lock_exit

nothing_locked:
	mov	[edi.DDS_size], 0
	stc
	jmp	scatter_lock_exit

phys_region_list:
	mov	eax, ecx		; # of dwords for page table copy
	shl	eax, 2			; # of bytes for page table copy
	mov	ebp, esp
	sub	esp, eax		; reserve space for page table
	push	ebp			; save original esp
	sub	ebp, eax		; ebp -> page table copy area
	push	0
	push	edi			; save ptr to DDS
	push	ecx			; and page count
	push	edx
	VMMCall _CopyPageTable <esi, ecx, ebp, 0>
	xchg	esi, ebp		; esi -> page table copy, ebp=lin pg #
	pop	ebx			; ebx = linear base of region
	and	ebx, P_SIZE - 1 	;	base in first page
	mov	[edi.DDS_used], 0
	cmp	[edi.DDS_avail], 0	;Q: any entries in the DDS table?
	je	phys_lock_failed	;   N:
	add	edi, SIZE Extended_DDS_Struc
	mov	eax, [esi]
	and	eax, NOT (P_SIZE - 1)
	mov	ecx, eax
	or	eax, ebx		; eax = physical address of start
	cld
	stosd
	neg	ebx
	mov	[edi], ebx		; set first size with negative # of bytes
	mov	[esp+8], ebx		; (bytes not in region)
	mov	ebx, ecx		; ebx = phys page address of first page
	pop	ecx
;
; copy regions into DDS table
;
lock_regions:
	push	ecx

	cld
	lodsd
	and	eax, NOT (P_SIZE - 1)	; eax = phys page address
	cmp	eax, ebx		;Q: consecutive page?
	je	short add_page		;   Y: add page size to entry size
	add	edi, 4			;   N: init next entry
	mov	ebx, [esp+4]
	movzx	ecx, [ebx.DDS_used]
	inc	ecx
	mov	[ebx.DDS_used], cx	; count used entry
	cmp	cx, [ebx.DDS_avail]	;Q: table filled?
	je	short count_regions	;   Y: count rest of regions
	stosd				; store physical address
	mov	dword ptr [edi], 0
	mov	ebx, eax
add_page:

	VMMCall _LinPageLock <ebp, 1, 0> ; lock page
	or	eax, eax		;Q: lock failed
	jz	short phys_lock_failed	;   Y:

	add	dword ptr [esp+8], P_SIZE ; inc total locked
	add	dword ptr [edi], P_SIZE ; inc region size
	add	ebx, P_SIZE		; next consecutive page address
	inc	ebp			; next linear page #
	pop	ecx
	loop	lock_regions
	mov	ecx, edi
	pop	edi
	movzx	edx, [edi.DDS_used]
	inc	edx
	mov	[edi.DDS_used], dx	; count used entry
	pop	eax			; discard lock count on stack
	sub	eax, [edi.DDS_size]	; extra bytes locked
	sub	[ecx], eax		; subtract extra from last region
	xor	eax, eax		; clear carry & set Z
	jmp	short scatter_phys_done

phys_lock_failed:
	mov	ecx, [esp+4]		; get DDS ptr
	movzx	ecx, [ecx.DDS_used]

count_regions:
	mov	edx, ecx		; # of entries used
	pop	ecx			; # of pages left
	jmp	short new_extra_region

next_extra_page:
	add	ebx, P_SIZE
	cld
	lodsd
	and	eax, NOT (P_SIZE - 1)	; eax = phys page address
	cmp	eax, ebx		;Q: consecutive page?
	je	short same_region
new_extra_region:
	inc	edx
	mov	ebx, eax
same_region:
	loop	next_extra_page

	cmp	dword ptr [esp+4], 0	;Q: anything locked?
	jg	short phys_partial	;   Y:
	pop	edi
	add	esp, 4
	pop	ebp
	mov	esp, ebp
	jmp	nothing_locked

phys_partial:
	pop	edi
	or	al, 1			; Z clear & C clear
	pop	[edi.DDS_size]		; return actual size locked

scatter_phys_done:
	pop	ebp
	mov	esp, ebp
	mov	[esp.Pushad_EDX], edx	; save # of entries needed

scatter_lock_exit:
	popad
	ret
EndProc VDMAD_Scatter_Lock


BeginDoc
;******************************************************************************
;
;   VDMAD_Scatter_Unlock
;
;   DESCRIPTION:    This service attempts to unlock all pages locked by a
;		    previous call to VDMAD_Scatter_Lock
;
;   ENTRY:	    EBX = VM Handle
;		    AL = 0, if the DDS table should be filled with physical
;			    addresses and sizes of the physical regions that
;			    make up the DMA region
;		    AL = 1, if the DDS table should be filled with the actual
;			    page table entries
;		    AL = 3, if the DDS table should be filled with the actual
;			    page table entries and not present pages should not
;			    be locked
;		    EDI -> extended DDS (DMA Descriptor Structure)
;			(The table at the end of the DDS is not required, so
;			 it is not necessary to maintain the table for this
;			 unlock call.)
;
;   EXIT:	    Carry clear
;			Lock counts have been decremented.  If no other VxD's
;			had pages locked, then the pages have been unlocked.
;		    Carry set
;			The memory was not locked.
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Scatter_Unlock, SERVICE

	Assert_VM_Handle ebx		; Debugging

	pushad
	movzx	eax, [edi.DDS_sel]
	or	eax, eax
	jz	short vsu_no_selector
	VMMcall _SelectorMapFlat, <ebx, eax, 0>

vsu_no_selector:
	mov	esi, [edi.DDS_linear]
	add	esi, eax
	cmp	byte ptr [esp.Pushad_EAX], 3 ;Q: page table copy with np holes?
	je	short unlock_scan	     ;	 Y:
	mov	ecx, [edi.DDS_size]
	dec	ecx
	add	ecx, esi
	shr	esi, 12 		; start page # (linear)
	shr	ecx, 12 		; last page #
	sub	ecx, esi
	inc	ecx			; # of pages in region
	VMMCall _LinPageUnLock <esi, ecx, 0>
	sub	eax, 1			; carry set, if failed (eax = 0)
su_exit:
	popad
	ret

unlock_scan:
	shr	esi, 12 		; start page # (linear)
	movzx	ecx, [edi.DDS_used]
	lea	edi, [edi+(SIZE Extended_DDS_Struc)]

unlock_lp:
	test	byte ptr [edi], P_PRES	;Q: page present?
	jz	short skip_unlock	;   N:
	push	ecx
	VMMCall _LinPageUnLock <esi, 1, 0>
	pop	ecx
skip_unlock:
	inc	esi
	add	edi, 4
	loop	unlock_lp
	clc
	jmp	su_exit

EndProc VDMAD_Scatter_Unlock


BeginDoc
;******************************************************************************
;
;   VDMAD_Request_Buffer
;
;   DESCRIPTION:    This device reserves the DMA buffer for a DMA transfer.
;
;   ENTRY:	    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear
;			EBX = buffer ID
;			EDX = the physical address of the buffer
;		    Carry set
;			AL = 5 (DMA_Buffer_Too_Small), region request is
;			       too large for buffer
;			   = 6 (DMA_Buffer_In_Use), buffer already in use
;
;
;   USES:	    EAX, EBX, ESI, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Request_Buffer, SERVICE

	push	ecx
	add	ecx, 0FFFh
	shr	ecx, 12 		; # of pages necessary for buffer
	cmp	ecx, [DMA_Buffer_Size]	;Q: VDMAD buffer big enough?
	pop	ecx
	jbe	short buffer_ok 	;   Y:

	mov	al, DMA_Buffer_Too_Small
	stc
	ret

buffer_ok:
	test	[DMA_Buffer_State], Buffer_In_Use   ;Q: already in use?
	jz	short buffer_available		    ;	N:

	mov	al, DMA_Buffer_In_Use
	stc
	ret

buffer_available:
	mov	[DMA_Buffer_State], Buffer_In_Use
	mov	edx, [DMA_Buffer_Phys]
	mov	ebx, 1
	clc
	ret

EndProc VDMAD_Request_Buffer


;******************************************************************************
;
;   VDMAD_Validate_Buffer
;
;   DESCRIPTION:    check to see that the DMA buffer is in use and assigned
;		    to the DMA channel
;
;   ENTRY:	    EBX = buffer id
;
;   EXIT:	    Carry set, if not valid
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Validate_Buffer

	test	[DMA_Buffer_State], Buffer_In_Use   ;Q: in use?
	jnz	short vb_chk_id 		    ;	Y:
	Debug_Out "VDMAD:  Buffer operation called when it isn't in use"
vb_error:
	stc
	jmp	short vb_exit

vb_chk_id:
	cmp	ebx, 1			;Q: valid id?
	jne	vb_error		;   N:
	clc
vb_exit:
	ret

EndProc VDMAD_Validate_Buffer


BeginDoc
;******************************************************************************
;
;   VDMAD_Release_Buffer
;
;   DESCRIPTION:
;	Release the VDMAD buffer assigned to a DMA channel from a previous
;	VDMAD_Request_Buffer call.  This routine exits from a critical section
;	and the DMA buffer will now be available for other users.  Any data
;	in the buffer is not automatically copied, so VDMAD_Copy_From_Buffer
;	must be called if the data is important.
;
;   ENTRY:	    EBX = Buffer id
;
;   EXIT:	    Carry clear
;			buffer released
;		    Carry set
;			bad ID
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Release_Buffer, SERVICE

	call	VDMAD_Validate_Buffer
	jc	SHORT release_exit

	mov	[DMA_Buffer_State], 0

	push	esi
	xor	esi, esi
	xchg	esi, [DMA_Buffer_Timeout]
	or	esi, esi		;Q: timeout scheduled?
	jz	short rb_no_timeout	;   N:
	VMMCall Cancel_Time_Out 	;   Y: cancel it
	DMA_Q_OUT "DMA buffer released"
rb_no_timeout:
	pop	esi

release_exit:
	ret

EndProc VDMAD_Release_Buffer


;******************************************************************************
;
;   Verify_Copy_Params
;
;   DESCRIPTION:    Verify parameters for VDMAD_Copy_To_Buffer and
;		    VDMAD_Copy_From_Buffer
;
;   ENTRY:	    EBX = buffer id
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			params okay
;		    Carry set
;			AL = error code
;
;   USES:
;
;==============================================================================
BeginProc Verify_Copy_Params

	cmp	ebx, 1
	je	short ct_valid_buffer
	mov	al, DMA_Invalid_Buffer
	stc
	ret

ct_valid_buffer:
	push	edi
	add	edi, ecx
	dec	edi
	shr	edi, 12
	cmp	edi, [DMA_Buffer_Size]
	pop	edi
	jb	short ct_size_valid
	mov	al, DMA_Copy_Out_Range
	stc
	ret

ct_size_valid:
	clc
	ret

EndProc Verify_Copy_Params


BeginDoc
;******************************************************************************
;
;   VDMAD_Copy_To_Buffer
;
;   DESCRIPTION:
;	This service allows another device to copy data into the VDMAD buffer
;	from the actual DMA region associated with the buffer.	This service
;	is called after VDMAD_Request_Buffer and before starting a memory
;	read transfer.
;
;   ENTRY:	    EBX = buffer id
;		    ESI = region linear
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			data copied from DMA region into buffer
;		    Carry set
;			AL = 0Ah (DMA_Invalid_Buffer) - invalid buffer
;				 id supplied
;			   = 0Bh (DMA_Copy_Out_Range) - (ESI + ECX) is
;				 greater than buffer size
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Copy_To_Buffer, SERVICE

	call	Verify_Copy_Params
	jc	SHORT copy_exit
	pushad
	add	edi, [DMA_Buffer_Linr]
	jmp	short transfer_region

EndProc VDMAD_Copy_To_Buffer

BeginDoc
;******************************************************************************
;
;   VDMAD_Copy_From_Buffer
;
;   DESCRIPTION:
;	This service allows another device to copy data from the VDMAD buffer
;	to the actual DMA region associated with the buffer.  This service
;	is called after VDMAD_Request_Buffer, after a memory write transfer
;	and before VDMAD_Release_Buffer.
;
;   ENTRY:	    EBX = buffer id
;		    ESI = region linear
;		    EDI = offset within buffer for start of copy
;		    ECX = size
;
;   EXIT:	    Carry clear
;			data copied from buffer into DMA region
;		    Carry set
;			AL = 0Ah (DMA_Invalid_Buffer) - invalid buffer
;				 id supplied
;			   = 0Bh (DMA_Copy_Out_Range) - (ESI + ECX) is
;				 greater than buffer size
;
;   USES:	    flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Copy_From_Buffer, SERVICE

	call	Verify_Copy_Params
	jc	SHORT copy_exit
	pushad
	xchg	esi, edi
	add	esi, [DMA_Buffer_Linr]

transfer_region:
	push	ecx
	and	ecx, 3			; # of extra bytes to transfer
	jecxz	short no_partial_dword_xfer ; jump if none
	cld
	rep movsb			; move extra bytes
no_partial_dword_xfer:
	pop	ecx
	shr	ecx, 2			; # of dwords to transfer
	rep movsd			; move rest of region as dwords
	popad
copy_exit:
	ret

EndProc VDMAD_Copy_From_Buffer


BeginDoc
;******************************************************************************
;
;   VDMAD_Get_EISA_Adr_Mode
;
;   DESCRIPTION:    Get EISA extended mode - the hardware doesn't allow for
;		    reading the extended mode for a channel, so VDMAD defaults
;		    to the ISA defaults (channels 0-3 are byte channels and
;		    5-7 are word channels with word addresses and counts)  An
;		    INI switch can specify an alternate setting.
;
;   ENTRY:	    EAX = Channel # (0..7) or
;			  DMA Handle
;
;   EXIT:	    CL = 0 - 8-bit I/O, with count in bytes
;		    CL = 1 - 16-bit I/O, with count in words and adr shifted
;		    CL = 2 - 32-bit I/O, with count in bytes
;		    CL = 3 - 16-bit I/O, with count in bytes
;
;   USES:	    ECX, Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Get_EISA_Adr_Mode, SERVICE

	push	eax
	cmp	eax, 7
	jbe	short get_w_ch_num

	Validate_DMA_Handle
	mov	eax, [eax.channel_num]

get_w_ch_num:
	mov	cl, [eax+DMA_EISA_Ext_Modes]	; EISA_Ext_Modes default
	and	cl, DMA_EM_Chan_Size		; to ISA values, so we can
	shr	cl, 2				; always allow this service

.errnz DMA_EM_Chan_Size xor 1100b	; assumming size is in bits 2 & 3
	pop	eax
	ret

EndProc VDMAD_Get_EISA_Adr_Mode


BeginDoc
;******************************************************************************
;
;   VDMAD_Set_EISA_Adr_Mode
;
;   DESCRIPTION:    Set EISA extended mode
;
;   ENTRY:	    EAX = Channel # (0..7) or
;			  DMA Handle
;		    CL = 0 - 8-bit I/O, with count in bytes
;		    CL = 1 - 16-bit I/O, with count in words and adr shifted
;		    CL = 2 - 32-bit I/O, with count in bytes
;		    CL = 3 - 16-bit I/O, with count in bytes
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
EndDoc
BeginProc VDMAD_Set_EISA_Adr_Mode, SERVICE

	IF_NOT_EISA_JMP <SHORT seam_exit>   ; don't change if not EISA machine

	push	eax
	push	ecx
	cmp	eax, 7
	jbe	short set_w_ch_num

	Validate_DMA_Handle

	mov	eax, [eax.channel_num]

set_w_ch_num:
	Trace_Out "VDMAD_Set_EISA_Adr_Mode #al, #cl"
	shl	cl, 2
.errnz DMA_EM_Chan_Size xor 1100b	; assumming size is in bits 2 & 3
	and	cl, DMA_EM_Chan_Size
	mov	[eax+DMA_EISA_Ext_Modes], cl

	pop	ecx
	pop	eax
seam_exit:
	ret

EndProc VDMAD_Set_EISA_Adr_Mode


;******************************************************************************
; routines to check and deal with DMA terminal counts at hardware int time
;******************************************************************************

;******************************************************************************
;
;   VDMAD_jmp_filter
;
;   DESCRIPTION:    indirect jump to current VDMAD filter or
;		    to the next Hw Int filter registered through VPICD
;
;		    Possible VDMAD filters
;			VDMAD_HW_Check_TC1  for controller 1, copy from VDMAD
;					    buffer to VM's region if necessary
;					    after DMA completed
;			VDMAD_HW_Check_TC2  same as VDMAD_HW_Check_TC1, for
;					    controller 2
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_jmp_filter

	jmp	[VDMAD_Hw_Int_Filter]

EndProc VDMAD_jmp_filter

;******************************************************************************
;
;   VDMAD_HW_Check_TC1
;
;   DESCRIPTION:    Check for a channel reaching Terminal Count in controller
;		    1, if one does, then schedule a global event to deal with
;		    completing the transfer
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_HW_Check_TC1

	pushfd
	push	eax
	in	al, DMA1_Status
	DMA_Q_OUT 'VDMAD_HW_Check_TC1 #al'
	test	al, 0Fh
	jz	short chk_b_exit
	or	[VDMAD_DMA1_status], al
	push	esi
	mov	esi, OFFSET32 VDMAD_C1_Event  ; Call this procedure
	jmp	short schedule_event

EndProc VDMAD_HW_Check_TC1

;******************************************************************************
;
;   VDMAD_HW_Check_TC2
;
;   DESCRIPTION:    Check for a channel reaching Terminal Count in controller
;		    1, if one does, then schedule a global event to deal with
;		    completing the transfer
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_HW_Check_TC2

	pushfd
	push	eax
	in	al, DMA2_Status
	DMA_Q_OUT 'VDMAD_HW_Check_TC2 #al'
	test	al, 0Fh
	jz	short chk_b_exit
	or	[VDMAD_DMA2_status], al
	push	esi
	mov	esi, OFFSET32 VDMAD_C2_Event  ; Call this procedure

schedule_event:
	push	ebx
	push	edx
	VMMCall Get_Cur_VM_Handle	; EBX = Current VM handle
	mov	edx, [VDMAD_CB_Offset]
	cmp	[edx][ebx.DMA_TC_event], 0
	jne	short chk_event_exists	; jump if event already scheduled
	mov	edx, ebx
	VMMCall Schedule_Global_Event
	add	ebx, [VDMAD_CB_Offset]
	mov	[ebx.DMA_TC_event], esi
chk_event_exists:
	pop	edx
	pop	ebx
	pop	esi

chk_b_exit:
	pop	eax
	popfd
	jmp	[VDMAD_Hw_Int_Next]	; pass control to next filter

EndProc VDMAD_HW_Check_TC2


;******************************************************************************
;
;   VDMAD_C1_Event
;
;   DESCRIPTION:    Handle completing a DMA transfer for channels 0-3
;
;   ENTRY:	    EDX is VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_C1_Event

	Assert_VM_Handle edx		; Debugging
	add	edx, [VDMAD_CB_Offset]
	mov	[edx.DMA_TC_event], 0
	movzx	ecx, [VDMAD_DMA1_status]

	bsf	eax, ecx		; get channel #
	call	VDMAD_Get_DMA_Handle	; EAX = DMA handle
	mov	edx, [eax.owner_VM]
	Assert_VM_Handle edx		; Debugging
	add	edx, [VDMAD_CB_Offset]
	or	[edx.ctl1.DMA_status], cl ; virtual status = physical

	DMA_Q_OUT "DMA/HW TC on chn #al", [eax.channel_num]
	call	VDMAD_Mask_Virtual
	CallRet VDMAD_DMA_Complete

EndProc VDMAD_C1_Event

;******************************************************************************
;
;   VDMAD_C2_Event
;
;   DESCRIPTION:    Handle completing a DMA transfer for channels 4-7
;
;   ENTRY:	    EDX is VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_C2_Event

	Assert_VM_Handle edx		; Debugging
	add	edx, [VDMAD_CB_Offset]
	mov	[edx.DMA_TC_event], 0
	movzx	ecx, [VDMAD_DMA2_status]

	bsf	eax, ecx		; eax = channel # in controller
	add	eax, 4			; channel 4 is first on 2nd controller
	call	VDMAD_Get_DMA_Handle	; EAX = DMA handle
	mov	edx, [eax.owner_VM]
	Assert_VM_Handle edx		; Debugging
	add	edx, [VDMAD_CB_Offset]
	or	[edx.ctl2.DMA_status], cl ; virtual status = physical

	DMA_Q_OUT "DMA/HW TC on chn #al", [eax.channel_num]
	call	VDMAD_Mask_Virtual
	CallRet VDMAD_DMA_Complete

EndProc VDMAD_C2_Event


;******************************************************************************
;
;   VDMAD_Mask_Virtual
;
;   DESCRIPTION:    Virtually mask the DMA port (This is called by the
;		    terminal count detection routines to simulate the hardware
;		    which automatically masks a channel when terminal count
;		    is reached.)
;
;   ENTRY:	    EAX = DMA handle
;		    EDX -> DMA info in VM's CB
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================

BeginProc VDMAD_Mask_Virtual

	Validate_DMA_Handle
;
; virtually mask the channel
;
	push	eax
	push	edx
	mov	eax, [eax.channel_num]
	cmp	al, 4			;Q: 2nd controller?
	jb	short mc_not_2nd	;   N:
	add	edx, SIZE DMA_Controller_State	;Y: point to 2nd ctrl. data
mc_not_2nd:
	and	al, DMA_chan_sel	; isolate channel # bits
	bts	dword ptr [edx.DMA_mask], eax ; set channel's mask bit
	mov	[eax*4][edx.DMA_counts], 0000FFFFh
	pop	edx
	pop	eax
	ret

EndProc VDMAD_Mask_Virtual


;******************************************************************************
;******************************************************************************

;******************************************************************************
;
;   VDMAD_Call_Def
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

BeginProc VDMAD_Call_Def

	VxDJmp	VDMAD_Default_Handler

EndProc VDMAD_Call_Def


BeginDoc
;******************************************************************************
;
;   VDMAD_Default_Handler
;
;   DESCRIPTION:
;	Default DMA channel I/O call back routine.  This routine receives
;	notifications of virtual state changes and handles setting up the
;	physical state to start DMA transfers.
;
;	    get virtual state
;	    If channel virtually unmasked then
;		lock region
;		If lock fails then
;		    request buffer
;		    If memory read opeartion then
;			copy data to buffer
;		set phyical state
;		physically unmask channel
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM handle
;
;   EXIT:	    nothing
;
;   USES:	    anything
;
;==============================================================================
EndDoc
BeginProc VDMAD_Default_Handler, ASYNC_SERVICE

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle

	mov	ecx, [eax.channel_num]
	DMA_Q_OUT "DMA I/O  handle #eax   VM handle #ebx"

	VxDCall VDMAD_Get_Virt_State	; ESI = linear address
					; ECX = count
					; DL/DH = mode/flags

	test	dl, DMA_requested	;Q: channel requested?
	jnz	short dh_setup_buffer	;   Y:

	test	dl, DMA_masked		;Q: channel masked?
	jnz	DEBFAR dh_masked	;   Y: must have just been masked, so
					;      free the buffer/region

dh_setup_buffer:
	cmp	[eax.locked_pages], 0	;Q: existing lock?
	jne	short dh_chk_region	;   Y: check for same region?
	cmp	[eax.buffer_id], 0	;Q: buffer currently assigned?
	je	short dh_lock		;   N: attempt to lock the new region

dh_chk_region:
	cmp	esi, [eax.region_base]	;   Y: Q: same region?
	jne	short dh_unlock_first	;	    N:
	cmp	ecx, [eax.region_size]	;	    Y: Q: really?
	je	short dh_lock_done	;		    Y: no change in region

dh_unlock_first:			; unlock a previously locked region first
	cmp	[eax.buffer_id], 0	;Q: buffer currently assigned?
	jne	short dh_free_buf	;   N:

	DMA_Q_OUT "unlock old region"
	call	VDMAD_Free_Region
	jmp	short dh_lock

dh_free_buf:
	DMA_Q_OUT "release old buffer"
	call	VDMAD_Free_Buffer

dh_lock:				; attempt to lock the new region
	DMA_Q_OUT "lock new region"

%OUT enter critical section before setting physical state?
	call	VDMAD_Attempt_Lock
	jnc	short dh_lock_done


					; EAX = DMA handle, ESI = linear adr
					; EBX = VM Handle
					; ECX = region byte size
	Assert_VM_Handle ebx		; Debugging
	call	VDMAD_Grab_Buffer

IFDEF allow_partial_virtualization
	test	edx, 80000000h		;Q: mode been set?
	jz	short chk_read_op	;   Y:
					;   N: flag copy out of buffer
	or	[DMA_Buffer_State], Buffer_Needs_Copying
	jmp	short dh_copy_into_buf
chk_read_op:
ENDIF

	test	dl, DMA_type_read	;Q: read transfer?
	jz	short dh_write_mode	;   N:

dh_copy_into_buf:
	pushad
	xor	edi, edi
	VxDCall VDMAD_Get_Region_Info
	VxDCall VDMAD_Copy_To_Buffer	; copy data into VDMAD buffer
	popad
	jmp	short dh_lock_done

dh_write_mode:
	; flag that buffer needs to be copied into VM's region after DMA
	or	[DMA_Buffer_State], Buffer_Needs_Copying

dh_lock_done:
	push	eax
	push	edx
	VxDCall VDMAD_Set_Phys_State
	pop	edx
	pop	eax
	test	dl, DMA_masked		;Q: unmask channel?
	jnz	short dh_exit		;   N:
	VxDCall VDMAD_UnMask_Channel	;   Y: do it!
	jmp	short dh_exit

dh_masked:
	DMA_Q_OUT "DMA complete (VM masking) chn #al", [eax.channel_num]
	call	VDMAD_DMA_Complete

dh_exit:
	ret

EndProc VDMAD_Default_Handler


;******************************************************************************
;
;   VDMAD_Attempt_Lock
;
;   DESCRIPTION:    attempt to lock a DMA region to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;		    ESI = linear address of actual DMA region
;		    ECX = # of bytes in DMA region
;
;   EXIT:	    Carry clear if memory locked
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Attempt_Lock

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle
	pushad

	mov	edx, [eax.channel_num]
	add	ebx, [VDMAD_CB_Offset]
	bt	[ebx.DMA_disabled], edx ;Q: translation disabled?
	mov	edx, esi		;      assume true - copy phys adr
	jc	short no_lock		;   Y: don't lock - esi = phys

%OUT is alignment ever a problem on EISA?
	xor	edx, edx
	IF_MCA_JMP <SHORT no_alignment
	mov	edx, [eax.channel_num]
	cmp	dl, 4
	mov	dl, 1
	jb	short dh_byte_channel
	shl	edx, 1
dh_byte_channel:
no_alignment:
	VxDCall VDMAD_Lock_DMA_Region	; EDX = physical address
	jc	SHORT attempt_failed

no_lock:
	mov	ebx, 0FF00h		; pages locked & no buffer
	VxdCall VDMAD_Set_Region_Info
	clc

attempt_failed:
	popad
	ret

EndProc VDMAD_Attempt_Lock

;******************************************************************************
;
;   VDMAD_Free_Region
;
;   DESCRIPTION:    unlock the DMA region locked to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Free_Region

	Validate_DMA_Handle
	pushad

	mov	edx, [eax.channel_num]
	mov	ebx, [eax.owner_VM]
	Assert_VM_Handle ebx		; Debugging
	add	ebx, [VDMAD_CB_Offset]
	bt	[ebx.DMA_disabled], edx ;Q: translation disabled?
	jc	short no_unlock 	;   Y: don't unlock - just reset region

	VxDCall VDMAD_Get_Region_Info
	VxDCall VDMAD_Unlock_DMA_Region

no_unlock:
	xor	ebx, ebx
	xor	esi, esi
	xor	ecx, ecx
	xor	edx, edx
	VxDCall VDMAD_Set_Region_Info

	popad
	ret

EndProc VDMAD_Free_Region


;******************************************************************************
;
;   VDMAD_Grab_Buffer
;
;   DESCRIPTION:    Allocate the DMA buffer and assign it to a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;		    EBX = VM Handle
;		    ESI = linear adr
;		    ECX = region byte size
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Grab_Buffer

	Assert_VM_Handle ebx		; Debugging
	Validate_DMA_Handle
	pushad
try_grab:
	VxDCall VDMAD_Request_Buffer
	jnc	DEBFAR buffer_grabbed

	cmp	al, 5
	jne	SHORT buffer_busy

	add	ecx, 1023
	shr	ecx, 10 		; # of Kb
	mov	eax, ecx
	aam
	add	al, '0'
	mov	[Buffer_Too_Small_value+2], al
	mov	al, ah
	xor	ah, ah
	aam
	add	ax, ('0' SHL 8) + '0'
	xchg	al, ah
	mov	word ptr [Buffer_Too_Small_value], ax

IFDEF DEBUG
	mov	eax, ecx
	VMMCall Debug_Convert_Hex_Decimal
	mov	ecx, eax
	mov	eax, [esp.Pushad_EAX]
	mov	edx, [eax.channel_num]
	Debug_Out "VDMAD: buffer too small, requested #ecxKb for channel #dl"
ENDIF
	Fatal_Error <OFFSET32 Buffer_Too_Small>

buffer_busy:
IFDEF DEBUG
	Trace_Out "VDMAD:  buffer in use"
%OUT Should a timeout be scheduled to abort when buffer busy?  Retail and/or Debugging
spin:	VxDCall VDMAD_Request_Buffer
	jnc	short buffer_grabbed
	mov	al, '.'
	VMMCall Out_Debug_Chr
	jmp	spin
ELSE
	jmp	try_grab			    ; try request again
ENDIF


buffer_grabbed:
	mov	eax, [esp.Pushad_EAX]
	VxdCall VDMAD_Set_Region_Info
	mov	ebx, [esp.Pushad_EBX]
	DMA_Q_OUT "DMA buffer assigned to #eax in VM #ebx"
	mov	ecx, Block_Svc_Ints
	VMMCall Begin_Critical_Section

	mov	eax, 10000		; 10 seconds
	mov	edx, ebx		; VM handle for reference data
	mov	esi, OFFSET32 VDMAD_Buffer_TimeOut
	VMMCall Set_Global_Time_Out
	mov	[DMA_Buffer_Timeout], esi
	popad
	ret

EndProc VDMAD_Grab_Buffer

;******************************************************************************
;
;   VDMAD_Buffer_TimeOut
;
;   DESCRIPTION:    Kill the VM, since it failed to complete its DMA into
;		    the VDMAD buffer
;
;   ENTRY:	    EDX is the VM handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Buffer_TimeOut

	Assert_VM_Handle edx		; Debugging
	mov	[DMA_Buffer_Timeout], 0
	Debug_Out "VDMAD:  Buffered DMA failed to complete within 10 seconds"
	VMMCall Crash_Cur_VM		    ; Will not return
	ret

EndProc VDMAD_Buffer_TimeOut



;******************************************************************************
;
;   VDMAD_Free_Buffer
;
;   DESCRIPTION:    Release the DMA buffer from a DMA handle
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================
BeginProc VDMAD_Free_Buffer

	Validate_DMA_Handle
	pushad
	movzx	ebx, [eax.buffer_id]
	VxDCall VDMAD_Release_Buffer

	xor	ebx, ebx
	xor	esi, esi
	xor	ecx, ecx
	xor	edx, edx
	VxdCall VDMAD_Set_Region_Info
	mov	ebx, [eax.owner_VM]	; get owner VM handle
	Assert_VM_Handle ebx		; Debugging
	mov	esi, OFFSET32 VDMAD_End_Crit_Section
	xor	edx, edx
	VMMCall Call_VM_Event
	popad
	ret

EndProc VDMAD_Free_Buffer


;******************************************************************************
;
;   VDMAD_End_Crit_Section
;
;   DESCRIPTION:    Event to call End_Critical_Section for the VM that owned
;		    the DMA buffer.
;
;   ENTRY:	    EBX = VM handle, EDX = 0
;
;   EXIT:	    nothing
;
;   USES:	    nothing
;
;==============================================================================

BeginProc VDMAD_End_Crit_Section

	Assert_VM_Handle ebx		; Debugging
	VMMJmp	End_Critical_Section

EndProc VDMAD_End_Crit_Section



;******************************************************************************
;
;   VDMAD_DMA_Complete
;
;   DESCRIPTION:    Complete DMA transfer.
;			If buffered then
;			    If writing to memory then
;				copy memory from buffer to DMA region
;			    free buffer
;			else
;			    unlock DMA region
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:	    nothing
;
;   USES:	    Flags
;
;==============================================================================

BeginProc VDMAD_DMA_Complete

	Validate_DMA_Handle
	cmp	[eax.buffer_id],0	;Q: buffer assigned?
	je	short dh_unlock 	;   N:
	test	[DMA_Buffer_State], Buffer_Needs_Copying
	jz	short dh_free_buffer	;   N:

	pushad
	xor	edi, edi
	VxDCall VDMAD_Get_Region_Info
	VxDCall VDMAD_Copy_From_Buffer	; copy data from the VDMAD buffer
	popad

dh_free_buffer:
	call	VDMAD_Free_Buffer
	jmp	short dh_mask

dh_unlock:
	cmp	[eax.locked_pages], 0	;Q: pages locked?
	je	short dh_mask		;   N:
	call	VDMAD_Free_Region	;   Y: unlock them
;
; physically mask the channel again
;
dh_mask:
					; EAX = DMA handle
	VxDCall VDMAD_Mask_Channel
	ret

EndProc VDMAD_DMA_Complete


;==============================================================================

IFDEF DEBUG
;******************************************************************************
;
;   VDMAD_Validate_Handle
;
;   DESCRIPTION:    check for a valid DMA handle
;
;   ENTRY:	    EAX = DMA handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Validate_Handle

	push	eax
	push	edx
	sub	eax, OFFSET32 DMA_Channels
	jb	short bad_handle
	cmp	eax, (SIZE DMA_Channel_Data) * 7
	ja	short bad_handle
	mov	edx, SIZE DMA_Channel_Data
	idiv	dl
	or	ah, ah
	jz	short handle_ok
bad_handle:
	Debug_Out "VDMAD: invalid DMA handle #EAX"
	Fatal_Error

handle_ok:
	pop	edx
	pop	eax
	ret

EndProc VDMAD_Validate_Handle

;******************************************************************************
;
;   VDMAD_Query
;
;   DESCRIPTION:    Display current VDMAD status information to the debugging
;		    terminal
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_Query

	Trace_Out <"VDMAD state", 13, 10>
	mov	eax, [VDMAD_CB_Offset]
	Trace_Out "CB offset          #eax"
	mov	eax, [DMA_Buffer_Size]
	Trace_Out "Buffer Size        #eax (pages)"
	mov	eax, [DMA_Buffer_Phys]
	Trace_Out "Buffer Physical    #eax"
	mov	eax, [DMA_Buffer_Linr]
	Trace_Out "Buffer Linear      #eax"
	mov	eax, [DMA_Buffer_State]
	Trace_Out "Buffer State       #eax"
	test	eax, Buffer_In_use	;Q: buffer in use?
	jz	short vq_buffer_free	;   N:
	Trace_Out "Buffer is in use"
vq_buffer_free:

	mov	eax, [VDMAD_Hw_Int_Filter]
	mov	ebx, [VDMAD_Hw_Int_Next]
	cmp	eax, ebx		;Q: assigned filter proc?
	je	short no_hw_int_hook	;   N:
	Trace_Out "Hw Int filter      ?eax"
	Trace_Out "Next Hw Int filter ?ebx"
	mov	al, [VDMAD_DMA1_status]
	Trace_Out "First DMA status   #al"
	mov	al, [VDMAD_DMA2_status]
	Trace_Out "Second DMA status  #al"
	jmp	short vq_1

no_hw_int_hook:
	Trace_Out "VPICD HW INT notification not hooked"
vq_1:

	Trace_Out <13, 10, "ESC to quit, or any char to see channel states:">
	VMMcall In_Debug_Chr
	jz	query_exit

	mov	esi, OFFSET32 DMA_Channels
	mov	ecx, 8
sc_loop:
	call	VDMAD_ShowChannel_State
	add	esi, SIZE DMA_Channel_Data
	loop	sc_loop


	Trace_Out <13, 10, "ESC to quit, or any char to see virtual states:">
	VMMcall In_Debug_Chr
	jz	query_exit

	VMMcall Get_Cur_VM_Handle
	Trace_Out <"virtual DMA state for CurVM #EBX", 13, 10>
	mov	esi, ebx		; set esi to point to DMA data in
	add	esi, [VDMAD_CB_Offset]	; the VM's control block

	IF_NOT_MCA_JMP	<SHORT skip_PS2_status>
	mov	al, [esi.DMA_PS2_cmd]
	Trace_Out 'PS2:DMA_PS2_cmd    #al'
	mov	al, [esi.DMA_writing]
	Trace_Out 'PS2:DMA_writing    #al'
	mov	al, [esi.DMA_bytePtr]
	Trace_Out 'PS2:DMA_bytePtr    #al'
	mov	al, [esi.DMA_dataBytes]
	Trace_Out 'PS2:DMA_dataBytes  #al'
	mov	eax, [esi.DMA_data]
	Trace_Out 'PS2:DMA_data       #eax'
skip_PS2_status:

	Trace_Out "Controller          1                 2"
	mov	al, [esi.ctl1.DMA_status]
	mov	bl, [esi.ctl2.DMA_status]
	Trace_Out "status             #al                #bl"
	mov	al, [esi.ctl1.DMA_mask]
	mov	bl, [esi.ctl2.DMA_mask]
	Trace_Out "mask               #al                #bl"
	mov	al, [esi.ctl1.DMA_flipflop]
	mov	bl, [esi.ctl2.DMA_flipflop]
	Trace_Out "flipflop           #al                #bl"
	mov	al, [esi.ctl1.DMA_request]
	mov	bl, [esi.ctl2.DMA_request]
	Trace_Out "request            #al                #bl"

	Trace_Out '                adr     cnt   mod em      adr     cnt   mod em'

	mov	ecx, 4
	xor	eax, eax
show_chn_state:
	push	ecx
	Trace_Out "channel #al   ",/noeol

	mov	ebx, [eax*4][esi.ctl1.DMA_adrs]
	mov	edi, [eax*4][esi.ctl1.DMA_counts]
	mov	dl, [eax][esi.ctl1.DMA_modes]
	mov	cl, [eax][esi.ctl1.DMA_Ext_Modes]
	Trace_Out "#ebx #edi #dl #cl",/noeol

IFDEF allow_partial_virtualization
	bt	[esi.ctl1.DMA_mode_set], ax
	jnc	short q_mode1_set
	Trace_Out "-",/noeol
	jmp	short show_ctrl2
q_mode1_set:
	Trace_Out "V",/noeol
show_ctrl2:
ENDIF

	mov	ebx, [eax*4][esi.ctl2.DMA_adrs]
	mov	edi, [eax*4][esi.ctl2.DMA_counts]
	mov	dl, [eax][esi.ctl2.DMA_modes]
	mov	cl, [eax][esi.ctl2.DMA_Ext_Modes]

IFDEF allow_partial_virtualization
	Trace_Out "  #ebx #edi #dl #cl",/noeol
	bt	[esi.ctl2.DMA_mode_set], ax
	jnc	short q_mode2_set
	Trace_Out "-"
	jmp	short next_chn
q_mode2_set:
	Trace_Out "V"
next_chn:
ELSE
	Trace_Out "  #ebx #di #dl #cl"
ENDIF

	inc	eax
	pop	ecx
	dec	ecx
	jecxz	short query_exit
	jmp	show_chn_state

query_exit:
	ret
EndProc VDMAD_Query

;******************************************************************************
;
;   VDMAD_ShowChannel_State
;
;   DESCRIPTION:
;
;   ENTRY:	    ESI points to DMA_Channel_Data struc
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_ShowChannel_State

	mov	eax, [esi.largest_request]
	add	eax, 1023
	shr	eax, 10 		; # of Kb
	VMMCall Debug_Convert_Hex_Decimal
	mov	ebx, eax

	mov	eax, [esi.channel_num]
	Trace_Out "DMA channel ##al  max=#bxKb  callback = ",/noeol
	mov	eax, [esi.call_back]
	or	eax, eax		;Q: call back assigned?
	jz	short no_call_back	;   N:
	Trace_Out "?EAX"
	jmp	short show_channel_state

no_call_back:
	Trace_Out "none"
show_channel_state:

	movzx	eax, [esi.locked_pages]
	or	eax, eax		;Q: locked region?
	jz	short no_region 	;   N:
	Trace_Out "    region locked"
	jmp	short show_region
no_region:
	movzx	eax, [esi.buffer_id]
	or	eax, eax		;Q: buffer assigned?
	jz	short no_buffer 	;   N:
	Trace_Out "    buffer id    #ax"

show_region:
	mov	eax, [esi.region_base]
	mov	ebx, [esi.region_size]
	Trace_Out "    region adr   #eax   region size  #ebx"
	mov	ax, [esi.xfer_page]
	mov	bx, [esi.xfer_base]
	Trace_Out "    DMA page     #ax       DMA base     #bx"
	mov	eax, [esi.owner_VM]
	Trace_Out "    owner VM     #eax"

no_buffer:
	ret

EndProc VDMAD_ShowChannel_State

ENDIF


VxD_LOCKED_CODE_ENDS

END
