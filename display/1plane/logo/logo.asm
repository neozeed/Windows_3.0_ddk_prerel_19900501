        page    ,132

;***************************************************************************
;                                                                          *
;   Copyright (C) 1985-1986 by Microsoft Inc.                              *
;                                                                          *
;***************************************************************************
;       Olivetti history
;       23 jul 87       plb     Removed big block comment, removed most of
;                               sample Microsoft logo data, added
;                               IFDEF ?OLIVETTI, IF OLI stuff....
;        1 sep 87       plb     Adding color enhancements from vers. 1.03.
;
;       14 sep 87       wsh     Added monitor detection for Olivetti displays.
;                               Now one logo will work for either bw or color.
;                               Also, fixed equipment problem flag.
;***************************************************************************

        title   LOGO - Windows Logo Code for CGA/EGA/Hercules Adapters
        %out    LOGO
page


;       The comments are still in MSLOGO.ASM, which is the original
;       file.

; This version adds:
;
;       ifdef ?OLIVETTI
;       ifdef ?OLICOLOR
;       if OLIVETTI
;       if OLICOLOR
;
; statements.

;
; Deb stuff
;
ENHANID         =       01000000b       ; set = affect IND board status register
EXPANSION_1     =       01000000b       ; set = DEB not present
EXPANSION_2     =       10000000b
VID_IO_ADDRESS  =       03ddh           ; video electronics i/o address register
DEB_MODE_CONTROL =      03dfh           ; deb mode control
INPUT_STATUS    =       003DAh          ; status register

LogoPtr macro   sr,ir                 ;;Get long pointer to logo data in es:di
        push    cs                    ;;Macro need only be valid immediately
        pop     es                    ;;  after frame is set up.
        lea     di,LogoData
        endm



errnz macro x
if2                                     ;;Generate error if expression not zero
if x
errnz1 <x>,%(x)
endif
endif
endm
errnz1 macro x1,x2                      ;;Generate error if $-exp not zero
= *ERRNZ* x1 = x2
endm
errn$ macro l,x
errnz <OFFSET $ - OFFSET l x>
ENDM



CEQU macro l,c,e,m,h,o                    ;;Conditional EQU macro
if CGA
l equ c
else
if EGA
l equ e
else
if EGAMONO
l equ m
else
if HERCULES
l equ h
else
if OLIVETTI
l equ o
endif
endif
endif
endif
endif
endm
page

;       Take the command line display definition and turn it into an
;       internal flag.  The valid command line definitions are:
;
;           -D?CGA      - logo is for CGA
;           -D?EGA      - logo is for EGA
;           -D?EGAMONO  - logo is for EGA monochrome mode
;           -D?HERCULES - logo is for the Hercules adapter
;
;       Currently ?CGA and ?EGA are the same.  If this changes, the release
;       batch files must also change (see \windows\release\retail\retfinl2.bat)
;
;       The CGA mode is also used for the EGA adapter.  The Enhanced
;       Color Display will be run in 200 scan line mode so that the
;       color overscan register can be used.


FALSE   equ     0
TRUE    equ     (NOT FALSE)

if1                                     ;Only parse command line on pass 1
EGAMONO =       FALSE                   ;Show no adapter defined yet
HERCULES=       FALSE
CGA     =       FALSE
EGA     =       FALSE
OLIVETTI =      FALSE
OLICOLOR =      FALSE

ifdef ?EGAMONO                          ;If defined, then EGA monochrome
EGAMONO = TRUE
%out ! Microsoft Windows Logo for EGA Monochrome monitor
endif

ifdef ?HERCULES                         ;If defined, then Hercules adapter
HERCULES = TRUE
%out ! Microsoft Windows Logo for Hercules Graphics card
endif

ifdef ?EGA                              ;If defined, then EGA adapter
EGA = TRUE
%out ! Microsoft Windows Logo for EGA
endif

ifdef ?CGA
CGA = TRUE                              ;If defined, then CGA adapter
%out ! Microsoft Windows Logo for CGA
endif

ifdef ?OLIVETTI
OLIVETTI = TRUE
%out ! Microsoft Windows Logo for Olivetti/AT&T 640 x 400
endif

ifdef ?OLICOLOR
%out !  .. Display Enhancement Board
endif

endif                                   ;End of pass1 conditionals

page


LogoDef         struc                   ;Definition of logo data (sans text)

; TextGoesHere  db      ? dup (?)       ;The text strings are first
  LDefMerge     db      ?               ;Merge flag,  0 = don't merge
  LDefHeight    db      ?               ;Height of the logo
  LDefWidth     db      ?               ;Width of the logo
  LDefBytes     db      ?               ;Logo bytes start here

LogoDef         ends


NumStrs         equ     8               ;Always 8 Asciiz strings



bptr            equ     byte ptr
wp              equ     word ptr


Equip_Flag      equ     410h            ;Equipment flag byte
CGAFont         equ     0FA6Eh          ;Offset into BIOS RAM of CGA font
CGAFontSEG      equ     0F000h          ;Segment of CGA font


black           equ     0               ;Colors for the ibm pc
blue            equ     1
green           equ     2
cyan            equ     3
red             equ     4
magenta         equ     5
yellow          equ     6
white           equ     7
bright          equ     8
coLogo          equ     bright+blue
page

;       Interleave      Used to adjust the starting scan line number
;                       to account for interleaved displays (i.e. CGA)
;
;       Indent          Number of bytes to indent logo on each side
;
;       RepLine         Index from current scan to replicated scan
;
;       NextScan        Value to add to get to next scan while rotating
;
;       RJScan          Value to add to get from left-justified scan to
;                       right-justified scan
;
;       CenterScan      Scanline to center logo on.
;
;       BytesPerScan    Number of bytes per scanline
;
;       MaxChars        Maximum number of characters per text line
;
;       MaxHeight       Maximum # of scans in logo source
;
;       MaxWidth        Maximum # of bytes in one logo scan line
;
;       ScreenSeg       Segment of the screen
;
;
;                            CGA     EGA   EGAMono   Herc    Olivetti

    CEQU    Interleave,        1,      1,      0,       2,       2
    CEQU    Indent,            0,      0,      0,       5,       0
    CEQU    RepLine            0       0      80,   2000h,   2000h
    CEQU    NextScan          80,     80,   80*4,      90,      80
    CEQU    RJScan         2000h,  2000h,   80*2,   4000h,   4000h
    CEQU    CenterScan        54,     54,    100,     100,      54
    CEQU    BytesPerScan      80,     80,     80,      90,      80
    CEQU    MaxChars          80,     80,     80,      80,      80
    CEQU    MaxHeight         50,     50,     50,      50,      50
    CEQU    MaxWidth          67,     67,     67,      67,      67
    CEQU    ScreenSeg     0B800h, 0B800h, 0A000h,  0B800h,  0B800h
page

        code    segment
        assume  cs:code,ds:code


;       Dispatch - Main entry points for logo code
;
;       Control is passed into the logo code following the "LOGO" ID,
;       based on the function to be performed.  Entry is at Dispatch+4
;       if the logo code is to be displayed, entry is at Dispatch+7 to
;       restore any state.
;
;       The "LOGO" ID must be the first 4 bytes of the segment.
;
;       The code that actually draws the logo, and the logo data will be
;       discarded by the caller upon our return.
;
;       Entry:  None
;
;       Exit:   (ax) = offset of first byte of disposible code
;                 for drawing the logo
;
;       Uses:   All


Dispatch:
        DB      'LOGO'                  ;Main entry point follows
        jmp     near ptr logo           ;Jump to disposable code
;       jmp     near ptr restore
        errn$   restore                 ;Must follow




;       Restore - Restore Video State
;
;       Any previously saved hardware video state is restored.
;
;       Entry:  None
;
;       Exit:   None
;
;       Uses:   All



if CGA OR EGA

RESTORE PROC    FAR

        xor     ax,ax                   ;Restore previous equipment flag
        mov     es,ax
        mov     al,[Equipment]
        mov     bptr es:[Equip_Flag],al
        mov     al,[prevMode]           ;Restore previous screen mode
        int     10h
        ret

prevMode        db      ?               ;Screen mode when powered up
Equipment       db      ?               ;Equipment byte when powered up

RESTORE ENDP

endif; CGA OR EGA

if OLIVETTI

RESTORE PROC    FAR

        xor     ax,ax                   ;Restore previous equipment flag
        mov     es,ax
        mov     al,[Equipment]
        mov     bptr es:[Equip_Flag],al
        mov     al,[prevMode]
        mov     bx,[debspecial]
        int     10h
        ret

prevMode        db      ?               ;Screen mode when powered up
debspecial      dw      0
Equipment       db      ?               ;Equipment byte when powered up

RESTORE ENDP

endif ; OLIVETTI

if EGAMONO

RESTORE PROC    FAR

        xor     ax,ax                   ;Restore previous screen mode
        mov     al,[prevMode]
        int     10h
        ret

prevMode        db      ?               ;Screen mode when powered up

RESTORE ENDP

endif; EGAMONO



if HERCULES

RESTORE PROC    FAR

        mov     ax,3                    ;Force into text mode
        int     10h
        ret

RESTORE ENDP

endif; HERCULES

BlowAway:                               ;All code past this point is
                                        ;  destroyed after boot
page

;       PrnMsg - Print Message
;
;       The given message is displayed on the screen in graphics mode
;
;       Entry:  ds:si --> Asciiz string to display
;               es:    =  Display segment
;               di     =  Address where string goes
;
;       Exit:   si    --> Past null terminator
;
;       Uses:   All


if CGA OR EGA OR EGAMONO OR OLIVETTI

;       For the IBM displays, the ROM BIOS will be called to display
;       the characters. Can't get much easier!

PrnMsg:
        cld
        mov     dx,di
        mov     bx,7                    ;bh = page, bl = no color for graphics
        mov     ah,2
        int     10h

Prn1:
        lodsb                           ;Get the current character
        or      al,al                   ;Is this the null terminator?
        jz      Prn2                    ;  Yes, exit
if OLIVETTI
        mov     bl,15
endif
        mov     ah,14                   ;Display the character
        int     10h
        jmp     Prn1                    ;Continue with next character

Prn2:
        ret

endif;  CGA OR EGA OR EGAMONO OR OLIVETTI


if HERCULES

;       For the Hercules card, the CGA Font in the ROM BIOS will be used.
;       The font definition will be fudged a little.  By selectively
;       duplicating some of the rows of the font, a 12 scan high font can
;       be synthesized.

PrnMsg:
        cld

Prn1:
        lodsb                           ;Get next byte to display
        or      al,al                   ;Terminator?
        jz      Prn5                    ;  Yes, all done

        push    ds                      ;Save some important registers
        push    si
        push    di
        xor     ah,ah                   ;Compute offset into font definition
        shl     ax,1                    ;  *2
        shl     ax,1                    ;  *4
        shl     ax,1                    ;  *8
        add     ax,CGAFont              ;Add base address of the font
        mov     si,ax                   ;Set ds:si --> first scan of the font
        mov     ax,CGAFontSEG
        mov     ds,ax


;       A sixteen bit mask will be used to control which scans are to
;       be replicated.  If the last scan will never be replicated, then
;       the following definition can be used:
;
;           A 1 bit indicates that the scan is to be copied.
;           A 0 bit indicates that the scan should be replicated.
;
;       Since the last scan is never replicated, if a shift is always
;       performed to see what the next operation is, when the mask becomes
;       0, the character has been drawn.
;
;       If the character has a descender, then we want to replicate
;       the 5th scan twice.  If it doesn't have a descender, then
;       the scan to replicate twice is the sixth.


        mov     cx,1101101100110000b    ;Assume no descender
        test    byte ptr [si+7],0FFh    ;Is there a descender?
        jz      Prn2                    ;  No
        mov     cx,1101101001110000b    ;  Yes, replicate 5th scan twice

Prn2:
        jcxz    Prn4                    ;Character has been drawn
        shl     cx,1                    ;Replicate or copy new scan?
        jnc     Prn3                    ;Just replicate previous scan
        lodsb                           ;Move next scan

Prn3:
        stosb                           ;One more row done
        add     di,2000H-1              ;--> next destination byte
        jns     Prn2                    ;Not time to wrap
        sub     di,8000H-90             ;Wrap
        jmp     Prn2

Prn4:
        pop     di                      ;Finished for this character
        pop     si
        pop     ds
        inc     di                      ;--> next destination
        jmp     Prn1

Prn5:
        ret


endif;  HERCULES
page

;       LogoRight - Rotate Left-justified Scan Lines of Logo Right
;
;       All left-justified scan lines of the logo are rotated right 2 pixels.
;
;       Some math:
;
;           On the CGA, since the even scan lines are contiguous,
;           the rotate could be considered one big continuous
;           rotate.  However, for a 67 byte image on a 80 byte
;           wide screen, 10 extra bytes would be rotated.  If working
;           with words, this would cost "5 * loop clocks * scans".
;           Assuming 45 clocks/word and 18 scan lines of height
;           (# even scans), this would cost 4050 clocks.  If the
;           clock speed is 5 MHz, this is almost a millisecond.
;           Too costly.  Only rotate the minimum required.  The
;           minimum when rotating words is:
;
;                   (Width of logo + 3) / 2
;
;           We must round up to the next word since we could
;           overlap a word boundary.
;
;       Entry:  es:di --> where to start rotating
;               bx     =  Index to replicated line (EGAMono and Hercules)
;               cx     =  Width of logo rounded up as above
;               dh     =  # of scanlines to rotate
;
;       Exit:   bx     =  Index to replicated line (EGAMono and Hercules)
;               cx     =  Width of logo rounded up as above
;
;       Uses:   ax,cx,dx,di,flags


LogoRight:
        cld

RRlp0:
        push    cx                      ;Save word count
        push    di                      ;Save start of scanline
        xor     dl,dl                   ;Set previous bits = 0 (including carry)

RRlp:
        mov     ax,es:[di]              ;Get current word
        xchg    al,ah                   ;Place into correct ording
        rcr     ax,1                    ;Rotate in new D14, 'C' = previous D0
        rcl     dl,1                    ;Save previous D0,  'C' = new D15
        rcr     ax,1                    ;Rotate in new D15, 'C' = previous D1
        rcr     dl,1                    ;Save previous D1,  'C' = previous D0
        xchg    al,ah                   ;Restore ording

if HERCULES OR EGAMONO OR OLIVETTI
        mov     es:[di+bx],ax           ;Store replicated line
endif

        stosw                           ;Store new word
        loop    RRlp                    ;Until all words moved
        pop     di                      ;Restore source pointer
        pop     cx                      ;Restore width word count
        add     di,NextScan             ;--> next scan line
        dec     dh                      ;More scan lines?
        jnz     RRlp0                   ;  Yes, rotate them
        ret
page

;       LogoLeft - Rotate Right-justified Scan Lines of Logo Left
;
;       All right-justified scan lines of the logo are rotated left 2 pixels.
;
;       Some math:
;
;           On the CGA, since the odd scan lines are contiguous,
;           the rotate could be considered one big continuous
;           rotate.  However, for a 67 byte image on a 80 byte
;           wide screen, 10 extra bytes would be rotated.  If working
;           with words, this would cost "5 * loop clocks * scans".
;           Assuming 45 clocks/word and 18 scan lines of height
;           (# even scans), this would cost 4050 clocks.  If the
;           clock speed is 5 MHz, this is almost a millisecond.
;           Too costly.  Only rotate the minimum required.  The
;           minimum when rotating words is:
;
;                   (Width of logo + 3) / 2
;
;           We must round up to the next word since we could
;           overlap a word boundary.
;
;       Entry:  es:di --> where to start rotating
;               bx     =  Index to replicated line (EGAMono and Hercules)
;               cx     =  Width of logo rounded up as above
;               dh     =  # of scanlines to rotate
;
;       Exit:   bx     =  Index to replicated line (EGAMono and Hercules)
;               cx     =  Width of logo rounded up as above
;
;       Uses:   ax,cx,dx,di,flags


LogoLeft:
        std                             ;Will be walking backwards

LLlp0:
        push    cx                      ;Save word count
        push    di                      ;Save start of scanline
        xor     dl,dl                   ;Set previous bits = 0 (including carry)

LLlp:
        mov     ax,es:[di]              ;Get current word
        xchg    al,ah                   ;Place into correct ording
        rcl     ax,1                    ;Rotate in new D14, 'C' = previous D0
        rcl     dl,1                    ;Save previous D0,  'C' = new D15
        rcl     ax,1                    ;Rotate in new D15, 'C' = previous D1
        rcr     dl,1                    ;Save previous D1,  'C' = previous D0
        xchg    al,ah                   ;Restore ording

if HERCULES OR EGAMONO or OLIVETTI
        mov     es:[di+bx],ax           ;Store replicated line
endif

        stosw                           ;Store new word
        loop    LLlp                    ;Until all words moved
        pop     di                      ;Restore source pointer
        pop     cx                      ;Restore width word count
        add     di,NextScan             ;--> next scan line
        dec     dh                      ;More scan lines?
        jnz     LLlp0                   ;  Yes, rotate them
        cld
        ret
page

;       DrawLogo - Draw Logo
;
;       The left-justified or right-justified scan lines of the source
;       are drawn at the given location.  For the Hercules and EGAMono
;       versions, the scan lines will be duplicated.
;
;       Entry:  ed:di --> where the scan lines go
;               ds:si --> source scans
;               ax     =  Index to next scan (less width of logo)
;               bh     =  Number of scans to copy
;               dl     =  Width of source
;
;       Exit:   ds:si --> next source scan
;               ax     =  Index to next scan (less width of logo)
;               dl     =  Width of source
;
;       Uses:   bh,cx,si,di,flags


DrawLogo:
        xor     cx,cx                   ;Zero CH

DrawLogo10:


if HERCULES OR EGAMONO OR OLIVETTI
        push    di                      ;Duplicate the scan
        push    si
        add     di,RepLine              ;--> where duplicate scan goes
        mov     cl,dl                   ;Set move count
        shr     cx,1                    ;/2 for words
        rep     movsw                   ;Move all the words
        jnc     DrawLogo20              ;Count was even
        movsb                           ;Move odd byte

DrawLogo20:
        pop     si                      ;Restore source/dest pointers
        pop     di
endif

        mov     cl,dl                   ;Set move count
        shr     cx,1                    ;/2 for words
        rep     movsw                   ;Move all the words
        jnc     DrawLogo30              ;Count was even
        movsb                           ;Move odd byte

DrawLogo30:
        add     di,ax                   ;Update pointer to destination
        dec     bh                      ;Update count of scan lines
        jnz     DrawLogo10              ;Continue if count is not zero
        ret
page

;       Init - Initialize Video Hardware
;
;       Any video hardware initialization required is performed.
;       The current state of the hardware is saved so that it may
;       be restored when Windows finally shuts down.
;
;       Entry:  None
;
;       Exit:   None
;
;       Uses:   All

if OLIVETTI

DebFileName     db      'DEBDRIVE',0

ifdef ?OLICOLOR
NoDeb           db      'Device driver DEDRIVER.DEV is not installed.',13,10,'$'
endif

Init:
        push    es
        push    di
        mov     ah,15                   ;Get and save current mode
        int     10h
        pop     di
        pop     es
        mov     [prevMode],al
        mov     [DebSpecial],bx

;       If a mono adapter is installed, switch to the CGA (EGA) adapter.
;       Upon exit, the adapter will be restored.

        xor     ax,ax
        mov     es,ax
        mov     al,bptr es:[Equip_Flag]
        mov     [EquipMent],al
        not     al
        test    al,00110000b            ;Monochrome card?
        jnz     ini1                    ;  No, ok as is
        not     al
        and     al,11101111b
        mov     bptr es:[Equip_Flag],al ;Make CGA active
        jmp     InitM24Mono             ; can't have a deb and a mono card
ini1:

; Determine if DEB board and driver installed by trying to open "DEBDRIVE"
; with Open Handle system call, and then checking whether it's a file
; or a device:

        mov     ax,3d00h        ; open handle, read mode
        mov     dx,offset DebFileName
        int     21h             ; try to open
        jc      InitM24Mono     ; no carry, driver is loaded, so close it
        push    ax              ; save handle
        mov     bx,ax           ; now get device IOCTL data word
        mov     ax,4400h        ; IOCTL read
        int     21h             ; DX contains IOCTL word
        pop     bx              ; get handle back
        mov     ah,3eh          ; close function
        int     21h
        test    dl,80h          ; bit 7 of data word is 1 if this is device
        jz      InitM24Mono

; Now test for DEB hardware and the monitor type.
; If a color monitor is found anywhere, do a DEB logo.

TestForDebBoard:
        mov     ch,00000011b    ; test value
        mov     al,1            ; targets DEB mode control reg
        mov     dx,VID_IO_ADDRESS
        out     dx,al
        mov     al,0            ; set DEB mode control register bit 6 to 0
        mov     dx,DEB_MODE_CONTROL
        out     dx,al
        mov     dx,INPUT_STATUS ; get IND status register into AL
        in      al,dx
        test    al,EXPANSION_1
        jnz     InitM24Mono     ; deb driver but no hardware board
; now for monitor type
        mov     al,ENHANID
        mov     dx,DEB_MODE_CONTROL
        out     dx,al
        mov     dx,INPUT_STATUS
        in      al,dx           ; read the value
        mov     cl,4
        shr     al,cl
        mov     ah,al           ; save al
        and     al,ch
        cmp     al,2
        je      InitM24Color            ; found color
        mov     cl,2
        shr     ah,cl
        and     ah,ch
        cmp     ah,2
        jne     InitM24Mono             ; last chance to find color failed

InitM24Color:

        mov     ax,66           ; set 16-color 640 x 400 mode
        int     10h

        push    cx              ; delay to let CRT synch settle
        xor     cx,cx
InitWait:
        jmp     InitSkip        ; flush queue to take time
InitSkip:
        loop    InitWait
        pop     cx

        ; Set palette colors for logo

        mov     ah,11           ; set palette
        mov     al,0            ; select palette position
        mov     bh,0            ; position -- background
        mov     bl,blue         ; color = blue
        int     10h

        mov     ah,11           ; set palette
        mov     al,0            ; select palette position
        mov     bh,8            ; position -- color of logo
        mov     bl,bright+white ; color = hi intensity white
        int     10h

        mov     ah,11           ; set palette
        mov     al,0            ; select palette position
        mov     bh,15           ; position -- color of text
        mov     bl,bright+white ; color = hi intensity white
        int     10h
        jmp     WaitAfterInit   ; done, but delay a little..

; Monochrome, so logo is white on black (or black on white for POSITIVE video).
; If we were trying to load a system with the Windows DEB driver, getting
; here is an error..

InitM24Mono:
ifdef ?OLICOLOR
                                ; no DEDRIVER.DEV, so display message and abort
        mov     ax,3            ; set text mode
        int     10h
        mov     DX,offset NoDeb ; display message
        mov     ah,9
        int     21h
        mov     ax,3            ; set monochrome 640 x 400 mode
        int     10h
        mov     ax,4c00h        ; terminate process
        int     21h
else
        mov     ax,64           ; set monochrome 640 x 400 mode
        int     10h
endif

WaitAfterInit:
        push    cx                      ; delay to let CRT synch settle
        xor     cx,cx                   ; (Olivetti enhancement)
WaitAfterILoop:
        jmp     WaitSkip                ; flush queue to take time
WaitSkip:
        loop    WaitAfterILoop
        pop     cx
        ret

endif   ; OLIVETTI


if CGA OR EGA

Init:
        mov     ah,15                   ;Get and save current mode
        int     10h
        mov     [prevMode],al


;       If a mono adapter is installed, switch to the CGA (EGA) adapter.
;       Upon exit, the adapter will be restored.  Also Set blue background
;       and overscan which looks real good on the EGA.

        xor     ax,ax
        mov     es,ax
        mov     al,bptr es:[Equip_Flag]
        mov     [EquipMent],al
        not     al
        test    al,00110000b            ;Monochrome card?
        jnz     ini1                    ;  No, ok as is
        not     al
        and     al,11101111b
        mov     bptr es:[Equip_Flag],al ;Make CGA active

ini1:
        mov     ax,6                    ;Set 640 x 200 mode
        int     10h
        mov     ax,1000H                ;Set blue background color
        mov     bh,1
        int     10h
        mov     ax,1001H                ;Set blue overscan
        mov     bh,1
        int     10h
        ret

endif; CGA OR EGA



if EGAMONO

Init:
        mov     ah,15                   ;Save current mode
        int     10h
        mov     [prevMode],al
        mov     ax,15                   ;Set 640 x 350 mode
        int     10h
        ret

endif;  EGAMONO



if HERCULES

;       For the Hercules, we have to initialize the adapter ourselves.
;       The alternate page (B800:0) will be used to be compatible with
;       the Hercules look-a-likes which only have that page on them.

Our6845 equ     3b4h
OurCtrl equ     3b8h
OurCnfg equ     3bfh

gTable  db      35h,2dh,2eh,07h         ;Data to output to 6845
        db      5bh,02h,57h,57h         ;  for graphics mode
        db      02h,03h,00h,00h


Init:
        cld                             ;Always a save thing to do
        mov     al,00000011b            ;Enable graphics and allow
        mov     dx,OurCnfg              ;  page two
        call    OutByte
        mov     al,82h                  ;Graphics, page 2, video disabled
        call    OutCtrl
        mov     cx,12                   ;# of bytes to send to 6845
        mov     bx,offset gTable        ;--> start of 6845 parameter table
        xor     ax,ax                   ;Set initial 6845 data register
        cli

sm2:
        mov     al,ah                   ;Get register number
        mov     dx,Our6845              ;--> address register
        call    OutByte                 ;Output the byte
        mov     al,cs:[bx]              ;Get data byte
        inc     bx                      ;--> next data byte
        call    OutByte                 ;Output the data byte
        inc     ah                      ;Set next register number
        loop    sm2                     ;Until all output
        sti

        mov     ax,ScreenSeg            ;Clear VRAM
        mov     es,ax
        mov     cx,8000H/2              ;Set number of words to clear
        xor     di,di                   ;es:di --> start of VRAM
        xor     ax,ax
        rep     stosw
        mov     al,10001010b            ;Graphics, page 2, video enabled

OutCtrl:
        mov     dx,OurCtrl              ;--> Hercules control register

OutByte:
        out     dx,al                   ;Output data to Hercules adapter
        inc     dx                      ;--> next register
        ret

endif;  HERCULES
page


;       TextOffsets is the table used to determine where a text string will
;       be placed on the screen.  For the CGA, EGA, and EGAMONO, this will
;       be a text mode cursor address (i.e. row 25, column 0).  For the
;       Hercules, this will be some arbitrary scan line address.  Any
;       indentation is implemented via this table.


TextOffsets     label   word


if CGA OR EGAMONO OR EGA or OLIVETTI

        db      Indent,24               ;Copyright line 6
        db      Indent,23               ;Copyright line 5
        db      Indent,22               ;Copyright line 4
        db      Indent,21               ;Copyright line 3
        db      Indent,20               ;Copyright line 2
        db      Indent,19               ;Copyright line 1
        db      Indent,15               ;Product description line 2
        db      Indent,14               ;Product description line 1

endif;  CGA or EGAMONO or EGA or OLIVETTI


if HERCULES


HAddr   macro x
        dw      ((x/4)*BytesPerScan)+((x AND 3)*2000h)+Indent
        endm

        HAddr      332                     ;Copyright line 6
        HAddr      316                     ;Copyright line 5
        HAddr      300                     ;Copyright line 4
        HAddr      284                     ;Copyright line 3
        HAddr      268                     ;Copyright line 2
        HAddr      252                     ;Copyright line 1
        HAddr      208                     ;Product description line 2
        HAddr      192                     ;Product description line 1

endif;  HERCULES




FrameData       struc

  TextAddrs     dw      NumStrs dup (?) ;Screen addr for each string
  PtrLogo       dd      ?               ;Pointer to logo data
  RJScanAddr    dw      ?               ;# Right justified start address
  LJScanAddr    dw      ?               ;# Left  justified start address
  LogoWidth     dw      ?               ;Width of logo
  DeltaBytes    dw      ?               ;# bytes to shift logo
  RJScanCount   db      ?               ;# Right justified scans in source
  LJScanCount   db      ?               ;# Left  justified scans in source
  Div4          db      ?               ;/4 counter

FrameData       ends

page

;       Logo - Draw Merging Logo on the Display
;
;       The given logo and text image is drawn on the screen.
;
;       Entry:  None
;
;       Exit:   ax = first disposable address
;
;       Uses:   ax,bx,cx,dx,es,flags


Logo    proc    FAR

        push    bp                      ;Set up frame
        sub     sp,((SIZE FrameData)+1) AND 0FFFEh
        mov     bp,sp
        push    si
        push    di
        LogoPtr                         ;Set es:di --> logo data
        mov     wp PtrLogo[bp],di       ;Save offset  of logo data pointer
        mov     wp PtrLogo+2[bp],es     ;Save segment of logo data pointer

        call    Init                    ;Initialize hardware



;       Compute the starting address for each string, checking
;       their lengths to make sure they aren't too long.
;
;       The starting address within the scan line will be 1/2 the
;       amount of white space in the string.  Since all the characters
;       are in an 8 bit wide font, everything can be calculated as
;       byte addresses.  Any indentation has been precomputed into
;       the TextOffsets table.



        mov     si,NumStrs*2-2          ;Set string count
        les     di,PtrLogo[bp]          ;Get back pointer to text

Logo10:
        call    HowLongIsIt             ;Get length of next string
        mov     ax,MaxChars             ;Compute amount of white space
        sub     ax,cx
        jnc     Logo15
        xor     ax,ax                   ;String too long, use no white space
Logo15:
        sar     ax,1                    ;Compute 1/2 white space

        if      1                       ;If odd char left of center
        add     ax,TextOffsets[si]      ;  Set actual starting address
        else                            ;If odd char right of center
        adc     ax,TextOffsets[si]      ;  Set actual starting address
        endif
        errnz   <HIGH MaxChars>         ;Assuming it will be a byte

        mov     TextAddrs[si][bp],ax    ;Save address
        dec     si                      ;Processed all strings yet?
        dec     si
        jns     Logo10                  ;  No




;       All the text strings are valid and their display addresses
;       have been placed onto the stack.  Now process the actual
;       logo data.
;
;       Currently:      es:di --> height of the logo data


        mov     al,es:LDefHeight[di]    ;Get height of the logo
        cmp     al,MaxHeight            ;Is the logo too high?
        ja      Logo20                  ;  Yes, don't draw it
        cmp     al,1                    ;Is the logo too low (0 or 1)?
        ja      Logo30                  ;  No, allow it

Logo20:
        jmp     LogoExit                ;  Yes, don't draw it


;       Compute the number of left-justified and right-justified
;       lines in the image.


Logo30:
        xor     ah,ah
        shr     ax,1
        adc     ah,al                   ;Any odd scan goes to the left
        mov     wp RJScanCount[bp],ax   ;Save # of scans in image
        errnz   LJScanCount-RJScanCount-1


;       Center the logo around the specified scan.  Since both
;       the EGAMono and Hercules double the image, the height
;       can simply be subtracted off of the centering scan.
;       The CGA doesn't double the image, so take 1/2 the size
;       of the image.


if      EGAMONO OR HERCULES or OLIVETTI
        add     al,ah                   ;Get back original height
endif;  EGAMONO OR HERCULES

        mov     ah,CenterScan
        sub     ah,al                   ;ah = first scan (more or less)



;       For the CGA, we want the first scan to be even.  For the Hercules,
;       we want the first scan to be in bank 0.  For the EGAMono, since
;       there is no interleaving, we don't care where the first scan starts.


        rept    Interleave
        shr     ah,1
        endm


;       We now have an index for the first scan line.  Compute it's
;       address, including any indentation.


        mov     al,BytesPerScan
        mul     ah

        if      Indent
        add     ax,Indent
        endif



;       The staring address of the first left-justified scan (always
;       the top most line of the image) has been computed, along with
;       the number of left-justified and right-justified scans.  Now
;       compute the width dependant data.
;
;       The width dependant data consists of the number of rotates
;       which will be required to align the images, and the starting
;       address of the first right-justified scan.
;
;       We will also assume that delta bytes will be less than
;       128 since the number of rotates will be 2 times this
;       value.


        xor     dx,dx
        or      dl,es:LDefWidth[di]     ;Get width of logo (in bytes)
        jz      Logo20                  ;Too narrow, don't allow logo
        cmp     dl,MaxWidth
        ja      Logo20                  ;Too wide, don't allow logo
        mov     LogoWidth[bp],dx        ;Save width of logo

        xor     cx,cx
        mov     cl,BytesPerScan-2*Indent;Compute delta bytes
        sub     cx,dx

        cmp     es:LDefMerge[di],dh     ;Merging logo?  (<> 0)
        jnz     Logo35                  ;  Yes, compute merge data

        shr     cx,1                    ;Indent logo by 1/2 delta
        add     ax,cx
        xor     cx,cx                   ;Show delta now is 0

Logo35:
        mov     LJScanAddr[bp],ax       ;Save left starting address
        mov     DeltaBytes[bp],cx       ;Save delta bytes
        add     cx,ax                   ;Compute right-justified start address
        add     cx,RJScan
        mov     RJScanAddr[bp],cx       ;Save starting address


;       Should now have enough info to actually draw the logo.


        push    es                      ;Set ds:si --> logo data
        pop     ds
        lea     si,LDefBytes[di]
        mov     di,ScreenSeg            ;Set es:di --> first dest addr
        mov     es,di
        mov     di,ax
        mov     bh,LJScanCount[bp]
        mov     ax,NextScan
        sub     ax,dx
        call    DrawLogo
        mov     di,RJScanAddr[bp]       ;--> first right justified scan
        mov     bh,RJScanCount[bp]
        call    DrawLogo



;       Now the two images must be merged.  The number of rotates
;       required is four times the delta bytes value (the rotate is
;       two bits at a time).  Since rotation occurs in pairs (both
;       directions), the number can be considered two times the delta.
;
;       Every time four rotate pairs have occurd, the starting address
;       for the scans must be updated since they just moved into a
;       new byte.  The Div4 counter is used to detect this.


        mov     si,DeltaBytes[bp]       ;Set # of rotate pairs needed
        shl     si,1
        jz      Logo60                  ;Incase delta is zero!
        mov     cx,LogoWidth[bp]        ;Set logo width
        add     RJScanAddr[bp],cx       ;--> end of first RJ scan
        sub     RJScanAddr[bp],2        ;  (stepping backwards by words)
        add     cx,3                    ;Round logo width to NEXT word boundary
        shr     cx,1

if      EGAMONO OR HERCULES or OLIVETTI
        mov     bx,RepLine
endif;  EGAMONO OR HERCULES or OLIVETTI

Logo40:
        mov     Div4[bp],4              ;Reset /4 counter

Logo50:
        mov     di,LJScanAddr[bp]       ;Rotate left-justifed scans right
        mov     dh,LJScanCount[bp]
        call    LogoRight
        mov     di,RJScanAddr[bp]       ;Rotate right-justified scans left
        mov     dh,RJScanCount[bp]
        call    LogoLeft
        dec     si                      ;More rotates to perform?
        jz      Logo60                  ;  No, all done
        dec     Div4[bp]                ;Update pointers
        jnz     Logo50                  ;  No
        inc     LJScanAddr[bp]          ;  Yes
        dec     RJScanAddr[bp]
        jmp     Logo40

Logo60:
        lds     si,PtrLogo[bp]          ;Get back pointer to text
        mov     di,NumStrs*2-2          ;Set string count

Logo70:
        push    di                      ;Save count
        mov     di,TextAddrs[di][bp]    ;Get address for text
        call    PrnMsg                  ;Processed the string
        pop     di                      ;Restore string counter
        dec     di                      ;Update string address index
        dec     di
        jns     Logo70                  ;More strings to output

LogoExit:
        pop     di
        pop     si
        add     sp,((SIZE FrameData)+1) AND 0FFFEh
        pop     bp
        lea     ax,BlowAway             ;Set discard address
        ret

Logo    endp

page

;       HowLongIsIt - Return the length of the given asciiz string
;
;       The given asciiz string's length is returned.
;
;       Entry:  es:di --> Start of string
;
;       Exit:   (cx) = length (a null string will return 0)
;               es:di --> End of string + 1 (past null)
;
;       Uses:   ax,cx,di,flags


HowLongIsIt     proc    near

        mov     cx,0FFFFH
        xor     ax,ax
        cld
        repnz   scasb
        not     cx                      ;Compute actual length
        dec     cx
        ret

HowLongIsIt     endp

;
;       Setup will append the logo data to the end of this file
;
LogoData:

if 0
;       The logo data consists of 2 product description lines,
;       six copyright lines, and the logo data.  This information
;       is appended to the .LGO file by SETUP.  A copy of the default
;       information is included for reference.  The actual data may be
;       different in content but not in format.
;

        db      "Microsoft Windows",0           ;Product description line 1
        db      "Version 1.02",0                ;Product description line 2

        db      0                               ;Copyright line 1
        db      0                               ;Copyright line 2
        db      0                               ;Copyright line 3
        db      0                               ;Copyright line 4
                                                ;Copyright line 5
        db      "Copyright (c) Microsoft Corporation, 1985, 1986."
        db          "  All Rights Reserved.",0
                                                ;Copyright line 6
        db      "Microsoft is a registered trademark of Microsoft Corp.",0

        db      1                               ;Merging logo
        db      36                              ;Height of logo (should be even)
        db      67                              ;Width  of logo
                                                ;Scan line data.  Even lines
                                                ; first, followed by odd lines.
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H ; line 0
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H
        db      03FH,0FFH,0C0H,000H,000H,000H,000H,03FH,0FFH,0C3H ; line 2
        db      0FFH,0C0H,000H,000H,000H,00FH,0FFH,0FCH,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0F0H,000H,000H,000H,000H
        db      000H,03FH,0FFH,0FFH,0FFH,0C0H,000H,000H,000H,000H
        db      00FH,0FFH,0F0H,000H,000H,000H,000H,000H,00FH,0FFH
        db      0FCH,000H,000H,000H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0F0H,000H,000H,000H,000H,0FFH,0FFH,0C3H ; line 4
        db      0FFH,0C0H,000H,000H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FCH,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,003H
        db      0FFH,0FFH,0FFH,0C0H,000H,000H,000H,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0C0H,000H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0FCH,000H,000H,000H,003H,0FFH,0FFH,0C3H ; line 6
        db      0FFH,0C0H,000H,03FH,0FFH,0FFH,0FFH,0FFH,0FFH,0FFH
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H,000H,00FH
        db      0FFH,0FFH,0C0H,000H,03FH,0FFH,0FFH,0C0H,000H,03FH
        db      0FFH,0FFH,0FFH,0FCH,000H,000H,03FH,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0FFH,000H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0FFH,000H,000H,000H,00FH,0FFH,0FFH,0C3H ; line 8
        db      0FFH,0C0H,003H,0FFH,0FCH,000H,000H,000H,00FH,0FFH
        db      0F0H,003H,0FFH,0C0H,000H,00FH,0FFH,0F0H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,0FFH
        db      0FFH,0FFH,0FFH,0FFH,000H,003H,0FFH,0FCH,000H,000H
        db      000H,00FH,0FFH,0F0H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FFH,0FFH,0C0H,000H,000H,03FH,0FFH,0FFH,0C3H ; line 10
        db      0FFH,0C0H,03FH,0FFH,000H,000H,000H,000H,000H,03FH
        db      0FFH,003H,0FFH,0C0H,000H,000H,0FFH,0FCH,003H,0FFH
        db      0C0H,000H,000H,000H,000H,000H,03FH,0FCH,000H,0FFH
        db      0FCH,000H,00FH,0FFH,000H,03FH,0FFH,000H,000H,000H
        db      000H,000H,03FH,0FFH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FFH,0FFH,0F0H,000H,000H,0FFH,0FFH,0FFH,0C3H ; line 12
        db      0FFH,0C0H,0FFH,0F0H,000H,000H,000H,000H,000H,003H
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,0FFH
        db      0FFH,000H,000H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,003H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FFH,0FFH,0FCH,000H,003H,0FFH,0FFH,0FFH,0C3H ; line 14
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,0FFH,0FCH,03FH,0FCH
        db      000H,000H,000H,000H,000H,000H,003H,0FFH,0C0H,03FH
        db      0FFH,0FCH,000H,000H,000H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,0FFH,0FFH,000H,00FH,0FFH,0F3H,0FFH,0C3H ; line 16
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0C0H,000H,00FH,0FFH,0F0H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,00FH
        db      0FFH,0FFH,0FFH,000H,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,03FH,0FFH,0C0H,03FH,0FFH,0C3H,0FFH,0C3H ; line 18
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H,0FFH,0FFH
        db      0FFH,0C0H,000H,000H,000H,03FH,0FFH,0FFH,0F0H,000H
        db      0FFH,0FFH,0FFH,0F0H,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,00FH,0FFH,0F0H,0FFH,0FFH,003H,0FFH,0C3H ; line 20
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FCH,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,0FFH,0FFH,0FFH,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,003H,0FFH,0FFH,0FFH,0FCH,003H,0FFH,0C3H ; line 22
        db      0FFH,0C0H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,000H,03FH,0FCH
        db      000H,000H,000H,000H,000H,000H,003H,0FFH,0C0H,000H
        db      000H,000H,03FH,0FFH,0C0H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,000H,0FFH,0FFH,0FFH,0F0H,003H,0FFH,0C3H ; line 24
        db      0FFH,0C0H,0FFH,0F0H,000H,000H,000H,000H,000H,003H
        db      0FFH,0C3H,0FFH,0C0H,000H,03FH,0FFH,0C0H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,003H,0FFH
        db      0C0H,000H,003H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,003H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,03FH,0C0H
        db      03FH,0FCH,000H,03FH,0FFH,0FFH,0C0H,003H,0FFH,0C3H ; line 26
        db      0FFH,0C0H,03FH,0FFH,000H,000H,000H,000H,000H,03FH
        db      0FFH,003H,0FFH,0C0H,000H,003H,0FFH,0F0H,003H,0FFH
        db      0C0H,000H,000H,000H,000H,000H,03FH,0FCH,003H,0FFH
        db      0F0H,000H,00FH,0FFH,0C0H,03FH,0FFH,000H,000H,000H
        db      000H,000H,03FH,0FFH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,003H,000H,00CH
        db      03FH,0FCH,000H,00FH,0FFH,0FFH,000H,003H,0FFH,0C3H ; line 28
        db      0FFH,0C0H,003H,0FFH,0FCH,000H,000H,000H,00FH,0FFH
        db      0F0H,003H,0FFH,0C0H,000H,000H,0FFH,0FCH,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,003H,0FFH
        db      0FFH,0FFH,0FFH,0FFH,0C0H,003H,0FFH,0FCH,000H,000H
        db      000H,00FH,0FFH,0F0H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,00CH,030H,0C3H
        db      03FH,0FCH,000H,003H,0FFH,0FCH,000H,003H,0FFH,0C3H ; line 30
        db      0FFH,0C0H,000H,03FH,0FFH,0FFH,0FFH,0FFH,0FFH,0FFH
        db      000H,003H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,00FH
        db      0FFH,0FFH,0C0H,000H,03FH,0FFH,0FFH,000H,000H,03FH
        db      0FFH,0FFH,0FFH,0FCH,000H,000H,03FH,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0FFH,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,00CH,033H,003H
        db      03FH,0FCH,000H,000H,0FFH,0F0H,000H,003H,0FFH,0C3H ; line 32
        db      0FFH,0C0H,000H,000H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H
        db      000H,003H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,003H
        db      0FFH,0FFH,0FFH,0C0H,000H,000H,000H,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0C0H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,003H,000H,00CH
        db      03FH,0FCH,000H,000H,03FH,0C0H,000H,003H,0FFH,0C3H ; line 34
        db      0FFH,0C0H,000H,000H,000H,00FH,0FFH,0FCH,000H,000H
        db      000H,003H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,000H
        db      000H,03FH,0FFH,0FFH,0FFH,0C0H,000H,000H,000H,000H
        db      00FH,0FFH,0F0H,000H,000H,000H,000H,000H,00FH,0FFH
        db      0FCH,000H,000H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,03FH,0C0H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H ; line 1
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,0FFH,0FFH,0F0H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H
        db      03FH,0FFH,0C0H,000H,000H,000H,000H,03FH,0FFH,0C3H ; line 3
        db      0FFH,0C0H,000H,000H,003H,0FFH,0FFH,0FFH,0F0H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,000H,000H,000H,000H
        db      00FH,0FFH,0FFH,0FFH,0FFH,0FFH,000H,000H,000H,000H
        db      0FFH,0FFH,0FFH,000H,000H,000H,000H,003H,0FFH,0FFH
        db      0FFH,0F0H,000H,000H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0F0H,000H,000H,000H,000H,0FFH,0FFH,0C3H ; line 5
        db      0FFH,0C0H,000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FCH,000H,000H,003H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FCH,000H,000H,00FH
        db      0FFH,0FFH,0FFH,0F0H,000H,000H,003H,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0F0H,000H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0FCH,000H,000H,000H,003H,0FFH,0FFH,0C3H ; line 7
        db      0FFH,0C0H,000H,0FFH,0FFH,0F0H,000H,003H,0FFH,0FFH
        db      0C0H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H,000H,03FH
        db      0FFH,0F0H,000H,000H,000H,0FFH,0FFH,0C0H,000H,03FH
        db      0FFH,0FFH,0FFH,0FFH,000H,000H,0FFH,0FFH,0F0H,000H
        db      003H,0FFH,0FFH,0C0H,000H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      03FH,0FFH,0FFH,000H,000H,000H,00FH,0FFH,0FFH,0C3H ; line 9
        db      0FFH,0C0H,00FH,0FFH,0F0H,000H,000H,000H,003H,0FFH
        db      0FCH,003H,0FFH,0C0H,000H,000H,0FFH,0F0H,003H,0FFH
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FCH,000H,0FFH
        db      0FFH,0C0H,0FFH,0FFH,0C0H,00FH,0FFH,0F0H,000H,000H
        db      000H,003H,0FFH,0FCH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FFH,0FFH,0C0H,000H,000H,03FH,0FFH,0FFH,0C3H ; line 11
        db      0FFH,0C0H,03FH,0FCH,000H,000H,000H,000H,000H,00FH
        db      0FFH,003H,0FFH,0C0H,000H,000H,03FH,0FCH,00FH,0FFH
        db      000H,000H,000H,000H,000H,000H,00FH,0FFH,000H,0FFH
        db      0FCH,000H,00FH,0FFH,000H,03FH,0FCH,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FFH,0FFH,0F0H,000H,000H,0FFH,0FFH,0FFH,0C3H ; line 13
        db      0FFH,0C0H,0FFH,0F0H,000H,000H,000H,000H,000H,003H
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,03FH,0FCH,03FH,0FFH
        db      0FFH,0FFH,000H,000H,00FH,0FFH,0FFH,0FFH,0C0H,03FH
        db      0FFH,0C0H,000H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,003H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,0FFH,0FCH,000H,003H,0FFH,0F3H,0FFH,0C3H ; line 15
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0C0H,000H,000H,0FFH,0F0H,03FH,0F0H
        db      000H,000H,000H,000H,000H,000H,000H,0FFH,0C0H,03FH
        db      0FFH,0FFH,0F0H,000H,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,03FH,0FFH,000H,00FH,0FFH,0C3H,0FFH,0C3H ; line 17
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0C0H,0FFH,0F0H
        db      000H,000H,000H,000H,000H,000H,000H,0FFH,0F0H,003H
        db      0FFH,0FFH,0FFH,0C0H,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,00FH,0FFH,0C0H,03FH,0FFH,003H,0FFH,0C3H ; line 19
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,000H,0FFH,0F0H
        db      000H,000H,000H,000H,000H,000H,000H,0FFH,0F0H,000H
        db      00FH,0FFH,0FFH,0FCH,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,003H,0FFH,0F0H,0FFH,0FCH,003H,0FFH,0C3H ; line 21
        db      0FFH,0C3H,0FFH,0C0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0FFH,0FFH,0FFH,0FCH,000H,03FH,0F0H
        db      000H,000H,000H,000H,000H,000H,000H,0FFH,0C0H,000H
        db      000H,003H,0FFH,0FFH,003H,0FFH,0C0H,000H,000H,000H
        db      000H,000H,000H,0FFH,0F0H,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,000H,0FFH,0FFH,0FFH,0F0H,003H,0FFH,0C3H ; line 23
        db      0FFH,0C0H,0FFH,0F0H,000H,000H,000H,000H,000H,000H
        db      000H,003H,0FFH,0C0H,000H,0FFH,0FFH,000H,03FH,0FFH
        db      0FFH,0FFH,000H,000H,00FH,0FFH,0FFH,0FFH,0C0H,000H
        db      000H,000H,00FH,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,003H,0FFH,0C0H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,000H,000H
        db      03FH,0FCH,000H,03FH,0FFH,0FFH,0C0H,003H,0FFH,0C3H ; line 25
        db      0FFH,0C0H,03FH,0FCH,000H,000H,000H,000H,000H,00FH
        db      0FFH,003H,0FFH,0C0H,000H,00FH,0FFH,0C0H,00FH,0FFH
        db      000H,000H,000H,000H,000H,000H,00FH,0FFH,003H,0FFH
        db      0F0H,000H,003H,0FFH,0C0H,03FH,0FCH,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,0C0H,030H
        db      03FH,0FCH,000H,00FH,0FFH,0FFH,000H,003H,0FFH,0C3H ; line 27
        db      0FFH,0C0H,00FH,0FFH,0F0H,000H,000H,000H,003H,0FFH
        db      0FCH,003H,0FFH,0C0H,000H,000H,0FFH,0F0H,003H,0FFH
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FCH,003H,0FFH
        db      0FCH,000H,03FH,0FFH,0C0H,00FH,0FFH,0F0H,000H,000H
        db      000H,003H,0FFH,0FCH,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,00CH,03FH,003H
        db      03FH,0FCH,000H,003H,0FFH,0FCH,000H,003H,0FFH,0C3H ; line 29
        db      0FFH,0C0H,000H,0FFH,0FFH,0F0H,000H,003H,0FFH,0FFH
        db      0C0H,003H,0FFH,0C0H,000H,000H,0FFH,0FCH,000H,03FH
        db      0FFH,0F0H,000H,000H,000H,0FFH,0FFH,0C0H,000H,0FFH
        db      0FFH,0FFH,0FFH,0FFH,000H,000H,0FFH,0FFH,0F0H,000H
        db      003H,0FFH,0FFH,0C0H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,00CH,03FH,003H
        db      03FH,0FCH,000H,000H,0FFH,0F0H,000H,003H,0FFH,0C3H ; line 31
        db      0FFH,0C0H,000H,003H,0FFH,0FFH,0FFH,0FFH,0FFH,0F0H
        db      000H,003H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,003H
        db      0FFH,0FFH,0FFH,0FFH,0FFH,0FFH,0FCH,000H,000H,00FH
        db      0FFH,0FFH,0FFH,0F0H,000H,000H,003H,0FFH,0FFH,0FFH
        db      0FFH,0FFH,0F0H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,00CH,030H,0C3H
        db      03FH,0FCH,000H,000H,03FH,0C0H,000H,003H,0FFH,0C3H ; line 33
        db      0FFH,0C0H,000H,000H,003H,0FFH,0FFH,0FFH,0F0H,000H
        db      000H,003H,0FFH,0C0H,000H,000H,03FH,0FCH,000H,000H
        db      00FH,0FFH,0FFH,0FFH,0FFH,0FFH,000H,000H,000H,000H
        db      03FH,0FFH,0FFH,000H,000H,000H,000H,003H,0FFH,0FFH
        db      0FFH,0F0H,000H,000H,000H,0FFH,0F0H,000H,000H,000H
        db      000H,000H,00FH,0FFH,000H,0C0H,030H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H ; line 35
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,0FFH,0FFH,0F0H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H,000H,000H,000H
        db      000H,000H,000H,000H,000H,000H,000H

endif

code    ends

end
