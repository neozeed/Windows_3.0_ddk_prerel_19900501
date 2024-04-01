PAGE 58,132
;******************************************************************************
TITLE ebios.asm -
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988-1990
;
;   Title:	ebios.asm -
;
;   Version:	1.00
;
;   Date:	21-Dec-1988
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   21-Dec-1988 RAP
;
;==============================================================================
	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

.XLIST
	INCLUDE VMM.INC
	INCLUDE Debug.INC

	Create_EBIOS_Service_Table EQU 1    ; EBIOS service table created
	INCLUDE EBIOS.INC
	INCLUDE SYSINFO.INC
.LIST

;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device EBIOS, 1, 0, EBIOS_Control, EBIOS_Device_ID, EBIOS_Init_Order

EBIOS_page equ EBIOS_DDB.DDB_Reference_Data


VxD_ICODE_SEG

;******************************************************************************
;
;   EBIOS_Device_Init
;
;   DESCRIPTION:
;
;   ENTRY:	    EBX = SYS VM's handle
;
;   EXIT:	    Carry clear if no error
;
;   USES:	    Nothing
;
;   ASSUMES:	    Interrupts have been (and still are) disabled.
;
;==============================================================================

BeginProc EBIOS_Device_Init

	push	ebp
	mov	ebp, esp
	sub	esp, 9*4		    ; reverve 9 dwords for array
	mov	edi, esp
	VMMCall _Get_Device_V86_Pages_Array <ebx, edi, 0>

	mov	edx, [EBIOS_page]
	mov	ecx, 1

	VMMCall _GetFirstV86Page
	xchg	edx, eax
	cmp	eax, edx		    ;Q: below first V86 page?
	ja	short chk_page_assignment   ;	N:
	add	eax, ecx
	dec	eax
	cmp	eax, edx		    ;Q: all pages?
	jbe	short init_done 	    ;	Y: don't need to assign them!
	sub	eax, edx		    ;	N: eax = # of pages above
	mov	ecx, eax		    ;	   ecx = # of pages to assign
	mov	eax, edx
	inc	eax			    ;	   eax = first page to assign

chk_page_assignment:
	mov	esi, eax
	shr	esi, 5			    ; dword index into array
chk_pages:
	and	eax, 31 		    ; bit index into array word
	bt	[esi+edi], eax		    ; test page assigned
	jc	short page_assigned	    ; jump if assigned, error!
	inc	eax
	mov	edx, eax
	shr	edx, 5
	add	esi, edx
	loop	chk_pages
					    ; all pages are free, so assign them

	mov	eax, [EBIOS_page]
	mov	ecx, 1
	VMMCall _Assign_Device_V86_Pages <eax, ecx, 0, 0>   ;global

init_done:
	mov	esp, ebp
	pop	ebp

no_EBIOS:
	clc
	ret

page_assigned:
IFDEF DEBUG
	shl	esi, 5
	add	eax, esi
	Debug_Out 'EBIOS: page already assigned (#eax)'
ENDIF
	Fatal_Error

EndProc EBIOS_Device_Init

VxD_ICODE_ENDS

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   EBIOS_Control
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

BeginProc EBIOS_Control

	Control_Dispatch Device_Init, EBIOS_Device_Init
	Control_Dispatch Sys_VM_Init, <short EBIOS_Map_pages>
	Control_Dispatch Create_VM,   <short EBIOS_Map_pages>
	clc
	ret

EndProc EBIOS_Control

VxD_LOCKED_CODE_ENDS


VxD_CODE_SEG

BeginDoc
;******************************************************************************
;
;   EBIOS_Get_Version
;
;   DESCRIPTION:    Get EBIOS device version and location/size of EBIOS pages
;
;   ENTRY:
;
;   EXIT:	    IF Carry clear
;			EAX is version
;			EDX is page #
;			ECX is # of pages
;		    ELSE EBIOS device not installed, and EBIOS pages are
;			not allocated
;
;   USES:
;
;==============================================================================
EndDoc

BeginProc EBIOS_Get_Version, SERVICE

	mov	eax, 100h
	mov	edx, [EBIOS_page]
	mov	ecx, 1
	clc
	ret

EndProc EBIOS_Get_Version


;******************************************************************************
;
;   EBIOS_Map_pages
;
;   DESCRIPTION:    Map EBIOS pages into the VM
;
;   ENTRY:	    EBX = Handle of VM being initialized
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc EBIOS_Map_pages

	mov	ecx, 1
	mov	eax, [EBIOS_page]
	VMMCall _PhysIntoV86 <eax, ebx, eax, ecx, 0>
	clc
	ret

EndProc EBIOS_Map_pages

VxD_CODE_ENDS


;******************************************************************************
;******************************************************************************
;
; Real mode initialization code
;
;******************************************************************************

VxD_REAL_INIT_SEG

BeginProc ebios_init

	test	bx, Duplicate_Device_ID
	jnz	short no_ebios_fnd  ; don't load if an ebios device has already
				    ; loaded!

	stc			    ; set carry so that if the BIOS doesn't
				    ; modify anything, then we will recognize
				    ; a failure
    ;
    ; Some machines have EBIOS but do not implement the EBIOS int 15h APIs.
    ;
IFNDEF	ARBEBIOS
	mov	ah, 0C0h
	int	15h
	jc	short no_StdEbios_fnd  ; jump if carry signifies no support
	or	ah, ah
	jnz	short no_StdEbios_fnd  ; jump if ah wasn't set to 0
	test	es:[bx.SD_feature1], SF1_EBIOS_allocated
	jz	short no_StdEbios_fnd
	xor	ax, ax
	mov	es, ax
	mov	ah, 0C1h	    ; get segment adr of EBIOS
	int	15h
	jc	short no_StdEbios_fnd
	mov	ax, es		    ; get EBIOS segment address
	or	ax, ax
	jz	short no_StdEbios_fnd	; jump if es = 0
DOEbios:
	shr	ax, 8		    ; convert to a page #
ELSE
    ;
    ; Under ARBEBIOS Always EBIOS at page 9F
    ;
DOEbios:
	mov	ax,9Fh
ENDIF
	movzx	edx, ax
	mov	bx, OFFSET exc_ebios_page
	mov	[bx], ax
	xor	si, si
	mov	ax, Device_Load_Ok
	jmp	short init_exit

    ;
    ; Try to detect HOSEBAG EBIOS machines by checking to see if DOS 640k
    ;	land doesn't actually end at 640k (A000) like it should
    ;
    ;	NOTE THAT THERE ARE SOME MACHINES THAT IMPLEMENT THE INT 15 API
    ;	BUT LIE!!!!!! LIKE AT&T.
    ;
no_StdEbios_fnd:
	mov	ax,5100h	    ; Get current PSP (win386 PSP)
	int	21h
	mov	es,bx
	mov	ax,word ptr es:[2]
	cmp	ax,0A000h	    ; Ends at 640k like it should?
	jae	short no_ebios_fnd  ; Yes, no EBIOS
	cmp	ax,9600h	    ; This is 600k ( > 40k EBIOS is very unlikely)
	ja	short DOEbios
no_ebios_fnd:
	xor	bx, bx
	xor	si, si
	xor	edx, edx
	mov	ax, Abort_Device_Load OR No_Fail_Message
init_exit:
	ret

exc_ebios_page	dw  0, 0

EndProc ebios_init


VxD_REAL_INIT_ENDS


	END ebios_init
