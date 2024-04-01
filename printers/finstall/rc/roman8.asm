; ROMAN8.ASM
; Copyright (c) 1989-1990, Microsoft Corporation.
;
; translation table ANSI -> ROMAN8.
;
; history
;
; 06 nov 89	peterbe		Added copyright
; 31 oct 89	peterbe		Fixed def. of single quotes.
; 18 oct 89	peterbe		Equivalents for multiply and divide defined.
; 26 oct 89	peterbe		Made same changes as in TRANS.C for HPPCL.
;				chars 91,92,a9,ac,ae,b2,b3,b6,b7,b8,b9,be

DATA SEGMENT

	DB	07fh,	00h	; 080h
	DB	07fh,	00h	; 081h
	DB	07fh,	00h	; 082h
	DB	07fh,	00h	; 083h
	DB	07fh,	00h	; 084h
	DB	07fh,	00h	; 085h
	DB	07fh,	00h	; 086h
	DB	07fh,	00h	; 087h
	DB	07fh,	00h	; 088h
	DB	07fh,	00h	; 089h
	DB	07fh,	00h	; 08ah
	DB	07fh,	00h	; 08bh
	DB	07fh,	00h	; 08ch
	DB	07fh,	00h	; 08dh
	DB	07fh,	00h	; 08eh
	DB	07fh,	00h	; 08fh
	DB	07fh,	00h	; 090h
	DB	060h,	00h	; 091h open single quote
	DB	027h,	00h	; 092h close single quote
	DB	07fh,	00h	; 093h
	DB	07fh,	00h	; 094h
	DB	07fh,	00h	; 095h
	DB	07fh,	00h	; 096h
	DB	07fh,	00h	; 097h
	DB	07fh,	00h	; 098h
	DB	07fh,	00h	; 099h
	DB	07fh,	00h	; 09ah
	DB	07fh,	00h	; 09bh
	DB	07fh,	00h	; 09ch
	DB	07fh,	00h	; 09dh
	DB	07fh,	00h	; 09eh
	DB	07fh,	00h	; 09fh
	DB	0a0h,	00h	; 0a0h
	DB	0b8h,	00h	; 0a1h
	DB	0bfh,	00h	; 0a2h
	DB	0bbh,	00h	; 0a3h
	DB	0bah,	00h	; 0a4h
	DB	0bch,	00h	; 0a5h
	DB	'|',	00h	; 0a6h
	DB	0bdh,	00h	; 0a7h
	DB	0abh,	00h	; 0a8h
	DB	'C',	00h	; 0a9h copyright
	DB	0f9h,	00h	; 0aah
	DB	0fbh,	00h	; 0abh
	DB	'-',	00h	; 0ach logical not
	DB	'-',	00h	; 0adh special dash
	DB	'R',	00h	; 0aeh registered trademark
	DB	0b0h,	00h	; 0afh
	DB	0b3h,	00h	; 0b0h
	DB	0feh,	00h	; 0b1h
	DB	'2',	00h	; 0b2h '2' superscript
	DB	'3',	00h	; 0b3h '3' superscript
	DB	0a8h,	00h	; 0b4h
	DB	'u',	00h	; 0b5h
	DB	189,	00h	; 0b6h paragraph -> section sign
	DB	242,	00h	; 0b7h raised dot
	DB	',',	00h	; 0b8h cedilla (used for dead key only!)
	DB	'1',	00h	; 0b9h '1' superscript
	DB	0fah,	00h	; 0bah
	DB	0fdh,	00h	; 0bbh
	DB	0f7h,	00h	; 0bch
	DB	0f8h,	00h	; 0bdh
	DB	245,	00h	; 0beh 3/4
	DB	0b9h,	00h	; 0bfh
	DB	0a1h,	00h	; 0c0h
	DB	0e0h,	00h	; 0c1h
	DB	0a2h,	00h	; 0c2h
	DB	0e1h,	00h	; 0c3h
	DB	0d8h,	00h	; 0c4h
	DB	0d0h,	00h	; 0c5h
	DB	0d3h,	00h	; 0c6h
	DB	0b4h,	00h	; 0c7h
	DB	0a3h,	00h	; 0c8h
	DB	0dch,	00h	; 0c9h
	DB	0a4h,	00h	; 0cah
	DB	0a5h,	00h	; 0cbh
	DB	0e6h,	00h	; 0cch
	DB	0e5h,	00h	; 0cdh
	DB	0a6h,	00h	; 0ceh
	DB	0a7h,	00h	; 0cfh
	DB	0e3h,	00h	; 0d0h
	DB	0b6h,	00h	; 0d1h
	DB	0e8h,	00h	; 0d2h
	DB	0e7h,	00h	; 0d3h
	DB	0dfh,	00h	; 0d4h
	DB	0e9h,	00h	; 0d5h
	DB	0dah,	00h	; 0d6h
	DB	'x',	00h	; 0d7h multiply
	DB	0d2h,	00h	; 0d8h
	DB	0adh,	00h	; 0d9h
	DB	0edh,	00h	; 0dah
	DB	0aeh,	00h	; 0dbh
	DB	0dbh,	00h	; 0dch
	DB	'Y',	0a8h	; 0ddh
	DB	0f0h,	00h	; 0deh
	DB	0deh,	00h	; 0dfh
	DB	0c8h,	00h	; 0e0h
	DB	0c4h,	00h	; 0e1h
	DB	0c0h,	00h	; 0e2h
	DB	0e2h,	00h	; 0e3h
	DB	0cch,	00h	; 0e4h
	DB	0d4h,	00h	; 0e5h
	DB	0d7h,	00h	; 0e6h
	DB	0b5h,	00h	; 0e7h
	DB	0c9h,	00h	; 0e8h
	DB	0c5h,	00h	; 0e9h
	DB	0c1h,	00h	; 0eah
	DB	0cdh,	00h	; 0ebh
	DB	0d9h,	00h	; 0ech
	DB	0d5h,	00h	; 0edh
	DB	0d1h,	00h	; 0eeh
	DB	0ddh,	00h	; 0efh
	DB	0e4h,	00h	; 0f0h
	DB	0b7h,	00h	; 0f1h
	DB	0cah,	00h	; 0f2h
	DB	0c6h,	00h	; 0f3h
	DB	0c2h,	00h	; 0f4h
	DB	0eah,	00h	; 0f5h
	DB	0ceh,	00h	; 0f6h
	DB	'-',	':'	; 0f7h divide
	DB	0d6h,	00h	; 0f8h
	DB	0cbh,	00h	; 0f9h
	DB	0c7h,	00h	; 0fah
	DB	0c3h,	00h	; 0fbh
	DB	0cfh,	00h	; 0fch
	DB	'y',	0a8h	; 0fdh
	DB	0f1h,	00h	; 0feh
	DB	0efh,	00h	; 0ffh
DATA ENDS
    END
