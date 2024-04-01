	title	Face Name Table
	page	,132
;/**[f******************************************************************
; * facetbl.asm -
; *
; * Copyright (C) 1989 Microsoft Corporation.  All rights reserved.
; * Company confidential.
; *
; **f]*****************************************************************/
;
;    2-24-89	jimmat	Converted from a resource to a table in the
;			_REALIZE code segement to speed up the
;			aliasFace() routine.  Also, this cuts out a lot
;			of code that used to load/lock/unlock the table.
;
;    2-27-89	chrisg	transmutated for the PostScript driver.
;			changed the segment to _ENUM (where AliasFace() is)

		.xlist
		include cmacros.inc

incLogical	equ	1
		include gdidefs.inc
		.list


		public	_NumFaces, _DefaultFace, _FaceTable

_ENUM	segment word public 'code'

_NumFaces	db	9			;9 entries in the table

_DefaultFace	db	"Courier",0		;Default Face name

_FaceTable	db	FF_ROMAN,1,"Tms Rmn",0
		db	FF_ROMAN,1,"Times Roman",0
		db	FF_ROMAN,1,"Times",0
		db	FF_ROMAN,1,"TmsRmn",0
		db	FF_ROMAN,1,"Varitimes",0
		db	FF_ROMAN,1,"Dutch",0

		db	FF_SWISS,2,"Helv",0
		db	FF_SWISS,2,"Helvetica",0
		db	FF_SWISS,2,"Swiss",0

_ENUM	ends

		end
