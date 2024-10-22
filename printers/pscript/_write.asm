;/**[f******************************************************************
; * _write.a - 
; *
; * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
; * Copyright (C) 1989 Microsoft Corporation.
; * Company confidential.
; *
; **f]*****************************************************************/

	title	_write - write from a file
	subttl	Copyright (C) 1985 Aldus Corporation.  All rights reserved.
	page	60,132

; abstract
;
; this module contains a c-language callable routine for invoking the dos
; function 0x40, write to a file.
;
;  $Revision:   1.2  $
;  $Date:   21 Oct 1988 15:49:36  $
;  $Author:   dar  $
;
;
	page

; system includes
.xlist
?PLM = 1				;yes, plm conventions
?WIN = 1				;yes, follow windows calling conventions
		include	cmacros.inc		;use to be:  \include\cmacros.inc
;		include	errdefs.i
SE_DSKFUL	equ	99
.list
		page

; _write - write to a file
;
; c-language declaration:
;
; 	int _write(handle, buffer, bytes, xferred)
;	    int handle;				/* file handle returned by _open */
;	    char far *buffer;		/* pointer to buffer */
;	    int bytes;				/* number of bytes to write */
;	    int far *xferred;		/* pointer to variable to receive
;								   count of bytes actually written. */
;
; returns:
;	zero if no error or error code as value of the function.
;
;	if the number of bytes written does not equal the number of bytes
;	which were attempted to be written, the doslib error ~"disk full,"
;	SE_DSKFUL, is returned.  the xferred variable will the number of bytes
;	which did get written.
;
;	if dos error codes 5 or 6 are returned, the xferred variable will be
;	set to zero.
;
; reference:
;	ibm pc dos version 2.00 manual, page d-37.

sBegin	CODE
assumes	CS, CODE

cProc	dos_write,<FAR,PUBLIC>,<ds>
	parmW	Handle
	parmD	pBuffer
	parmW	Bytes
	parmD	pXferred

cBegin
	xor		ax,ax			;clear ax
	lds		bx,pXferred		;get pointer to xferred variable
	mov		[bx],ax			; and pre-set xferred amount to zero
	mov		bx,Handle		;get file handle
	lds		dx,pBuffer		;get buffer pointer
	mov		cx,Bytes		;get number of bytes
	mov		ah,40h			;ask dos to write the data
	int		21h				;do it
	jb		$1				;return error if error
	lds		bx,pXferred		;get pointer to xferred variable
	mov		[bx],ax			;store number of bytes xferred
	cmp		ax,Bytes		;correct number of bytes written?
	je		$2				;yes
	mov		ax,SE_DSKFUL	;no, return "disk full" as error
	jmp short $1			; and return

$2:	xor		ax,ax			;return zero if no error
$1:	cld						;take no chances with dos
cEnd
sEnd
end
