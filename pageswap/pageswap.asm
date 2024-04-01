PAGE 58,132
;******************************************************************************
TITLE PageSwap.Inc - Demand Paging Swap Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988
;
;   Title:	PageSwap.Inc - Demand Paging Swap Device
;
;   Version:	2.00
;
;   Date:	09-Oct-1988
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   09-Oct-1988 RAL Original
;   02-Feb-1989 RAL Complete re-design (version 2.0)
;   07-Feb-1989 RAL Victim page finding works
;   17-Feb-1989 RAL Lots of debugging code
;   27-Feb-1989 RAL Fixed TONS of bugs.  Works with a partition file.
;   28-Feb-1989 RAL Checksum pages in debug version to verify reads.
;   05-Mar-1989 RAL Improved debugging report
;   10-Apr-1989 RAL Removed anoying trace_outs
;   27-Apr-1989 RAL Grow DOS swap file by one byte and flush to disk to avoid
;		    strange overwrite FAT bug of doom.
;   30-May-1989 RAL Added "MaxPagingFileSize" system.ini entry
;   06-Jun-1989 RAL Works with new partion file format
;   31-Jul-1989 RAL Uses V86MMGR to get page to map paging buffer
;   18-Sep-1989 RAL Direct to hardware paging enabled
;   02-Oct-1989 RAL Put contig lin pages in adjcent disk sectors
;   13-Nov-1989 RAL New service + calls Call_When_Idle service + Lock cache
;   04-Dec-1989 RAL Allocates private V86 stack/Fixed bugs in Grow File code
;   13-Jan-1990 RAL Enter critical section on page in/out services
;
;==============================================================================

	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

	INCLUDE VMM.Inc
	INCLUDE Debug.Inc
	INCLUDE DOSMgr.Inc
	INCLUDE V86MMGR.Inc
	INCLUDE Shell.Inc
	INCLUDE VHD.Inc
	INCLUDE VKD.Inc
	INCLUDE SmartDrv.Inc
	INCLUDE PDB.Inc

	Create_PageSwap_Service_Table EQU TRUE

	INCLUDE PageSwap.Inc

	INCLUDE SPART.INC
	INCLUDE SPOEM.INC

;******************************************************************************
;		V I R T U A L	D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device PageSwap, 2, 0, PageSwap_Control, PageSwap_Device_ID, \
		       PageSwap_Init_Order

;******************************************************************************
;			      E Q U A T E S
;******************************************************************************

;
;   Flag for page entries (used for LRU)
;
PSF_Recent_Swap_In  EQU     80000000h
PSF_Recent_Swap_In_Bit EQU  31

;
;   Flag equates for PS_Idle_Flags
;
PS_IF_Writing	    EQU     0002h
PS_IF_Writing_Bit   EQU     1
PS_IF_Test_Dirty    EQU     0004h
PS_IF_Test_Dirty_Bit EQU    2
PS_IF_Restart	    EQU     0008h
PS_IF_Restart_Bit   EQU     3
PS_IF_File_Full     EQU     0010h
PS_IF_File_Full_Bit EQU     4
PS_IF_Prepaging     EQU     0020h
PS_IF_Prepaging_Bit EQU     5
PS_IF_2nd_Pass	    EQU     0040h
PS_IF_2nd_Pass_Bit  EQU     6

PS_Hash_Entries     EQU     100h	    ; MUST BE A POWER OF 2!!!
PS_Hash_Mask	    EQU     PS_Hash_Entries-1

PS_Sector_Size	    EQU     200h

PS_Min_File_Grow    EQU     100h	    ; Grow file in 1Mb chunks
PS_Min_File_Pages   EQU     80h 	    ; Must have at least 512K to page

PS_Reserve_Pages    EQU     10h 	    ; # pages we won't overcommit

PS_Idle_Dirty_Test_Time EQU 5000	    ; Test dirty count every 5 seconds

PS_V86_Stack_Size   EQU     400 	    ; 400 byte stack while paging

;******************************************************************************
;******************************************************************************

VxD_IDATA_SEG

EXTRN PS_Enable_Ini:BYTE
EXTRN PS_Swap_Drive_Ini:BYTE
EXTRN PS_Min_Free_Ini:BYTE
EXTRN PS_Max_Size_Ini:BYTE
EXTRN PS_Invalid_Part_Msg:BYTE
EXTRN PS_Caption_Title_Msg:BYTE


PS_Init_Last_Sector	dd	?
PS_Init_DOS_Start	dd	?
PS_Init_DOS_End 	dd	?

PS_Spart_File_Name	db	"SPART.PAR", 0

PS_Spart_File_Buffer	db	128 dup(?)

PS_Error_Msg_Ptr	dd	0

PS_Swap_File_Name db "WIN386.SWP", 0
PS_Swap_File_Name_Len EQU $-PS_Swap_File_Name

VxD_IDATA_ENDS



PS_Read_Data	    EQU     0
PS_Write_Data	    EQU     1

VxD_LOCKED_DATA_SEG
	ALIGN 4

PS_Idle_Flags	    dd	    0
PS_Last_Idle_Time   dd	    0

PS_Reenter_Sem	    dd	    ?
PS_Disk_IO_Sem	    dd	    ?

PS_Disk_Handle	    dd	    0
PS_Async_Buff_Addr  dd	    ?

PS_Orig_DOS_Vector  dd	    ?

PS_Max_File_Pages   dd	    0			; If 0 then paging is disabled
PS_Cur_File_Pages   dd	    0

PS_Lin_Page_Num     dd	    -1

PS_Base_Lin_Page    dd	    ?
PS_V86_Stack_Seg_Off dd     ?

PS_Hash_Table_Base  dd	    ?
PS_Page_Entry_Base  dd	    ?
PS_Next_Entry_Base  dd	    ?

PS_Free_Page_Count  dd	    0
PS_Orig_Free_Count  dd	    0

PS_Cache_Lock_Ptr   dd	    ?

PS_Free_Page_List   dw	    -1


PS_Next_Possible_Victim dd  0

PS_Save_User_Stack  dd	    ?
PS_Client_PSP	    dw	    ?
PS_Our_PSP	    dw	    ?
PS_File_Handle	    dw	    ?

PS_Our_File_Name    db	    128 dup (?)

PS_Int13_Num_Heads	dd	?
PS_Int13_Sec_Per_Track	dd	?
PS_Int13_Base_Sector	dd	?

PS_Int13_Drive_Num	db	?


PS_Have_Partition	db	False
PS_DOS_IO_Count 	db	0		; If <> 0 then we called DOS
PS_IO_In_Progress	db	False

;
;   Debugging data
;
IFDEF DEBUG

PS_Deb_Struc_Base dd     ?

PS_Deb_Struc_Size	EQU 18	    ; 18 bytes for debug structure
PS_Deb_TimeIn_Offset	EQU 2
PS_Deb_TimeOut_Offset	EQU 6
PS_Deb_CountIn_Offset	EQU 10
PS_Deb_CountOut_Offset	EQU 12
PS_Deb_LastSwap_Offset	EQU 14

PS_Deb_Checksum_Size EQU    400h     ; 400h DWORDS for checksum

PS_DQ_Total_In	    dd	    0
PS_DQ_Read_In	    dd	    0
PS_DQ_Read_Time     dd	    0
PS_DQ_Total_Out     dd	    0
PS_DQ_Written_Out   dd	    0
PS_DQ_Write_Time    dd	    0

PS_Start_Time	    dd	    0



PS_DS_Idle	    EQU     0
PS_DS_Reading	    EQU     1
PS_DS_Writing	    EQU     2

PS_Debug_State	    dd	    PS_DS_Idle


PS_DQ_State_Tab LABEL DWORD
	dd	OFFSET32 PS_DS0
	dd	OFFSET32 PS_DS1
	dd	OFFSET32 PS_DS2


PS_DS0	db  "Idle", 0
PS_DS1	db  "Paging in", 0
PS_DS2	db  "Paging out", 0

ENDIF


VxD_LOCKED_DATA_ENDS


;******************************************************************************


Call_DOS MACRO
	push	ecx
	push	edx
	mov	ecx, [PS_Orig_DOS_Vector]
	movzx	edx, cx
	shr	ecx, 10h
	VMMcall Build_Int_Stack_Frame
	VMMcall Resume_Exec
	bt	[ebp.Client_Flags], CF_Bit
	pop	edx
	pop	ecx
	ENDM




;******************************************************************************
;		  R E A L   M O D E   I N I T	C O D E
;******************************************************************************

;******************************************************************************
;
;   PageSwap_Real_Init
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

BeginProc PageSwap_Real_Init

;
;   If another pageswap device is loaded then don't load -- Just abort our load
;
	test	bx, Duplicate_From_INT2F OR Duplicate_Device_ID
	jnz	SHORT PageSwap_RI_Abort_Load

;
;   No other PageSwap is loaded.  Get SmartDrv info about cache lock pointer
;
	mov	ax, 3D00h		    ; Open file DOS function
	mov	dx, OFFSET Smart_Drv_Name
	int	21h
	jc	SHORT PageSwap_RI_No_SmartDrv

	mov	bx, ax			    ; BX = File handle
	mov	ax, 4400h		    ; Get Device Data IOCTL
	int	21h
	jc	SHORT PageSwap_RI_Failed
	test	dx, 80h 		    ; Q: Is it a device?
	jz	SHORT PageSwap_RI_Failed	  ;    N: Not Smart Drive
					    ;	 Y: Read control strings
	mov	dx, OFFSET Smart_Drv_Read_Buff
	mov	cx, SIZE SD_IOCTL_Read
	mov	ax, 4402h
	int	21h
	jc	SHORT PageSwap_RI_Failed
	cmp	ax, SIZE SD_IOCTL_Read	    ; Q: Get it all?  (New Smartdrv?)
	jne	SHORT PageSwap_RI_Failed    ;	 N: Darn Darn Darn

	mov	ah, 3Eh
	int	21h
	xor	edx, edx		    ; Assume no cache lock
	cmp	[Smart_Drv_Read_Buff.SD_IR_Major_Ver], 3
	jb	SHORT PageSwap_RI_Exit	    ; Must be ver 3 or above for this
	mov	edx, [Smart_Drv_Read_Buff.SD_IR_Cache_Lock_Ptr]
	jmp	SHORT PageSwap_RI_Exit

PageSwap_RI_Failed:
	mov	ah, 3Eh 		    ; Close file handle
	int	21h			    ; And abort operation

PageSwap_RI_No_SmartDrv:
	xor	edx, edx

PageSwap_RI_Exit:
	xor	bx, bx
	xor	si, si
	mov	ax, Device_Load_Ok
	ret

PageSwap_RI_Abort_Load:
	xor	bx, bx
	xor	si, si
	mov	ax, Abort_Device_Load + No_Fail_Message
	ret

EndProc PageSwap_Real_Init

VxD_REAL_INIT_ENDS



;******************************************************************************
;	       P R O T E C T E D   M O D E   I N I T   C O D E
;******************************************************************************

VxD_ICODE_SEG

;******************************************************************************
;
;   PageSwap_Sys_Critical_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = System VM handle
;	EDX = Reference data
;	      (Real mode seg:offset of smartdrv lock byte.  0 if no smartdrv)
;
;   EXIT:
;	Carry clear
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Sys_Critical_Init

;
;   Convert real mode SEG:OFF to a linear address for smart drive cache lock
;
	movzx	eax, dx
	shr	edx, 16
	shl	edx, 4
	add	edx, eax
	mov	[PS_Cache_Lock_Ptr], edx

;
;   Get information for overcommit.
;
	.ERRNZ DemandInfoStruc MOD 4
	sub	esp, SIZE DemandInfoStruc
	mov	edi, esp
	VMMcall _GetDemandPageInfo, <edi, 0>

	mov	eax, [edi.DILin_Total_Count]
	sub	eax, [edi.DILin_Total_Free]
	add	eax, [edi.DIFree_Count]
	add	eax, 40h			; Force overcommit
	mov	[PS_Orig_Free_Count], eax
	add	esp, SIZE DemandInfoStruc

	clc
	ret

EndProc PageSwap_Sys_Critical_Init

;******************************************************************************
;
;   PageSwap_Device_Init
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

BeginProc PageSwap_Device_Init

	Push_Client_State
	VMMcall Begin_Nest_V86_Exec

;
;   Test for paging enabled
;
	mov	eax, True			; Default value = ON
	xor	esi, esi			; Use Win386 section
	mov	edi, OFFSET32 PS_Enable_Ini	; Look for this string
	VMMcall Get_Profile_Boolean		; Get the value
	test	eax, eax			; Q: Has user disabled it
	jz	PS_Init_No_Paging		;    Y: Done!
						;    N: Try to open swap file

;
;   Get the REAL DOS Int 21h vector so we can call it directly at all times.
;
	mov	eax, 21h
	VMMcall Get_V86_Int_Vector
	shl	ecx, 10h
	mov	cx, dx				; ECX = Seg:Offset of Int 21h
	mov	[PS_Orig_DOS_Vector], ecx

;
;   Open the swap partition or file.  If the procedure returns with PS_Max_File
;   Pages = 0 then the file could not be opened or created and paging is
;   disabled.  Otherwise, PS_Max_File_Pages contains the maximum number of pages
;   that can be written to the swap file.
;
	call	PageSwap_Open_Partition 	; Try to open the partition
	test	eax, eax			; Q: Did we get Temp V86 area?
	jz	PS_Init_No_Paging		;    N: HORRIBLE ERROR!
	cmp	[PS_Max_File_Pages], 0		; Q: Do we have a partition?
	jne	SHORT PS_Init_Free_V86_Area	;    Y: Good!  We'll use it.
	call	PageSwap_Open_DOS_Swap_File	;    N: Try to use a DOS file

PS_Init_Free_V86_Area:
	VMMcall _Free_Temp_V86_Data_Area

	mov	ebx, [PS_Max_File_Pages]
	test	ebx, ebx
	jz	PS_Init_No_Paging

;
;   Allocate enough memory for our data structures.  Each page requires 6 bytes.
;
	lea	edx, [ebx*4]			; EDX = # entries * 4
	lea	edx, [edx][ebx*2]		; EDX = # entries * 6 (# bytes)
IFDEF DEBUG
; note on debug data structure:
;	2 bytes for checksum	(Ralph's debug code)
;	4 bytes: running total for time in memory (dword)
;	4 bytes: running total for time swapped out (dword)
;	2 bytes + 2 bytes: number of times page came in and out, respectively
;	4 bytes: system time of last swap in/out (dword)

	lea	edx, [edx*4]			; EDX = # entries * 24
ENDIF

	add	edx, 0FFFh			; Round up to next page
	shr	edx, 12 			; EDX = # pages needed
	VMMcall _PageAllocate,<edx,PG_SYS,0,0,0,0,0,<PageLocked + PageZeroInit>>
	test	eax, eax
	jz	PS_Init_No_Paging
     ;;;   mov	   [PS_Priv_Data_Handle], eax  ; <<-- DON'T REALLY NEED THIS!
	mov	[PS_Page_Entry_Base], edx
IFDEF DEBUG
	push	ebx
	lea	ebx, [ebx][ebx*2]		; EBX = entries * 3
	lea	ebx, [edx][ebx*2]		; EBX = base + entries * 6
	mov	[PS_Deb_Struc_Base], ebx
	pop	ebx
ENDIF
	lea	ebx, [edx][ebx*4]
	mov	[PS_Next_Entry_Base], ebx

;
;   If we don't have a direct to hardware pager then reserve one mapping
;   page in the V86MMGR map region.
;
	cmp	[PS_Disk_Handle], 0
	jne	SHORT PS_Init_Get_Hash
	xor	eax, eax
	mov	bx, 0001h
	xor	ecx, ecx
	VxDcall V86MMGR_Set_Mapping_Info

;
;   Allocate memory for the hash table (1 word per entry).
;
PS_Init_Get_Hash:
	VMMcall _HeapAllocate,<<PS_Hash_Entries*2>, 0>
	test	eax, eax
	jz	PS_Cant_Get_Heap
	mov	[PS_Hash_Table_Base], eax

;
;   Fill hash table with hash_list_end entries (all buckets empty)
;
	mov	edi, eax
	mov	eax, -1
	mov	ecx, PS_Hash_Entries/2
	cld
	rep stosd

;
;   Add every page in the CURRENT swap file to the free list.
;
	cld
	xor	eax, eax
	mov	[PS_Free_Page_List], ax
	mov	edi, [PS_Next_Entry_Base]
	mov	ecx, [PS_Cur_File_Pages]
	mov	[PS_Free_Page_Count], ecx
	dec	ecx
	jz	SHORT PS_Init_Set_Tail_Ptr
PS_Init_Free_Loop:
	inc	eax
	stosw
	loopd	PS_Init_Free_Loop
PS_Init_Set_Tail_Ptr:
	mov	ax, -1
	stosw


;
;   Create a semaphore to prevent reentering pageswap operations
;
	mov	ecx, 1				; One token
	VMMcall Create_Semaphore		; Get a semaphore
	jc	PS_Cant_Get_Semaphore		; If failed then no paging
	mov	[PS_Reenter_Sem], eax

;
;   Get the base page of the linear address space for allocating file pages
;
	.ERRNZ DemandInfoStruc MOD 4
	sub	esp, SIZE DemandInfoStruc
	mov	edi, esp
	VMMcall _GetDemandPageInfo, <edi, 0>
	mov	eax, [edi.DILinear_Base_Addr]
	add	esp, SIZE DemandInfoStruc
	shr	eax, 12
	mov	[PS_Base_Lin_Page], eax

;
;   Have time-slicer call us back when all VMs are idle so we can pre-page.
;
	mov	esi, OFFSET32 PageSwap_Idle
	VMMcall Call_When_Idle

;
;   If we are using a direct paging device there is no need to switch
;   stacks.  Otherwise, we need to allocate V86 memory for our stack.
;

	cmp	[PS_Disk_Handle], 0		; Q: Direct paging?
	jne	SHORT PS_Init_Exit		;    Y: Don't need stack

	VMMCall _Allocate_Global_V86_Data_Area, <PS_V86_Stack_Size, <GVDADWordAlign OR GVDAZeroInit>>
	test	eax, eax
	jz	SHORT PS_Cant_Get_Stack
	shl	eax, 12
	shr	ax, 12				; EAX = Seg:Off of base of stack
	add	ax, PS_V86_Stack_Size		; AX = SP to start at
	mov	[PS_V86_Stack_Seg_Off], eax	; Save pointer for later

;
;   Now do different stuff depending on wether we have a partition or are
;   swapping to a DOS file.
;
	cmp	[PS_Have_Partition], True	; Q: Do we have a partition?
	je	SHORT PS_Init_Exit		;    Y: Done!
						;    N: Do DOS type things

;
;   Swapping to a DOS file.  Hook Int 24h so we can fail it while paging.
;
	mov	esi, OFFSET32 PageSwap_Int_24h
	mov	eax, 24h
	VMMcall Hook_V86_Int_Chain

;
;   The end!
;
PS_Init_Exit:
	VMMcall End_Nest_Exec
	Pop_Client_State
	clc
	ret

;
;   Paging could not be intialized for some reason.  Set Max_File_Pages to 0.
;
PS_Cant_Get_Stack:
PS_Cant_Get_Semaphore:
	VMMcall _HeapFree, <[PS_Hash_Table_Base], 0>

PS_Cant_Get_Heap:
	call	PageSwap_System_Exit

PS_Init_No_Paging:
	mov	[PS_Max_File_Pages], 0	       ; Make sure it's really off
	jmp	PS_Init_Exit

EndProc PageSwap_Device_Init





;******************************************************************************
;
;   PageSwap_Open_Partition
;
;   DESCRIPTION:
;
;   ENTRY:
;	None
;
;   EXIT:
;	If [PS_Max_File_Pages] != 0 then
;	    Partition was found and can be used
;	else
;	    Partition file not found or corrupted
;	EAX = Linear Address of Temp_V86_Data_Area buffer (== 0 if couldn't alloc)
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Open_Partition

	pushad
;
;   Try to open the SPART.PAR partition description file
;
	mov	edi, OFFSET32 PS_Spart_File_Buffer
	mov	edx, OFFSET32 PS_Spart_File_Name

	VMMCall OpenFile

	push	eax				; Save file handle
	pushfd					; And carry setting

	VMMcall _Allocate_Temp_V86_Data_Area, <400h, 0>

	pop	ecx				; Flags from OpenFile Call
	pop	[ebp.Client_EBX]		; File handle to BX register
	mov	[esp.Pushad_EAX],eax		; Return addr of Temp V86 buffer
	test	ecx, CF_MASK			; Did OpenFile Work?
	jnz	PS_OP_Exit			; If carry then not found
						; but NOT an error, just RET
	or	eax,eax 			; Did we get Temp_V86 area?
	jnz	short ProcessSpart		; Yes
	mov	[ebp.Client_AX], 3E00h		; Close file
	Call_DOS
	jmp	PS_OP_Exit

ProcessSpart:
;
;   We opened SPART.PAR successfully.  Now read the file's contents.
;
	shl	eax, 12 			; EAX = Seg:Offset for V86
	shr	ax, 12				; temp data area
	mov	[ebp.Client_DX], ax		; Point client's DS:DX
	shr	eax, 16 			; to the buffer
	mov	[ebp.Client_DS], ax		; DS:DX points to buffer
;
;   We opened SPART.PAR successfully.  Now read the file's contents.
;
	mov	[ebp.Client_CX], SIZE PFileForm ; Read this many bytes
	mov	[ebp.Client_AH], 3Fh		; DOS read
	Call_DOS
	jc	SHORT PS_OP_Close_Part_File	; If carry invalid SPART.PAR
	cmp	[ebp.Client_AX], SIZE PFIleForm ;    Y: Q: Read all the data?
	jne	SHORT PS_OP_Close_Part_File	;	   N: Corrupted
	mov	[PS_Have_Partition], True	;	   Y: Looks like we've
						;	      got a partition
PS_OP_Close_Part_File:
	mov	[ebp.Client_AH], 3Eh		; DOS close
	Call_DOS				; (BX still contains handle)

	cmp	[PS_Have_Partition], True
	jne	PS_OP_Invalid_Partition

	mov	edi, [esp.Pushad_EAX]		; EDI -> V86 buffer again
	lea	esi, [edi].PFileName		; ESI -> File name
	cmp	BYTE PTR [esi], 0		; Q: Null file?
	je	PS_OP_Invalid_Partition 	;    Y: No partition exists
						;    N: GOOD!  We have one!
;
;   Make sure this is a partition we understand.
;
	cmp	[edi.PFileVersion], PARTCURVERSION
	jne	PS_OP_Invalid_Partition

;
;   We have read the contents of the SPART.PAR file into memory.  Now copy
;   the data that we will need to use into local variables.
;

	lea	esi, [edi].OEMField

	mov	ax, [esi.INT13DrvNum]
	mov	[PS_Int13_Drive_Num], al
	movzx	eax, [esi.Int13NumHeads]
	mov	[PS_Int13_Num_Heads], eax
	movzx	eax, [esi.INT13SecPerTrk]
	mov	[PS_Int13_Sec_Per_Track], eax

;
;   Calculate the first sector of our partition and save it.
;
	movzx	eax, [esi.StartCyln]
	movzx	ebx, [esi.HeadNumStart]
	imul	eax, [PS_Int13_Num_Heads]
	add	eax, ebx
	imul	eax, [PS_Int13_Sec_Per_Track]
	mov	[PS_Int13_Base_Sector], eax

;
;   Calculate the last sector of the partition and compute the size of the
;   partition in pages.
;
	movzx	eax, [esi.EndCyln]
	movzx	ebx, [esi.HeadNumEnd]
	imul	eax, [PS_Int13_Num_Heads]
	add	eax, ebx
	inc	eax				; Last track is ours too!
	imul	eax, [PS_Int13_Sec_Per_Track]	; EAX = First sector, Last trace
	dec	eax				; Back up to last sector
	mov	[PS_Init_Last_Sector], eax	; Save for use later
	sub	eax, [PS_Int13_Base_Sector]	; EAX = Total # sectors in part
	shr	eax, 3				; EAX = # pages possible (/ 8)
	mov	[PS_Max_File_Pages], eax
	mov	[PS_Cur_File_Pages], eax

;
;   Move some data we will need for initilization only into some variables
;   in the init data segment.
;
	mov	eax, [edi.DOSStartOffset]
	mov	[PS_Init_DOS_Start], eax
	mov	eax, [edi.DOSEndOffset]
	mov	[PS_Init_DOS_End], eax

;
;   Now we know where everything SHOULD be.  Now we have to make sure it really
;   is there or else we might start doing Int 13s to bad places.  To verify
;   that the file has not been moved by some random compaction utility we will
;   write to it through DOS and read it back using Int 13h.  If the last sector
;   and first sector match what we write with DOS then we will assume that the
;   file has not moved.

;
;   First try to open the file.  If this fails then were hosed.
;
	mov	[ebp.Client_AX], 3D02h		; DOS Open file
	lea	eax, [edi].PFileName		; EAX = Linear ptr to file name
	shl	eax, 12 			; EAX = Seg:Offset for part
	shr	ax, 12				; file name
	mov	[ebp.Client_DX], ax		; Point client's DS:DX
	shr	eax, 16 			; to the file name
	mov	[ebp.Client_DS], ax
	Call_DOS				; Call Mr. Operating system
	jc	PS_OP_Invalid_Partition 	; If error then no partition
	mov	ebx, [ebp.Client_EAX]		; Else move file handle to BX
	mov	[ebp.Client_EBX], ebx		; for further DOS calls

;
;   We opened it!  Now read the last sector into the buffer.
;
	mov	eax, [PS_Init_DOS_End]
	mov	[ebp.Client_DX], ax
	shr	eax, 16
	mov	[ebp.Client_CX], ax
	mov	[ebp.Client_AX], 4200h		; Set file pointer
	Call_DOS				; (BX still contains handle)
	jc	PS_OP_Invalid_Partition 	; If error then exit

	mov	[ebp.Client_AH], 3Fh		; DOS Read File
	mov	eax, edi			; EAX = Linear ptr to buffer
	shl	eax, 12 			; EAX = Seg:Offset of buffer
	shr	ax, 12				; to read sector into
	mov	[ebp.Client_DX], ax		; Point client's DS:DX
	shr	eax, 16 			; to the file name
	mov	[ebp.Client_DS], ax
	mov	[ebp.Client_CX], PS_Sector_Size ; Read one sector (512 bytes)
	Call_DOS				; Call Mr. Operating system
	jc	SHORT PS_OP_Close_Partition	; If error then no partition
	cmp	[ebp.Client_AX], PS_Sector_Size ; Q: Did we read all 512 bytes?
	jne	SHORT PS_OP_Close_Partition	;    N: Error!

;
;   We successfully read the last sector of the file.  Now change it so
;   we will be sure of writing something that is not currently there.
;
	mov	ecx, PS_Sector_Size/4
PS_OP_Alter_Loop:
	xor	DWORD PTR [edi][ecx*4][-4], ecx ; Change the data
	loopd	PS_OP_Alter_Loop

;
;   Now make sure the partition is still in the right place.
;
	mov	ecx, [PS_Init_Last_Sector]
	mov	edx, [PS_Init_DOS_End]
	call	PageSwap_Test_Correct_Sector
	jc	SHORT PS_OP_Close_Partition

	mov	ecx, [PS_Int13_Base_Sector]
	mov	edx, [PS_Init_DOS_Start]
	call	PageSwap_Test_Correct_Sector

PS_OP_Close_Partition:
	pushfd
	mov	[ebp.Client_AH], 3Eh		; DOS close
	Call_DOS				; (BX still contains handle)
	popfd

	jc	SHORT PS_OP_Invalid_Partition
	test	[ebp.Client_Flags], CF_Mask
	jnz	SHORT PS_OP_Invalid_Partition

;
;   SUCCESS!  THE PARTITION IS VALID!
;   If there is a direct to hardware disk then get a handle so we can do
;   intelligent paging
;
	VxDcall VHD_Get_Version
	jc	SHORT PS_OP_Exit
	test	ecx, 1				; Q: Direct to hardware?
	jz	SHORT PS_OP_Exit		;    N: Useless!

	mov	esi, OFFSET32 PageSwap_VHD_Call_Back
	VxDcall VHD_Allocate_Handle
	jc	SHORT PS_OP_Exit
	mov	[PS_Disk_Handle], eax

	mov	ecx, 1
	VMMcall Create_Semaphore
	jc	SHORT PS_OP_Exit
	mov	[PS_Disk_IO_Sem], eax

	VMMcall _PageAllocate,<1,PG_SYS,0,0,0,0,0,PageLocked>
	test	eax,eax
	jz	SHORT PS_OP_Exit
	mov	[PS_Async_Buff_Addr], edx

	;;;;	Trace_Out "*************** U S I N G   S W A P   P A R T I T I O N ****************"

PS_OP_Exit:
	popad
	ret

;
;   For some reason the partition is not useable.  Return with Max_Pages=0.
;
PS_OP_Invalid_Partition:
	mov	[PS_Error_Msg_Ptr], OFFSET32 PS_Invalid_Part_Msg
	xor	ecx, ecx
	mov	[PS_Max_File_Pages], ecx
	mov	[PS_Cur_File_Pages], ecx
	.ERRNZ False
	mov	[PS_Have_Partition], cl
	jmp	PS_OP_Exit

EndProc PageSwap_Open_Partition


;******************************************************************************
;
;   PageSwap_Test_Correct_Sector
;
;   DESCRIPTION:
;
;   ENTRY:
;	ECX = Sector number (0 based) for Int 13h read
;	EDX = DOS starting position
;	EDI -> Linear address of V86 memory to use.
;	       First 512 bytes contain data to write.
;	       Second 512 bytes are buffer to read into.
;	Client_BX = Handle of open partition file
;
;   EXIT:
;	If carry set then
;
;   USES:
;	All client registers except EBX
;
;==============================================================================

BeginProc PageSwap_Test_Correct_Sector

	pushad

	mov	[ebp.Client_DX], dx
	shr	edx, 16
	mov	[ebp.Client_CX], dx
	mov	[ebp.Client_AX], 4200h		; Set file pointer
	Call_DOS				; (BX contains handle)
	jc	PS_TCS_Error			; If error then exit

	mov	[ebp.Client_AH], 40h		; DOS Write File
	mov	eax, edi			; EAX = Linear ptr to buffer
	shl	eax, 12 			; EAX = Seg:Offset of buffer
	shr	ax, 12				; to read sector into
	mov	[ebp.Client_DX], ax		; Point client's DS:DX
	shr	eax, 16 			; to the file name
	mov	[ebp.Client_DS], ax
	mov	[ebp.Client_CX], PS_Sector_Size ; Read one sector (512 bytes)
	Call_DOS				; Call Mr. Operating system
	jc	PS_TCS_Error			; If error then no partition
	cmp	[ebp.Client_AX], PS_Sector_Size ; Q: Did we write all 512 bytes?
	jne	PS_TCS_Error			;    N: Error!

;
;   Flush the DOS buffers to disk so we can read it back with Int 13h
;
	mov	[ebp.Client_AH], 0Dh
	Call_Dos				; No possible error so must
						; ignore the carry flag
;
;   Read back data
;
	push	[ebp.Client_EBX]
	mov	eax, ecx
	xor	edx, edx
	idiv	[PS_Int13_Sec_Per_Track]
	mov	ecx, edx			; Remainder = Starting sector
	xor	edx, edx
	idiv	[PS_Int13_Num_Heads]
	;   ECX = Sector # (0 BASED)!
	;   EDX = Starting head
	;   EAX = Cylinder number
	mov	ch, al
	inc	cl
	and	cl, 00111111b
	shl	ah, 6
	or	cl, ah
	mov	[ebp.Client_CX], cx		; Set cylinder and sector number
	mov	[ebp.Client_DH], dl
	mov	al, [PS_Int13_Drive_Num]
	mov	[ebp.Client_DL], al

	lea	eax, [edi+PS_Sector_Size]
	shl	eax, 12
	shr	ax, 12
	mov	[ebp.Client_BX], ax
	shr	eax, 16
	mov	[ebp.Client_ES], ax

	mov	[ebp.Client_AX], 0201h		; Read 1 sector
	mov	eax, 13h
	VMMcall Exec_Int
	pop	[ebp.Client_EBX]
	test	[ebp.Client_Flags], CF_Mask
	jnz	SHORT PS_TCS_Error

;
;   Compare the data to make sure it is identical
;
	lea	esi, [edi+PS_Sector_Size]
	mov	ecx, PS_Sector_Size / 4
	cld
	repe cmpsd
	jne	SHORT PS_TCS_Error

	clc
	popad
	ret

PS_TCS_Error:
	stc
	popad
	ret

EndProc PageSwap_Test_Correct_Sector


;******************************************************************************
;
;   PageSwap_Open_DOS_Swap_File
;
;   DESCRIPTION:
;
;   ENTRY:
;	EAX -> TEMP V86 DATA AREA
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Open_DOS_Swap_File

	pushad

;
;   Create the paging file name
;
	xor	esi, esi
	mov	edi, OFFSET32 PS_Swap_Drive_Ini
	VMMcall Get_Profile_String
	mov	edi, OFFSET32 PS_Our_File_Name
	cld
	jc	SHORT PS_ODSF_Use_Config_Dir

	mov	bl, BYTE PTR [edx]
	and	bl, NOT ("a"-"A")	; make drive letter UPPER case
	cmp	bl, "A"
	jb	SHORT PS_ODSF_Use_Config_Dir
	cmp	bl, "Z"
	ja	SHORT PS_ODSF_Use_Config_Dir

;
;   Make sure the specified paging drive is valid.  If not, use the config
;   directory.
;
	mov	[ebp.Client_AH], 36h
	mov	al, bl
	sub	al, "A"-1
	mov	[ebp.Client_DL], al
	Call_DOS
	cmp	[ebp.Client_AX], 0FFFFh
	je	SHORT PS_ODSF_Use_Config_Dir

	mov	al, bl
	stosb
	mov	ax, "\:"
	stosw
	jmp	SHORT PS_ODSF_Copy_File_Name

PS_ODSF_Use_Config_Dir:
	VMMcall Get_Config_Directory
	mov	esi, edx
PS_ODSF_Copy_Loop:
	lodsb
	test	al, al
	jz	SHORT PS_ODSF_Copy_File_Name
	stosb
	jmp	PS_ODSF_Copy_Loop

PS_ODSF_Copy_File_Name:
	mov	esi, OFFSET32 PS_Swap_File_Name
	mov	ecx, PS_Swap_File_Name_Len
	rep movsb

;
;   If the swap file is left around from a previous run of Win386 nuke it.
;
	mov	edi, [esp.Pushad_EAX]
	call	PageSwap_Nuke_Swap_File

;
;   Find out how much memory is available on swap disk.
;
	mov	al, [PS_Our_File_Name]
	and	al, NOT ("a"-"A")		; Make drive letter UPPER case
	sub	al, ("A"-1)
	mov	[ebp.Client_DL], al		; For swap drive
	mov	[ebp.Client_AH], 36h		; Get Disk Free Space
	Call_DOS				; AX * BX * CX = Free space

	movzx	eax, [ebp.Client_AX]
	cmp	eax, 0FFFFh			; Q: Error?
	je	PS_ODSF_Exit			;    Y: Invalid drive

	movzx	ebx, [ebp.Client_BX]
	movzx	ecx, [ebp.Client_CX]
	imul	ebx, eax
	imul	ebx, ecx			; EBX = Total free space

	mov	eax, 500			; Default = Reserve 500K
	xor	esi, esi			; Use Win386 section
	mov	edi, OFFSET32 PS_Min_Free_Ini	; Look for this string
	VMMcall Get_Profile_Decimal_Int 	; EAX = # K to reserve on disk
	shl	eax, 10 			; EAX = # bytes to reserve

	sub	ebx, eax			; Q: Is there ANY free space?
	jl	PS_ODSF_Exit			;    N: Nothing to do
						;    Y: Allocate data struc mem
	mov	eax, 10000h			; Never bigger than 64 meg
	xor	esi, esi			; Nil means use [Win386] section
	mov	edi, OFFSET32 PS_Max_Size_Ini	; EDI -> "MaxPagingFileSize"
	VMMcall Get_Profile_Decimal_Int 	; EAX = Maximum file size

	shr	ebx, 10 			; EBX = Max size in K
	cmp	ebx, eax			; Q: More than desired max
	jb	SHORT PS_ODSF_Have_Max_Size	;    N: Use this value
	mov	ebx, eax			;    Y: DOS max file = User's
PS_ODSF_Have_Max_Size:
	shr	ebx, 2				; EBX = Ideal file size in pages
	cmp	ebx, PS_Min_File_Pages		; Q: Enough pages available?
	jb	PS_ODSF_Exit			;    N: Don't page




	.ERRNZ DemandInfoStruc MOD 4
	sub	esp, SIZE DemandInfoStruc
	mov	edi, esp
	VMMcall _GetDemandPageInfo, <edi, 0>
	mov	edi, [edi.DILin_Total_Count]
	add	esp, SIZE DemandInfoStruc

	cmp	ebx, edi
	jbe	SHORT PS_ODSF_Fixed_Max_Size
	mov	ebx, edi

PS_ODSF_Fixed_Max_Size:
	mov	[PS_Max_File_Pages], ebx


;
;   Get the Win386 PSP segment so we can switch back to it.
;
	VMMcall Get_PSP_Segment
	mov	[PS_Our_PSP], ax

	call	PageSwap_Set_Our_PSP

;
;   Copy the file name into V86 address space.
;
	mov	edi, [esp.Pushad_EAX]		; EDI -> Temp data area
	mov	eax, edi			; EAX -> Temp data area
	mov	esi, OFFSET32 PS_Our_File_Name
	mov	ecx, 128/4
	cld
	rep movsd

  ;
  ;   Open the file by creating it
  ;
	  mov	  [ebp.Client_AH], 3Ch
	  mov	  [ebp.Client_CX], 0
	  shl	  eax, 12			  ; Cvt temp data ptr to SEG:OFF
	  shr	  ax, 12
	  mov	  [ebp.Client_DX], ax
	  shr	  eax, 16
	  mov	  [ebp.Client_DS], ax
	  Call_DOS
	  jc	  PS_ODSF_Cant_Open_File
	  mov	  eax, [ebp.Client_EAX]
  ;
  ;   Strange hack for strange DOS bug.  DO NOT REMOVE THIS CODE!
  ;
  ;   This code writes one byte to the swap file and then flushes it to disk
  ;   by closing the file handle.  For some reason, DOS will sometimes write
  ;   over the first FAT entry if you grow a file from zero length to an
  ;   enormous size.  It uses FAT entry zero as a temporary pointer to a fake
  ;   cluster and it can sometimes be written to disk, causing CHKDSK to
  ;   report "Probable non-DOS disk".
  ;
	  mov	  [ebp.Client_CX], 1
	  mov	  [ebp.Client_AH], 40h
	  mov	  [ebp.Client_EBX], eax
	  Call_DOS
	  jc	  PS_ODSF_Cant_Open_File
  ;
  ;   Close the handle to flush the one byte file.
  ;
	  mov	  [ebp.Client_EBX], eax
	  mov	  [ebp.Client_AH], 3Eh
	  Call_DOS
	  jc	  PS_ODSF_Cant_Open_File
  ;
  ;   Now re-open the file in "don't inherit" mode so that the open handle
  ;	is not passed across EXECs.
  ;
	  mov	  [ebp.Client_AH], 3Dh
	  mov	  [ebp.Client_AL], 82h ; Read/write, not inherited
	  Call_DOS
	  jc	  PS_ODSF_Cant_Open_File
	  mov	  eax, [ebp.Client_EAX]
	  mov	  [PS_File_Handle], ax

;
;   There!  It's flushed and DOS won't do strange things with the FAT anymore.
;   Grow the file by the minimum allowable amount (or to max size)
;
	mov	eax, [PS_Max_File_Pages]
	cmp	eax, PS_Min_File_Grow
	jbe	SHORT PS_ODSF_Grow_File
	mov	eax, PS_Min_File_Grow
PS_ODSF_Grow_File:
	mov	[PS_Cur_File_Pages], eax
	shl	eax, 12 			; Convert pages to bytes
	mov	[ebp.Client_DX], ax
	shr	eax, 16
	mov	[ebp.Client_CX], ax
	mov	ax, [PS_File_Handle]
	mov	[ebp.Client_BX],ax		; Put the file handle back in BX
	mov	[ebp.Client_AX], 4200h
	Call_DOS
	jc	PS_ODSF_Cant_Grow

	mov	[ebp.Client_AH], 40h
	mov	[ebp.Client_CX], 0
	Call_DOS
	jc	PS_ODSF_Cant_Grow

;
;   Seek to the end of the file to make sure the file size is correct.
;
	mov	[ebp.Client_AX], 4202h
	mov	[ebp.Client_CX], 0
	mov	[ebp.Client_DX], 0
	Call_DOS
	jc	DEBFAR PS_ODSF_Cant_Grow
	mov	eax, [ebp.Client_EDX]
	shl	eax, 16
	mov	ax, [ebp.Client_AX]
	test	eax, 00FFFh
	jnz	DEBFAR PS_ODSF_Cant_Grow
	shr	eax, 12
	cmp	eax, [PS_Cur_File_Pages]
	jne	DEBFAR PS_ODSF_Cant_Grow

;
;   Flush the file to disk by duplicating the file handle and closing the
;   duplicate.	If either of these calls fails the error is ignored since
;   the file has been grown successfully (we just couldn't flush it).
;
	; Client_BX already contains handle
	mov	[ebp.Client_AH], 45h
	Call_DOS
IFDEF DEBUG
	jnc	SHORT PS_ODSF_Dup_Worked
	Debug_Out "WARNING:  Unable to duplicate file handle for paging file"
PS_ODSF_Dup_Worked:
ENDIF
	jc	SHORT PS_ODSF_Restore_PSP

;
;   Close the duplicate handle
;
	mov	eax, [ebp.Client_EAX]
	mov	[ebp.Client_EBX], eax
	mov	[ebp.Client_AH], 3Eh
	Call_DOS
IFDEF DEBUG
	jnc	SHORT PF_ODSF_Flush_Worked
	Debug_Out "WARNING:  Unable to close duplicate file handle to flush paging file"
PF_ODSF_Flush_Worked:
ENDIF

PS_ODSF_Restore_PSP:
	call	PageSwap_Set_Client_PSP

PS_ODSF_Exit:
	popad
	ret


;
;   Unable to grow the file.  Close file handle and destroy the file.
;
PS_ODSF_Cant_Grow:
	mov	ax, [PS_File_Handle]
	mov	[ebp.Client_BX], ax
	mov	[ebp.Client_AH], 3Eh
	Call_DOS
	mov	edi, [esp.Pushad_EAX+4]
	call	PageSwap_Nuke_Swap_File

;
;   Set Max_File_Pages to 0 to indicate that paging is disabled.
;
PS_ODSF_Cant_Open_File:
	mov	[PS_Max_File_Pages], 0
	jmp	PS_ODSF_Restore_PSP

EndProc PageSwap_Open_DOS_Swap_File


;******************************************************************************
;
;   PageSwap_Init_Complete
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

BeginProc PageSwap_Init_Complete

	cmp	[PS_Max_File_Pages], 0
	je	SHORT PS_IC_Print_Error_Msg
;
;   Allocate a page in V86 global memory for mapping.
;
	cmp	[PS_Disk_Handle], 0
	jne	SHORT PS_IC_Print_Error_Msg

	xor	esi, esi
	mov	ecx, 1000h
	VxDcall V86MMGR_Map_Pages
	jc	SHORT PS_IC_Cant_Get_Map_Page
	shr	edi, 12
	mov	[PS_Lin_Page_Num], edi
	call	PageSwap_Map_Null
	jmp	SHORT PS_IC_Print_Error_Msg

;
;   STRANGE!  Unable to allocate a mapping page.
;
PS_IC_Cant_Get_Map_Page:
	Trace_Out "ERROR:  Unable to allocate mapping page for PageSwap device"
	Debug_Out "This will probably be fatal (the MMGR thinks paging is enabled)"
	call	PageSwap_System_Exit
	mov	[PS_Max_File_Pages], 0

;
;   If the partition is corrupted then we may need to display an error message.
;
PS_IC_Print_Error_Msg:
	mov	ecx, [PS_Error_Msg_Ptr]
	jecxz	PS_IC_Exit
	mov	eax, MB_OK + MB_ICONEXCLAMATION
	mov	edi, OFFSET32 PS_Caption_Title_Msg
	xor	esi, esi
	VxDcall Shell_Message

PS_IC_Exit:
	clc
	ret

EndProc PageSwap_Init_Complete


;******************************************************************************
;
;   PageSwap_Nuke_Swap_File
;
;   DESCRIPTION:
;
;   ENTRY:
;	Caller must have called Begin_Nest_V86_Exec
;	EDI -> Linear V86 address to use for DOS calls
;	EBP -> Client register structure
;
;   EXIT:
;	If carry set then could not delete file
;
;   USES:
;	All CLIENT registers and Flags (caller's registers preserved)
;
;==============================================================================

BeginProc PageSwap_Nuke_Swap_File

	pushad

	mov	eax, edi			; Save this for later
;
;   Copy our file name into the V86 temp data area.
;
	mov	esi, OFFSET32 PS_Our_File_Name
	mov	ecx, 128/4
	cld
	rep movsd

;
;   Change the attributes so we can delete it.
;
	mov	[ebp.Client_AX], 4301h		; Set file attributes
	shl	eax, 12
	shr	ax, 12
	mov	[ebp.Client_DX], ax
	shr	eax, 16
	mov	[ebp.Client_DS], ax
	mov	[ebp.Client_CX], 0
	Call_DOS
	jc	SHORT PS_NSF_Exit

;
;   Now just delete it.  Client's DS:DX already point to the file name.
;
	mov	[ebp.Client_AH], 41h
	Call_DOS

PS_NSF_Exit:
	popad
	ret

EndProc PageSwap_Nuke_Swap_File


VxD_ICODE_ENDS

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   PageSwap_Control
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

BeginProc PageSwap_Control

	Control_Dispatch Sys_Critical_Init, PageSwap_Sys_Critical_Init
	Control_Dispatch Device_Init,	    PageSwap_Device_Init
	Control_Dispatch Init_Complete,     PageSwap_Init_Complete
	Control_Dispatch System_Exit,	    <SHORT PageSwap_System_Exit>
	Control_Dispatch VM_Resume,	    PageSwap_Test_Dirty_Count
	Control_Dispatch VM_Suspend,	    PageSwap_Test_Dirty_Count

IFDEF DEBUG
	Control_Dispatch Debug_Query, PageSwap_Debug_Query
ENDIF

	clc
	ret

EndProc PageSwap_Control


;******************************************************************************
;
;   PageSwap_System_Exit
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

BeginProc PageSwap_System_Exit

	cmp	[PS_Max_File_Pages], 0		; Q: Is paging enabled?
	je	PS_SE_Exit			;    N: Quit right away
						;    Y: Map back the right mem
	cmp	[PS_Have_Partition], True	; Q: Using a disk partition?
	je	PS_SE_Exit			;    Y: GOOD!  Do nothing else!

	Push_Client_State
	VMMcall Begin_Nest_V86_Exec

	call	PageSwap_Set_Our_PSP

	mov	ax, [PS_File_Handle]
	mov	[ebp.Client_BX], ax
	mov	[ebp.Client_AH], 3Eh
	Call_DOS

	mov	ax, 4301h
	mov	edx, OFFSET32 PS_Our_File_Name
	xor	cx, cx
	VxDint	21h

	mov	ah, 41h
	VxDint	21h

	call	PageSwap_Set_Client_PSP

	VMMcall End_Nest_Exec
	Pop_Client_State

PS_SE_Exit:
	clc
	ret

EndProc PageSwap_System_Exit


;******************************************************************************
;			     S E R V I C E S
;******************************************************************************


;******************************************************************************
;
;   PageSwap_Get_Version
;
;   DESCRIPTION:
;
;   ENTRY:
;	No entry parameters
;
;   EXIT:
;	EAX = Version number (0 if not installed)
;	 BL  = Pager Type (1 = No pager, 2 = DOS pager, 3 = Direct hardware pg)
;	ECX = Maximum size of swap file in pages
;	Carry flag clear if page-swapper device installed
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Get_Version, Service

	mov	eax, 200h

	mov	bl, 1				; Assume no pager (type 1)
	mov	ecx, [PS_Max_File_Pages]	; ECX = # pages in file
	jecxz	SHORT PS_GV_Exit		; If 0 then paging is off
	inc	bl				; Assume DOS pager (type 2)
	cmp	[PS_Disk_Handle], 0		; Q: Async hard disk?
	je	SHORT PS_GV_Exit		;    N: Done
	inc	bl				;    Y: Smart pager (type 3)
PS_GV_Exit:
	clc
	ret

EndProc PageSwap_Get_Version


BeginDoc
;******************************************************************************
;
;   PageSwap_Test_Create
;
;   DESCRIPTION:
;	This service is called by the memory manager to determine if it can
;	satisfy a memory request.  For this version of pageswap, the formula
;	used to determine this is:
;
;	IF (TotalLinSpace-FreeLinSpace)+ReqestSize <= FileSize+PhysMemory THEN
;	    Request can be satisfied
;	ELSE
;	    Memory can not be allocated
;
;   ENTRY:
;	ECX = Page count
;	EAX = Flags
;
;   EXIT:
;	If carry clear then OK to create memory handle/realloc
;
;   USES:
;	Flags
;
;==============================================================================
EndDoc

BeginProc PageSwap_Test_Create, Service

	pushad

;
;   Make sure we have grown the file to a proper size
;
	mov	ebx, [PS_Free_Page_Count]
	sub	ebx, ecx
	jge	SHORT PS_TC_Grow_Done
	push	ecx
	mov	ecx, ebx
	neg	ecx
	call	PageSwap_Grow_File
	pop	ecx
PS_TC_Grow_Done:


;
;   Test for
;
	.ERRNZ DemandInfoStruc MOD 4
	sub	esp, SIZE DemandInfoStruc
	mov	edi, esp
	mov	esi, ecx			; ESI = Size of request
	VMMcall _GetDemandPageInfo, <edi, 0>
	mov	eax, [edi.DILin_Total_Count]
	sub	eax, [edi.DILin_Total_Free]
	add	eax, esi
	mov	ecx, [PS_Cur_File_Pages]
	add	ecx, [PS_Orig_Free_Count]
	add	esp, SIZE DemandInfoStruc

	sub	ecx, eax			; Carry if EAX > ECX
	popad
	ret

EndProc PageSwap_Test_Create


;******************************************************************************
;
;   PageSwap_Create
;
;   DESCRIPTION:
;
;   ENTRY:
;	EAX = Flags
;	ESI = Memory handle (ignored)
;	EDX = New linear start
;	ECX = New size
;	EBX = Old linear start (0 if none)  <-- These parameters are used
;	EDI = Old size (0 if new handle)    <-- when a handle moves
;
;   EXIT:
;	Carry clear if could create, else error
;
;   USES:
;	Flags
;
;==============================================================================

BeginProc PageSwap_Create, Service

	pushad

	cmp	[PS_Max_File_Pages], 0		; Q: Paging on?
	je	SHORT PSC_Exit			;    N: Ignore this call

	shr	edx, 12 			; Convert linear addresses to
	shr	ebx, 12 			; linear page numbers

;
;   Make sure we have grown the file to a proper size
;
	push	ecx
	lea	ecx, [edx+ecx-1]
	sub	ecx, [PS_Base_Lin_Page]
	sub	ecx, [PS_Cur_File_Pages]	; ECX = # extra pages needed
	jle	SHORT PSC_Grow_Done		; Signed compare is correct!
	call	PageSwap_Grow_File
PSC_Grow_Done:
	pop	ecx

;
;   Test for
;
	test	ebx, ebx
	jz	SHORT PSC_New_Handle

;
;   Handle is moving or changing size.	 If shrinking then free pages past
;   end of new size.
;
	cmp	ecx, edi			; Q: Is handle shrinking
	jae	SHORT PSC_Move_Pages		;    N: Move pages around
						;    Y: Free ones at end
      ;;  Debug_Out "Shrink a handle"
	push	edx
	push	ecx
	add	edx, edi			; Add old size
	sub	ecx, edi			; ECX = -Delta of shrink
	neg	ecx				; ECX = # pages to shrink
PSC_Free_Loop:
	dec	edx
	call	PageSwap_Free_Page
	loopd	PSC_Free_Loop
	pop	ecx
	pop	edx

PSC_Move_Pages:
	sub	ebx, edx			; Q: Has lin address changed?
	jz	SHORT PSC_Exit			;    N: Nothing more to do
						;    Y: EBX = -Delta of move
       ;; Debug_Out "Move a handle!"
PSC_Move_Loop:
	call	PageSwap_Find_Page
	jc	SHORT PSC_Next_Page
;
;   EDI -> Page node
;   ESI -> Entry that pointed to node EDI.  Take page off of hash table.
;
	mov	eax, [PS_Next_Entry_Base]
	mov	ax, WORD PTR [eax][edi*2]	; AX = Index of next after EDI
	mov	WORD PTR [esi], ax		; Set prev node past this node

	push	edx
	sub	edx, ebx
	call	PageSwap_Add_To_Hash

	mov	eax, [PS_Page_Entry_Base]
	mov	esi, DWORD PTR [eax][edi*4]
	and	esi, PSF_Recent_Swap_In
	or	esi, edx
	mov	DWORD PTR [eax][edi*4], esi

	pop	edx

PSC_Next_Page:
	loopd	PSC_Move_Loop
	jmp	SHORT PSC_Exit


PSC_New_Handle:
PSC_Exit:
	popad
	clc
	ret

EndProc PageSwap_Create

;******************************************************************************
;
;   PageSwap_Destroy
;
;   DESCRIPTION:
;
;   ENTRY:
;	ESI = Memory handle
;	EDX = Linear start
;	ECX = Number of pages
;	EAX = Flags
;
;   EXIT:
;
;   USES:
;
;   NOTE:
;	This procedure frees pages in REVERSE order.  This is so that the
;	first page of a freed handle will be the first page on the free list.
;	Since pages are normally touched from lowest to highest, it is usually
;	more efficent to store the pages in the same order in the file.
;
;==============================================================================

BeginProc PageSwap_Destroy, Service

	jecxz	PSD_Exit

	cmp	[PS_Max_File_Pages], 0
	je	SHORT PSD_Exit

	push	ecx
	push	edx
	shr	edx, 12
	add	edx, ecx
PSD_Loop:
	dec	edx
	call	PageSwap_Free_Page
	loopd	PSD_Loop
	pop	edx
	pop	ecx
PSD_Exit:
	clc
	ret

EndProc PageSwap_Destroy



;******************************************************************************
;
;   PageSwap_In
;
;   DESCRIPTION:
;
;   ENTRY:
;	EDX = Linear address of page to read in (not present page)
;	EDI = Linear address of memory that pageswap can touch
;	EAX = Flags
;
;   EXIT:
;	If carry set then error reading page
;
;   USES:
;	Flags
;
;==============================================================================

BeginProc PageSwap_In, Async_Service

	pushad

	mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Begin_Critical_Section

	mov	eax, [PS_Reenter_Sem]
    ;;;;mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Wait_Semaphore
	mov	eax, [esp.Pushad_EAX]


IFDEF DEBUG
	pushad

	cmp	[PS_Debug_State], PS_DS_Idle
	je	SHORT PSI_State_OK
	Debug_Out "ERROR:  PageSwap_In called while page swap device busy"
PSI_State_OK:
	mov	[PS_Debug_State], PS_DS_Reading
	VMMcall Get_System_Time
	mov	[PS_Start_Time], eax

	call	PS_analyze_swapin


	cmp	[PS_Disk_Handle], 0
	jne	SHORT PSI_DOS_Calls_OK
	VxDcall DOSMGR_Get_DOS_Crit_Status
	je	SHORT PSI_DOS_Test_InDOS
	Debug_Out "WARNING: PageSwap_In called when DOS calls not allowed!"

PSI_DOS_Test_InDOS:
	VxDcall DOSMGR_Get_IndosPtr
	cmp	WORD PTR ds:[eax], 0
	je	SHORT PSI_Dos_Calls_OK
	Debug_Out "WARNING:  PageSwap_In called when InDOS flag is non-zero!"

PSI_DOS_Calls_OK:
	inc	[PS_DQ_Total_In]
	popad
ENDIF


	mov	esi, edi			; ESI = Linear addr we can touch

	cmp	[PS_Max_File_Pages], 0
	je	DEBFAR PSI_Exit

	test	eax, PS_Zero_Init_Mask OR PS_Fixed_Page_Mask
	jnz	DEBFAR PSI_Exit

	test	eax, PS_Ever_Dirty_Mask
	jz	DEBFAR PSI_Exit

	shr	edx, 12

	push	esi
	call	PageSwap_Find_Page		; Q: Is page in file?
	pop	esi
	jc	DEBFAR PSI_Exit 		;    N: NOT AN ERROR!

IFDEF DEBUG
	pushad
	VMMcall Get_Cur_VM_Handle
	mov	ebp, [ebx.CB_Client_Pointer]
	mov	bx, [ebp.Client_CS]
	shl	ebx, 10h
	mov	bx, [ebp.Client_IP]
	Queue_Out "PAGE IN #EAX CS:IP = #EBX", edx, ebx
	cmp	ebx, 390000h
	jne	SHORT BarFoo
	Debug_Out "IN WITH CS:IP = #EBX"
BarFoo:

	cmp	[PS_Disk_Handle], 0
	jne	SHORT PSI_DOS_Calls_OK_2
	VxDcall DOSMGR_Get_DOS_Crit_Status
	je	SHORT PSI_DOS_Calls_OK_2
	Debug_Out "ERROR: PageSwap_In called when DOS calls not allowed!"
PSI_DOS_Calls_OK_2:
	inc	[PS_DQ_Read_In]
	popad
ENDIF

	mov	ch, PS_Read_Data		; Want to read data
	call	PageSwap_Read_Or_Write		; Try to do it
	jc	SHORT PSI_Error_Exit		; If error then return error

;
;   In the debugging version we checksum the page to make sure we read
;   the right data.
;
IFDEF DEBUG
	pushad					; Don't change ANYTHING!
	mov	ecx, PS_Deb_Checksum_Size	; Only check this many dwords
	xor	eax, eax			; Start with 0 checksum
PSI_Checksum:
	add	eax, DWORD PTR [esi][ecx*2]
	loopd	PSI_Checksum
	mov	esi, [PS_Deb_Struc_Base]
	push	edi
	lea	edi,[edi][edi*8]		; EDI * 9
	cmp	WORD PTR [esi][edi*2], ax	; base + EDI * 18
	pop	edi
	je	SHORT PSI_Checksums_Matched
	Trace_Out "ERROR:  Checksum on PageSwap_In did not match out checksum!"
	Debug_Out "        Lin page = #EDX, PageSwap file entry =#DI"
PSI_Checksums_Matched:
	popad
ENDIF
;
;   Mark the page entry as swapped in.
;
	mov	eax, [PS_Page_Entry_Base]
	or	edx, PSF_Recent_Swap_In
	mov	[eax][edi*4], edx

PSI_Exit:
	clc
PSI_Error_Exit:
IFDEF DEBUG
	pushfd
	pushad
	VMMcall Get_System_Time
	sub	eax, [PS_Start_Time]
	add	[PS_DQ_Read_Time], eax
	mov	[PS_Debug_State], PS_DS_Idle
	popad
	popfd
ENDIF
	pushfd
	mov	eax, [PS_Reenter_Sem]
	VMMcall Signal_Semaphore
	VMMcall End_Critical_Section
	popfd

	popad
	ret

EndProc PageSwap_In



;******************************************************************************
;
;   PageSwap_Out
;
;   DESCRIPTION:
;
;   ENTRY:
;	EDX = Linear address of page to read in (not present page)
;	ESI = Linear address of memory that pageswap can touch
;	EAX = Flags
;
;   EXIT:
;	If carry set then error writing page
;
;   USES:
;	Flags
;
;==============================================================================


BeginProc PageSwap_Out, Async_Service

IFDEF DEBUG
	pushad

	cmp	[PS_Debug_State], PS_DS_Idle
	je	SHORT PSO_State_OK
	Debug_Out "ERROR:  PageSwap_Out called while page swap device busy"
PSO_State_OK:
	mov	[PS_Debug_State], PS_DS_Writing
	VMMcall Get_System_Time
	mov	[PS_Start_Time], eax

	call	PS_analyze_swapout

	cmp	[PS_Disk_Handle], 0
	jne	SHORT PSO_DOS_Calls_OK
	VxDcall DOSMGR_Get_DOS_Crit_Status
	je	SHORT PSO_DOS_Test_InDOS
	Debug_Out "WARNING: PageSwap_Out called when DOS calls not allowed!"

PSO_DOS_Test_InDOS:
	VxDcall DOSMGR_Get_IndosPtr
	cmp	WORD PTR ds:[eax], 0
	je	SHORT PSO_Dos_Calls_OK
	Debug_Out "WARNING:  PageSwap_Out called when InDOS flag is non-zero!"

PSO_DOS_Calls_OK:
	inc	[PS_DQ_Total_Out]
	popad
ENDIF

;
;   Restart check for dirty pages at idle time.
;
	or	[PS_Idle_Flags], PS_IF_Test_Dirty


	bt	eax, PS_Ever_Dirty_Bit		; Q: Has page ever been dirty?
	jnc	DEBFAR PSO_Ignore_OK		;    N: Just ignore this one

	pushad

	mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Begin_Critical_Section

	mov	eax, [PS_Reenter_Sem]
    ;;;;mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Wait_Semaphore
	mov	eax, [esp.Pushad_EAX]

	cmp	[PS_Max_File_Pages], 0
	je	DEBFAR PSO_Error

	shr	edx, 12

	push	esi
	call	PageSwap_Find_Page
	pop	esi
	jc	DEBFAR PSO_Find_Free_Page

	.ERRNZ P_DIRTY-40h
	bt	eax, PS_Dirty_Bit		; Q: Is the page dirty?
	jnc	SHORT PSO_Success		;    N: YAHOO!	It's out!

PSO_Write_Page:
IFDEF DEBUG
	pushad
	VMMcall Get_Cur_VM_Handle
	mov	ebp, [ebx.CB_Client_Pointer]
	mov	bx, [ebp.Client_CS]
	shl	ebx, 10h
	mov	bx, [ebp.Client_IP]
	Queue_Out "PAGE OUT #EAX CS = #EBX", edx, ebx
	inc	[PS_DQ_Written_Out]
	popad
ENDIF
	mov	ch, PS_Write_Data
	call	PageSwap_Read_Or_Write
	jc	SHORT PSO_Error

;
;   In the debugging version we checksum the page so that we can verify the
;   data when the page is read in again.
;
IFDEF DEBUG
	pushad					; Don't change ANYTHING!
	mov	ecx, PS_Deb_Checksum_Size	; Only check this many dwords
	xor	eax, eax			; Start with 0 checksum
PSO_Checksum:
	add	eax, DWORD PTR [esi][ecx*2]
	loopd	PSO_Checksum
	mov	esi, [PS_Deb_Struc_Base]
	push	edi
	lea	edi,[edi][edi*8]		; EDI * 9
	mov	WORD PTR [esi][edi*2], ax	; base + EDI * 18
	pop	edi
	popad
ENDIF

;
;   Fill in the page entry.
;
PSO_Success:
	mov	eax, [PS_Reenter_Sem]
	VMMcall Signal_Semaphore
	VMMcall End_Critical_Section
	popad
PSO_Ignore_OK:

IFDEF DEBUG
	pushfd
	pushad
	VMMcall Get_System_Time
	sub	eax, [PS_Start_Time]
	add	[PS_DQ_Write_Time], eax
	mov	[PS_Debug_State], PS_DS_Idle
	popad
	popfd
ENDIF
	clc
	ret

PSO_Error:
	Debug_Out "ERROR PAGING OUT PAGE #EDX"
	mov	eax, [PS_Reenter_Sem]
	VMMcall Signal_Semaphore
	VMMcall End_Critical_Section

	popad
	stc
	ret


;------------------------------------------------------------------------------
;
;   Code to locate position in swap file for pages that currently not in file.
;
;------------------------------------------------------------------------------

;
;   Page is not currently in swap file.  Find a place to put it.
;
PSO_Find_Free_Page:
	pushad
	xor	eax, eax
	xchg	eax, edx
	sub	eax, [PS_Base_Lin_Page]
	idiv	[PS_Cur_File_Pages]

	mov	eax, [PS_Page_Entry_Base]
	cmp	DWORD PTR [eax][edx*4], 0
	jne	SHORT PSO_Get_First_Free

	mov	edi, edx
	movzx	eax, [PS_Free_Page_List]
	cmp	eax, edi
	je	SHORT PSO_Get_First_Free
	mov	ebx, [PS_Next_Entry_Base]
PSO_Find_Page_Loop:
	mov	ecx, eax			; ECX = Previous position
	mov	ax, WORD PTR [ebx][eax*2]	; EAX = Next position
	cmp	eax, edi			; Q: Is this the one?
	jne	PSO_Find_Page_Loop		;    N: Keep looking
	mov	ax, WORD PTR [ebx][eax*2]	;    Y: EAX -> Next free
	mov	WORD PTR [ebx][ecx*2], ax	;	Remove node from list

	jmp	SHORT PSO_Add_New_Page


PSO_Get_First_Free:
	Queue_Out "Cant put page in best postion!"
	movzx	edi, [PS_Free_Page_List]	; EDI = Index of first free page
	cmp	di, -1				; Q: Any pages free?
	je	SHORT PSO_Find_Victim_Page	;    N: Try to extend file
						;    Y: Remove from free list
	mov	ecx, [PS_Next_Entry_Base]
	mov	cx, WORD PTR [ecx][edi*2]
	mov	[PS_Free_Page_List], cx

PSO_Add_New_Page:
	dec	[PS_Free_Page_Count]
	mov	edx, [esp.Pushad_EDX]
	call	PageSwap_Add_To_Hash
	mov	ecx, [PS_Page_Entry_Base]
	mov	[ecx][edi*4], edx
	mov	[esp.Pushad_EDI], edi
	popad
	jmp	PSO_Write_Page


;
;   No free pages.  Can not extend file.  Find a victim page to replace.
;
PSO_Find_Victim_Page:
	Queue_Out "Swap file full -- Find victim page"

	or	[PS_Idle_Flags], PS_IF_File_Full

	sub	esp, 4
	mov	ebp, esp			; EBP -> Page table copy addr

	mov	ecx, [PS_Cur_File_Pages]	; Scan this many pages
	add	ecx, ecx			; TWICE!
	mov	edi, [PS_Next_Possible_Victim]	; EDI = Index of next victim
	mov	esi, [PS_Page_Entry_Base]	; ESI -> Page entry base
PSO_Find_Victim_Loop:
	btr	DWORD PTR [esi][edi*4], PSF_Recent_Swap_In_Bit
	jc	SHORT PS_Find_Not_This_One
	push	ecx
	VMMcall _CopyPageTable, <<DWORD PTR [esi][edi*4]>, 1, ebp, 0>
	pop	ecx
	test	BYTE PTR [ebp], P_PRES		; Q: Is the page present?
	jnz	SHORT PSO_Found_Victim		;    Y: Reuse this one
PS_Find_Not_This_One:
	inc	edi
	cmp	edi, [PS_Cur_File_Pages]
	jb	SHORT PSO_Find_Try_Next
	xor	edi, edi
PSO_Find_Try_Next:
	loopd	PSO_Find_Victim_Loop
;
;   ERROR:  Could not locate a victim page!  Return error.
;
	Debug_Out "ERROR:  Could not locate a victim page for replacement in swapfile"
	add	esp, 4
	popad
	jmp	PSO_Error


PSO_Found_Victim:
	lea	edx, [edi+1]
	cmp	edx, [PS_Cur_File_Pages]
	jb	SHORT PSO_Set_Next_Victim
	xor	edx, edx
PSO_Set_Next_Victim:
	mov	[PS_Next_Possible_Victim], edx
	mov	edx, DWORD PTR [esi][edi*4]
	call	PageSwap_Free_Page
	add	esp, 4
	jmp	PSO_Get_First_Free

EndProc PageSwap_Out

;******************************************************************************
;
;   PageSwap_Test_IO_Valid
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;	If carry flag set then
;	    Do not do anything that may cause paging
;	else
;	    OK to cause paging now
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Test_IO_Valid, Service

	cmp	[PS_Disk_Handle], 0
	jne	SHORT PS_TIV_Paging_OK
	VxDcall DOSMGR_Get_DOS_Crit_Status
	jne	SHORT PS_TIV_Dont_Page
	push	eax
	VxDcall DOSMGR_Get_IndosPtr
	cmp	WORD PTR ds:[eax], 0
	pop	eax
	jne	SHORT PS_TIV_Dont_Page
PS_TIV_Paging_OK:
	clc
	ret

PS_TIV_Dont_Page:
	stc
	ret

EndProc PageSwap_Test_IO_Valid



;******************************************************************************
;		     L O C A L	 P R O C E D U R E S
;******************************************************************************



;******************************************************************************
;
;   PageSwap_VM_Idle
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = System VM Handle
;
;   EXIT:
;	If carry clear then
;	    This procedure "ate" the idle call -- We paged something.
;	else
;	    VM is still considered idle
;
;   USES:
;	All except EBP
;
;==============================================================================

BeginProc PageSwap_Idle

	cmp	[PS_IO_In_Progress], True	; Q: Paging right now?
	je	PS_VI_Do_Nothing		;    Y: Do nothing

	cmp	[PS_Free_Page_Count], 0 	; Q: Any pages free?
	jne	SHORT PS_VI_Test_Write		;    Y: Try to do it
	test	[PS_Idle_Flags], PS_IF_File_Full;    N: Q: File completely full?
	jnz	PS_VI_Do_Nothing

PS_VI_Test_Write:
	and	[PS_Idle_Flags], NOT PS_IF_File_Full

	test	[PS_Idle_Flags], PS_IF_Writing OR PS_IF_Restart
	jnz	SHORT PS_VI_Write_Pages

	btr	[PS_Idle_Flags], PS_IF_Test_Dirty_Bit
	jc	SHORT PS_VI_Get_Dirty_Count
	VMMcall Get_System_Time
	mov	ecx, eax
	sub	ecx, [PS_Last_Idle_Time]
	cmp	ecx, PS_Idle_Dirty_Test_Time
	jb	PS_VI_Do_Nothing
	mov	[PS_Last_Idle_Time], eax

;
;   Check the dirty page count.  Since this happens fairly infrequently and
;   since the call to _PageOutDirtyPages can take a long time, we'll always
;   "eat" this idle call even if we decide not to page.  We won't eat the next
;   one.
;
PS_VI_Get_Dirty_Count:
	VMMcall _PageOutDirtyPages, <0, PagePDPQueryDirty>
	test	eax, eax
	jz	PS_VI_Ate_It
	mov	edi, eax
	sub	esp, SIZE DemandInfoStruc
	mov	esi, esp
	VMMcall _GetDemandPageInfo, <esi, 0>
	mov	eax, [esi.DIUnlock_Count]
	shr	eax, 1
	cmp	[esi.DIFree_Count], 0
	ja	SHORT PS_VI_Test_Unlock
	shr	eax, 1
PS_VI_Test_Unlock:
	add	esp, SIZE DemandInfoStruc
	cmp	edi, eax
	jb	SHORT PS_VI_Ate_It
	or	[PS_Idle_Flags], PS_IF_Restart

;
;   It's OK to page stuff out!  Check flags and start writing
;
PS_VI_Write_Pages:
	xor	edx, edx
	btr	[PS_Idle_Flags], PS_IF_Restart_Bit
	jnc	SHORT PS_VI_Do_Write
	and	[PS_Idle_Flags], NOT PS_IF_2nd_Pass
	or	[PS_Idle_Flags], PS_IF_Writing
	or	edx, PagePDPSetBase

PS_VI_Do_Write:
	mov	ecx, (Block_Svc_Ints OR Block_Enable_Ints)
	VMMcall Begin_Critical_Section

PS_VI_Write_Again:
	or	[PS_Idle_Flags], PS_IF_Prepaging
	VMMcall _PageOutDirtyPages, <1, edx>
	and	[PS_Idle_Flags], NOT PS_IF_Prepaging
	test	eax, eax
	jnz	SHORT PS_VI_End_Crit
	mov	edx, PagePDPSetBase
	bts	[PS_Idle_Flags], PS_IF_2nd_Pass_Bit
	jnc	SHORT PS_VI_Write_Again
	or	[PS_Idle_Flags], PS_IF_Test_Dirty
	and	[PS_Idle_Flags], NOT (PS_IF_Writing OR PS_IF_2nd_Pass)

PS_VI_End_Crit:
	VMMcall End_Critical_Section


;
;   We did something that took a long time.  Return with carry clear to
;   indicate that the system VM should NOT be considered idle.
;
PS_VI_Ate_It:
	clc
	ret

;
;   We don't care about this one.  Pass idle call on to next handler.
;
PS_VI_Do_Nothing:
	stc
	ret

EndProc PageSwap_Idle




;******************************************************************************
;
;   PageSwap_Test_Dirty_Count
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;	Carry flag clear
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Test_Dirty_Count

	or	[PS_Idle_Flags], PS_IF_Test_Dirty
	clc
	ret

EndProc PageSwap_Test_Dirty_Count



;******************************************************************************
;
;   PageSwap_Int_24h
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

BeginProc PageSwap_Int_24h

	cmp	 [PS_DOS_IO_Count], 0
	jne	 SHORT PS_I24_Fail
	stc
	ret

PS_I24_Fail:
	Debug_Out "ERROR:  Int 24h while paging"
	mov	[ebp.Client_AL], 3
	clc
	ret

EndProc PageSwap_Int_24h


;******************************************************************************
;
;   PageSwap_Grow_File
;
;   DESCRIPTION:
;
;   ENTRY:
;	ECX = Number of pages to grow paging file by
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Grow_File

IFDEF DEBUG
	cmp	ecx, 10000h
	jb	SHORT PS_GF_Reasonable
	Debug_Out "PageSwap_Grow_File by #ECX pages???"
PS_GF_Reasonable:
ENDIF

	push	eax
	mov	eax, [PS_Cur_File_Pages]
	cmp	eax, [PS_Max_File_Pages]
	pop	eax
	jb	SHORT PS_GF_Try_To_Grow
	ret

PS_GF_Try_To_Grow:
	pushad

	mov	edi, PS_Min_File_Grow
	cmp	edi, ecx
	jae	SHORT PS_GF_Enter_Crit
	mov	edi, ecx			; EDI = # pages to really grow

PS_GF_Enter_Crit:
	mov	ecx, (Block_Svc_Ints OR Block_Enable_Ints)
	VMMcall Begin_Critical_Section

	Push_Client_State
	VMMcall Begin_Nest_V86_Exec
	inc	[PS_DOS_IO_Count]


	VMMcall Get_Cur_VM_Handle
	mov	ebp, [ebx.CB_Client_Pointer]

	mov	eax, [PS_V86_Stack_Seg_Off]
	mov	[ebp.Client_SP], ax
	shr	eax, 16
	mov	[ebp.Client_SS], ax

;
;   Set PSP to Win386.Exe PSP to make DOS calls
;
	call	PageSwap_Set_Our_PSP

;
;   Set the file handle in BX to the paging file
;
	mov	ax, [PS_File_Handle]
	mov	[ebp.Client_BX], ax

;
;   Position the file pointer to the proper position and truncate file to
;   grow it to the new size.
;
	add	edi, [PS_Cur_File_Pages]
	cmp	edi, [PS_Max_File_Pages]
	jbe	SHORT PS_GF_Grow_Now
	mov	edi, [PS_Max_File_Pages]

;
;   Seek to new desired size and do a 0 byte write to grow the file.  Note that
;   the DOS call will NOT fail if the disk is full.  A later test will shrink
;   the max file size if the file is not grown to the desired size.
;
PS_GF_Grow_Now:
	mov	eax, edi
	shl	eax, 12 			; Convert pages to bytes
	mov	[ebp.Client_DX], ax
	shr	eax, 16
	mov	[ebp.Client_CX], ax
	mov	[ebp.Client_AX], 4200h
	Call_DOS
	jc	PS_GF_Cant_Grow

	mov	[ebp.Client_AH], 40h
	mov	[ebp.Client_CX], 0
	Call_DOS
	jc	PS_GF_Cant_Grow


;
;   Duplicate our file handle and close the duplicate to flush the FAT entries
;   to disk.  Note that we ignore errors here since not being able to do this
;   is not fatal (we may just be out of handles).
;
	; Client_BX already contains handle
	mov	[ebp.Client_AH], 45h
	Call_DOS
IFDEF DEBUG
	jnc	SHORT PS_GF_Dup_Worked
	Debug_Out "WARNING:  PageSwap unable to duplicate file handle -- Can't flush file"
PS_GF_Dup_Worked:
ENDIF
	jc	SHORT PS_GF_Test_Size

	mov	eax, [ebp.Client_EAX]
	mov	[ebp.Client_EBX], eax
	mov	[ebp.Client_AH], 3Eh
	Call_DOS
IFDEF DEBUG
	jnc	SHORT PS_GF_Test_Size
	Debug_Out "WARNING:  PageSwap unable to close duplicate file handle"
ENDIF

;
;   Seek to the end of the file to make sure the file size is correct.	If we
;   were unable to extend the file (disk is full) then set a new Max_File_-
;   Pages value so we won't thrash.
;
PS_GF_Test_Size:
	mov	ax, [PS_File_Handle]		; Must restore file handle in
	mov	[ebp.Client_BX], ax		; client's BX

	mov	[ebp.Client_AX], 4202h
	mov	[ebp.Client_CX], 0
	mov	[ebp.Client_DX], 0
	Call_DOS				; Seek to end of file
	jc	DEBFAR PS_GF_Cant_Grow		; If error then just give up

	mov	eax, [ebp.Client_EDX]
	shl	eax, 16
	mov	ax, [ebp.Client_AX]
	shr	eax, 12 			; EAX = File size in pages

	cmp	eax, edi			; Q: Desired size?
	je	SHORT GF_Grew_Successfully	;    Y: Good!
	mov	[PS_Max_File_Pages], eax	;    N: Set new maximum to
						;	prevent growing any more
	Trace_Out "WARNING:  Out of disk space.  Unable to grow paging file."

;
;   Add the new pages to the free list.
;
GF_Grew_Successfully:
	mov	ecx, eax			; ECX = # pages in file
	sub	ecx, [PS_Cur_File_Pages]	; ECX = # NEW pages in file
	jz	SHORT GF_Exit			; If none then just quit

	mov	edi, eax			; EDI = Total pages in file
	xchg	[PS_Cur_File_Pages], edi	; Set new current size
	add	[PS_Free_Page_Count], ecx	; Add to total free count
	mov	eax, [PS_Next_Entry_Base]	; EAX -> Next ptr table
	mov	bx, [PS_Free_Page_List] 	; BX = Index of 1st free page
GF_Add_To_Free_List:
	mov	WORD PTR [eax][edi*2], bx	; This page -> Prev free page
	mov	ebx, edi			; BX -> This page
	inc	edi				; EDI -> Next page
	loopd	GF_Add_To_Free_List		; Do it for every new page
	mov	[PS_Free_Page_List], bx 	; Set new free page ptr

;
;   Done!  Restore original PSP and return.
;
GF_Exit:
	call	PageSwap_Set_Client_PSP

	dec	[PS_DOS_IO_Count]
	VMMcall End_Nest_Exec
	Pop_Client_State

	VMMcall End_Critical_Section

	clc
	popad
	ret

;
;   DOS returned an error from some call.  Set max=current size so we won't
;   try to grow the file again.
;
PS_GF_Cant_Grow:
	Debug_Out "ERROR:  DOS Error while attempting to grow paging file"
	mov	eax, [PS_Cur_File_Pages]
	mov	[PS_Max_File_Pages], eax
	jmp	GF_Exit

EndProc PageSwap_Grow_File


;******************************************************************************
;
;   PageSwap_Find_Page
;
;   DESCRIPTION:
;
;   ENTRY:
;	EDX = Page number
;
;   EXIT:
;	If carry clear then
;	    EDI = Offset in file of page
;	    ESI -> Next pointer that points to this hash node (used to free)
;	else
;	    Page not in file
;
;   USES:
;
;
;==============================================================================

BeginProc PageSwap_Find_Page, High_Freq

	push	eax
	push	ebx
	push	ecx

	mov	ebx, [PS_Page_Entry_Base]
	mov	ecx, [PS_Next_Entry_Base]


	mov	esi, [PS_Hash_Table_Base]	; ESI -> Base of hash buckets
	mov	eax, PS_Hash_Mask
	and	eax, edx
	lea	esi, [esi][eax*2]

PS_FP_Loop:
	movzx	edi, WORD PTR [esi]		; EDI = Index of first entry
	cmp	di, -1				; Q: Bucket empty
	je	SHORT PS_FP_Not_Found		;    Y: Not found!

	mov	eax, DWORD PTR [ebx][edi*4]
	and	eax, NOT PSF_Recent_Swap_In
	cmp	eax, edx			; Q: Right place?
	je	SHORT PS_FP_Exit		;    Y: Exit (cmp cleared carry)
	lea	esi, [ecx][edi*2]		;    N: Keep looking
	jmp	PS_FP_Loop

PS_FP_Not_Found:
	stc
PS_FP_Exit:
	pop	ecx
	pop	ebx
	pop	eax
	ret

EndProc PageSwap_Find_Page


;******************************************************************************
;
;   PageSwap_Add_To_Hash
;
;   DESCRIPTION:
;
;   NOTE:
;	THIS SERVICE DOES NOT CHANGE ANY DATA IN THE PAGE ENTRY TABLE.	It
;	is up to the caller to make sure that the page entry table contains
;	the page number indicated by EDX.
;
;   ENTRY:
;	EDX = Page number
;	EDI = Hash node index
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Add_To_Hash

	push	eax
	push	ebx

	mov	eax, edx
	and	eax, PS_Hash_Mask
	add	eax, eax
	add	eax, [PS_Hash_Table_Base]
	mov	bx, di
	xchg	bx, WORD PTR [eax]
	mov	eax, [PS_Next_Entry_Base]
	mov	WORD PTR [eax][edi*2], bx

	pop	ebx
	pop	eax
	ret

EndProc PageSwap_Add_To_Hash


;******************************************************************************
;
;   PageSwap_Free_Page
;
;   DESCRIPTION:
;
;   ENTRY:
;	EDX = Page number (must be on hash table)
;
;   EXIT:
;
;   USES:
;	Flags
;
;==============================================================================

BeginProc PageSwap_Free_Page

	pushad

	call	PageSwap_Find_Page
	jc	SHORT PS_Free_Exit

IFDEF DEBUG
	mov	ebx, [PS_Deb_Struc_Base]
	push	edi
	lea	edi, [edi][edi*8]
	lea	edi, [ebx][edi*2]	; base + entry * 18
	xor	ecx,ecx
	mov	[edi][PS_Deb_TimeOut_Offset],ecx
	mov	[edi][PS_Deb_CountIn_Offset],cx
	mov	[edi][PS_Deb_CountOut_Offset],cx
	mov	[edi][PS_Deb_LastSwap_Offset],ecx
	pop	edi
ENDIF

	mov	ebx, edi
	xchg	bx, [PS_Free_Page_List]
	mov	eax, [PS_Next_Entry_Base]
	xchg	bx, WORD PTR [eax][edi*2]
	mov	WORD PTR [esi], bx
	inc	[PS_Free_Page_Count]

	clc

PS_Free_Exit:
	popad
	ret

EndProc PageSwap_Free_Page


;******************************************************************************
;
;   PageSwap_Read_Or_Write
;
;   DESCRIPTION:
;
;   ENTRY:
;	Current VM is in critical section
;	CH = 0 if read, 1 if write
;	EDX = Linear page number to read/write
;	EDI = PAGE offset in file to read/write
;	ESI = Linear address of memory we can touch
;
;   EXIT:
;	If carry set then
;	    ERROR:  Could not read/write page
;	else
;	    Page read successfully
;
;   USES:
;	Flags
;
;==============================================================================

BeginProc PageSwap_Read_Or_Write

	cmp	edi, [PS_Cur_File_Pages]
	jae	PS_ROW_Invalid_Page

	%OUT Fastdisk gets to PageSwap with ints disabled -- Figure out why for 3.1
	sti		 ; SOMEHOW GET HERE WITH INTS DISABLED!

	pushad

	test	[PS_Idle_Flags], PS_IF_Prepaging
	jnz	SHORT PS_ROW_Test_Disk_Type
	or	[PS_Idle_Flags], PS_IF_Restart

PS_ROW_Test_Disk_Type:
	cmp	[PS_Disk_Handle], 0
	je	PS_ROW_Stupid_Pager

;
;   Direct to hardware paging!
;
;   Wait until it is OK to read/write from our disk handle.  This wait will
;   also block us from reentrantly blasting out async write buffer
;
	mov	edx, ecx			; Save read/write status in EDX
	mov	eax, [PS_Disk_IO_Sem]
	mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Wait_Semaphore
	mov	[PS_IO_In_Progress], True

;
;   If this is a write operation copy the page to our buffer for async write
;
	.ERRNZ PS_Read_Data
	test	dh, dh
	jz	SHORT PS_ROW_Do_IO
	cld
	push	edi
	mov	ecx, 1000h / 4
	mov	edi, [PS_Async_Buff_Addr]
	rep movsd
	mov	esi, [PS_Async_Buff_Addr]
	pop	edi

;
;   Do read/write operation now
;
PS_ROW_Do_IO:
	mov	eax, [PS_Disk_Handle]		; Used for VHD
	lea	ebx, [edi*8]
	add	ebx, [PS_Int13_Base_Sector]	; EBX = Starting sector
	mov	ecx, 8				; Read/Write 8, 512 byte sectors
	mov	ch, [PS_Int13_Drive_Num]	; CH = Drive number (80h or 81h)
	and	ch, NOT 80h			; Reset high bit of drive #
	.ERRNZ PS_Read_Data
	test	dh, dh				; Q: Write data?
	jz	SHORT PS_ROW_Direct_Read	;    N: Read this page
	Queue_Out "Pageswap start Write"
	VxDcall VHD_Write			;    Y: Start write
	jmp	PS_ROW_Exit			;	AND EXIT RIGHT NOW!!!

PS_ROW_Direct_Read:
	Queue_Out "Pageswap start read"
	VxDcall VHD_Read

	VMMcall Get_System_Time
	push	eax
	mov	eax, [PS_Disk_IO_Sem]
	mov	ecx, (Block_Svc_If_Ints_Locked OR Block_Enable_Ints)
	VMMcall Wait_Semaphore
	VMMcall Signal_Semaphore
	pop	ecx
	VMMcall Get_System_Time
	sub	ecx, eax
	jz	PS_ROW_Exit
	mov	eax, ecx
	VMMcall Get_Cur_VM_Handle
	VMMcall Adjust_Execution_Time
	jmp	PS_ROW_Exit


;
;   Map the appropriate page into V86 address space
;
PS_ROW_Stupid_Pager:
	VMMcall Get_Cur_VM_Handle
	shr	esi, 12
	push	ecx
	VMMcall _LinMapIntoV86, <esi, ebx, [PS_Lin_Page_Num], 1, 0>
	pop	ecx
	test	eax, eax
IFDEF DEBUG
	jnz	SHORT PS_ROW_Mapped_OK
	Debug_Out "PAGESWAP ERROR:  Unable to map page"
PS_ROW_Mapped_OK:
ENDIF
	jz	PS_ROW_Cant_Map_Page

	mov	eax, [PS_Cache_Lock_Ptr]
	test	eax, eax
	jz	SHORT PS_ROW_Nest_Exec
	inc	BYTE PTR [eax]

PS_ROW_Nest_Exec:
	Push_Client_State
	VMMcall Begin_Nest_V86_Exec


	VMMcall Get_Cur_VM_Handle
	mov	ebp, [ebx.CB_Client_Pointer]

	mov	eax, [PS_V86_Stack_Seg_Off]
	mov	[ebp.Client_SP], ax
	shr	eax, 16
	mov	[ebp.Client_SS], ax

	cmp	[PS_Have_Partition], True
	jne	PS_ROW_Use_DOS

	add	ch, 2
	mov	[ebp.Client_AH], ch
	mov	[ebp.Client_AL], 1000h / PS_Sector_Size ; 8 Secs = 4K read/write

	lea	eax, [edi*8]			; EAX = Offset in file
	add	eax, [PS_Int13_Base_Sector]	; EAX = Off from start of disk
	xor	edx, edx
	idiv	[PS_Int13_Sec_Per_Track]
	mov	ecx, edx			; Remainder = Starting sector
	xor	edx, edx
	idiv	[PS_Int13_Num_Heads]
	;   ECX = Sector # (0 BASED!)
	;   EDX = Starting head
	;   EAX = Cylinder number
	inc	cl				; Make sector 1 based
	mov	ch, al				; CH = Cylinder number
	shl	ah, 6				; Move bits 6 and 7 up
	or	cl, ah				; Set high bits of cylinder
	mov	[ebp.Client_CX], cx		; Set cylinder and sector number
	mov	[ebp.Client_DH], dl
	mov	al, [PS_Int13_Drive_Num]
	mov	[ebp.Client_DL], al

	mov	eax, [PS_Lin_Page_Num]
	shl	eax, 8				; * 4K / 10h
	mov	[ebp.Client_ES], ax
	mov	[ebp.Client_BX], 0

	mov	edx, [ebp.Client_EAX]		; Save for possible retry
	mov	ecx, 3				; Retry up to 3 times
PS_ROW_Retry_Loop:
	mov	eax, 13h
	VMMcall Exec_Int
	test	[ebp.Client_Flags], CF_Mask
	jz	PS_ROW_End_Exec
	Trace_Out "WARNING:  PageSwap failed to read/write page through Int 13h.  Retry."
	mov	[ebp.Client_EAX], edx		; Restore original AH/AL
	loopd	PS_ROW_Retry_Loop		; Try up to 3 times
	jmp	PS_ROW_Error

;------------------------------------------------------------------------------
;   Read/Write to DOS file
;------------------------------------------------------------------------------
PS_ROW_Use_DOS:
	inc	[PS_DOS_IO_Count]
;
;   Set PSP to Win386.Exe PSP to make DOS calls
;
	call	PageSwap_Set_Our_PSP

;
;   Move file pointer to appropriate position
;
	mov	[ebp.Client_AX], 4200h
	mov	ax, [PS_File_Handle]
	mov	[ebp.Client_BX], ax
	shl	edi, 12
	mov	[ebp.Client_DX], di
	shr	edi, 16
	mov	[ebp.Client_CX], di
	Call_DOS
	jc	PS_ROW_Error

;
;   Set up for read or write.
;
	add	ch, 3Fh
	mov	[ebp.Client_AH], ch
	mov	[ebp.Client_CX], 1000h
	mov	eax, [PS_Lin_Page_Num]
	shl	eax, 8
	mov	[ebp.Client_DS], ax
	mov	[ebp.Client_DX], 0
	Call_DOS
	jc	SHORT PS_ROW_Error
	cmp	[ebp.Client_AX], 1000h
	jne	SHORT PS_ROW_Error

;
;   Restore original PSP
;
	call	PageSwap_Set_Client_PSP

	dec	[PS_DOS_IO_Count]

PS_ROW_End_Exec:
;
;   Map null memory over the page
;
	call	PageSwap_Map_Null

	VMMcall End_Nest_Exec
	Pop_Client_State

	mov	eax, [PS_Cache_Lock_Ptr]
	test	eax, eax
	jz	SHORT PS_ROW_Exit
	dec	BYTE PTR [eax]

PS_ROW_Exit:
	clc
	popad
	ret

;
;   OOOPPPSSS!!  It didn't work.  Return with carry set.
;
PS_ROW_Error:
	Debug_Out "PAGESWAP ERROR READING OR WRITING PAGES!  Type .VR to see status"
	call	PageSwap_Set_Client_PSP
	call	PageSwap_Map_Null
	VMMcall End_Nest_Exec
	Pop_Client_State

	mov	eax, [PS_Cache_Lock_Ptr]
	test	eax, eax
	jz	SHORT PS_ROW_Cant_Map_Page
	dec	BYTE PTR [eax]

PS_ROW_Cant_Map_Page:
	popad
	stc
	ret


PS_ROW_Invalid_Page:
	Debug_Out "ERROR:  Invalid file offset #EDI passed to PS_Read_Or_Write_Page!"
	stc
	ret

EndProc PageSwap_Read_Or_Write


;******************************************************************************
;
;   PageSwap_VHD_Call_Back
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

BeginProc PageSwap_VHD_Call_Back

	mov	[PS_IO_In_Progress], False
	mov	eax, [PS_Disk_IO_Sem]
	VMMcall Signal_Semaphore
	ret

EndProc PageSwap_VHD_Call_Back


;******************************************************************************
;
;   PageSwap_Map_Null
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = Cur_VM_Handle
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc PageSwap_Map_Null

	pushad

	VMMcall _GetNulPageHandle
	VMMcall Get_Cur_VM_Handle
	VMMcall _MapIntoV86, <eax, ebx, [PS_Lin_Page_Num], 1, 0, PageDEBUGNulFault>

	popad

	ret

EndProc PageSwap_Map_Null


;******************************************************************************
;
;   PageSwap_Set_Our_PSP
;
;   DESCRIPTION:
;
;   ENTRY:
;	None
;
;   EXIT:
;
;   USES:
;	EAX, Flags, All client registers and flags
;
;==============================================================================

BeginProc PageSwap_Set_Our_PSP

;
;   Get current PSP
;
	mov	[ebp.Client_AH], 51h
	Call_DOS
	mov	eax, [ebp.Client_EBX]
	mov	[PS_Client_PSP], ax

;
;   Set PSP to Win386.Exe PSP to make DOS calls
;
	movzx	eax, [PS_Our_PSP]

	mov	[ebp.Client_BX], ax

	shl	eax, 4
	push	[eax.PDB_User_stack]
	pop	[PS_Save_User_Stack]

	mov	[ebp.Client_AH], 50h
	Call_DOS

	ret

EndProc PageSwap_Set_Our_PSP


;******************************************************************************
;
;   PageSwap_Set_Client_PSP
;
;   DESCRIPTION:
;
;   ENTRY:
;	None (PageSwap_Set_Our_PSP must have been called previously)
;
;   EXIT:
;	None
;
;   USES:
;	EAX, Client registers, Flags
;
;==============================================================================

BeginProc PageSwap_Set_Client_PSP

	mov	ax, [PS_Client_PSP]
	mov	[ebp.Client_BX], ax
	mov	[ebp.Client_AH], 50h
	Call_DOS

;
;   Restore the user stack dword on the Win386 PSP
;
	movzx	eax, [PS_Our_PSP]
	shl	eax, 4
	push	[PS_Save_User_Stack]
	pop	[eax.PDB_User_stack]

	ret

EndProc PageSwap_Set_Client_PSP


;******************************************************************************
;			D E B U G G I N G   C O D E
;******************************************************************************


IFDEF DEBUG

;******************************************************************************
;
;   PageSwap_Debug_Query
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


BeginProc PageSwap_Debug_Query

	cmp	[PS_Max_File_Pages], 0
	ja	SHORT PS_DQ_Enabled
	Trace_Out "Demand paging is disabled"
	clc
	ret

PS_DQ_Enabled:
	mov	edi, [PS_DQ_Total_In]
	Trace_Out "Total calls to pageswap_in    = #EDI"
	test	edi, edi
	jz	SHORT Skip_Read_Perf
	mov	ecx, [PS_DQ_Read_In]
	mov	eax, ecx
	xor	edx, edx
	imul	eax, 100
	idiv	edi
	VMMcall Debug_Convert_Hex_Decimal
	Trace_Out "Total number of pages read    = #ECX (#AX%)"
	xor	edx, edx
	mov	eax, [PS_DQ_Read_Time]
	mov	ebx, 1000
	idiv	ebx
	VMMcall Debug_Convert_Hex_Decimal
	xchg	eax, edx
	VMMcall Debug_Convert_Hex_Decimal
	shl	eax, 4
	Trace_Out "Total time for page in        = #EDX.#AX (decimal)"
	jecxz	SHORT Skip_Read_Perf
	mov	eax, ecx
	xor	edx, edx
	shl	eax, 12
	mov	ebx, 1000
	imul	ebx
	idiv	[PS_DQ_Read_Time]
	VMMcall Debug_Convert_Hex_Decimal
	Trace_Out "Bytes per second              = #EAX (decimal)"
Skip_Read_Perf:


	Trace_Out " "

	mov	edi, [PS_DQ_Total_Out]
	Trace_Out "Total calls to pageswap_out   = #EDI"
	test	edi, edi
	jz	SHORT Skip_Write_Perf
	mov	ecx, [PS_DQ_Written_Out]
	mov	eax, ecx
	xor	edx, edx
	imul	eax, 100
	idiv	edi
	VMMcall Debug_Convert_Hex_Decimal
	Trace_Out "Total number of pages written = #ECX (#AX%)"
	xor	edx, edx
	mov	eax, [PS_DQ_Write_Time]
	mov	ebx, 1000
	idiv	ebx
	VMMcall Debug_Convert_Hex_Decimal
	xchg	eax, edx
	VMMcall Debug_Convert_Hex_Decimal
	shl	eax, 4
	Trace_Out "Total time for page out       = #EDX.#AX (decimal)"
	jecxz	Skip_Write_Perf
	mov	eax, ecx
	xor	edx, edx
	shl	eax, 12
	mov	ebx, 1000
	imul	ebx
	idiv	[PS_DQ_Write_Time]
	VMMcall Debug_Convert_Hex_Decimal
	Trace_Out "Bytes per second              = #EAX (decimal)"

Skip_Write_Perf:
	Trace_Out " "
	Trace_Out "Press any key to continue, [ESC] to quit...", NO_EOL
	VMMcall In_Debug_Chr
	Trace_Out " "
	jz	PS_DQ_Exit

	Trace_Out " "
	Trace_Out " "


	Trace_Out "Swap file = ", No_EOL
	pushad
	mov	esi, OFFSET32 PS_Our_File_Name
	VMMcall Out_Debug_String
	popad
	Trace_Out " "
	mov	edi, [PS_Max_File_Pages]
	mov	eax, [PS_Cur_File_Pages]
	Trace_Out "File contains #EAX pages of a possible #EDI"
	mov	eax, [PS_Free_Page_Count]
	mov	ecx, eax
	xor	edx, edx
	imul	eax, 100
	idiv	edi
	VMMcall Debug_Convert_Hex_Decimal
	Trace_Out "#ECX pages are on free list (#AX%)"
	Trace_Out " "
	mov	ax, [PS_Our_PSP]
	Trace_Out "Win386 startup PSP = #AX"
	mov	ax, [PS_File_Handle]
	Trace_Out "File handle        = #AX"
	Trace_Out " "
	mov	eax, [PS_Orig_DOS_Vector]
	mov	ebx, eax
	shr	eax, 10h
	Trace_Out "Original DOS vector  = #AX:#BX"
	Trace_Out " "
	mov	eax, [PS_Lin_Page_Num]
	Trace_Out "V86 lin mapping page = #AX"
	mov	eax, [PS_Hash_Table_Base]
	Trace_Out "Hash table lin base  = #EAX"
	mov	eax, [PS_Page_Entry_Base]
	Trace_Out "Page entry lin base  = #EAX"
	mov	eax, [PS_Next_Entry_Base]
	Trace_Out "Next entry lin base  = #EAX"
	mov	eax, [PS_Next_Possible_Victim]
	Trace_Out "Next possible victim = #EAX"
	Trace_Out " "
	Trace_Out "Current state: ", NO_EOL
	mov	esi, [PS_Debug_State]
	mov	esi, PS_DQ_State_Tab[esi*4]
	pushad
	VMMcall Out_Debug_String
	popad
	Trace_Out " "

	xor	ecx, ecx
	mov	ebx, [PS_Page_Entry_Base]

PS_DQ_Pause:
	Trace_Out " "
	Trace_Out "Press any key to continue, [ESC] to quit...", NO_EOL
	VMMcall In_Debug_Chr
	Trace_Out " "
	jz	PS_DQ_Exit
	Trace_Out " "

PS_DQ_Print_Page_Info:
	mov	edx, DWORD PTR [ebx][ecx*4]
	and	edx, NOT PSF_Recent_Swap_In
	call	PageSwap_Find_Page
	jc	PS_DQ_Free
	cmp	edi, ecx
	jne	PS_DQ_Free

	Trace_Out "#CX #EDX   ", No_EOL

	push	ecx
	mov	ecx,[PS_Deb_Struc_Base]
	lea	edi,[edi][edi*8]	; EDI * 9
	lea	edi,[ecx][edi*2]	; base + EDI * 18

	xor	eax,eax
	xor	ecx,ecx
	mov	cx,[edi][PS_Deb_CountIn_Offset]
	test	cx,cx
	jz	SHORT PS_DQ_Analysis_out1
	mov	eax,[edi][PS_Deb_TimeIn_Offset]
	xor	edx,edx
	div	ecx
PS_DQ_Analysis_out1:
	VMMCall	Debug_Convert_Hex_Decimal
	xchg	eax,ecx
	VMMCall	Debug_Convert_Hex_Decimal
	Trace_Out "In: #AX, avg #ECX msec ; ", No_EOL
	xor	ecx,ecx
	xor	eax,eax
	mov	cx,[edi][PS_Deb_CountOut_Offset]
	test	cx,cx
	jz	SHORT PS_DQ_Analysis_out2
	mov	eax,[edi][PS_Deb_TimeOut_Offset]
	xor	edx,edx
	div	ecx
PS_DQ_Analysis_out2:
	VMMCall	Debug_Convert_Hex_Decimal
	xchg	eax,ecx
	VMMCall	Debug_Convert_Hex_Decimal
	Trace_Out "Out: #AX, avg #ECX msec"
	pop	ecx

	jmp	SHORT PS_DQ_Next_Page

PS_DQ_Free:
	Trace_Out "#CX --Free--      "


PS_DQ_Next_Page:
	inc	ecx
	cmp	ecx, [PS_Cur_File_Pages]
	jae	SHORT PS_DQ_Exit
	test	ecx, 1111b
	jz	PS_DQ_Pause
	jmp	PS_DQ_Print_Page_Info

PS_DQ_Exit:
	clc
	ret

EndProc PageSwap_Debug_Query



;******************************************************************************
;
;   PS_analyze_swapin
;
;   DESCRIPTION:
;	maintains a running total of time page spent swapped out, as well
;	as a count of out-swaps.
;
;   ENTRY:
;	EAX ->	system time
;	EDX ->	linear address of page
;
;   EXIT:
;
;   USES:
;
;==============================================================================


BeginProc	PS_analyze_swapin

	push	ebx
	push	ecx
	push	edx
	push	edi

	push	esi
	shr	edx,12
	call	PageSwap_Find_Page	; EDI contains index into table now
	pop	esi
	jc	SHORT analyze_swapin_exit	; if page not found, no stats

	push	eax			; save current system time
	mov	ebx,[PS_Deb_Struc_Base]
	lea	edi,[edi][edi*8]	; index * 9
	lea	edi,[ebx][edi*2]	; EDI = base + index * 18
					; (points to current debug struc)

	mov	ecx,[edi][PS_Deb_LastSwap_Offset]	; last swap time
	test	ecx,ecx			; if zero, then has never swapped
	jz	SHORT analyze_swapin_settime	; so can't compute time since last

	sub	eax,ecx			; EAX contains time since last swap
	add	[edi][PS_Deb_TimeOut_Offset],eax
					; update total time swapped-out
	inc	WORD PTR [edi][PS_Deb_CountOut_Offset]
					; another swap-out period complete

analyze_swapin_settime:
	pop	eax			; restore system time
	mov	[edi][PS_Deb_LastSwap_Offset],eax
					; save for next time around

analyze_swapin_exit:
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx

	ret
 
EndProc PS_analyze_swapin


;******************************************************************************
;
;   PS_analyze_swapout
;
;   DESCRIPTION:
;	maintains a running total of time page spent swapped in, as well
;	as a count of in-swaps.
;
;   ENTRY:
;	EAX ->	system time
;	EDX ->	linear address of page
;
;   EXIT:
;
;   USES:
;
;==============================================================================


BeginProc	PS_analyze_swapout

	push	ebx
	push	ecx
	push	edx
	push	edi

	push	esi
	shr	edx,12
	call	PageSwap_Find_Page	; EDI contains index into table now
	pop	esi
	jc	SHORT analyze_swapout_exit	; if page not found, no stats

	push	eax			; save current system time
	mov	ebx,[PS_Deb_Struc_Base]
	lea	edi,[edi][edi*8]	; index * 9
	lea	edi,[ebx][edi*2]	; EDI = base + index * 18
					; (points to current debug struc)
	mov	ecx,[edi][PS_Deb_LastSwap_Offset]	; last swap time
	test	ecx,ecx			; if zero, then has never swapped
	jz	SHORT analyze_swapout_settime	; so can't compute time since last

	sub	eax,ecx			; EAX contains time since last swap
	add	[edi][PS_Deb_TimeIn_Offset],eax
					; update total time swapped-in
	inc	WORD PTR [edi][PS_Deb_CountIn_Offset]
					; another swap-in period complete

analyze_swapout_settime:
	pop	eax			; restore system time
	mov	[edi][PS_Deb_LastSwap_Offset],eax
					; save for next time around

analyze_swapout_exit:
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx

	ret
 
EndProc PS_analyze_swapout



ENDIF

VxD_LOCKED_CODE_ENDS

	END PageSwap_Real_Init
