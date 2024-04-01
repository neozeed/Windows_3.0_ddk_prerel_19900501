page,132
;---------------------------Module-Header-------------------------------;
; Module Name: IBMCOM.ASM
;
; !!!
;
; Created: Fri 06-Feb-1987 10:45:12
; Author:  Walt Moore [waltm]
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
; General Description:
;
; History:
;
;   ***************************************************************
;	Tue Dec 19 1989 09:32:15   -by-  Amit Chatterjee [amitc]
;   ---------------------------------------------------------------
;   Modified the 'InitAPort' routine called from 'ReactivateOpenCommPort'.
;   If the out queue for a port has characters to send out then we must
;   restart the trasmission process by faking a comm interrupt on that
;   port.
;   ***************************************************************
;	Tue Nov 21 1989 09:46:50    -by- Amit Chatterjee [amitc]
;   ---------------------------------------------------------------
;   The base port addresses in the COMM1,COMM2,COMM3,COMM4 structures
;   are being zeroed out when the corresponding comm port is closed.
;   This is because the  'ReactivateOpenCommPort' function looks at it
;   and if the port address is not zero decides that comm ports are
;   open. 
;   ***************************************************************
;	Tue Nov 14 1989 18:42:00     ADDED TWO EXPORTED FUNCTIONS
;   ---------------------------------------------------------------
;   Added two exported functions 'SuspendOpenCommPorts' and 
;   'ReactivateOpenCommPorts' for 286 winoldap support. The first one simply 
;   releases the comm int vects and installs the originall one, the second one
;   hooks back the comm driver comm vectors and then reads the receive buffer,
;   the status and the IIR registers of all the available comm ports to 
;   remove pending interrupts. It also reprograms the PIC to enable interrupts
;   on all open comm channels.
;   ---------------------------------------------------------------
;   -by- Amit Chatterjee [amitc]    
;   ***************************************************************
;	Tue Aug 30 198? 12:52:00      MAJOR FIX TO HANDLE 8250B
;   ---------------------------------------------------------------
;   
;   8250B has the following peculiar charactersistic
;             . The very first time (after reset) the Tx Holding Empty
;               interrupt is enabled, an immediate interrupt is generated
;
;             . After the first time, switching the Tx Holding Empty
;               interrupt enable bit from disabled to enabled will NOT
;               generate an immediate interrupt (unlike in 8250)
;       Because of this the KICKTX routine fails to set the transmit cycle
;       on if the machine has a 8250B
;   
;       This has been taken care as follows:
;             . For the very first byte that is being transmitted, KICKTX
;               is used to generate the first Tx Holding Empty interrupt
;             . Subsequently, whenever we find that the transmit buffer
;		is empty, we use a SOFTWARE INT (either INT 0Bh, or INT 0Ch)
;               to force the first character out, once this is done the
;               Tx Holding Empty interrupt will be generated once the buffer
;               really is empty
;             . Now we no longer disable the Tx Holding Empty interrupt
;               in the Xmit ISR to ensure that even m/cs with 8250, use
;               the software int to kick the tx interrupt on after the
;               first time.
;             . The software interrupt is also forced whenever an X-ON 
;               character is received.   
;
;       The code that implements the above logic is marked out with a line
;       asterixes.   
;   ------------------------------------------------------------------
;   -by- Amit Chatterjee [amitc]    
;       ******************************************************************
;
;   062587   HSFlag and Evtmask in DoLPT.  These fields do not exist
;      for LPT type devices.  The code which manipulated them
;      was removed
;
;      KickTx from $SndCom - interrupts were not disabled when
;      calling KickTx.
;
;      $SetCom - added CLD at the start
;
;      $SetQue - movsw ==> stosw
;
;       111285  Changed the Timeout from 7 to 30 seconds.
;
;       110885  Forgot to set EV_RxChar event when a character
;               was received.
;
;       102985  INS8250, INS8250B bug with enabling interrupts.
;               Setting ACE_ETBEI in the Interrupt Enable Register
;               will cause an immediate interrupt regardless of
;               whether the transmitter register is empty or not.
;               The first interrupt MAY also be missed.
;
;               The first case is not a problem since we only enable
;               interrupts if the transmitter register is empty.  The
;               second problem was showing up on Microsoft System Cards
;               in PC-XTs.  The first interrupt was missed after a cold
;               boot.  National claims the fix is to write the register
;               twice, which SEEMS to work...
;
;               Added timeout code to $TRMCOM.  If the number of
;               characters in the output queue doesn't decrease
;               in "Timeout" seconds, then the port will be closed
;               anyway.  Also flushed the input queue and added a
;               discard-input flag for the data available interrupt
;               code to discard any input received while terminating
;               a port.  $TRMCOM will return an error code if it
;               discarded any output data.
;
;               Removed infinite timeout test in MSRWait routine.
;               Still bad, but it will timeout around 65 seconds
;               instead of never.
;
;       102785  LPT initialization code was jumping to InitCom90,
;               which was setting EFlags[si] to null.  Well, LPTs
;               don't have an EFlags field, so the null was getting
;               stuffed over the LSB of BIOSPortLoc of the next LPT
;               device.
;
;       101185  Save interrupt vector when opening a comm port
;               and restore it when closing.  Would you believe
;               there are actually programs that assume the
;               vector points to a non-specific 8259 ACK and
;               an IRET!
;
;       100985  Added MS-NET support to gain exclusive control
;               of an LPT port if DOS 3.x and not running in as
;               a server, receiver, or messenger.   Required to
;               keep another application, such as command.com
;               from closing the stream or mixing their output
;               with ours.
;-----------------------------------------------------------------------;

title   IBMCom - IBM PC, PC-XT, PC-AT, PS/2 Communications Interface

.xlist
include cmacros.inc
include comdev.inc
include ins8250.inc
include ibmcom.inc
.list

MULTIPLEX   equ       2Fh	    ; multiplex interrupt number
GET386API   equ     1684h	    ; Get API entry point from VxD
VPD	    equ     000Fh	    ; driver ID of VPD driver
VPD_GETPORT equ     0004h	    ; function: assign port to current VM
VPD_RELPORT equ     0005h	    ; function: release port
VCD	    equ     000Eh	    ; driver ID of VCD driver
VCD_GETPORT equ     0004h	    ; function: assign port to current VM
VCD_RELPORT equ     0005h	    ; function: release port

MESSAGEBOX  equ     1		    ; export ordinal of MessageBox()
MB_TASKMODAL equ    2000h
MB_YESNO    equ     0004h	    ; messagebox flags
MB_ICONEXCLAMATION equ 0030h
IDYES	    equ     6

; The DOS Extender used for Standard mode Windows remaps the master 8259 from
; Int vectors 8h-Fh to 50h-57h.  In order to speed up com port interrupt
; response as much as possible, the COMM driver hooks real mode interrupts
; when running in Standard mode.  It currently uses the following adjustment
; value to hook the real hardware int vector.  When time permits, this
; HARDCODED equate should be changed to be adjustible at run time.

DOSX_IRQ_ADJ	equ	(50h - 8h)	; Adjustment for DOSX remapping the
					; master 8259 from 8h to 50h

externFP CreateSystemTimer
externFP KillSystemTimer
externFP get_int_vector
externA  __F000h

externW  deb_com1
externW  deb_com2
externW  deb_com3
externW  deb_com4

externD  OldIntVecIntB
externD  OldIntVecIntC
externD  OurIntVecIntB
externD  OurIntVecIntC
externB  OldMask8259IRQ3
externB  OldMask8259IRQ4

externFP AllocCStoDSAlias
externFP FreeSelector
externFP GetSelectorBase
externFP GetSystemMsecCount
externFP GetModuleHandle
externFP GetProcAddress
externW  RM_IntDataSeg
externD  RealModeIntVectB
externD  RealModeIntVectC
externD  RMOurIntVectB
externD  RMOurIntVectC
externA  CODE_DIFFERENCE
externA  __WinFlags

sBegin   Data

        public  Comm1
        public  Comm2
        public  Comm3                                    ;[rkh] ...
        public  Comm4
Comm1	ComDEB	<,,,RS232B+0,,,,,,,,,,,,,,,,,,,,,,,,,>
Comm2	ComDEB	<,,,RS232B+2,,,,,,,,,,,,,,,,,,,,,,,,,>
Comm3	ComDEB	<,,,RS232B+4,,,,,,,,,,,,,,,,,,,,,,,,,>
Comm4	ComDEB	<,,,RS232B+6,,,,,,,,,,,,,,,,,,,,,,,,,>

        public  LPT1
        public  LPT2
        public  LPT3
LPT1    LptDEB  <,,,LPTB+0>            ;LPT1 device equipment block
LPT2    LptDEB  <,,,LPTB+2>            ;LPT2 device equipment block
LPT3    LptDEB  <,,,LPTB+4>            ;LPT3 device equipment block

public myMachineID
public IntVecIntBcount
public IntVecIntCcount

myMachineID       db 0                 ;PC=0, PS/2=1 [rkh]
IntVecIntBcount   db 0
IntVecIntCcount   db 0

$MachineID        db 0                 ;IBM Machine ID
InitRetry         dw 0                 ;LPT Retry count
First_After_Boot  db 0ffh
TicCount	  dw 0		       ;Timeout counter

public lpfnMessageBox, lpfnVPD, fVPD, szMessage, pLPTByte, szTitle

lpfnMessageBox	  dd 0

lpfnVPD 	  dd 0		; far pointer to win 386 VPD entry point
fVPD		  db 0		; 0-not checked, 1 vpd present, -1 no vpd
lpfnVCD 	  dd 0		; far pointer to win 386 VCD entry point
fVCD		  db 0		; 0-not checked, 1 vcd present, -1 no vcd
VCD_XMIT_CALLBACK dd 0

szMessage   db 'The LPT'
pLPTByte    db '?'
	    db ' port is currently assigned to a DOS application.  Do you '
	    db 'want to reassign the port to Windows?',0
szCOMMessage db 'The COM'
pCOMByte    db '?'
	    db ' port is currently assigned to a DOS application.  Do you '
	    db 'want to reassign the port to Windows?',0
szTitle     db 'Device Conflict',0
szUser	    db 'USER',0

public DosXFlag

DosXFlag	  db -1
   ;flag => (-1) - haven't checked if 286 DOS extender present
   ;	    (0)  - DOSX is not present
   ;	    (1)  - DOSX is installed
RM_IntCodeSegment    dw  0
   ; segment (NOT selector) of _INTERRUPT code selector

sEnd Data

ROMBios           segment  at 0F000h
                  org         0FFFEh

MachineID label byte
RomBios Ends


createSeg _INTERRUPT,IntCode,word,public,CODE
sBegin IntCode
assumes cs,IntCode

	externFP FakeCOMIntFar
	externFP CommIntFar

sEnd IntCode

createSeg _INTERRUPT,IntCode,word,public,CODE

sBegin Code
assumes cs,Code
assumes ds,Data

page

;----------------------------Private-Routine----------------------------;
; SegmentFromSelector
;
;   Converts a selector to a segment...note that this routine assumes
;   the memory pointed to by the selector is below the 1Meg line!
;
; Params:
;   AX = selector to convert to segment
;
; Returns:
;   AX = segment of selector given
;
; Error Returns:
;   None
;
; Registers Destroyed:
;   none
;
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public SegmentFromSelector
SegmentFromSelector proc far

    push    bx
    push    dx
    push    ds				;save stuff that kernel routine craps

    cCall   GetSelectorBase,<ax>	;DX:AX = segment of selector
rept 4
    shr     dx,1
    rcr     ax,1
endm
    ;AX now points to interrupt data *segment*

    pop     ds
    pop     dx
    pop     bx
    ret

SegmentFromSelector endp

;----------------------------Private-Routine----------------------------;
;
; IBMModel - Check which IBM model
;
; Inquire in BIOS area which model IBM
;
; Entry:
;   None
;
; Returns:
;   C - set IBM PC, XT, AT or PS/2 Model 30?? (i.e. port address 2E8 & 3E8)
;   C - clear IBM PS/2 Model 50, 60, 70 and 80 (i.e. port address at 40:4, 40:6)
;
; Error Returns:
;   None
;
; Registers Destroyed:
;   FLAGS
;
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public IBMmodel                     ; [rkh]...
IBMmodel proc far
      push  ax
      push  bx
      push  es

      mov   ah, 0c0h
      int   15h
      jc    OldBios

      assumes es,nothing

      cmp   by es:[bx+2], 0f8h      ; PS/2 80
      je    NewBios

      cmp   by es:[bx+2], 0fch	    ; AT or PS/2 50 or 60
      jne   OldBios		    ; assume OldBios

      cmp   by es:[bx+3], 04h       ; PS/2 50
      je    NewBios
      cmp   by es:[bx+3], 05h       ; PS/2 60
      jne   OldBios		    ; AT

NewBios:
      or    ah,ah
      mov   myMachineID,1           ; PS/2
      jmp   short IBMmodel_exit

OldBios:
      stc

IBMmodel_exit:
      pop   es
      pop   bx
      pop   ax
      ret

IBMmodel endp

page

;----------------------------Private-Routine----------------------------;
;
;   Contention_Dlg
;
;   If running under Win386, this routine can be called to ask the user to
;   resolve contention for a COM or LPT port.
;
;   entry - CX is offset of message string for dialog box
;
;   exit  - Z flag set, if user specified that Windows should steal the port

Contention_Dlg proc near
PUBLIC Contention_Dlg

    xor     ax,ax
    push    ax			; hwndOwner
    push    ds
    push    cx			; message ptr

    cmp     word ptr lpfnMessageBox[2], 0   ;Q: ptr to MessageBox proc valid?
    jne     short gmbp_done		    ;	Y: we can call it
    push    ds				    ;	N: get module handle of USER
    lea     ax, szUser
    push    ax
    cCall   GetModuleHandle

    push    ax				    ; module handle
    mov     ax, MESSAGEBOX
    cwd
    push    dx
    push    ax
    cCall   GetProcAddress
    mov     word ptr lpfnMessageBox[0], ax  ; save received proc address
    mov     word ptr lpfnMessageBox[2], dx
gmbp_done:

    push    ds
    lea     ax, szTitle
    push    ax
    mov     ax, MB_ICONEXCLAMATION or MB_YESNO or MB_TASKMODAL
    push    ax
    cCall   lpfnMessageBox
    cmp     ax, IDYES		; user allows us to take the port?
    ret
Contention_Dlg endp


;----------------------------Private-Routine----------------------------;
;
;   GetPort386
;
;   If running under Win386, tell the VPD to assign an LPT port to us.
;   The comm driver will handle contention.
;
;   entry - DI contains offset in ROM area of port...
;		8 - LPT1, A - LPT2, etc
;
;   exit  - registers saved, carry = clear if OK to proceed, set if
;	    user won't allow assignment of port or Win386 error
;

GetPort386  proc near
public GetPort386

    cmp     fVPD, 0
    jnz     getport_AlreadyChecked

    push    di

    xor     di, di
    mov     es, di
    mov     ax, GET386API
    mov     bx, VPD
    int     MULTIPLEX
    mov     word ptr lpfnVPD[0], di
    mov     word ptr lpfnVPD[2], es
    mov     ax, es
    or	    ax, di

    pop     di
    jnz     short getport_CallVPD

    mov     fVPD, -1

getport_VPDNotInstalled:
    clc
    jmp     short getport_exit

getport_AlreadyChecked:
    jl	    getport_VPDNotInstalled

getport_CallVPD:
    mov     fVPD, 1
    push    di
    sub     di, LPTB
    shr     di, 1		; turn DI into port number

    xor     ax, ax
    mov     dx, VPD_GETPORT
    mov     cx, di
    call    lpfnVPD
    jnc     getport_gotit

;   port owned by another VM... ask the user for it

    add     cl, '1'		; fix up the port name...
    mov     pLPTByte, cl	; HACK HACK HACK
    lea     cx, szMessage
    call    Contention_Dlg
    jnz     getport_userwontallow

    mov     ax, 1		; tell win386 we really do want it
    mov     cx, di		;
    mov     dx, VPD_GETPORT	;
    call    lpfnVPD		; return with C set or clear...
    jmp     short getport_gotit

getport_userwontallow:
    stc

getport_gotit:
    pop     di

getport_exit:
    ret

GetPort386  endp

;----------------------------Private-Routine----------------------------;
;
;   ReleasePort386
;
;   If running under Win386, tell the VPD to deassign an LPT port.
;
;   entry - DI contains offset in ROM area of port...
;		8 - LPT1, A - LPT2, etc
;

ReleasePort386	proc near

    cmp     fVPD, 1
    jne     release_noVPD

    push    di
    sub     di, LPTB
    shr     di, 1
    mov     cx, di
    mov     dx, VPD_RELPORT
    call    lpfnVPD
    pop     di

release_noVPD:
    ret

ReleasePort386	endp


;----------------------------Private-Routine----------------------------;
;
;   GetCOMport386
;
;   If running under Win386, tell the VCD to assign a COM port to us.
;   The comm driver will handle contention.
;
;   entry - DI contains offset in ROM area of port...
;		0 - COM1, 2 - COM2, etc
;	    DS:SI -> COMDEB
;
;   exit  - registers saved, carry = clear if OK to proceed, set if
;	    user won't allow assignment of port or Win386 error
;
GetCOMport386 proc near
public GetCOMport386

    push    es
    push    ax
    push    bx
    cmp     fVCD, 0
    jnz     getcomport_AlreadyChecked

    push    di
    xor     di, di
    mov     es, di
    mov     ax, GET386API
    mov     bx, VCD
    int     MULTIPLEX
    mov     word ptr lpfnVCD[0], di
    mov     word ptr lpfnVCD[2], es
    mov     ax, es
    or	    ax, di
    pop     di

    jnz     short getcomport_CallVCD

getcomport_checknovcd:
    mov     fVPD, -1

getcomport_VCDNotInstalled:
    clc
    jmp     short getcomport_exit

getcomport_AlreadyChecked:
    jl	    getcomport_VCDNotInstalled

getcomport_CallVCD:
    mov     fVCD, 1
    push    si
    push    di
    shr     di, 1		; turn DI into port number

    xor     ax, ax
    mov     dx, VCD_GETPORT
    mov     cx, di
    call    lpfnVCD		; return with C set or clear...
    jnc     getcomport_gotit

;   port owned by another VM... ask the user for it

    add     cl, '1'		; fix up the port name...
    mov     pCOMByte, cl	; HACK HACK HACK
    lea     cx, szCOMMessage
    call    Contention_Dlg
    jnz     getcomport_userwontallow

    mov     ax, 1		; tell win386 we really do want it
    mov     cx, di		;
    mov     dx, VCD_GETPORT	;
    call    lpfnVCD
    jmp     short getcomport_gotit

getcomport_userwontallow:
    stc

getcomport_gotit:
    mov     word ptr [VCD_XMIT_CALLBACK], di
    mov     word ptr [VCD_XMIT_CALLBACK+2], es
    pop     di
    pop     si

getcomport_exit:
    pop     bx
    pop     ax
    pop     es
    ret

GetCOMport386 endp

;----------------------------Private-Routine----------------------------;
;
;   ReleaseCOMport386
;
;   If running under Win386, tell the VPD to deassign an LPT port.
;
;   entry - DI contains offset in ROM area of port...
;		8 - LPT1, A - LPT2, etc
;

ReleaseCOMport386  proc near

    cmp     fVCD, 1
    jne     release_noVCD

    push    di
    shr     di, 1
    mov     cx, di
    mov     dx, VCD_RELPORT
    call    lpfnVCD
    pop     di

release_noVCD:
    ret

ReleaseCOMport386  endp



;----------------------------Public Routine-----------------------------;
;
; $INICOM - Initialize A Port
;
; Initalizes the requested port if present, and sets
; up the port with the given attributes when they are valid.
; This routine also initializes communications buffer control
; variables.  This routine is passed the address of a device
; control block.
;
; The RLSD, CTS, and DSR signals should be ignored by all COM
; routines if the corresponding timeout values are 0.
;
; For the LPT ports, a check is performed to see if the hardware
; is present (via the LPT port addresses based at 40:8h.  If the
; port is unavailable, an error is returned.  If the port is
; available, then the DEB is set up for the port.  $SETCOM will
; be called to set up the DEB so that there will be something
; valid to pass back to the caller when he inquires the DEB.
;
; No hardware initialization will be performed to prevent the
; RESET line from being asserted and resetting the printer every
; time this routine is called.
;
; Entry:
;   EX:BX --> Device Control Block with all fields set.
; Returns:
;   AX = 0 if no errors occured
; Error Returns:
;   AX = initialization error code otherwise
; Registers Preserved:
;   None
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public  $INICOM
$INICOM  proc  near
         push  si                      ;As usual, save register variables
         push  di
         mov   ah,es:ID[bx]            ;Get device i.d.
         call  GetDEB                  ;--> DEB for this device
	 jnc   InitCom30	       ;Valid device

InitCom10:
         mov   ax,IE_BadID             ;Show bad device

InitCom20:
	 jmp   InitCom100	       ;Return

InitCom30:
         push  ds
         mov   di,BIOSPortLoc[si]      ;Get offset to port address
	 mov   ax,__F000h	       ;Get Machine ID
         mov   ds,ax
         assumes ds,ROMBios

         mov   al,[MachineID]

;  Determine if the hardware for the requested device is present. This is
;  flagged at boot time by the BIOS, which places the I/O address of the:
;     Comm ports at 40:0 and 40:2 (COM1 and COM2) if the hardware is present
;     Comm ports at 40:4 and 40:6 (COM3 and COM4) if the hardware is present
;        on PS/2 machines otherwise, assume ports at frequently used address
;     LPT ports at 40:8, 40:A, and 40:C if the hardware is present.

         mov   cx,40h                  ;[rkh] ...
         mov   ds,cx                   ;Point DS: at ROS Save Area.
         assumes ds,nothing

	 cmp   di, LPTB
	 jb    InitCom_CheckComX

	 mov   cx, wo [di]
	 jcxz  InitCom34	       ; if zero, no hardware

	 cmp   ch, 0		       ; zero hibyte -> not valid (redir)
	 jz    InitLPT_Redir

	 cmp   di, LPTB 	       ; LPT1?
	 jz    InitLPT_Installed       ; yes, must be installed

	 cmp   cx, wo [di-2]	       ; duplicate of previous port
	 jnz   InitLPT_Installed       ; if not, installed hardware

InitLPT_Redir:
	 pop   ds		       ; get DS back
	 mov   ax, IE_BADID	       ; call it a bad id (spooler uses DOS)
	 jmp   InitCom100

InitLPT_Installed:
if 0				       ; THIS PAINS 386
	 mov   wo [di], 0	       ; reserved port
endif
	 jmp   short InitCom34	       ; do more stuff

InitCom_CheckComX:
if 0				       ; THIS PAINS 386
	 sub   cx,cx
	 xchg  cx, wo [di]	       ; get port address
else
	 mov   cx, wo [di]
endif

         cmp   cx,0                    ; Found a port?
	 jne   InitCom34	       ;  yes, continue

         cmp   di,4                    ; COM3 ?
	 je    InitCom32	       ;  yes, check PC or PS/2
         cmp   di,6                    ; COM4 ?
	 je    InitCom32	       ;  yes, check PC or PS/2
	 jmp   short InitCom34	       ;  no, continue, no port available

InitCom32:
         call  IBMmodel
	 jnc   InitCom34	       ; PS/2, then port not available

         mov   cx,3e8h                 ; assumed COM3 base port for PC
         cmp   di,4                    ; COM3 offset ?
	 je    InitCom34	       ;   yes, continue
         mov   cx,2e8h                 ;   no, assumed COM4 base port for PC

InitCom34:
         pop   ds                      ;Done playing with DS (for a while)
         assumes ds,Data

         mov   [$MachineID],al         ;Save now that DS is restored
         mov   ax,IE_Hardware          ;Hardware present?
	 jcxz  InitCom20	       ;  No, return error code
         mov   Port[si],cx             ;Save the port address
         test  di,LPTB-RS232B          ;Is this an LPT port?
	 jz    InitCom40	       ;  No, set up interrupt stuff

; ***  Set up parallel port ***
;
	 push  cx		       ; save old port address
	 call  $SETCOM		       ;Set up LPT Device

	 call  GetPort386	       ; tell win386 we're using the port
	 pop   cx		       ; get it back
	 jnc   InitCom37	       ; if no error (no contention), go ahead

InitCOM345:
if 0				       ; THIS PAINS 386
	 push  ds
	 mov   ax, 0040h
	 mov   ds, ax
	 mov   ds:[di], cx	       ; restore port address
	 pop   ds
endif

	 mov   ax, IE_OPEN	       ; port already open (by another VM)

	 jmp   InitCom100	       ; return error

if 0
;
;   don't need this... doing direct writes
;
         mov   ax,30                   ;Assume timeout for an AT (~2 mSec)
         cmp   [$MachineID],0FCh       ;Is this an AT?
	 je    InitCom35	       ;  Yes
         mov   ax,10                   ;  No, set XT timeout (~2 mSec)

InitCom35:
	 mov	[InitRetry],ax

;   Check to see if running under >= DOS 3.x and need to gain
;   exclusive control of the spooler for the given LPT port.

         call  TestInt2F               ;See if int 2F calls allowed
	 jc    InitCom37	       ;Cannot make int 2F calls
         mov   dl,ID[si]               ;Set port number for checking
         and   dx,11b                  ;  for Int 2F
         mov   ax,Lock2F               ;Gain exclusive control
         int   2Fh
         mov   ax,Open                 ;Start a new stream
         int   2Fh
         mov   bx,BIOSPortLoc[si]      ;  incase redirector stuffed
         push  ds
         mov   cx,40h                  ;  a non-zero value there
         mov   ds,cx
         assumes ds,nothing

         xor   cx,cx                   ;Zero BIOS port address just
         mov   word ptr [bx],cx
         pop   ds
	 assumes ds,Data

endif

InitCom37:
	 jmp   InitCom95	       ;That's all

; ***  Set up serial port ***
;
InitCom40:
	 push  cx
	 call  GetCOMport386
	 pop   cx
	 jc    InitCOM345
	 push  es		       ;Save these registers
	 push  di
	 push  cx		       ;needed later for $SETCOM etc
	 push  bx

         call  get_int_vector          ;Get interrupt vector for port/deb
				       ;AX,DI:DX will have return values

         mov   wo Mask8259[si],ax      ;Save 8259 int mask, int vector
         .errnz   IntVecNum-Mask8259-1

	 cmp   ah,IRQ3		       ;on interrupt 0x0B (IRQ3) ?
	 jne   InitCom41	       ;  no, must be 0x0C (IRQ4)

	 inc   IntVecIntBcount
         cmp   by IntVecIntBcount,1    ;have we installed handler on this IRQ ?
	 je    InitCom42
	 jmp   InitCom59

InitCom41:
	 inc   IntVecIntCcount
	 cmp   by IntVecIntCcount,1    ;have we installed handler on this IRQ ?
	 je    InitCom42
	 jmp   InitCom59

; *** Set interrupt vectors ***
;
InitCom42:
         cli                           ;Disable 8259 interrupts for
         pin   al,INTA1                ;  the given COM port
         pause
	 mov   cx,ax
         or    al,Mask8259[si]         ;turn off ints while installing handler
         pout  INTA1,al
         sti

	 ; *** save standard mode interrupt vectors, and mask ***

	 mov   al,ah		       ;interrupt number
	 mov   ah,35h		       ;Get the DOS vector
	 int   21h		       ;DOS Get Vector Function

	 mov   ax,cx
	 cmp   ah,IRQ3		       ;on interrupt 0x0B (IRQ3) ?
	 jne   InitCom45	       ;   no, must be 0x0C (IRQ4)

         mov   OldMask8259IRQ3,al      ;save old IRQ mask
         mov   wo OldIntVecIntB[0],bx  ;save old vector
         mov   wo OldIntVecIntB[2],es  ;save old vector
	 jmp   short InitCom50

InitCom45:
         mov   OldMask8259IRQ4,al      ;save old IRQ mask
         mov   wo OldIntVecIntC[0],bx  ;save old vector
         mov   wo OldIntVecIntC[2],es  ;save old vector

InitCom50:
         assumes ds,nothing
         push  ds                      ;Save original DS

         mov   ds,di                   ;Interrupt handler address in ds:dx
	 mov   al,ah		       ;Interrupt #
         mov   ah,25h                  ;DOS Set Vector Function
         int   21h                     ;Set the DOS vector

         pop   ds                      ;Original DS
	 assumes ds,Data

; *** if running under 286 dos extender, set real mode interrupt handler stuff
; *
; *   286 DOS EXTENDER REAL MODE INTERRUPT HANDLER SETUP
; *
	 push  dx		       ;interrupt handler code offset
	 push  ax		       ;AL - interrupt #

	 cmp   DosXFlag,0
	 jg    InitCom54	       ;yes - running 286 DOSX'dr, mod int vect's
	 jl    InitCom53	       ;? - don't know...

InitCom52:
	 pop   ax		       ;clean stack
	 pop   dx
	 jmp   short InitCom59	       ;skip -> not running 286 DOSX'dr

; We may or may not be running under the 286 DOS Extender (DOSX - Standard
; mode Windows).  If we are, setup SEGEMENT (not selector) variables that
; locate the code and data segments for real mode operation.

InitCom53:
	 inc   DosXFlag 		;we've checked for DOSX presence now

	 mov   bx,__WinFlags
	 and   bx,WF_PMODE or WF_WIN286 ;running Standard mode (DOSX) if
	 cmp   bx,WF_PMODE or WF_WIN286 ;  both pMode & Win286 bits set
	 jne   InitCom52

	 inc   DosXFlag 		;yes, we're under DOSX

; Under DOSX--do one time initialization of code/data SEGMENT variables

	 mov   ax,ds			;get SEGMENT of our data segment
	 call  SegmentFromSelector
	 push  ax			;save on stack

	 mov   ax,_INTERRUPT		;write data SEGMENT into _INTERRUPT
	 cCall AllocCStoDSAlias,<ax>	; code segment -- requires a data alias
	 mov   es,ax
	 pop   ax
	 mov   es:RM_IntDataSeg,ax
	 cCall FreeSelector,<es>	;don't need CS alias any longer

	 mov   ax,_INTERRUPT		;save _INT* SEGMENT into our data seg
	 call  SegmentFromSelector
	 mov   RM_IntCodeSegment,ax

; Under DOSX--also hook real mode interrupt vector

InitCom54:
	 pop   bx		       ;get interrupt # saved on stack
	 add   bl,DOSX_IRQ_ADJ	       ;DOS extender remapped h/w interrupt
	   ;  in --> bl = int #
	   ; out --> cx:dx = real mode interrupt vector
	 mov   ax,Get_RM_IntVector     ;get the real mode interrupt vector
	 int   31h		       ;DOSX get real mode vector

	 cmp   bl,IRQ3+DOSX_IRQ_ADJ    ;on interrupt 0x0B (IRQ3) ?
	 jne   InitCom55	       ;   no, must be 0x0C (IRQ4)

	 ; *** save current real mode interrupt vectors
	 mov   di,offset RealModeIntVectB
	 jmp   short InitCom57

InitCom55:
	 ; *** save current real mode interrupt vectors
	 mov   di,offset RealModeIntVectC

InitCom57:
	 mov   word ptr ds:[di], dx    ;save old vector
	 inc   di
	 inc   di
	 mov   word ptr ds:[di], cx    ;save old vector

	 pop   dx		       ;Offset of standard mode int handler.
	 sub   dx,CODE_DIFFERENCE      ;Point to real mode glue code before
				       ;main interrupt code.
	 mov   cx,RM_IntCodeSegment    ;Segment of interrupt code

	    ; in --> bl = int #
	    ; in --> cx:dx = real mode interrupt handler address
	 mov   ax,Set_RM_IntVector     ;DOSX Set Vector Function
	 int   31h		       ;Set the DOS vector real mode

	 call  Rotate_Pic	; rotate IRQ priorities to favor comm ports

; *** Interrupt handler set : jump here if handler is already installed ***
;
InitCom59:
	 pop   bx
	 pop   cx
         pop   di
	 pop   es

InitCom60:
         mov   dx,cx                   ;Set comm card address
         xor   ax,ax                   ;Need a zero
         inc   dx                      ;--> Interrupt Enable Register
         .errnz ACE_IER-ACE_RBR-1
         pout  dx,al                   ;Turn off interrupts
         add   dl,ACE_MCR-ACE_IER      ;--> Modem Control Register
         pause                         ;Delay
         pin   al,dx
         and   al,ACE_DTR+ACE_RTS      ;Leave DTR, RTS high if already so
         pause                         ;  but tri-state IRQ line
         pout  dx,al

InitCom70:
         push  es                      ;Zero queue counts and indexes

         push  ds
         pop   es
         assumes es,Data

         lea   di,QInCount[si]
         mov   cx,(EFlags-QInCount)/2
         .errnz (EFlags-QInCount) AND 1
	 xor   ax,ax
	 cld
         rep   stosw

         .errnz   QInGet-QInCount-2
         .errnz   QInPut-QInGet-2
         .errnz   QOutCount-QInPut-2
         .errnz   QOutGet-QOutCount-2
         .errnz   QOutPut-QOutGet-2
         .errnz   EFlags-QOutPut-2     ;First non-queue item

         pop   es
         assumes es,nothing

         mov   HSFlag[si],al           ;Show no handshakes yet
         mov   EvtWord[si],ax          ;Show no events

;Call $SETCOM to perform further hardware initialization.

InitCom80:
	 ; needs dx, si, di, and es to be saved from the beginning of inicom
         call  $SETCOM                 ;Set up Comm Device
	 jz    InitCom85	       ;Success

; DANGER! *** Call into middle of Terminate to clean things up *** DANGER!

         push  ax                      ;Failure, save error code
         call  Terminate45             ;Restore port address, int vec
	 pop   ax		       ;Restore error code and exit

         jmp   short InitCom100

InitCom85:
	 cmp   DosXFlag,1
	 jne   InitCom87	       ;running under 286 DOS extender ?

	 ; find segment values for the selector:offset given for
	 ; the input and output queues, so the queue's may be used
	 ; during a real mode interrupt
	 ; [gps]

	 mov   ax,wo QInAddr[2+si]     ;input queue selector
	 call  SegmentFromSelector
	 ;AX now points to input queue *segment*
	 mov   wo QInSegment[2+si],ax  ;segment address of input queue
	 mov   ax,wo QInAddr[si]       ; offset into segment
	 mov   wo QInSegment[si],ax

	 mov   ax,wo QOutAddr[2+si]    ;output queue selector
	 call  SegmentFromSelector
	 ;AX now points to output queue *segment*
	 mov   wo QOutSegment[2+si],ax ;segment address of output queue
	 mov   ax,wo QOutAddr[si]      ; offset into segment
	 mov   wo QOutSegment[si],ax

InitCom87:
         cli
         pin   al,INTA1                ;Enable interrupts at 8259
         mov   ah,Mask8259[si]         ;Zero turns it on.
         not   ah
         and   al,ah
         pout  INTA1,al
         sti

InitCom90:
         mov   EFlags[si],0            ;Clear internal state

InitCom95:
         xor   ax,ax                   ;Return AX = 0 to show success
         mov   ComErr[si],ax           ;Get rid of any bogus init error

InitCom100:
         pop   di
         pop   si
         ret

$INICOM endp
page

;--------------------------Private Routine-----------------------------;
;
; rotating the PIC interrupt priorities so the
; communication ports are highest priority
;
; Entry:
; Returns:
; Error Returns:
; Registers Destroyed:
; History:
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

public rotate_pic
Rotate_PIC proc near

	 push  ax
	 xor   ah,ah

	 cmp   IntVecIntBcount,ah
	 jz    rotate10

	 mov   al,0C2h		       ;IRQ3 highest priority
	 ;IRQ3 still installed
	 jmp   short rotate30

rotate10:
	 cmp   IntVecIntCcount,ah
	 jz    rotate20

	 mov   al,0C3h		       ;IRQ4 highest priority
	 ;IRQ4 still installed
	 jmp   short rotate30

rotate20:
	 mov   al,0C7h		       ;IRQ0 highest priority
	 ;no more IRQ's installed->reset priority scheme

rotate30:
	 pout  INTA0,al 	       ;tell PIC about the new state

	 pop   ax
	 ret

Rotate_PIC endp
page

;----------------------------Public Routine-----------------------------;
;
; $SNDIMM - Send A Character Immediately
;
; This routine either sends a character to the port immediately,
; or places the character in a special location which is used by
; the next transmit interrupt to transmit the character prior to
; those in the normal transmit queue.
;
; For LPT ports, the character is always sent immediately.
;
; Entry:
;   AH = Device ID
;   AL = Character
; Returns:
;   AX = 0
; Error Returns:
;   AX = 8000H if Bad ID
;   AX = 4000H if couldn't send because another character
;        transmitted "immediately" is waiting to be sent
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;


   assumes ds,Data
   assumes es,nothing

   public   $SNDIMM
$SNDIMM proc   near

        push    si
        call    GetDEB                  ;Get pointer to the DEB
	jc	SendImm30		;Bad ID, return an error
	jns	SendImm10		;Its a COM port


;       For LPT ports, call $SNDCOM to do the dirty work.  If $SNDCOM
;       returns an error code, map it to 4000h.

        call    $SNDCOM                 ;Do the dirty work here
        or      ax,ax                   ;Error occur?
	jnz	SendImm40		;  Yes, return 4000h
	jmp	short SendImm20 	;  No, show all is OK


SendImm10:
        test    EFlags[si],fTxImmed     ;Another char waiting "immediately"?
	jnz	SendImm40		;  Yes, return error
        mov     ah,al                   ;Set char for TXI
        cli                             ;TXI is critical section code
        call    TXI                     ;Set character to tx immediately
        sti

SendImm20:
        xor     ax,ax                   ;Show all is OK

SendImm30:
        pop     si
        ret

SendImm40:
        mov     ax,4000h                ;In case we cannot send
        pop     si
        ret

$SNDIMM endp
page

;----------------------------Public Routine-----------------------------;
;
; $SNDCOM - Send Byte To Port
;
; The given byte is sent to the passed port if possible.
; If the output queue is full, an error will be returned.
;
; Entry:
;   AH = Device ID
;   AL = Character
; Returns:
;   AX = 0
; Error Returns:
;   AX = error code
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public  $SNDCOM
$SNDCOM  proc  near

         push  si
         push  di
         call  GetDEB                  ;--> DEB
	 jc    SendCom40	       ;Invalid ID
	 jns   SendCom20	       ;Its a COM port

; Handle the transmission of a LPT character.  The ROM BIOS int 17
; call will be made to do the transmission.  The port address will
; be restored during the call, then zeroed out upon return.

SendCom10:
         xor   ch,ch                   ;Show xmit character
         call  DoLPT                   ;Do the work here
         jmp   short SendCom40         ;Return the status to caller

; Send a character to a COM port.  Return an error if control
; line timeout occurs or there is no room in the output queue.

SendCom20:
         push  ax                      ;Save character

         call  MSRWait                 ;See if lines are correct for output
         pop   ax                      ;Restore char
	 jnz   SendCom60	       ;Timeout occured, return error
         mov   cx,QOutSize[si]         ;See if queue is full
         cmp   cx,QOutCount[si]
	 jle   SendCom50	       ;There is no room in the queue
         les   di,QOutAddr[si]         ;--> output queue
         assumes es,nothing

         mov   bx,QOutPut[si]          ;Get index into queue
         mov   es:[bx][di],al          ;Store the byte
         inc   bx                      ;Update index
         cmp   bx,cx                   ;Wrap time?
	 jc    SendCom30	       ;  No
         xor   bx,bx                   ;Wrap-around is a new zero pointer

SendCom30:

;******************************************************************************
;ForcedInt is used as a flag in the case the forced software interrupt is
;generated.
;First_After_Boot is 0ffh when the driver is loaded and for the very first
;time the software int is not necessary

         cli
         mov   QOutPut[si],bx          ;Store updated pointer
         mov   ax,QOutCount[si]        ; get the count
         inc   ax                      ; have the updated value in AX for test later
         mov   QOutCount[si],ax        ;Update queue population
         push  ax
         call  KickTx                  ;Make sure xmit interrupt is armed
         sti
         pop   ax
         cmp   First_After_Boot,0ffh   ; is this the very first char sent ?   
	 jz    Forced_Int_Not_Req      ; first int comes after enable
         cmp   ax,1                    ; 1st char in new packet
	 jnz   Forced_Int_Not_Req
	 mov   ForcedInt[si],al        ; set forced interrupt flag, al == 1

	 cmp   word ptr [VCD_XMIT_CALLBACK+2], 0
	 jne   short call_vcd

	 pushf
	 cli			       ;go process the forced interrupt

	 call  CommIntFar	       ;this routine does an IRET to return
	 jmp   short Forced_Int_Not_req

call_vcd:
	 call  [VCD_XMIT_CALLBACK]

Forced_Int_Not_Req:
         mov   First_After_Boot,0      ; reset
         xor   ax,ax                   ;Show no error (that we know of)

;****************************************************************************

SendCom40:
         pop   di
         pop   si
         ret

SendCom50:
         or    by ComErr+1[si],HIGH CE_TXFULL
         .errnz LOW CE_TXFULL

SendCom60:
         mov   ax,ComErr[si]           ;Return error code to caller
	 jmp   short SendCom40

$SNDCOM endp

page

;----------------------------Private-Routine----------------------------;
;
; TimerProc - Decrement Timeout Counter
;
; The timer is decremented by one.  Since Windows is a
; non-preemptive system, there will not be two timers
; running at the same time.
;
; Entry:
;   None
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,nothing
assumes es,nothing


TimerProc   proc   far

         push  ax
         push  ds
         mov   ax,_DATA
         mov   ds,ax
         assumes ds,data
         dec   [TicCount]
         pop   ds
         assumes ds,nothing
         pop   ax
         ret

TimerProc   endp
page

;----------------------------Public Routine-----------------------------;
;
; $TRMCOM - Terminate Communications Channel
;
; Wait for any outbound data to be transmitted, drop the hardware
; handshaking lines, and disable interrupts.  If the output queue
; contained data when it was closed, an error will be returned
;
; LPT devices have it easy.  They just need to restore the I/O port
; address.
;
; Entry:
;   AH = Device ID
; Returns:
;   AX = 0
; Error Returns:
;   AX = 8000h if invalid device ID
;   AX = -2 if output queue timeout occured
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   $TRMCOM
$TRMCOM  proc   near

         push  si
         push  di
         xor   cx,cx                   ;Show no error if LPT port
         call  GetDEB
	 jc    TermCom60	       ;ID is invalid, return error
	 js    TermCom30	       ;Port is a LPT port
         push  ax                      ;Save port id
         or    HSFlag[si],Discard      ;Show discarding serial data
         mov   ComErr[si],cx           ;Clear error flags
         mov   QInCount[si],cx         ;Show no chars in input queue
         call  $RECCOM                 ;Send XON if needed

;-----------------------------------------------------------------------;
;   We have to wait for the output queue to empty.   To do this,
;   a timer will be created.  If no character has been transmitted
;   when the timeout occurs, then an error will be indicated and
;   the port closed anyway.  If the timer cannot be created, then
;   just loop until the queue empties, which will be better than
;   discarding charatcers if there are any
;-----------------------------------------------------------------------;

         mov   cx,QOutCount[si]        ;Any chars in output queue?
	 jcxz  TermCom20	       ;  No, skip timer stuff

         mov   ax,1000                 ;Create a 1 second timer
         mov   bx,CodeOFFSET TimerProc ;--> timer processor
         farPtr lpTime,cs,bx
         cCall CreateSystemTimer,<ax,lpTime>
         assumes es,nothing

TermCom10:
         mov   cx,QOutCount[si]        ;Get current queue count
	 jcxz  TermCom16	       ;No characters in queue
         mov   [TicCount],Timeout      ;Restart timeout counter

TermCom12:
         cmp   QOutCount[si],cx        ;Queue count change?
	 jne   TermCom10	       ;  Yes, restart timeout
         cmp   [TicCount],0            ;Timeout reached?
	 jge   TermCom12	       ;  No, keep waiting
         mov   cx,TimeoutError         ;  Yes, show timeout error

TermCom16:
         or    ax,ax                   ;Was the timer created?
	 jz    TermCom20	       ;  No, cannot kill it
         push  cx                      ;Save timeout error code
         cCall KillSystemTimer,<ax>
         assumes es,nothing
         pop   cx

TermCom20:
         pop   ax                      ;Restore cid

TermCom30:

         mov   dx,Port[si]             ;Get port base address
         call  Terminate               ;The real work is done here
	 mov   Port[si],0	       ;reset for closed ports
         mov   ax,cx                   ;Set return code

TermCom60:
         pop   di
         pop   si
         ret

$TRMCOM endp
page

;----------------------------Private-Routine----------------------------;
;
; Terminate - Terminate Device
;
; Restore the port I/O address and make sure that interrupts are off
;
; Entry:
;   AH = Device Id.
;   DX = Device I/O port address.
;   SI --> DEB
; Returns:
;   AX = 0
; Error Returns:
;   AX = -1
; Registers Destroyed:
;   AX,BX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public Terminate                       ;Public for debugging
Terminate proc near

         or    ah,ah                   ;LPT port?
	 jns   Terminate10	       ;  No, process COM port
	 .errnz   LPTx-10000000b

if 0

;-----------------------------------------------------------------------;
;   Check to see if running and need to release exclusive
;   control of the spooler for the given LPT port.
;-----------------------------------------------------------------------;

         call  TestInt2F               ;See if int 2F calls allowed
	 jc    Terminate5	       ;Cannot make int 2F calls
         mov   dl,ID[si]               ;Set port number for checking
	 and   dx,11b		       ;  for Int 2F
         mov   ax,Close                ;Close stream
         int   2Fh
         mov   ax,Unlock2F             ;Release exclusive control
	 int   2Fh

endif

Terminate5:
	 mov   di, BIOSPortLoc[si]
	 call  ReleasePort386	       ; give port back to 386...
         mov   dx,Port[si]             ;Restore port address
	 jmp   Terminate50	       ;That's all


;-----------------------------------------------------------------------;
;   It is a com port!
;
;       We delay for a bit while the last character finishes transmitting
;       Then we drop DTR and RTS, and disable the interrupt generation at
;       the 8250.  Even if fRTSDisable or fDTRDisable is set, those lines
;       will be dropped when the port is closed.
;-----------------------------------------------------------------------;

Terminate10:
         inc   dx                      ;Disable chip interrupts
         .errnz ACE_IER-ACE_RBR-1
         xor   ax,ax
         pout  dx,al
         add   dl,ACE_LSR-ACE_IER      ;--> line status register
         pause

Terminate20:
         pin   al,dx                   ;Wait until xmit is empty
         and   al,ACE_THRE+ACE_TSRE
         cmp   al,ACE_THRE+ACE_TSRE
	 jne   Terminate20	       ;Not empty yet

Terminate30:
         dec   dx                      ;--> Modem Status Register
         .errnz   ACE_LSR-ACE_MCR-1
         pin   al,dx
         and   al,ACE_DTR+ACE_RTS      ;Leave DTR, RTS high if already so
         pause                         ;  but tri-state IRQ line
         pout  dx,al
	 sub   dl,ACE_MCR-ACE_RBR      ;Set back to base address


;******* DANGER! ***** NOTICE! ***** DANGER! ***** WARNING! ***** NOTICE!
;
; Terminate45 is a secondary entrypoint into this routine--it's called
; by the initialization code when that code is unable to properly init
; a com port and needs to clean-up the mess it's made.
;
;******* DANGER! ***** NOTICE! ***** DANGER! ***** WARNING! ***** NOTICE!

Terminate45:
	 push  cx		       ;Save original cx
         push  bx                      ;Save original bx

	 mov   ah,IntVecNum[si]        ;Get the interrupt number

	 cmp   ah,IRQ3		       ; on IRQ3 ?
	 jne   Terminate46	       ;   no

	 dec   IntVecIntBcount	       ; need to disable IRQ 3 handler ?
	 jne   Terminate49	       ;   no...
	 mov   cl,OldMask8259IRQ3      ;   yes, get prior interrupt status
	 lea   bx,RealModeIntVectB     ;   ptr to Real mode int handler in bx
	 les   dx,OldIntVecIntB        ;   Int handler address in es:dx

	 jmp   short Terminate47       ; futuresoft fix by rkhx (craigc)

Terminate46:
	 dec   IntVecIntCcount	       ; need to disable IRQ 4 handler ?
	 jne   Terminate49	       ;   no...
	 mov   cl,OldMask8259IRQ4      ;   yes, get prior interrupt status
	 lea   bx,RealModeIntVectC     ;   ptr to Real mode int handler in bx
	 les   dx,OldIntVecIntC        ;   Int handler address in es:dx

Terminate47:

; Set the 8259 interrupt mask bit for this IRQ.  Leave interrupts enabled
; if they were already enabled when the comm port was initialized by us.

	cli				;cl has PIC mask when comm port init'd
	and	cl,Mask8259[si] 	;isolate bit for this IRQ
	jz	@f			;leave alone if was already enabled
	pin	al,INTA1		;was disabled, so disable again
	or	al,cl
	pause
	pout	INTA1,al		;new mask = current mask + IRQ bit
@@:
	sti

	 cmp   DosXFlag,1
	 jne   Terminate48	       ;running under 286 dos extender ?

	 ; *** reset real mode int vector to it's previous state
	 push  dx

	 mov   dx,[bx]		       ;offset
	 mov   cx,[2+bx]	       ;segment of int handler
	 mov   bl,ah
	 mov   bh,ah
	 add   bl,DOSX_IRQ_ADJ	       ;DOS extender remapped h/w interrupt
	 mov   ax,Set_RM_IntVector     ;DOS Set Vector Function
	 int   31h		       ;Set the DOS vector
	 mov   ah,bh		       ;Int vect # back into AH

	 pop   dx

	 call  Rotate_Pic	; rotate IRQ priorities to favor comm ports

Terminate48:
	 ; *** reset standard mode int vector to it's previous state
         assumes ds,nothing
	 push  ds		       ;Save original DS [rkh] ...
	 push  es
	 pop   ds
	 mov   al,ah		       ;Interrupt vector number
         mov   ah,25h                  ;DOS Set Vector Function
         int   21h                     ;Set the DOS vector
         pop   ds                      ;Original DS
	 assumes ds,data

; *** interrupt vectors have been reset if needed at this point ***
;
Terminate49:
         pop   bx                      ;Original BX
	 pop   cx		       ;Original CX

	 mov   di, BIOSPortLoc[si]
	 call  ReleaseCOMport386       ; give port back to 386...

         mov   dx,Port[si]             ;Get port address to restore

Terminate50:                           ;Also called from $INICOM !
         mov   bx,BIOSPortLoc[si]      ;Get offset where port addr goes

ifdef GO_AWAY
         cmp   bx,4
	 jb    Terminate60

         call  IBMmodel
	 jc    Terminate70	       ; PC
endif

Terminate60:
if 0				       ; THIS PAINS 386
         push  ds
         mov   ax,40h                  ;Indicate no error (also BIOS data seg)
         mov   ds,ax                   ;Point DS: at BIOS Save Area.
         assumes ds,nothing

         mov   wo [bx],dx              ;Restore I/O addr.
	 pop   ds
endif

Terminate70:
         assumes ds,Data

Terminate80:
         cmp   BIOSPortLoc[si], 0
	 jne   Terminate85
         mov   wo [deb_com1],0
	 jmp   short Terminate100

Terminate85:
         cmp   BIOSPortLoc[si], 2
	 jne   Terminate90
         mov   wo [deb_com2],0
	 jmp   short Terminate100

Terminate90:
         cmp   BIOSPortLoc[si], 4
	 jne   Terminate95
         mov   wo [deb_com3],0
	 jmp   short Terminate100

Terminate95:
         mov   wo [deb_com4],0

Terminate100:

         xor   ax,ax                   ;Indicate no error
         ret                           ;Port is closed and deallocated

Terminate   endp
page

;----------------------------Private-Routine----------------------------;
;
; MSRWait - Modem Status Register Wait
;
; This routine checks the modem status register for CTS, DSR,
; and/or RLSD signals.   If a timeout occurs while checking,
; the appropriate error code will be returned.
;
; This routine will not check for any signal with a corresponding
; time out value of 0 (ignore line).
;
; Entry:
;   SI --> DEB
; Returns:
;   AL = error code
;   ComErr[si] updated
;   'Z' set if no timeout
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   MSRWait	  ;Public for debugging

MSRWait proc   near

        push    di

MSRRestart:
        xor     di,di                   ;Init Timer

MSRWait10:
	mov	cx,11			;Init Delay counter (used on non-ATs)

MSRWait20:
        xor     dh,dh                   ;Init error accumulator
        mov     al,MSRShadow[si]        ;Get Modem Status
        and     al,MSRMask[si]          ;Only leave bits of interest
        xor     al,MSRMask[si]          ;0 = line high
	jz	MSRWait90		;All lines of interest are high
	mov	ah,al			;ah has 1 bits for down lines

        shl     ah,1                    ;Line Signal Detect low?
	jnc	MSRWait30		;  No, it's high
	.errnz	ACE_RLSD-10000000b
        cmp     di,RLSTimeout[si]       ;RLSD timeout yet?
	jb	MSRWait30		;  No
        or      dh,CE_RLSDTO            ;Show modem status timeout

MSRWait30:
	shl	ah,1			;Data Set Ready low?
	shl	ah,1
	.errnz	ACE_DSR-00100000b
	jnc	MSRWait40		;  No, it's high
        cmp     di,DSRTimeout[si]       ;DSR timeout yet?
	jb	MSRWait40		;  No
        or      dh,CE_DSRTO             ;Show data set ready timeout

MSRWait40:
	shl	ah,1			;CTS low?
	jnc	MSRWait50		;  No, it's high
	.errnz	ACE_CTS-00010000b
        cmp     di,CTSTimeout[si]       ;CTS timeout yet?
	jb	MSRWait50		;  No
        or      dh,CE_CTSTO             ;Show clear to send timeout

MSRWait50:
        or      dh,dh                   ;Any timeout occur?
	jnz	MSRWait80		;  Yes

        cmp     [$MachineID],0FCh       ;Is this a PC-AT? [rkh debug for PS/2]
	je	MSRWait60		;  Yes, use ROM function
	loop	MSRWait20		;  No, continue until timeout
        jmp     short MSRWait70         ;Should have taken about a millisecond

MSRWait60:
        push    bx                      ;Special SALMON ROM routine to delay
        push    di
        xor     cx,cx                   ;Number of Microseconds to delay
        mov     dx,1000                 ;  in CX:DX
        mov     ah,86h
        int     15h                     ;Wait 1 millisecond
        pop     di
        pop     bx

MSRWait70:
        inc     di                      ;Timer +1
	jmp	short MSRWait10 	;Until Timeout or Good status

MSRWait80:
        xor     ah,ah
        mov     al,dh
        or      by ComErr[si],al        ;Return updated status
        .errnz   HIGH CE_CTSTO
        .errnz   HIGH CE_DSRTO
        .errnz   HIGH CE_RLSDTO

MSRWait90:
        or      al,al                   ;Set 'Z' if no timeout
        pop     di
        ret

MSRWait endp
page

;----------------------------Public Routine-----------------------------;
;
; $SETCOM - Set Communications parameters
;
; Re-initalizes the requested port if present, and sets up the
; port with the given attributes when they are valid.
;
; For LPT ports, just copies whatever is given since it's ignored
; anyway.
;
; Entry:
;   ES:BX --> DCB with all fields set.
; Returns:
;   'Z' Set if no errors occured
;   AX = 0
; Error Returns:
;   'Z' clear if errors occured
;   AX = initialization error code.
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public $SETCOM
$SETCOM proc near

         cld
         push  si
         push  di
         mov   ah,es:ID[bx]            ;Get device i.d.
         call  GetDEB                  ;Get DEB pointer in SI
         mov   ax,IE_BadID             ;Assume unknown device
	 jc    SetCom10 	       ;Invalid device, return error
	 jns   SetCom20 	       ;COM port
	 call  SetCom100	       ;Copy the DCB

SetCom5:
         xor   ax,ax                   ;Show no error

SetCom10:
         or    ax,ax                   ;Set/clear 'Z'
         pop   di                      ;  and exit
         pop   si
         ret

;-----------------------------------------------------------------------;
;       Have a comm device, check all the serial parameters to make
;       sure they are correct before moving the new DCB into our space
;       and changing the ACE parameters.
;-----------------------------------------------------------------------;

SetCom20:
         call  SetCom300               ;Baud rate valid?
	 jcxz  SetCom10 	       ;  No, return error
         call  SetCom400               ;Byte size/parity/stop bits correct?
	 jc    SetCom10 	       ;  No, return error

; The parameters seem correct.  Copy the DCB into our space and
; initialize the ACE with the new parameters

         mov   dx,Port[si]             ;Disable interrupts from the 8250
         inc   dx
         .errnz ACE_IER-1
         xor   ax,ax
         pout  dx,al

         call  SetCom100               ;Copy the DCB
         mov   bx,si                   ;Set ES:BX --> DCB
         call  SetCom200               ;Get timeout masks
         xchg  al,ah                   ;Want them in the correct registers
         mov   wo MSRMask[si],ax
         .errnz MSRInfinite-MSRMask-1

         call  SetCom400               ;Get line control byte
         push  ax                      ;  and save LCR value
         inc   dx                      ;--> LCR
         inc   dx
         .errnz ACE_LCR-ACE_IER-2
         or    al,ACE_DLAB             ;Want access to divisor latch
         pout  dx,al
         mov   RxMask[si],ah           ;Save Receive character mask
         mov   ax,di                   ;Get flags mask, error mask
         and   Flags[si],ah            ;Disable parity checking if no parity
         mov   ErrorMask[si],al        ;Save line status error mask

         call  SetCom300               ;Get baud rate
         sub   dl,ACE_LCR-ACE_DLL      ;--> LSB of divisor latch
         mov   al,cl
         pout  dx,al
         mov   al,ch
         inc   dx                      ;--> MSB of divisor latch
         .errnz ACE_DLM-ACE_DLL-1
         pause
         pout  dx,al
         inc   dx                      ;--> LCR and clear divisor access bit
         inc   dx
         .errnz ACE_LCR-ACE_DLM-2
         pop   ax
         pout  dx,al

         inc   dx                      ;--> Modem Control Register
         .errnz ACE_MCR-ACE_LCR-1

;-----------------------------------------------------------------------;
;       Compute initial state of DTR and RTS.  If they have been disabled,
;       then do not raise them, and disallow being used as a handshaking
;       line.  Also compute the bits to use as hardware handshake bits
;       (DTR and/or RTS as indicated, qualified with the disabled flags).
;-----------------------------------------------------------------------;

         mov   al,Flags[si]            ;Align DTR/RTS disable flags for 8250
         and   al,fRTSDisable+fDTRDisable
         rol   al,1                    ;d0 = DTR, d2 = RTS  (1 = disabled)
         shr   al,1                    ;'C'= DTR, d1 = RTS
         adc   al,0                    ;d0 = DTR, d1 = RTS
         .errnz   fRTSDisable-00000010b
         .errnz   fDTRDisable-10000000b
         .errnz   ACE_DTR-00000001b
         .errnz   ACE_RTS-00000010b

         mov   ah,al                   ;Save disable mask
         xor   al,ACE_DTR+ACE_RTS+ACE_OUT2
         pout  dx,al                   ;Set Modem Control Register

         mov   al,Flags2[si]           ;Get hardware handshake flags
         rol   al,1                    ;Align flags as needed
         rol   al,1
         rol   al,1
         and   al,ACE_DTR+ACE_RTS      ;Mask bits of interest
         not   ah                      ;Want inverse of disable mask
         and   al,ah                   ;al = bits to handshake with
         mov   HHSLines[si],al         ;Save for interrupt code

         .errnz   fDTRFlow-00100000b
         .errnz   fRTSFlow-01000000b
         .errnz    ACE_DTR-00000001b
         .errnz    ACE_RTS-00000010b

         mov   al,Flags[si]            ;Compute the mask for the output
         shl   al,1                    ;  hardware handshake lines
         and   al,ACE_DSR+ACE_CTS
         mov   OutHHSLines[si],al

         .errnz   fOutXCTSFlow-00001000b
         .errnz   fOutXDSRFlow-00010000b
         .errnz        ACE_CTS-00010000b
         .errnz        ACE_DSR-00100000b

; Compute the queue count where XOff should be issued (or hardware
; lines dropped).  This will prevent having to do it at interrupt
; time.

         mov   ax,QInSize[si]          ;Get where they want it
         sub   ax,XOFFLim[si]          ;  and compute queue count
         mov    XOffPoint[si],ax

         sub   dl,ACE_MCR              ;Delay a bit waiting for things
         call  initialize_delay
         add   dl,ACE_MSR              ;--> Modem Status reg
         pause
         pin   al,dx                   ;Throw away 1st status read
         pause
         pin   al,dx                   ;Save 2nd for MSRWait (Clear MSR int)
         mov   MSRShadow[si],al

;-----------------------------------------------------------------------;
;       Now, at last, interrupts can be enabled.  Don't enable the
;       transmitter empty interrupt.  It will be enabled by the first
;       call to KickTx.
;-----------------------------------------------------------------------;

         sub   dl,ACE_MSR-ACE_IER      ;--> Interrupt Enable Register
         mov   al,ACE_ERBFI+ACE_ELSI+ACE_EDSSI
         cli
         pout  dx,al                   ;Enable interrupts.
         add   dl,ACE_LSR-ACE_IER      ;--> Line Status Register
         pause
         pin   al,dx                   ;Clear any Line Status interrupt
         sub   dl,ACE_LSR              ;--> Receiver Buffer Register
         pause
         pin   al,dx                   ;Clear any Received Data interrupt
         sti
	 jmp   SetCom5		       ;All done

$SETCOM endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom100
;
;  Copy the given DCB into the appropriate DEB.  The check has
;  already been made to determine that the ID was valid, so
;  that check can be skipped.
;
; Entry:
;   ES:BX --> DCB
;   DS:SI --> DEB
; Returns:
;   DS:SI --> DEB
;   ES     =  Data
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,ES,DI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom100 proc near
         push  si                      ;Save DEB pointer
         mov   di,si
         mov   si,bx
         push  es
         mov   ax,ds
         pop   ds
         assumes ds,nothing

         mov   es,ax
         assumes es,Data

	 mov   cx,DCBSize
	 cld
         rep   movsb
         mov   ds,ax
         assumes ds,Data

         pop   si                      ;Restore DEB pointer
         ret

SetCom100   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom200
;
; Based on whether or not a timeout has been specified for each
; signal, set up a mask byte which is used to mask off lines for
; which we wish to detect timeouts.  0 indicates that the line is
; to be ignored.
;
; Also set up a mask to indicate those lines which are set for
; infinite timeout.  1 indicates that the line has infinite
; timeout.
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   AH = lines to check
;   AL = lines with infinite timeout
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom200 proc near

         xor   ax,ax
         xor   cx,cx                   ;Get mask of lines with timeout = 0
         call  SetCom210
         not   al                      ;Invert result to get lines to check
         and   al,ACE_CTS+ACE_DSR+ACE_RLSD
         xchg  ah,al
         dec   cx                      ;Get mask of infinite timeouts

SetCom210:
         cmp   es:RLSTimeout[bx],cx    ;Timeout set to passed value?
	 jne   SetCom220	       ;  No
         or    al,ACE_RLSD             ;  Yes, show checking line

SetCom220:
         cmp   es:CTSTimeout[bx],cx    ;Timeout set to passed value?
	 jne   SetCom230	       ;  No
         or    al,ACE_CTS              ;  Yes, show checking line

SetCom230:
         cmp   es:DSRTimeout[bx],cx    ;Timeout set to passed value?
	 jne   SetCom240	       ;  No
         or    al,ACE_DSR              ;  Yes, show checking line

SetCom240:
         ret

SetCom200   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom300
;
; Calculate the correct baudrate divisor for the comm chip.
;
; Note that the baudrate is allowed to be any integer in the
; range 2-19200.  The divisor is computed as 115,200/baudrate.
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   CX = baudrate
; Error Returns:
;   CX = 0 if error
;   AX = error code if invalid baud rate
; Registers Destroyed:
;   AX,CX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom300 proc near

         push  dx
         mov   cx,es:BaudRate[bx]      ;Get requested baud rate
         xor   ax,ax                   ;Assume error
         cmp   cx,2                    ;Within valid range?
	 jnae  SetCom310	       ;  No, return error
         cmp   cx,19201
	 jae   SetCom310	       ;  No, return error

         mov   dx,1                    ;(dx:ax) = 115,200
         mov   ax,0C200h
         div   cx                      ;(ax) = 115,200/baud

SetCom310:
         mov   cx,ax                   ;(cx) = baud rate, or error code (0)
         mov   ax,IE_Baudrate          ;Set error code incase bad baud
         pop   dx
         ret

SetCom300   endp
page

;----------------------------Private-Routine----------------------------;
;
; SetCom400
;
; Check the line configuration (Parity, Stop bits, Byte size)
;
; Entry:
;   ES:BX --> DCB
; Returns:
;   ES:BX --> DCB
;   'C' clear if OK
;   AL = Line Control Register
;   AH = RxMask
;   DI[15:8] = Flags mask (to remove parity checking)
;   DI[7:0]  = Error mask (to remove parity error)
; Error Returns:
;   'C' set if error
;   AX = error code
; Registers Destroyed:
;   AX,CX,DI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SetCom400   proc   near

         mov   ax,wo es:ByteSize[bx]   ;al = byte size, ah = parity
         cmp   ah,SpaceParity          ;Parity out of range?
	 ja    SetCom470	       ;  Yes, return error
         mov   di,0FF00h+ACE_OR+ACE_PE+ACE_FE+ACE_BI
         or    ah,ah                   ;Is parity "NONE"?
	 jnz   SetCom410	       ;  No, something is there for parity
         xor   di,(fParity*256)+ACE_PE ;Disable parity checking

SetCom410:
         cmp   al,8                    ;Byte size out of range?
	 ja    SetCom460	       ;  Yes, error

SetCom420:
         sub   al,5                    ;Shift byte size to bits 0&1
         .errnz ACE_WLS-00000011b      ;Word length must be these bits
	 jc    SetCom460	       ;Byte size is illegal, return error
         add   ah,ah                   ;Map parity to ACE bits
	 jz    SetCom430	       ;0=>0, 1=>1, 2=>3, 3=>5, 4=>7
         dec   ah

SetCom430:
         shl   ah,1                    ;Align with 8250 parity bits
         shl   ah,1
         shl   ah,1
         or    al,ah                   ;Add to byte size

         .errnz NoParity-0
         .errnz OddParity-1
         .errnz EvenParity-2
         .errnz MarkParity-3
         .errnz SpaceParity-4
         .errnz ACE_PEN-00001000b
         .errnz ACE_PSB-00110000b
         .errnz ACE_EPS-00010000b
         .errnz  ACE_SP-00100000b

         or    al,ACE_2SB              ;Assume 2 stop bits
         mov   ah,es:StopBits[bx]      ;Get # of stop bits 0=1,1/2= .GT. 1
         or    ah,ah                   ;Out of range?
	 js    SetCom470	       ;  Yes, return error
	 jz    SetCom440	       ;One stop bit
         sub   ah,2
	 jz    SetCom450	       ;Two stop bits
	 jns   SetCom470	       ;Not 1.5, return error
         test  al,ACE_WLS              ;1.5 stop bits, 5 bit words?
	 jnz   SetCom470	       ;  No, illegal
         .errnz OneStopBit-0
         .errnz One5StopBits-1
         .errnz TwoStopBits-2
         .errnz ACE_5BW

SetCom440:
         and   al,NOT ACE_2SB          ;Show 1 (or 1.5) stop bit(s)


; From the byte size, get a mask to be used for stripping
; off unused bits as the characters are received.

SetCom450:
         push  dx
         mov   cl,es:ByteSize[bx]      ;Get data byte size
         mov   dx,00FFh                ;Turn into mask by shifting bits
         shl   dx,cl
         mov   ah,dh                   ;Return mask in ah
         pop   dx
         clc                           ;Show all is fine
         ret

SetCom460:
         mov   ax,IE_ByteSize          ;Show byte size is wrong
         stc                           ;Show error
         ret

SetCom470:
         mov   ax,IE_Default           ;Show something is wrong
         stc                           ;Show error
         ret

SetCom400 endp
page

;----------------------------------------------------------------------------;
; SuspendOpenCommPorts:							     ;
;									     ;
; This routine is called from 286 Winoldaps to simply deinstall the comm port;
; hooks. The comm int vectors are released.				     ;
;----------------------------------------------------------------------------;

cProc	SuspendOpenCommPorts,<FAR,PUBLIC,PASCAL>

cBegin

	assumes	cs,Code
	assumes	ds,Data

	mov	bx,__WinFlags		;test mode we are running in
	and   	bx,WF_PMODE or WF_WIN286;want to see if in dosx mode or not
	cmp   	bx,WF_PMODE or WF_WIN286;both bits must be set
	jne	SuspendOpenCommPortsPM	;not in dosx.
	mov	dx,word ptr [RealmodeIntVectB]
	mov	ax,word ptr [RealmodeIntVectB+2]
	mov	bx,0bh + DOSX_IRQ_ADJ	;mapped vector for vector B
	call	ProgramRMCommVector	;program the vector
	mov	word ptr [RMOurIntVectB],dx
	mov	word ptr [RMOurIntVectB+2],ax
	mov	dx,word ptr [RealmodeIntVectC]
	mov	ax,word ptr [RealModeIntVectC+2]
	mov	bx,0ch + DOSX_IRQ_ADJ	;mapped vector for vector C
	call	ProgramRMCommVector	;program the vector
	mov	word ptr [RMOurIntVectC],dx
	mov	word ptr [RMOurIntVectC+2],ax

SuspendOpenCommPortsPM:

	mov	dx,word ptr [OldIntVecIntB]
	mov	ax,word ptr [OldIntVecIntB+2]
	mov	cx,0bh			;vector B
	call	ProgramCommVector	;program the vector
	mov	word ptr [OurIntVecIntB],dx
	mov	word ptr [OurIntVecIntB+2],ax
	mov	dx,word ptr [OldIntVecIntC]
	mov	ax,word ptr [OldIntVecIntC+2]
	mov	cx,0ch			;vector C
	call	ProgramCommVector	;program the vector
	mov	word ptr [OurIntVecIntC],dx
	mov	word ptr [OurIntVecIntC+2],ax

cEnd
;----------------------------------------------------------------------------;
; ReactivateOpenCommPorts:						     ;
;									     ;
; This routine reinstalls the comm hooks in real mode and reads the 8250     ;
; data and status registers to clear pending interrupts.		     ;
;----------------------------------------------------------------------------;

cProc	ReactivateOpenCommPorts,<FAR,PASCAL,PUBLIC>

cBegin

	mov	dx,word ptr [OurIntVecIntB]
	mov	ax,word ptr [OurIntVecIntB+2]
	mov	cx,0bh			;vector B
	call	ProgramCommVector	;program the vector
	mov	dx,word ptr [OurIntVecIntC]
	mov	ax,word ptr [OurIntVecIntC+2]
	mov	cx,0ch			;vector C
	call	ProgramCommVector	;program the vector
	mov	bx,__WinFlags		;test mode we are running in
	and   	bx,WF_PMODE or WF_WIN286;want to see if in dosx mode or not
	cmp   	bx,WF_PMODE or WF_WIN286;both bits must be set
	jne	ReactivateOpenCommPortsRet;not in dosx.
	mov	dx,word ptr [RMOurIntVectB]
	mov	ax,word ptr [RMOurIntVectB+2]
	mov	bx,0bh + DOSX_IRQ_ADJ	;mapped vector for vector B
	call	ProgramRMCommVector	;program the vector
	mov	dx,word ptr [RMOurIntVectC]
	mov	ax,word ptr [RMOurIntVectC+2]
	mov	bx,0ch + DOSX_IRQ_ADJ	;mapped vector for vector C
	call	ProgramRMCommVector	;program the vector

ReactivateOpenCommPortsRet:

	call	rotate_pic		;make comm ports highest priority
	call	ReadCommPortRegs	;read relevant comm port registers

cEnd
;----------------------------------------------------------------------------;
; ProgramCommVector:							     ;
;									     ;
; entry: cl	-	vector number					     ;
;	 ax:dx	-	new address for ISR				     ;
; 									     ;
; rets:  ax:dx	-	original address of ISR				     ;
;----------------------------------------------------------------------------;

ProgramCommVector  proc near

	push	di
	mov	di,ax
	or	di,dx			;is the new vector NULL ?
	pop	di			;restore
	jz	NoVector 		;nothing to do
	push	es			;save
	push	ax			;save
	mov	al,cl			;get the vector number
	mov	ah,35h			;want to getthe vector.
	int	21h			;es:bx has the bector
	pop	ax			;get back seg of new vector
	push	ds			;save
	mov	ds,ax			;ds:dx has new vector
	mov	al,cl			;get the vector
	mov	ah,25h			;want to set the vector
	int	21h			;vector set
	pop	ds			;restore ds
	mov	ax,es			
	mov	dx,bx			;ax:dx has original vector
	pop	es			;restore
	jmp	short @f

NoVector:

	xor	dx,dx
	xor	ax,ax			;return NULL

@@:

	ret

ProgramCommVector  endp
;----------------------------------------------------------------------------;
; ProgramRMCommVector:							     ;
;									     ;
; entry: bl	-	vector number					     ;
;	 ax:dx	-	new address for ISR				     ;
; 									     ;
; rets:  ax:dx	-	original address of ISR				     ;
;									     ;
; (This programs real mode IDT when in PMODE)				     ;
;----------------------------------------------------------------------------;

ProgramRMCommVector  proc near

	push	di
	mov	di,ax
	or	di,dx			;is the new vector NULL ?
	pop	di			;restore
	jz	RMNoVector 		;nothing to do
	push	si			;save
	push	di			;save
	push	ax			;save
	push	dx			;save
	mov	ax,Get_RM_IntVector	;returns in cx:dx
	int	31h			;obtained vector in cx:dx
	mov	si,cx
	mov	di,dx			;save vector in si:di
	pop	dx
	pop	cx			;get new vector back in cx:dx
	mov	ax,Set_RM_IntVector	;want to set the vector
	int	31h			;vector set
	mov	ax,si			
	mov	dx,di			;ax:dx has original vector
	pop	di
	pop	si			;restore
	jmp	short @f

RMNoVector:

	xor	dx,dx
	xor	ax,ax			;return NULL

@@:

	ret

ProgramRMCommVector  endp
;----------------------------------------------------------------------------;
; ReadCommPortRegs:							     ;
;									     ;
; Reads the comm port registers to take care of pending interrupts.	     ;
;----------------------------------------------------------------------------;

ReadCommPortRegs  proc near

	assumes	ds,Data

	push	si
	mov	bl,11101111b		;IRQ 4 to be enabled
	mov	si,DataOFFSET Comm1	;get structure for comm1
	mov	dx,Port[si]		;get the base port
	call	InitAPort		;do for one
	mov	bl,11110111b		;IRQ 3 to be enabled
	mov	si,DataOFFSET Comm2	;get structure for comm1
	mov	dx,Port[si]		;get the base port
	call	InitAPort		;do for one
	mov	bl,11101111b		;IRQ 4 to be enabled
	mov	si,DataOFFSET Comm3	;get structure for comm1
	mov	dx,Port[si]		;get the base port
	call	InitAPort		;do for one
	mov	bl,11110111b		;IRQ 3 to be enabled
	mov	si,DataOFFSET Comm4	;get structure for comm1
	mov	dx,Port[si]		;get the base port
	call	InitAPort		;do for one
	pop	si
	ret

ReadCommPortRegs  endp
;----------------------------------------------------------------------------;
; InitAPort:								     ;
;									     ;
; reads the data,status & IIR registers of a port (has to be 8250!)	     ;
;									     ;
; If the port has an out queue pending, then this woutine will also start    ;
; the transmit process by faking a comm interrupt.			     ;
;----------------------------------------------------------------------------;

public	   InitAPort
InitAPort  proc near

	or	dx,dx			;present ?
	jz	@f			;no.
	in	al,INTA1		;read the PIC IRQ enable register
	and	al,bl			;enable the IRQ this port is on
	jmp	short $+2		;i/o delay
	out	INTA1,al		;IRQ enabled
	add	dl,ACE_RBR		;dx=receive buffer register
	in	al,dx			;read the data port
	jmp	short $+2		;i/o delay
	add	dl,ACE_LSR - ACE_RBR	;get to the status port
	in	al,dx			;read it too.
	jmp	short $+2		;i/o delay
   	add	dl,ACE_IIDR - ACE_LSR	;get to the line status register
	in	al,dx			;read it once more
	jmp	short $+2		;i/o delay
   	add	dl,ACE_MSR - ACE_IIDR	;get to the modem status register
	in	al,dx			;read it once more
	jmp	short $+2		;i/o delay
   	add	dl,ACE_RBR - ACE_MSR	;get to the receive buffer register
	in	al,dx			;read it once more
	jmp	short $+2		;i/o delay

; now if the port has characters pending to be sent out then we must fake a
; comm interrupt.

	cmp	[si].QOutCount,0	;characters pending to be sent ?
	jz	@f			;no.
	cli				;disable interrupts
	call	FakeCOMIntFar		;fake an interrupt
	sti				;renable interrupts
	
@@:
	ret

InitAPort endp

if 0
;----------------------------Private-Routine----------------------------;
;
; TestInt2F - Test For Interrupt 2F
;
; A test is made to see if interrupt 2F can be made.
; This interrupt is used to gain exclusive access to
; a spooled LPT port.  Calls via Int17 are allowed to
; succeed, but calls to the DOS will fail, including
; calls like "Close All Spool Files", which command.com
; will issue every time it displays it's prompt.
;
; Entry:
;   None
; Returns:
;   'C' set if OK to use int 2F
;   'C' clear if we cannot use int 2F
; Error Returns:
;   None
; Registers Destroyed:
;   AX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

TestInt2F proc near

; *** MS windows 3.0 requires DOS 3.x or higher
;
;	  mov	ax,3000h		;Get DOS version number
;	  int	21h
;	  cmp	al,3			;DOS 3.x?
;	  jb	NoInt2Fs		;  No, disallow int 2Fh
;
; *** [gps] 6-01-89

         mov   ax,0B800h               ;Test network configuration
         int   2Fh
         or    al,al                   ;Network not running at all
	 jz    NoInt2Fs 	       ;No, disallow int 2Fh
         test  bx,0000000011000100b    ;SRV, RCV, or MSG?
	 jz    TextInt2FExit	       ;  No, allow int 2Fh ('C' clear)

NoInt2Fs:
         stc                           ;'C' set, int 2F not allowed

TextInt2FExit:
         ret

TestInt2F   endp

endif

page

;----------------------------Private-Routine----------------------------;
;
; initialize_delay - Delay After Initializing the ACE
;
; A delay loop based on the BIOS timer is entered while waiting for
; the 8250 to settle.  This use to be done as a loop, but there are
; two problems with that:
;
;   1)   processor speed
;
;   2)   Win386 where the ACE is a virtual device and we'll trap on
;   every I/O operation
;
; To get around this, we will use the BIOS area clock to determine
; when to terminate the loop.
;
; Entry:
;   DX = ACE_RBR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AL,CX,FLAGS
; History:
;   Thu 20-Aug-1987 12:24:47 -by-  Walt Moore [waltm]
;   created
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

BIOSTIME   equ   06Ch
TIC_MSEC   equ   55                    ;A tick is 55 milliseconds
DELAY_TIME equ   500                   ;Delay at least 500 milliseconds

assumes ds,nothing
assumes es,nothing

initialize_delay proc near

         push  ds
         mov   cx,40h
         mov   ds,cx         ; point to BIOS area
         assumes ds,nothing

         mov   ch,((DELAY_TIME+TIC_MSEC-1)/TIC_MSEC)+1

init_delay_outer_loop:
         mov   cl,ds:[BIOSTIME]   ;Current timer value

init_delay_loop:
         pin   al,dx                   ;Read it once
         cmp   cl,ds:[BIOSTIME]        ;Wait 0-55 milliseconds
	 je    init_delay_loop	       ;Timer hasn't advanced
         dec   ch                      ;Waited enough ticks?
	 jnz   init_delay_outer_loop   ;  No, wait for more ticks

         pop   ds
         assumes ds,nothing

         ret

initialize_delay endp

include ibmcom1.asm

ifdef DEBUG
   public   InitCom10
   public   InitCom20
   public   InitCom30
   public   InitCom37
   public   InitCom40
   public   InitCom41
   public   InitCom42
   public   InitCom45
   public   InitCom50
   public   InitCom52
   public   InitCom53
   public   InitCom54
   public   InitCom55
   public   InitCom57
   public   InitCom59
   public   InitCom60
   public   InitCom70
   public   InitCom80
   public   InitCom85
   public   InitCom87
   public   InitCom90
   public   InitCom95
   public   InitCom100
   public   SendImm10
   public   SendImm20
   public   SendImm30
   public   SendImm40
   public   SendCom10
   public   SendCom20
   public   SendCom30
   public   SendCom40
   public   SendCom50
   public   SendCom60
   public   TermCom10
   public   TermCom12
   public   TermCom16
   public   TermCom20
   public   TermCom30
   public   TermCom60
   public   Terminate5
   public   Terminate10
   public   Terminate20
   public   Terminate30
   public   Terminate45
   public   Terminate47
   public   Terminate48
   public   Terminate49
   public   Terminate50
   public   MSRRestart
   public   MSRWait10
   public   MSRWait20
   public   MSRWait30
   public   MSRWait40
   public   MSRWait50
   public   MSRWait60
   public   MSRWait70
   public   MSRWait80
   public   MSRWait90
   public   SetCom5
   public   SetCom10
   public   SetCom20
   public   SetCom210
   public   SetCom220
   public   SetCom230
   public   SetCom240
   public   SetCom310
   public   SetCom410
   public   SetCom420
   public   SetCom430
   public   SetCom440
   public   SetCom450
   public   SetCom460
   public   SetCom470
   public   initialize_delay
   public   init_delay_outer_loop
   public   init_delay_loop
endif

sEnd    code
End
