; GENERIC7.asm
; Copyright (c) 1989-1990, Microsoft Corporation.

; History
; 06 nov 89	peterbe		Added copyright

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
	DB	07fh,	00h	; 091h
	DB	07fh,	00h	; 092h
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
	DB	07fh,	00h	; 0a0h
	DB	07fh,	00h	; 0a1h
	DB	07fh,	00h	; 0a2h
	DB	07fh,	00h	; 0a3h
	DB	07fh,	00h	; 0a4h
	DB	07fh,	00h	; 0a5h
	DB	07fh,	00h	; 0a6h
	DB	07fh,	00h	; 0a7h
	DB	07fh,	00h	; 0a8h
	DB	07fh,	00h	; 0a9h
	DB	07fh,	00h	; 0aah
	DB	07fh,	00h	; 0abh
	DB	07fh,	00h	; 0ach
	DB	07fh,	00h	; 0adh
	DB	07fh,	00h	; 0aeh
	DB	07fh,	00h	; 0afh
	DB	07fh,	00h	; 0b0h
	DB	07fh,	00h	; 0b1h
	DB	07fh,	00h	; 0b2h
	DB	07fh,	00h	; 0b3h
	DB	07fh,	00h	; 0b4h
	DB	07fh,	00h	; 0b5h
	DB	07fh,	00h	; 0b6h
	DB	07fh,	00h	; 0b7h
	DB	07fh,	00h	; 0b8h
	DB	07fh,	00h	; 0b9h
	DB	07fh,	00h	; 0bah
	DB	07fh,	00h	; 0bbh
	DB	07fh,	00h	; 0bch
	DB	07fh,	00h	; 0bdh
	DB	07fh,	00h	; 0beh
	DB	07fh,	00h	; 0bfh
	DB	07fh,	00h	; 0c0h
	DB	07fh,	00h	; 0c1h
	DB	07fh,	00h	; 0c2h
	DB	07fh,	00h	; 0c3h
	DB	07fh,	00h	; 0c4h
	DB	07fh,	00h	; 0c5h
	DB	07fh,	00h	; 0c6h
	DB	07fh,	00h	; 0c7h
	DB	07fh,	00h	; 0c8h
	DB	07fh,	00h	; 0c9h
	DB	07fh,	00h	; 0cah
	DB	07fh,	00h	; 0cbh
	DB	07fh,	00h	; 0cch
	DB	07fh,	00h	; 0cdh
	DB	07fh,	00h	; 0ceh
	DB	07fh,	00h	; 0cfh
	DB	07fh,	00h	; 0d0h
	DB	07fh,	00h	; 0d1h
	DB	07fh,	00h	; 0d2h
	DB	07fh,	00h	; 0d3h
	DB	07fh,	00h	; 0d4h
	DB	07fh,	00h	; 0d5h
	DB	07fh,	00h	; 0d6h
	DB	07fh,	00h	; 0d7h
	DB	07fh,	00h	; 0d8h
	DB	07fh,	00h	; 0d9h
	DB	07fh,	00h	; 0dah
	DB	07fh,	00h	; 0dbh
	DB	07fh,	00h	; 0dch
	DB	07fh,	00h	; 0ddh
	DB	07fh,	00h	; 0deh
	DB	07fh,	00h	; 0dfh
	DB	07fh,	00h	; 0e0h
	DB	07fh,	00h	; 0e1h
	DB	07fh,	00h	; 0e2h
	DB	07fh,	00h	; 0e3h
	DB	07fh,	00h	; 0e4h
	DB	07fh,	00h	; 0e5h
	DB	07fh,	00h	; 0e6h
	DB	07fh,	00h	; 0e7h
	DB	07fh,	00h	; 0e8h
	DB	07fh,	00h	; 0e9h
	DB	07fh,	00h	; 0eah
	DB	07fh,	00h	; 0ebh
	DB	07fh,	00h	; 0ech
	DB	07fh,	00h	; 0edh
	DB	07fh,	00h	; 0eeh
	DB	07fh,	00h	; 0efh
	DB	07fh,	00h	; 0f0h
	DB	07fh,	00h	; 0f1h
	DB	07fh,	00h	; 0f2h
	DB	07fh,	00h	; 0f3h
	DB	07fh,	00h	; 0f4h
	DB	07fh,	00h	; 0f5h
	DB	07fh,	00h	; 0f6h
	DB	07fh,	00h	; 0f7h
	DB	07fh,	00h	; 0f8h
	DB	07fh,	00h	; 0f9h
	DB	07fh,	00h	; 0fah
	DB	07fh,	00h	; 0fbh
	DB	07fh,	00h	; 0fch
	DB	07fh,	00h	; 0fdh
	DB	07fh,	00h	; 0feh
	DB	07fh,	00h	; 0ffh
DATA ENDS
    END
