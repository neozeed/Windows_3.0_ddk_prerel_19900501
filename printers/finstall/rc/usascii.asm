; USASCII.ASM
; Copyright (c) 1989-1990, Microsoft Corporation
;
; translation table ANSI -> USASCII (composed characters).
;
; history
;
; 06 nov 89	peterbe		Added copyright
; 18 oct 89	peterbe		Equivalents for multiply and divide defined.

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
	DB	'`',	00h	; 091h
	DB	027h,	00h	; 092h
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
	DB	07fh,	00h	; 0a1h
	DB	'c',	'|'	; 0a2h
	DB	07fh,	00h	; 0a3h
	DB	07fh,	00h	; 0a4h
	DB	'=',	'Y'	; 0a5h
	DB	'|',	00h	; 0a6h
	DB	07fh,	00h	; 0a7h
	DB	'"',	00h	; 0a8h
	DB	07fh,	00h	; 0a9h
	DB	'_',	'a'	; 0aah
	DB	07fh,	00h	; 0abh
	DB	07fh,	00h	; 0ach
	DB	'-',	00h	; 0adh
	DB	07fh,	00h	; 0aeh
	DB	07fh,	00h	; 0afh
	DB	07fh,	00h	; 0b0h
	DB	'_',	'+'	; 0b1h
	DB	07fh,	00h	; 0b2h
	DB	07fh,	00h	; 0b3h
	DB	07fh,	00h	; 0b4h
	DB	'u',	00h	; 0b5h
	DB	07fh,	00h	; 0b6h
	DB	'*',	00h	; 0b7h
	DB	07fh,	00h	; 0b8h
	DB	07fh,	00h	; 0b9h
	DB	'_',	'o'	; 0bah
	DB	07fh,	00h	; 0bbh
	DB	07fh,	00h	; 0bch
	DB	07fh,	00h	; 0bdh
	DB	07fh,	00h	; 0beh
	DB	07fh,	00h	; 0bfh
	DB	'A',	00h	; 0c0h
	DB	'A',	00h	; 0c1h
	DB	'A',	00h	; 0c2h
	DB	'A',	00h	; 0c3h
	DB	'A',	00h	; 0c4h
	DB	'A',	00h	; 0c5h
	DB	'A',	00h	; 0c6h
	DB	'C',	','	; 0c7h
	DB	'E',	00h	; 0c8h
	DB	'E',	00h	; 0c9h
	DB	'E',	00h	; 0cah
	DB	'E',	00h	; 0cbh
	DB	'I',	00h	; 0cch
	DB	'I',	00h	; 0cdh
	DB	'I',	00h	; 0ceh
	DB	'I',	00h	; 0cfh
	DB	'D',	'-'	; 0d0h
	DB	'N',	00h	; 0d1h
	DB	'O',	00h	; 0d2h
	DB	'O',	00h	; 0d3h
	DB	'O',	00h	; 0d4h
	DB	'O',	00h	; 0d5h
	DB	'O',	00h	; 0d6h
	DB	'x',	00h	; 0d7h multiply
	DB	'O',	'/'	; 0d8h
	DB	'U',	00h	; 0d9h
	DB	'U',	00h	; 0dah
	DB	'U',	00h	; 0dbh
	DB	'U',	00h	; 0dch
	DB	'Y',	00h	; 0ddh
	DB	'p',	'b'	; 0deh
	DB	07fh,	00h	; 0dfh
	DB	'a',	'`'	; 0e0h
	DB	'a',	027h	; 0e1h
	DB	'a',	'^'	; 0e2h
	DB	'a',	00h	; 0e3h
	DB	'a',	'"'	; 0e4h
	DB	'a',	00h	; 0e5h
	DB	'a',	00h	; 0e6h
	DB	'c',	','	; 0e7h
	DB	'e',	'`'	; 0e8h
	DB	'e',	027h	; 0e9h
	DB	'e',	'^'	; 0eah
	DB	'e',	'"'	; 0ebh
	DB	'`',	'i'	; 0ech
	DB	027h,	'i'	; 0edh
	DB	'^',	'i'	; 0eeh
	DB	'"',	'i'	; 0efh
	DB	'd',	'-'	; 0f0h
	DB	'n',	00h	; 0f1h
	DB	'o',	'`'	; 0f2h
	DB	'o',	027h	; 0f3h
	DB	'o',	'^'	; 0f4h
	DB	'o',	00h	; 0f5h
	DB	'o',	'"'	; 0f6h
	DB	'-',	':'	; 0f7h divide
	DB	'o',	'/'	; 0f8h
	DB	'u',	'`'	; 0f9h
	DB	'u',	027h	; 0fah
	DB	'u',	'^'	; 0fbh
	DB	'u',	'"'	; 0fch
	DB	'y',	027h	; 0fdh
	DB	'p',	'b'	; 0feh
	DB	'y',	'"'	; 0ffh
DATA ENDS
    END