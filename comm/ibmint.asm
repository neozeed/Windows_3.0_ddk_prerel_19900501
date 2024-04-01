page,132
;---------------------------Module-Header-------------------------------
; Module Name: IBMINT.ASM
;
; Created: Fri 06-Feb-1987 10:45:12
; Author:  Walt Moore [waltm]
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved
;
; General Description:
;   This file contains the interrupt time routines for the
;   IBM Windows communications driver.
;
;   The interrupt code is preloaded and fixed.
;
; 21-Jan-90  jimmat  NOTICE!! ******************************************
;
; When running under Windows 386 Enhanced mode, COMM interrupts are not
; processed by this driver!  The 386 Virtual Comm Device (VCD) duplicates
; most of this module's interrupt processing for performance reasons.  If
; changes are made to the logic of this module, it may be necessary to
; make corresponding changes in the VCD device.
;
; **********************************************************************
;
; History:
;
; **********************************************************************
;    Tue Dec 19 1989 09:35:15	-by-  Amit Chatterjee  [amitc]
; ----------------------------------------------------------------------
;    Added a far entry point 'FakeCOMIntFar' so that the routine 'FakeCOMInt'
; could be called from the 'InitAPort' routine in IBMCOM.ASM
;-----------------------------------------------------------------------;

subttl  Communications Hardware Interrupt Service Routines

.xlist
include cmacros.inc
include comdev.inc
include ibmcom.inc
include ins8250.inc
.list

externFP IBMmodel

externB  myMachineID
externB  Comm1
externB  Comm2
externB  Comm3
externB  Comm4
externB  DosXFlag

sBegin Data

public deb_com1 
public deb_com2 
public deb_com3 
public deb_com4
public OldIntVecIntB
public OldIntVecIntC
public OurIntVecIntB
public OurIntVecIntC
public OldMask8259IRQ3 
public OldMask8259IRQ4

public RealModeIntVectB
public RealModeIntVectC
public RMOurIntVectB
public RMOurIntVectC

deb_com1 label word
         dw    0

deb_com2 label word
         dw    0

deb_com3 label word                     ; [rkh] ...
         dw    0

deb_com4 label word
         dw    0

OldIntVecIntB     dd 0                 ;Old interrupt vector (IRQ3)
OldIntVecIntC     dd 0                 ;Old interrupt vector (IRQ4)
OurIntVecIntB     dd 0                 ;Our interrupt vector (IRQ3)
OurIntVecIntC     dd 0                 ;Our interrupt vector (IRQ4)
OldMask8259IRQ3   db 0
OldMask8259IRQ4   db 0

RealModeIntVectB  dd 0		       ;Old real mode interrupt vector (IRQ3)
RealModeIntVectC  dd 0		       ;Old real mode interrupt vector (IRQ4)
RMOurIntVectB	  dd 0		       ;our real mode interrupt vector (IRQ3)
RMOurIntVectC	  dd 0		       ;our real mode interrupt vector (IRQ4)

sEnd data

createSeg _INTERRUPT,IntCode,word,public,CODE
sBegin IntCode
assumes cs,IntCode

page

public RM_IntDataSeg
RM_IntDataSeg	  dw 0
  ; this variable is written into by a routine in inicom
  ; if the 286 DOS extender is present.  This variable
  ; contains the SEGMENT value of the data selector "_DATA"
  ; so that the real mode interrupt handler may use the
  ; data segment, and not it's selector !

;--------------------------Interrupt Hanlder----------------------------;
; COM_IRQ3 - Interrupt handler for com port on IRQ3
;
; Setup is performed for the main interrupt handler.  This involves
; setting the index of which
;
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing
                                        ; could be com 2, 3, or 4
public	 RM_COM_IRQ3
RM_COM_IRQ3 proc far			;Interrupt server enters here

         push  ax
	 mov   ax,cs:RM_IntDataSeg
	 jmp   short COM3_start

RM_COM_IRQ3 endp

public CODE_DIFFERENCE
CODE_DIFFERENCE equ $-RM_COM_IRQ3

public	 COM_IRQ3
COM_IRQ3 proc far                       ;Interrupt server enters here

         push  ax
         mov   ax,_DATA

COM3_start:
         push  ds
         push  si

         mov   ds,ax                   ;Get data seg

         cmp   [deb_com2],0
	 je    COM_IRQ3_COM3
         mov   si,DataOFFSET Comm2
         call  CommInt

COM_IRQ3_COM3:			       ;Only for PS/2
         cmp   [myMachineID],0         ;PC=0, PS/2=1
	 je    COM_IRQ3_COM4
         cmp   [deb_com3],0
         je    COM_IRQ3_COM4
         mov   si,DataOFFSET Comm3
         call  CommInt

COM_IRQ3_COM4:
         cmp   [deb_com4],0
	 je    COM_IRQ3_DONE
         mov   si,DataOFFSET Comm4
         call  CommInt

COM_IRQ3_DONE:

         mov   al,OldMask8259IRQ3      ; get save IRQ mask
         test  al,Mask8259[si]         ; was this IRQ off ? (i.e. no mouse)
	 jnz   COM_IRQ3_normal_int     ;  yes, so IRET

	 cmp   DosXFlag,1	       ; running under 286 DOS extender ?
	 jne   COM_IRQ3_calloldintvect
.286P
	 smsw  ax		       ; running in protect mode ?
.8086
	 test  al,1
	 jnz   COM_IRQ3_calloldintvect ; if not in real mode...call PM vector

	 pushf			       ; simulate real mode interrupt
	 cli
	 call  RealModeIntVectB        ;  no, call old real mode vector
	 jmp   short COM_IRQ3_normal_int

COM_IRQ3_calloldintvect:
	 pushf			       ; simulate standard mode interrupt
	 cli
	 call  OldIntVecIntB	       ; call old vector

COM_IRQ3_normal_int:
         mov   al,EOI                  ;Send End-of-Interrupt
         pout  INTA0,al

         pop   si
         pop   ds
         assumes ds,nothing
         pop   ax
         sti
         iret

COM_IRQ3 endp
page

;--------------------------Interrupt Hanlder----------------------------;
; COM_IRQ4 - Interrupt handler for com port on IRQ4
;
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

                                        ; could be com 1 or 3

public	 RM_COM_IRQ4
RM_COM_IRQ4 proc far			;Real mode Interrupt server enters here

         push  ax
	 mov   ax,cs:RM_IntDataSeg
	 jmp   short COM4_start

RM_COM_IRQ4 endp

public	 COM_IRQ4
COM_IRQ4 proc far			;Standard mode Interrupt server start

         push  ax
         mov   ax,_DATA

COM4_start:
         push  ds
         push  si

         mov   ds,ax                   ;Get data seg

         cmp   [deb_com1],0
	 je    COM_IRQ4_COM3
         mov   si,DataOFFSET Comm1
         call  CommInt

COM_IRQ4_COM3:                         ;Only for PC
         cmp   [myMachineID],0         ;PC=0, PS/2=1
	 jne   COM_IRQ4_DONE
         cmp   [deb_com3],0
	 je    COM_IRQ4_DONE
         mov   si,DataOFFSET Comm3
         call  CommInt

COM_IRQ4_DONE:

         mov   al,OldMask8259IRQ4      ; get save IRQ mask
         test  al,Mask8259[si]         ; was this IRQ off ? (i.e. no mouse)
	 jnz   COM_IRQ4_normal_int     ;  yes, so IRET

	 cmp   DosXFlag,1	       ; running under 286 DOS extender ?
	 jne   COM_IRQ4_calloldintvect
.286P
	 smsw  ax		       ; running in protect mode ?
.8086
	 test  al,1
	 jnz   COM_IRQ4_calloldintvect ; if not in real mode...call PM vector

	 pushf			       ; simulate real mode interrupt
	 cli
	 call  RealModeIntVectC        ; call old real mode vector
	 jmp   short COM_IRQ4_normal_int

COM_IRQ4_calloldintvect:
	 pushf			       ; simulate standard mode interrupt
	 cli
	 call  OldIntVecIntC	       ; call old vector

COM_IRQ4_normal_int:
         mov   al,EOI                  ;Send End-of-Interrupt
         pout  INTA0,al

         pop   si
         pop   ds
         assumes ds,nothing
         pop   ax
         sti
         iret

COM_IRQ4 endp
page

;--------------------------Fake a Hardware Interrupt----------------------;
; FakeCOMInt
;
; This routine fakes a hardware interrupt to IRQ3 or IRQ4
; to clear out characters pending in the buffer
;
; Entry:
;   DS:SI --> DEB
;   INTERRUPTS DISABLED!
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;
; Registers Destroyed:
;   AX,DX,FLAGS
; History: glenn steffler 5/17/89
;-----------------------------------------------------------------------;

public FakeCOMInt
FakeCOMInt proc near

; "Kick" the transmitter interrupt routine into operation.
; If the Transmitter Holding Register isn't empty, then
; nothing needs to be done.  If it is empty, then the xmit
; interrupt needs to enabled in the IER.

       ; cli                           ;Done by caller
         mov   dx,Port[si]             ;Get device I/O address
         add   dl,ACE_LSR              ;Point at the line status reg
         pin   al,dx                   ;And get it
         and   al,ACE_THRE             ;Check transmitter holding reg status
	 jz    int_dont_kick_tx        ;Busy, interrupt will hit soon enough

         sub   dl,ACE_LSR-ACE_IER      ;--> Interrupt enable register
         pin   al,dx                   ;Get current IER state
         test  al,ACE_THREI            ;Interrupt already enabled?
	 jnz   int_dont_kick_tx        ;  Yes, don't reenable it
         or    al,ACE_THREI            ;  No, enable it
         pout  dx,al
         pause                         ;8250, 8250-B bug requires
         pout  dx,al                   ;  writting register twice

int_dont_kick_tx:
       ; sti                           ;Done by caller

	 mov   ForcedInt[si],1	       ; set the Forced Interrupt flag on

	 cli
	 call  CommInt		       ;Process the fake interrupt, DS:SI is
				       ;  already pointing to proper DEB
	 ret

FakeCOMInt endp

public	FakeCOMIntFar
FakeCOMIntFar proc far

	call	FakeComInt
	ret

FakeCOMIntFar endp

;--------------------------Interrupt Handler----------------------------
;
; CommInt - Interrupt handler for com ports
;
; Interrupt handlers for PC com ports.	This is the communications
; interrupt service routine for RS232 communications.  When an RS232
; event occurs the interrupt vectors here.  This routine determines
; who the caller was and services the appropriate interrupt.  The
; interrupts are prioritized in the following order:
;
;     1.  line status interrupt
;     2.  read data available interrupt
;     3.  transmit buffer empty interrupt
;     4.  modem service interrupt
;
; This routine continues to service until all interrupts have been
; satisfied.
;
; 21-Jan-90  jimmat  NOTICE!! ******************************************
;
; When running under Windows 386 Enhanced mode, COMM interrupts are not
; processed by this driver!  The 386 Virtual Comm Device (VCD) duplicates
; most of this module's interrupt processing for performance reasons.  If
; changes are made to the logic of this module, it may be necessary to
; make corresponding changes in the VCD device.
;
; **********************************************************************
;
;-----------------------------------------------------------------------


assumes ds,nothing
assumes es,nothing

	public	CommInt

CommInt proc near

;   assumes ds,Data

         push  bx
         push  cx
         push  dx
         push  di
         push  es

         mov   dx,Port[si]             ;Get comm I/O port
         add   dl,ACE_IIDR             ;--> Interrupt ID Register

;Test the ForcedInt flag to decide if this interrupt has actually been forced
;by the software or not, if so directly jump to the XmitEmpty ISR

         cmp   ForcedInt[si],0         ; forced interrupt flag set ?
	 jz    IntLoop10	       ; no, must be from hardware...

	 mov   ForcedInt[si],0	       ; yes, clear the flag and treat
	 push  dx		       ;   like an XmitEmpty interrupt
	 jmp   XmitEmpty


;   Dispatch table for interrupt types

SrvTab label word
         dw    ModemStatus             ;Modem Status Interrupt
         dw    XmitEmpty               ;Tx Holding Reg. Interrupt
         dw    DataAvail               ;Rx Data Available Interrupt
         dw    LineStat                ;Reciever Line Status Interrupt

InterruptLoop:
         pop   dx                      ;Get ID reg I/O address

IntLoop10:
         pin   al,dx                   ;Get Interrupt Id
         test  al,1                    ;Interrupt need servicing?
	 jnz   IntLoop20	       ;No, all done

         xor   ah,ah
         mov   di,ax
         push  dx                      ;Save Id register
         jmp   SrvTab[di]              ;Service the Interrupt

IntLoop20:
         mov   ax,EvtMask[si]          ;Mask the event word to only the
         and   EvtWord[si],ax          ;  user specified bits
         pop   es
assumes es,nothing

         pop   di
         pop   dx
         pop   cx
         pop   bx
         ret

CommInt endp

	public	CommIntFar

CommIntFar	proc	far		;Far IRET interface to Commint

	call	CommInt
	iret				;Note: an IRET, not RETF!

CommIntFar	endp

page

;----------------------------Private-Routine----------------------------;
;
; LineStat - Line Status Interrupt Handler
;
; Break detection is handled and set in the event word if
; enabled.  Other errors (overrun, parity, framing) are
; saved for the data available interrupt.
;
; This routine used to fall into DataAvail for the bulk of its processing.
; This is no longer the case...  A very popular internal modem seems to
; operate differently than a real 8250 when parity errors occur.  Falling
; into the DataAvail handler on a parity error caused the same character
; to be received twice.  Having this routine save the LSR status, and
; return to InterruptLoop fixes the problem, and still works on real COMM
; ports.  The extra overhead isn't a big deal since this routine is only
; entered when there is an exception like a parity error.
;
; This routine is jumped to, and will perform a jump back into
; the dispatch loop.
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,FLAGS
; History:
;-----------------------------------------------------------------------;


; assumes ds,Data
assumes es,nothing

public LineStat                        ;Public for debugging
LineStat proc near

	or	by EvtWord[si],EV_Err	;Show line status error

	add	dl,ACE_LSR-ACE_IIDR	;--> Line Status Register
	pin	al,dx

	test	al,ACE_PE+ACE_FE+ACE_OR ;Parity, Framing, Overrun error?
	jz	@f

	mov	LSRShadow[si],al	;yes, save status for DataAvail
@@:
	test	al,ACE_BI		;Break detect?
	jz	InterruptLoop		;Not break detect interrupt

	or	by EvtWord[si],EV_Break ;Show break

	jmp	short InterruptLoop

LineStat   endp

page

;----------------------------Private-Routine----------------------------;
;
; DataAvail - Data Available Interrupt Handler
;
; The available character is read and stored in the input queue.
; If the queue has reached the point that a handshake is needed,
; one is issued (if enabled).  EOF detection, Line Status errors,
; and lots of other stuff is checked.
;
; This routine is jumped to, and will perform a jump back into
; the dispatch loop.
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

; assumes ds,Data
assumes es,nothing

public DataAvail                       ;public for debugging
DataAvail   proc   near

	sub	dl,ACE_IIDR-ACE_RBR	;--> receiver buffer register
	pin	al,dx			;Read received character

	mov	ah,LSRShadow[si]	;what did the last Line Status intrpt
	mov	bh,ah			;  have to say?
	or	ah,ah
	jz	@f

	and	ah,ErrorMask[si]	;there was an error, record it
	or	by ComErr[si],ah
	mov	LSRShadow[si],0
@@:
	.errnz	ACE_OR-CE_OVERRUN	;Must be the same bits
	.errnz	ACE_PE-CE_RXPARITY
	.errnz	ACE_FE-CE_FRAME
	.errnz	ACE_BI-CE_BREAK

; Regardless of the character received, flag the event in case
; the user wants to see it.

	 or    by EvtWord[si],EV_RxChar ;Show a character received
         .errnz HIGH EV_RxChar

; Check the input queue, and see if there is room for another
; character.  If not, or if the end of file character has already
; been received, then go declare overflow.

         mov   cx,QInCount[si]         ;Get queue count (used later too)
         cmp   cx,QInSize[si]          ;Is queue full?
	 jge   DataAvail40	       ;  Yes, comm overrun
         test  EFlags[si],fEOF         ;Has end of file been received?
	 jnz   DataAvail40	       ;  Yes - treat as overflow

; Test to see if there was a parity error, and replace
; the character with the parity character if so

	 test  bh,ACE_PE	       ;Parity error
	 jz    DataAvail25	       ;  No
         test  Flags2[si],fPErrChar    ;Parity error replacement character?
	 jz    DataAvail25	       ;  No
         mov   al,PEChar[si]           ;  Yes, get parity replacement char

; Skip all other processing except event checking and the queing
; of the parity error replacement character

         jmp   short DataAvail80       ;Skip all but event check, queing

; See if we need to strip null characters, and skip
; queueing if this is one.  Also remove any parity bits.

DataAvail25:
         and   al,RxMask[si]           ;Remove any parity bits
	 jnz   DataAvail30	       ;Not a Null character
         test  Flags2[si],fNullStrip   ;Are we stripping received nulls?
	 jnz   DataAvail50	       ;  Yes, put char in the bit bucket

; Check to see if we need to check for EOF characters, and if so
; see if this character is it.

DataAvail30:
         test  Flags[si],fBinary       ;Is this binary stuff?
	 jnz   DataAvail60	       ;  Yes, skip EOF check
         cmp   al,EOFChar[si]          ;Is this the EOF character?
	 jnz   DataAvail60	       ;  No, see about queing the charcter
         or    EFlags[si],fEOF         ;Set end of file flag
         jmp   short DataAvail50       ;Skip the queing process

DataAvail40:
         or    by ComErr[si],CE_RXOVER ;Show queue overrun

DataAvail50:
	 jmp   InterruptLoop

; If output XOn/XOff is enabled, see if the character just received
; is either an XOn or XOff character.  If it is, then set or
; clear the XOffReceived flag as appropriate.

DataAvail60:
         test  Flags2[si],fOutX        ;Output handshaking?
	 jz    DataAvail80	       ;  No
         cmp   al,XOFFChar[si]         ;Is this an X-Off character?
	 jnz   DataAvail70	       ;  No, see about XOn or Ack
         or    HSFlag[si],XOffReceived ;Show XOff received, ENQ or ETX [rkh]
         test  Flags[si],fEnqAck+fEtxAck ;Enq or Etx Ack?
	 jz    DataAvail50	       ;  No
         mov   cx,QInCount[si]         ;Get current count of input chars
         cmp   cx,XONLim[si]           ;See if at XOn limit
	 ja    DataAvail50	       ;  No
         and   HSFlag[si],NOT XOffReceived ;Show ENQ or ETX not received
         or    HSFlag[si], XOnPending
	 jmp   short DataAvail75       ;Done

DataAvail70:
         cmp   al,XONChar[si]          ;Is this an XOn character?
	 jnz   DataAvail80	       ;  No, just a normal character
         and   HSFlag[si],NOT XOffReceived
         test  Flags[si],fEnqAck+fEtxAck ;Enq or Etx Ack?
	 jz    DataAvail75	       ;  No
         and   HSFlag[si],NOT EnqSent

DataAvail75:
	 call  FakeCOMInt

	 jmp   short DataAvail50       ;Done

; Now see if this is a character for which we need to set an event as
; having occured. If it is, then set the appropriate event flag


DataAvail80:
         cmp   al,EVTChar[si]          ;Is it the event generating character?
	 jne   DataAvail90	       ;  No
         or    by EvtWord[si],EV_RxFlag   ;Show received specific character

; Finally, a vaild character that we want to keep, and we have
; room in the queue. Place the character in the queue.
; If the discard flag is set, then discard the character

DataAvail90:
         test  HSFlag[si],Discard      ;Discarding characters ?
	 jnz   DataAvail50	       ;  Yes

	 cmp   DosXFlag,1	       ; running under 286 DOS extender ?
	 jne   DataAvail95
.286P
	 smsw  bx		       ; running in protect mode ?
.8086
	 test  bl,1
	 jnz   DataAvail95

	 les   di,QInSegment[si]       ; real mode segment of queue
	 jmp   short DataAvail97

DataAvail95:
         les   di,QInAddr[si]          ;Get queue base pointer

DataAvail97:
         assumes es,nothing
         mov   bx,QInPut[si]           ;Get index into queue
         mov   es:[bx][di],al          ;Store the character
         inc   bx                      ;Update queue index
         cmp   bx,QInSize[si]          ;See if time for wrap-around
	 jc    DataAvail100	       ;Not time to wrap
         xor   bx,bx                   ;Wrap-around is a new zero pointer

DataAvail100:
         mov   QInPut[si],bx           ;Store updated pointer
         inc   cx                      ;And update queue population
         mov   QInCount[si],cx

; If flow control has been enabled, see if we are within the
; limit that requires us to halt the host's transmissions

         cmp   cx,XOffPoint[si]        ;Time to see about XOff?
	 jc    DataAvail120	       ;  Not yet
         mov   cl,HSFlag[si]           ;Get handshake flag
         test  cl,HSSent+XOffPending   ;Handshake already sent?
	 jnz   DataAvail120	       ;  Yes, don't send it again

         mov   ah,HHSLines[si]         ;Should hardware lines be dropped?
         or    ah,ah                   ;  (i.e. do we have HW HS enabled?)
	 jz    DataAvail110	       ;  No
         add   dl,ACE_MCR              ;  Yes
         pin   al,dx                   ;Clear the nessecary bits
         not   ah
         and   al,ah
         or    cl,HHSDropped           ;Show lines have been dropped
	 pout  dx,al		       ;  and drop the lines
	 mov   HSFlag[si],cl	       ;  and remember they were dropped

DataAvail110:
         test  Flags2[si],fInX         ;Input Xon/XOff handshaking
	 jz    DataAvail115	       ;  No
         or    cl,XOffPending          ;Show XOFF needed
         mov   HSFlag[si],cl           ;Save updated handshake flags [rkhx] 3.00.01/002
	 call  FakeCOMInt	       ;Get pending char's from buffer

DataAvail115:
;         mov   HSFlag[si],cl           ;Save updated handshake flags [rkhx] 3.00.01/002

DataAvail120:
	 jmp   InterruptLoop

DataAvail endp
page

;----------------------------Private-Routine----------------------------;
;
; XmitEmpty - Transmitter Register Empty
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

; assumes ds,Data
assumes es,nothing

public XmitEmpty
XmitEmpty proc near

         add   dl,ACE_LSR-ACE_IIDR     ;--> Line Status Register
         pin   al,dx                   ;Is xmit really empty?
         test  al,ACE_THRE
	 jnz   XmitEmpty5	       ;Transmitter not empty, cannot send
	 jmp   XmitEmpty90

XmitEmpty5:
         sub   dl,ACE_LSR-ACE_THR      ;--> Transmitter Holding Register

; If the hardware handshake lines are down, then XOff/XOn cannot
; be sent.  If they are up and XOff/XOn has been received, still
; allow us to transmit an XOff/XOn character.  It will make
; a dead lock situation less possible (even though there are
; some which could happen that cannot be handled).

         mov   ah,HSFlag[si]           ;Get handshaking flag
         test  ah,HHSDown+BreakSet     ;Hardware lines down or break set?
	 jz    XmitEmpty10	       ;  No, can transmit
	 jmp   XmitEmpty100	       ;  Yes, cannot transmit

; Give priority to any handshake character waiting to be
; sent.  If there are none, then check to see if there is
; an "immediate" character to be sent.  If not, try the queue.

XmitEmpty10:
         test  Flags[si],fEnqAck+fEtxAck ;Enq or Etx Ack?
	 jz    XmitEmpty15	       ;  No
	 jmp   short XmitEmpty40       ;  Yes

XmitEmpty15:
         test  ah,HSPending            ;XOff or XOn pending
	 jz    XmitEmpty40	       ;  No
         test  ah,XOffPending          ;Is it XOff that needs to be sent?
	 jz    XmitEmpty20	       ;  No, it must be XOn
         and   ah,NOT XOffPending      ;Clear XOff pending
         or    ah,XOffSent             ;Show XOff sent
         mov   al,XOFFChar[si]         ;Get XOff character
         jmp   short XmitEmpty30       ;Send the character

XmitEmpty20:
         and   ah,NOT XOnPending+XOffSent
         mov   al,XONChar[si]          ;Get XOn character

XmitEmpty30:
         mov   HSFlag[si],ah           ;Save updated handshake flag
	 jmp   XmitEmpty70	       ;Go output the character

; If any of the lines which were specified for a timeout are low, then
; don't send any characters.  Note that by putting the check here,
; XOff and Xon can still be sent even though the lines might be low.

; Also test to see if a software handshake was received.  If so,
; then transmission cannot continue.  By delaying the software check
; to here, XOn/XOff can still be issued even though the host told
; us to stop transmission.

XmitEmpty40:
         test  ah,CannotXmit           ;Anything preventing transmission?
	 jz    XmitEmpty45	       ;  No
	 jmp   XmitEmpty100	       ;  Yes, disarm and exit

Xmit_jumpto90:
	 jmp   XmitEmpty90

; If a character has been placed in the single character "transmit
; immediately" buffer, clear that flag and pick up that character
; without affecting the transmitt queue.

XmitEmpty45:
         test  EFlags[si],fTxImmed     ;Character to xmit immediately?
	 jz    XmitEmpty515	       ;  No, try the queue
         and   EFlags[si],NOT fTxImmed ;Clear xmit immediate flag
         mov   al,ImmedChar[si]        ;Get char to xmit
	 jmp   XmitEmpty70	       ;Transmit the character

; Nothing immediate, see if there is a character in the
; transmit queue, and remove one if there is.

XmitEmpty50:
         test  ah,AckPending           ;Ack need to be sent?
	 jz    XmitEmpty515	       ;  No
         and   ah,NOT AckPending       ;  turn offf Ack need to be sent
         mov   al,XONChar[si]          ;Get Ack character
         mov   HSFlag[si],ah           ;Save updated handshake flag
	 jmp   XmitEmpty70	       ;Go output the character

XmitEmpty515:
         mov   cx,QOutCount[si]        ;Output queue empty?
	 jcxz  Xmit_jumpto90	       ;  Yes, go set an event

         test  Flags[si],fEtxAck       ;Etx Ack?
	 jz    XmitEmpty55	       ;  No
         mov   cx,QOutMod[si]          ;Get number bytes sent since last ETX
         cmp   cx,XONLim[si]           ;At Etx limit yet?
	 jne   XmitEmpty51	       ;  No, inc counter
         mov   QOutMod[si],0           ;  Yes, zero counter
         or    HSFlag[si],EtxSent      ;Show ETX sent
         mov   al,XOFFChar[si]         ;Get ETX char
	 jmp   short XmitEmpty70       ;Go output the character

XmitEmpty51:
         inc   cx                      ; Update counter
         mov   QOutMod[si],cx          ; Save counter
	 jmp   short XmitEmpty59       ; Send queue character

XmitEmpty55:
         test  Flags[si],fEnqAck       ;Enq Ack?
	 jz    XmitEmpty59	       ;  No, send queue character
         mov   cx,QOutMod[si]          ;Get number bytes sent since last ENQ
         cmp   cx,0                    ;At the front again?
	 jne   XmitEmpty56	       ;  No, inc counter
         mov   QOutMod[si],1           ;  Yes, send ENQ
         or    HSFlag[si],EnqSent      ;Show ENQ sent
         mov   al,XOFFChar[si]         ;Get ENQ char
	 jmp   short XmitEmpty70       ;Go output the character

XmitEmpty56:
         inc   cx                      ;Update counter
         cmp   cx,XONLim[si]           ;At end of our out buffer len?
	 jne   XmitEmpty58	       ;  No
         mov   cx,0                    ;Show at front again.

XmitEmpty58:
         mov   QOutMod[si],cx          ;Save counter

XmitEmpty59:
	 cmp   DosXFlag,1	       ; running under 286 DOS extender ?
	 jne   XmitEmpty594
.286P
	 smsw  bx		       ; running in protect mode ?
.8086
	 test  bl,1
	 jnz   XmitEmpty594

	 les   di,QOutSegment[si]      ; real mode segment of queue
	 jmp   short XmitEmpty598

XmitEmpty594:
	 les   di,QOutAddr[si]	       ;Get queue base pointer

XmitEmpty598:
         assumes es,nothing

         mov   bx,QOutGet[si]          ;Get pointer into queue
         mov   al,es:[bx][di]          ;Get the character

         inc   bx                      ;Update queue pointer
         cmp   bx,QOutSize[si]         ;See if time for wrap-around
	 jc    XmitEmpty60	       ;Not time for wrap
         xor   bx,bx                   ;Wrap by zeroing the index

XmitEmpty60:
         mov   QOutGet[si],bx          ;Save queue index
         mov   cx,QOutCount[si]        ;Output queue empty?
         dec   cx                      ;Dec # of bytes in queue
         mov   QOutCount[si],cx        ;  and save new population

; Finally!  Transmit the character

XmitEmpty70:
         pout  dx,al                   ;Send char
	 jmp   InterruptLoop

; No more characters to transmit.  Flag this as an event.

XmitEmpty90:
         or    by EvtWord[si],EV_TxEmpty

; Cannot continue transmitting (for any of a number of reasons).
; Disable the transmit interrupt.  When it's time resume, the
; transmit interrupt will be reenabled, which will generate an
; interrupt.

XmitEmpty100:
         inc   dx                      ;--> Interrupt Enable Register
         .errnz   ACE_IER-ACE_THR-1
         pin   al,dx                   ;I don't know why it has to be read
         and   al,NOT ACE_ETBEI        ;  first, but it works this way
       ; jmp   InterruptLoop
	 jmp   short XmitEmpty70       ;Set new interrupt value

XmitEmpty endp

page

;----------------------------Private-Routine----------------------------;
;
; ModemStatus - Modem Status Interrupt Handler
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;


; assumes ds,Data
assumes es,nothing

public ModemStatus                     ;Public for debugging
ModemStatus proc near

; Get the modem status value and shadow it for MSRWait.

         add   dl,ACE_MSR-ACE_IIDR     ;--> Modem Status Register
         pin   al,dx
         mov   MSRShadow[si],al        ;Save MSR data for others
         mov   ch,al                   ;Save a local copy

; Create the event mask for the delta signals

         mov   ah,al                   ;Just a lot of shifting
         shr   ax,1
         shr   ax,1
         shr   ah,1
         mov   cl,3
         shr   ax,cl
         and   ax,EV_CTS+EV_DSR+EV_RLSD+EV_Ring
         or    EvtWord[si],ax

         mov   ah,ch                                  ;[rkh]...
         shr   ah,1
         shr   ah,1
         and   ax,EV_CTSS+EV_DSRS
         or    EvtWord[si],ax

         mov   ah,ch
         mov   cl,3
         shr   ah,cl
         and   ax,EV_RLSD
         or    EvtWord[si],ax

         mov   ah,ch
         mov   cl,3
         shl   ah,cl
         and   ax,EV_RingTe
         or    EvtWord[si],ax

         .errnz      EV_CTS-0000000000001000b
         .errnz      EV_DSR-0000000000010000b
         .errnz     EV_RLSD-0000000000100000b
         .errnz     EV_Ring-0000000100000000b

         .errnz       EV_CTSS-0000010000000000b       ;[rkh]
         .errnz       EV_DSRS-0000100000000000b
         .errnz      EV_RLSDS-0001000000000000b
         .errnz     EV_RingTe-0010000000000000b

         .errnz    ACE_DCTS-00000001b
         .errnz    ACE_DDSR-00000010b
         .errnz   ACE_DRLSD-00001000b
         .errnz      ACE_RI-01000000b

         .errnz    ACE_TERI-00000100b                 ;[rkh]
         .errnz     ACE_CTS-00010000b
         .errnz     ACE_DSR-00100000b
         .errnz    ACE_RLSD-10000000b

ModemStatus10:
         mov   al,OutHHSLines[si]      ;Get output hardware handshake lines
         or    al,al                   ;Any lines that must be set?
	 jz    ModemStatus30	       ;No hardware handshake on output
         and   ch,al                   ;Mask bits of interest
         cmp   ch,al                   ;Lines set for Xmit?
	 je    ModemStatus20	       ;  Yes
         or    HSFlag[si],HHSDown      ;Show hardware lines have dropped
         jmp   short ModemStatus30

; Lines are set for xmit.  Kick an xmit interrupt if needed

ModemStatus20:
         and   HSFlag[si],NOT HHSDown  ;Show hardware lines back up
	 call  FakeCOMInt	       ;Get pending char's from buffer

ModemStatus30:
	 jmp   InterruptLoop

ModemStatus endp

page

;-----------------------------Public-Routine----------------------------;
;
; get_int_vector - get interrupt vector and IRQ
;
; The interrupt vector number and the handler address are returned
; to the caller.
;
; Entry:
;   DS:SI --> DEB
;   CX = Port base address (3F8,3E8 or 2F8, 2E8, PS/2)
;   DS = Data
; Returns:
;   AH = IRQ number
;   AL = 8259 Mask
;   DI:DX --> interrupt handler
; Error Returns:
;   None
; Registers Destroyed:
;   FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public get_int_vector
get_int_vector proc far

         cmp   BIOSPortLoc[si], 0      ; com1 ? [rkh]
	 je    get_int_vect_com1

         cmp   BIOSPortLoc[si], 2      ; com2 ? 
	 je    get_int_vect_com2

         cmp   BIOSPortLoc[si], 6      ; com4 ? 
	 je    get_int_vect_com4

         mov   [deb_com3],cx           ; must be com3
         call  IBMmodel
	 jc    get_int_vect_irq4       ; PC
	 jmp   short get_int_vect_irq3 ; PS/2

get_int_vect_com1:
         mov   [deb_com1],cx
	 jmp   short get_int_vect_irq4

get_int_vect_com2:
         mov   [deb_com2],cx
	 jmp   short get_int_vect_irq3

get_int_vect_com4:
         mov   [deb_com4],cx
	 jmp   short get_int_vect_irq3

get_int_vect_irq4:
         mov   dx,IntCodeOFFSET COM_IRQ4
         mov   ax,IRQ4*256+00010000b   ;Int vector + 8259 mask
	 jmp   short get_int_vect_exit

get_int_vect_irq3:
         mov   dx,IntCodeOFFSET COM_IRQ3
         mov   ax,IRQ3*256+00001000b   ;Int vector + 8259 mask

get_int_vect_exit:
         mov   di,cs                   ;Return DI:DX --> int handler
         ret

get_int_vector   endp

;-----------------------------------------------------------------------;
; WEP
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 13-Jan-1990 18:33:48  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	WEP,<PUBLIC,FAR>
cBegin nogen
	nop				; You don't want to know why.
	mov	ax,1
	ret	2
cEnd nogen


page

ifdef DEBUG
   public   deb_com1
   public   deb_com2
   public   deb_com3
   public   deb_com4
   public   InterruptLoop
   public   IntLoop10
   public   IntLoop20
   public   DataAvail25
   public   DataAvail30
   public   DataAvail40
   public   DataAvail50
   public   DataAvail60
   public   DataAvail70
   public   DataAvail80
   public   DataAvail90
   public   DataAvail95
   public   DataAvail97
   public   DataAvail100
   public   DataAvail110
   public   DataAvail115
   public   DataAvail120
   public   XmitEmpty10
   public   XmitEmpty20
   public   XmitEmpty30
   public   XmitEmpty40
   public   XmitEmpty50
   public   XmitEmpty59
   public   XmitEmpty60
   public   XmitEmpty70
   public   XmitEmpty90
   public   XmitEmpty100
   public   ModemStatus10
   public   ModemStatus20
   public   ModemStatus30
   public   int_dont_kick_tx
   public   get_int_vect_exit
endif

sEnd   IntCode
end
