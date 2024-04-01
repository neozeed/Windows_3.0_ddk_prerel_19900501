PAGE 58,132
;******************************************************************************
TITLE vdmadapi.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989
;
;   Title:	vdmadapi.asm -
;
;   Version:	1.00
;
;   Date:	15-Sep-1989
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Sep-1989 RAP
;
;==============================================================================

	.386p

.XLIST
	include VMM.INC
	include DEBUG.INC
	include VDMAD.INC
	include DMASYS.INC
.LIST


VxD_DATA_SEG

EXTRN DMA_Buffer_Size:DWORD

original_PM_4B_EIP  dd	?
original_PM_4B_CS   dw	?
original_40_7B	    db	?
chain_4B	    db	?

VxD_DATA_ENDS

VxD_ICODE_SEG

;******************************************************************************
;
;   VDMAD_API_Device_Init
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

BeginProc VDMAD_API_Device_Init

	mov	al, ds:[47Bh]
	mov	[original_40_7B], al
	mov	[chain_4B], al

	IF_NOT_MCA_JMP <short check_vector>
	test	al, DMA_Chain_4B	    ;Q: chain bit set?
	jnz	SHORT hook_4B		    ;	Y:

; check current value segment for not E000 or F000
;
check_vector:
	and	[chain_4B], NOT DMA_Chain_4B; force the bit off
	movzx	ecx, word ptr ds:[4Bh*4+2]
	jecxz	short set_4B_iret	    ; jump, if no segment
	cmp	ecx, 0F000h		    ;Q: segment of system ROM?
	je	short set_4B_iret	    ;	Y: don't chain
	cmp	ecx, 0E000h		    ;Q: segment of system ROM?
	je	short set_4B_iret	    ;	Y: don't chain
	or	[chain_4B], DMA_Chain_4B    ; chaining required
	jmp	short hook_4B

set_4B_iret:
;
; search system ROM for an IRET instruction
;
	mov	ecx, 0FFFFh
	mov	edi, 0F0000h
	mov	al, 0CFh		    ; iret instruction
	cld
	repne	scasb
	dec	edi
	and	edi, 0FFFFh
	or	edi, 0F0000000h
	mov	ds:[4Bh*4], edi 	    ; set 4Bh vector to point to IRET

hook_4B:
	mov	eax, 4Bh
	mov	esi, OFFSET32 VDMAD_V86_DMA_Services
	VMMcall Hook_V86_Int_Chain

	mov	eax, 4Bh
	VMMcall Get_PM_Int_Vector
	mov	[original_PM_4B_CS], cx
	mov	[original_PM_4B_EIP], edx

	mov	esi, OFFSET32 VDMAD_PM_DMA_Services
	xor	edx, edx
	VMMcall Allocate_PM_Call_Back
	mov	ecx, eax
	movzx	edx, ax
	shr	ecx, 10h
	mov	eax, 4Bh
	VMMcall Set_PM_Int_Vector

	or	byte ptr ds:[47Bh], DMA_Services_Avail OR DMA_Chain_4B

	ret

EndProc VDMAD_API_Device_Init


VxD_ICODE_ENDS


VxD_CODE_SEG

EXTRN VDMAD_Get_DMA_Handle:NEAR


;******************************************************************************
;
;   VDMAD_API_System_Exit
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

BeginProc VDMAD_API_System_Exit

	mov	al, [original_40_7B]
	mov	ds:[47Bh], al
%OUT could reset original 4B vector, if bit 3 in 40:7B wasn't set
; but since it was set to point to an IRET in BIOS, it is probably okay to
; just leave it

	ret

EndProc VDMAD_API_System_Exit


;******************************************************************************
;
;   VDMAD_PM_DMA_Services
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
BeginProc VDMAD_PM_DMA_Services

	mov	eax, [ebp.Client_EAX]
	cmp	ah, 81h 		;Q: DMA service?
	jne	SHORT v86_unknown	;   N:
	VMMCall Simulate_Iret
	CallRet VDMAD_DMA_Services

PM_unknown:
	test	[chain_4B], DMA_Chain_4B_Bit  ;Q: chain?
	jnz	SHORT PM_chain		;	 Y:
	VMMjmp	Simulate_Iret

PM_chain:
	mov	cx, [original_PM_4B_CS]
	mov	edx, [original_PM_4B_EIP]
	VMMjmp	Simulate_Far_Jmp	; jump to previous 4B handler

EndProc VDMAD_PM_DMA_Services


;******************************************************************************
;
;   VDMAD_V86_DMA_Services
;
;   DESCRIPTION:    DMA services for V86
;
;   ENTRY:	    CLIENT_AH = major function #
;		    CLIENT_AL = minor function #
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_V86_DMA_Services

	mov	eax, [ebp.Client_EAX]
	cmp	ah, 81h 		;Q: DMA service?
	jne	SHORT v86_unknown	;   N:
	call	VDMAD_DMA_Services
	clc				; consume the int
	ret

v86_unknown:
	test	[chain_4B], DMA_Chain_4B_Bit ;Q: chain?
	jz	SHORT v86_nochain	;   N: return carry clear to consume int
	stc				;   Y: return carry set to chain
v86_nochain:
	ret

EndProc VDMAD_V86_DMA_Services


;******************************************************************************
;
;   VDMAD_DMA_Services
;
;   DESCRIPTION:    Common handler of DMA services called from V86 and PM
;
;   ENTRY:	    AL = function #
;
;   EXIT:
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_DMA_Services

FirstService = 2
MaxService   = 0Ch

	cmp	al, MaxService
	ja	SHORT bad_service
	sub	al, FirstService
	jb	SHORT bad_service
	movzx	eax, al
	shl	eax, 2
	and	[ebp.Client_Flags], NOT CF_Mask ; assume okay
IFDEF DEBUG
;
; put pseudo random garbage in AX, to discourage people from assume that some-
; thing in it is of any value!
;
	push	eax
	VMMCall Get_System_Time
	or	eax, ebp
	dec	eax
	mov	[ebp.Client_AX], ax
	pop	eax
ENDIF
	jmp	cs:[DMA_services+eax]

bad_service:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Func_Not_Supported
	ret	; carry cleared by OR above

DMA_services	label dword
	dd  OFFSET32 VDMAD_API_GetVersion   ;[2]
	dd  OFFSET32 VDMAD_API_Lock	    ;[3]
	dd  OFFSET32 VDMAD_API_Unlock	    ;[4]
	dd  OFFSET32 VDMAD_API_SLock	    ;[5]
	dd  OFFSET32 VDMAD_API_SUnlock	    ;[6]
	dd  OFFSET32 VDMAD_API_RequestBuf   ;[7]
	dd  OFFSET32 VDMAD_API_ReleaseBuf   ;[8]
	dd  OFFSET32 VDMAD_API_CopyToBuf    ;[9]
	dd  OFFSET32 VDMAD_API_CopyFromBuf  ;[A]
	dd  OFFSET32 VDMAD_API_DisableAuto  ;[B]
	dd  OFFSET32 VDMAD_API_EnableAuto   ;[C]

bad_flags:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_NonZero_Reserved_Flags
	ret	; carry cleared by OR above

EndProc VDMAD_DMA_Services


;******************************************************************************
;
;   VDMAD_API_GetVersion
;
;   DESCRIPTION:    Get Version information
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 02h
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Client_AH = 1	   (Version 1.00 of specification)
;			Client_AL = 0
;			Client_BX = 3	   (Microsoft Windows/386 product #)
;			Client_CX = 100h   (Version 1.00 of implementation)
;			Client_SI:DI = maximum size of DMA buffer available
;			Client_DX = flags
;				    Bit 0 = 1 if PC/XT bus architecture
;				    Bit 1 = 1 if physical buffer/remp region in
;					      first 1Mb of memory
;				    Bit 2 = 1 if automatic remap supported
;				    Bit 3 = 1 if all memory physically contiguous
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_API_GetVersion
	test	[ebp.Client_DX], 0FFFFh ;Q: any flag bits set
	jnz	bad_flags		;   Y:

	mov	[ebp.Client_AX], 100h	; spec version 1.0
	mov	[ebp.Client_BX], 3	; Microsoft OEM #
	mov	[ebp.Client_CX], 100h	; MS version 1.0

	mov	eax, [DMA_Buffer_Size]	; # of pages
	shl	eax, 12 		; # of bytes
	mov	[ebp.Client_DI], ax
	shr	eax, 16
	mov	[ebp.Client_SI], ax

%OUT THIS IS INCORRECT NEEDS TO BE DRIVEN BY INIT SERVICE NOT A CONDITIONAL
%OUT set bit 1, if buffer in first 1Mb
IFDEF PCXT
	mov	ax, 1
ELSE
	xor	ax, ax
ENDIF

	mov	[ebp.Client_DX], ax
	clc
	ret
EndProc VDMAD_API_GetVersion

;******************************************************************************
;
;   VDMAD_API_Lock
;
;   DESCRIPTION:    Attempt to lock a DMA region and optionally allocate a
;		    DMA buffer, if the region is not DMAable.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 03h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied into buffer
;					(ignored if bit 2 = 1)
;				Bit 2 = 1 if buffer should NOT be allocated
;					if lock would otherwise fail
;				Bit 3 = 1 if automatic remap should NOT be
;					attempted (fail, if region not contiguous)
;				Bit 4 = 1 if region must not overlap a 64Kb
;					boundary
;				Bit 5 = 1 if region must not overlap a 128Kb
;					boundary
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Memory locked
;			Physical address field of the DDS contains the starting
;			  physical address of the region or buffer
;			The buffer ID field contains the ID of the allocated
;			  buffer, or 0 if no buffer
;		    Carry set in Client_Flags
;			Memory is not locked
;			Region size field of the DDS contains the maximum
;			  contiguous length in bytes.
;			Client_AL = 01h (DMA_Not_Contiguous)
;				    02h (DMA_Not_Aligned)
;				    03h (DMA_Lock_Failed)
;				    04h (DMA_No_Buffer)
;				    05h (DMA_Buffer_Too_Small)
;				    06h (DMA_Buffer_In_Use)
;				    07h (DMA_Invalid_Region)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_Lock
	test	[ebp.Client_DX], 1111111111000001b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	mov	ecx, [edi.DDS_size]

	mov	[edi.DDS_bufferID], 0
	mov	[edi.DDS_physical], 0

	mov	dl, [ebp.Client_DL]
	and	dl, DMA_Align_64K OR DMA_Align_128K
	shr	dl, DMA_Align_64K_bit
.erre DMA_Align_128K_bit GT DMA_Align_64K_bit

	VxDCall VDMAD_Lock_DMA_Region
	jc	SHORT al_lock_failed
	mov	[edi.DDS_physical], edx
	jmp	SHORT al_exit

al_lock_failed:
	test	[ebp.Client_DL], DMA_No_Alloc_Buf   ;Q: get buffer?
	jnz	SHORT al_fail_call		    ;	N:

	VxDCall VDMAD_Request_Buffer		    ;	Y:
	jc	SHORT al_fail_call

	mov	[edi.DDS_bufferID], bx
	mov	[edi.DDS_physical], edx

	test	[ebp.Client_DL], DMA_Buf_Copy	    ;Q: copy into buffer?
	jz	SHORT al_exit			    ;	N:

	xor	edi, edi
	VxDCall VDMAD_Copy_To_Buffer
	jnc	SHORT al_exit

al_fail_call:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Region_Not_Locked

al_exit:
	clc
	ret

EndProc VDMAD_API_Lock

;******************************************************************************
;
;   VDMAD_API_Unlock
;
;   DESCRIPTION:    Unlock a previously locked region or release an
;		    automatically allocated DMA buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 04h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied out of buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory unlocked
;		    Carry set in Client_Flags
;			Client_AL = 08h (DMA_Region_Not_Locked)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_Unlock
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	mov	ecx, [edi.DDS_size]

	movzx	ebx, [edi.DDS_bufferID]
	or	ebx, ebx			    ;Q: buffer allocated?
	jz	SHORT aul_unlock		    ;	N: unlock the region

	test	[ebp.Client_DL], DMA_Buf_Copy	    ;Q: copy from buffer?
	jz	SHORT aul_no_copy		    ;	N: just release it

	xor	edi, edi
	VxDCall VDMAD_Copy_From_Buffer		    ;	Y: copy from the buffer
	jc	SHORT aul_error 		    ; jump if error
aul_no_copy:
	VxDCall VDMAD_Release_Buffer
	jnc	SHORT aul_exit			    ; done, if no error
	jmp	SHORT aul_error

aul_unlock:
	VxDCall VDMAD_Unlock_DMA_Region
	jnc	SHORT aul_exit

aul_error:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Region_Not_Locked
aul_exit:
	clc
	ret
EndProc VDMAD_API_Unlock

;******************************************************************************
;
;   VDMAD_API_SLock
;
;   DESCRIPTION:    This service is provided so that hardware devices that
;		    support automatic scatter/gather can determine the actual
;		    physical regions of and lock an entire linear address range
;		    in one call.  It can return a list of physical regions that
;		    make up the linear region, or it can return a copy of the
;		    page map entries that are spanned by the linear region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 05h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 6 = 1 if page table entries should be copied
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory locked
;			The # used field in the DDS contains the number of
;			physical regions or page table entries filled in.
;			If bit 6 of DX was set, then BX is the offset into the
;			first page for the start of the region
;		    Carry set in Client_Flags
;			Client_AL = 03h (DMA_Lock_Failed)
;				    07h (DMA_Invalid_Region)
;				    09h (DMA_Table_Too_Small)
;
;   USES:
;
;==============================================================================

BeginProc VDMAD_API_SLock
	test	[ebp.Client_DX], NOT (DMA_Get_PgTable OR DMA_Allow_NPs)
					    ;Q: any flag bits set
	jnz	bad_flags		    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	xchg	[edi.DDS_linear], esi
	xor	cx, cx
	xchg	cx, [edi.DDS_seg]

	mov	al, [ebp.Client_DL]
	and	al, DMA_Get_PgTable OR DMA_Allow_NPs
	shr	al, DMA_Get_PgTable_bit
.errnz DMA_Allow_NPs_bit - DMA_Get_PgTable_bit - 1
	push	esi			    ; save original linear address
	VxDCall VDMAD_Scatter_Lock
	jbe	short asl_00		    ; jump if C (lock failed) or
					    ; Z (whole region locked) set
	VxDCall VDMAD_Scatter_Unlock
	mov	[edi.DDS_used], dx	    ; set # needed
	stc

asl_00:
	pop	[edi.DDS_linear]
	mov	[edi.DDS_seg], cx
	jc	short asl_error
	test	[ebp.Client_DX], DMA_Get_PgTable
	jz	short asl_exit
	and	si, 0FFFh
	mov	[ebp.Client_BX], si	    ; return offset into first page
	jmp	short asl_exit

asl_error:
	or	[ebp.Client_Flags], CF_Mask
	movzx	edx, [edi.DDS_used]
	mov	[ebp.Client_AL], DMA_Lock_Failed
	cmp	dx, [edi.DDS_avail]
	jbe	short asl_exit
	mov	[ebp.Client_AL], DMA_Table_Too_Small

asl_exit:
	clc
	ret

EndProc VDMAD_API_SLock

;******************************************************************************
;
;   VDMAD_API_SUnlock
;
;   DESCRIPTION:    Unlock a previous scatter/gather locked region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 06h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			memory unlocked
;		    Carry set in Client_Flags
;			Client_AL = 08h (DMA_Region_Not_Locked)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_SUnlock
	test	[ebp.Client_DX], NOT (DMA_Get_PgTable OR DMA_Allow_NPs)
					    ;Q: any flag bits set
	jnz	bad_flags		    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	xchg	[edi.DDS_linear], esi
	xor	cx, cx
	xchg	cx, [edi.DDS_seg]

	VxDCall VDMAD_Scatter_Unlock

	mov	[edi.DDS_seg], cx
	mov	[edi.DDS_linear], esi
	jnc	short asu_exit
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Region_Not_Locked

asu_exit:
	clc
	ret

EndProc VDMAD_API_SUnlock

;******************************************************************************
;
;   VDMAD_API_RequestBuf
;
;   DESCRIPTION:    Request a DMA region to perform DMA transfers to/from.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 07h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied into buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			Physical address field of the DDS contains the starting
;			physical address of the buffer.  The Buffer ID field
;			contains the ID of the allocated buffer
;		    Carry set in Client_Flags
;			Client_AL = 04h (DMA_No_Buffer)
;				    05h (DMA_Buffer_Too_Small)
;				    06h (DMA_Buffer_In_Use)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_RequestBuf
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	mov	ecx, [edi.DDS_size]
	VxDCall VDMAD_Request_Buffer
	jc	short req_buf_err

	mov	[edi.DDS_bufferID], bx
	mov	[edi.DDS_physical], edx

	test	[ebp.Client_DX], DMA_Buf_Copy	;Q: copy into buffer?
	jz	short req_buf_exit		;   N:

	xor	edi, edi
	VxDCall VDMAD_Copy_To_Buffer
	jnc	short req_buf_exit

req_buf_err:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

req_buf_exit:
	clc
	ret
EndProc VDMAD_API_RequestBuf

;******************************************************************************
;
;   VDMAD_API_ReleaseBuf
;
;   DESCRIPTION:    Release a previously requested DMA buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 08h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;				Bit 1 = 1 if data should be copied out of buffer
;		    Client_ES:DI -> DMA Descriptor Structure
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			buffer released
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_ReleaseBuf
	test	[ebp.Client_DX], NOT DMA_Buf_Copy   ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs

	movzx	ebx, [edi.DDS_bufferID]

	test	[ebp.Client_DX], DMA_Buf_Copy
	jz	short rel_buf_exit
	mov	ecx, [edi.DDS_size]
	xor	edi, edi
	VxDCall VDMAD_Copy_From_Buffer
	jc	short rel_buf_err
	VxDCall VDMAD_Release_Buffer
	jnc	short rel_buf_exit

rel_buf_err:
	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], DMA_Invalid_Buffer

rel_buf_exit:
	clc
	ret
EndProc VDMAD_API_ReleaseBuf

;******************************************************************************
;
;   VDMAD_API_CopyToBuf
;
;   DESCRIPTION:    Copy from the original DMA region into the allocated DMA
;		    buffer.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 09h
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;		    Client_BX:CX = starting offset in DMA buffer to copy
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			copy successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;				    0Bh (DMA_Copy_Out_Range)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_CopyToBuf
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	movzx	ebx, [edi.DDS_bufferID]
	mov	di, [ebp.Client_BX]
	shl	edi, 16
	mov	di, [ebp.Client_CX]
	mov	ecx, [edi.DDS_size]
	VxDCall VDMAD_Copy_To_Buffer
	jnc	short copyto_exit

	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

copyto_exit:
	clc
	ret
EndProc VDMAD_API_CopyToBuf

;******************************************************************************
;
;   VDMAD_API_CopyFromBuf
;
;   DESCRIPTION:    Copy from the allocated DMA buffer into the original DMA
;		    region.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ah
;		    Client_DX = flags
;				Bit 0 = 1 if 32-bit addressing mode to be used
;		    Client_ES:DI -> DMA Descriptor Structure
;		    Client_BX:CX = starting offset in DMA buffer to copy
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			copy successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ah (DMA_Invalid_Buffer)
;				    0Bh (DMA_Copy_Out_Range)
;
;   USES:
;
;==============================================================================
BeginProc VDMAD_API_CopyFromBuf
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	call	VDMAD_Get_DDS_ptrs
	movzx	ebx, [edi.DDS_bufferID]
	mov	di, [ebp.Client_BX]
	shl	edi, 16
	mov	di, [ebp.Client_CX]
	mov	ecx, [edi.DDS_size]
	VxDCall VDMAD_Copy_From_Buffer
	jnc	short copyfrom_exit

	or	[ebp.Client_Flags], CF_Mask
	mov	[ebp.Client_AL], al

copyfrom_exit:
	clc
	ret
EndProc VDMAD_API_CopyFromBuf

;******************************************************************************
;
;   VDMAD_API_DisableAuto
;
;   DESCRIPTION:    Disable automatic DMA remapping.
;
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ch
;		    Client_BX = DMA channel #
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			enable successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ch (DMA_Invalid_Channel)
;				    0Dh (DMA_Disable_Cnt_Overflow)
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_API_DisableAuto
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	movzx	eax, [ebp.Client_BX]
	call	VDMAD_Get_DMA_Handle
	jc	SHORT disable_bad_channel
	VxDCall VDMAD_Disable_Translation
	jnc	SHORT disable_exit
	mov	[ebp.Client_AL], DMA_Disable_Cnt_Overflow
	jmp	SHORT disable_error

disable_bad_channel:
	mov	[ebp.Client_AL], DMA_Invalid_Channel
disable_error:
	or	[ebp.Client_Flags], CF_Mask
disable_exit:
	clc
	ret
EndProc VDMAD_API_DisableAuto

;******************************************************************************
;
;   VDMAD_API_EnableAuto
;
;   DESCRIPTION:    This service must be called after calling
;		    VDMAD_API_DisableAuto to re-enable automatic DMA remapping.
;
;   ENTRY:	    Client_AH = 81h
;		    Client_AL = 0Ch
;		    Client_BX = DMA channel #
;
;   EXIT:	    Carry clear
;		    Carry clear in Client_Flags
;			enable successful
;		    Carry set in Client_Flags
;			Client_AL = 0Ch (DMA_Invalid_Channel)
;				    0Eh (DMA_Disable_Cnt_Underflow)
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_API_EnableAuto
	test	[ebp.Client_DX], 1111111111111111b  ;Q: any flag bits set
	jnz	bad_flags			    ;	Y:

	movzx	eax, [ebp.Client_BX]
	call	VDMAD_Get_DMA_Handle
	jc	SHORT enable_bad_channel
	VxDCall VDMAD_Enable_Translation
	jnc	SHORT enable_exit
	mov	[ebp.Client_AL], DMA_Disable_Cnt_Underflow
	jmp	SHORT enable_error

enable_bad_channel:
	mov	[ebp.Client_AL], DMA_Invalid_Channel
enable_error:
	or	[ebp.Client_Flags], CF_Mask
enable_exit:
	clc
	ret
EndProc VDMAD_API_EnableAuto


;******************************************************************************
;
;   VDMAD_Get_DDS_ptrs
;
;   DESCRIPTION:    retrieve linear pointers to DDS and DMA region indentified
;		    in DDS
;
;   ENTRY:	    EBP -> Client frame
;
;   EXIT:	    EDI -> DDS
;		    ESI -> DMA region indentified in DDS
;
;   USES:	    EAX, Flags
;
;==============================================================================
BeginProc VDMAD_Get_DDS_ptrs

	Client_Ptr_Flat edi, ES, DI

	mov	esi, [edi.DDS_linear]
	mov	ax, [edi.DDS_seg]
	or	ax, ax
	jz	short getDDS_no_DDS_xlat

	push	dword ptr [ebp.Client_ES]
	mov	[ebp.Client_ES], ax
	Client_Ptr_Flat eax, ES
	add	esi, eax		; adjust edi with segment/selector base
	pop	dword ptr [ebp.Client_ES]

getDDS_no_DDS_xlat:
	ret

EndProc VDMAD_Get_DDS_ptrs


VxD_CODE_ENDS

	END
