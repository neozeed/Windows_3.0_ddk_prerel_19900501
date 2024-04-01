/**[f******************************************************************
 * trans.h -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

 // History
 // 26 oct 89	peterbe	Change Roman8 trans for a9,ac,ae,b2,b3,b6,b7,b8,b9,be,
 //			all punctuation or special symbols.
 // 18 oct 89	peterbe	Multiply is 'x' and divide is '-' plus ':' in
 //			USASCII, Roman8.
 // 01 sep 89	peterbe	Added copyright.
 // 25 aug 89	craigc	Added MATH8 symbol set translation.

/* translation table using standard characters */
unsigned char USASCII_Trans[] = {
		    HP_DF_CH, NULL,	/*  80  */
		    HP_DF_CH, NULL,	/*  81  */
		    HP_DF_CH, NULL,	/*  82  */
		    HP_DF_CH, NULL,	/*  83  */
		    HP_DF_CH, NULL,	/*  84  */
		    HP_DF_CH, NULL,	/*  85  */
		    HP_DF_CH, NULL,	/*  86  */
		    HP_DF_CH, NULL,	/*  87  */
		    HP_DF_CH, NULL,	/*  88  */
		    HP_DF_CH, NULL,	/*  89  */
		    HP_DF_CH, NULL,	/*  8a  */
		    HP_DF_CH, NULL,	/*  8b  */
		    HP_DF_CH, NULL,	/*  8c  */
		    HP_DF_CH, NULL,	/*  8d  */
		    HP_DF_CH, NULL,	/*  8e  */
		    HP_DF_CH, NULL,	/*  8f  */
		    HP_DF_CH, NULL,	/*  90  */
		    0x60, NULL,		/*  91  */
		    0x27, NULL,		/*  92  */
		    HP_DF_CH, NULL,	/*  93  */
		    HP_DF_CH, NULL,	/*  94  */
		    HP_DF_CH, NULL,	/*  95  */
		    HP_DF_CH, NULL,	/*  96  */
		    HP_DF_CH, NULL,	/*  97  */
		    HP_DF_CH, NULL,	/*  98  */
		    HP_DF_CH, NULL,	/*  99  */
		    HP_DF_CH, NULL,	/*  9a  */
		    HP_DF_CH, NULL,	/*  9b  */
		    HP_DF_CH, NULL,	/*  9c  */
		    HP_DF_CH, NULL,	/*  9d  */
		    HP_DF_CH, NULL,	/*  9e  */
		    HP_DF_CH, NULL,	/*  9f  */
		    0xa0, NULL,		/*  a0  */
		    HP_DF_CH, NULL,	/*  a1  */
		    'c' , '|' ,		/*  a2  */
		    HP_DF_CH, NULL,	/*  a3  */
		    HP_DF_CH, NULL,	/*  a4  */
		    '=' , 'Y' ,		/*  a5  */
		    '|' , NULL,		/*  a6  */
		    HP_DF_CH, NULL,	/*  a7  */
		    '"' , NULL,		/*  a8  */
		    HP_DF_CH, NULL,	/*  a9  */
		    '_' , 'a' ,		/*  aa  */
		    HP_DF_CH, NULL,	/*  ab  */
		    HP_DF_CH, NULL,	/*  ac  */
		    '-' , NULL,		/*  ad  */
		    HP_DF_CH, NULL,	/*  ae  */
		    HP_DF_CH, NULL,	/*  af  */
		    HP_DF_CH, NULL,	/*  b0  */
		    '_' , '+' ,		/*  b1  */
		    HP_DF_CH, NULL,	/*  b2  */
		    HP_DF_CH, NULL,	/*  b3  */
		    HP_DF_CH, NULL,	/*  b4  */
		    'u' , NULL,		/*  b5  */
		    HP_DF_CH, NULL,	/*  b6  */
		    '*' , NULL,		/*  b7  */
		    HP_DF_CH, NULL,	/*  b8  */
		    HP_DF_CH, NULL,	/*  b9  */
		    '_' , 'o' ,		/*  ba  */
		    HP_DF_CH, NULL,	/*  bb  */
		    HP_DF_CH, NULL,	/*  bc  */
		    HP_DF_CH, NULL,	/*  bd  */
		    HP_DF_CH, NULL,	/*  be  */
		    HP_DF_CH, NULL,	/*  bf  */
		    'A' , NULL,		/*  c0  */
		    'A' , NULL,		/*  c1  */
		    'A' , NULL,		/*  c2  */
		    'A' , NULL,		/*  c3  */
		    'A' , NULL,		/*  c4  */
		    'A' , NULL,		/*  c5  */
		    'A' , NULL,		/*  c6  */
		    'C' , ',' ,		/*  c7  */
		    'E' , NULL,		/*  c8  */
		    'E' , NULL,		/*  c9  */
		    'E' , NULL,		/*  ca  */
		    'E' , NULL,		/*  cb  */
		    'I' , NULL,		/*  cc  */
		    'I' , NULL,		/*  cd  */
		    'I' , NULL,		/*  ce  */
		    'I' , NULL,		/*  cf  */
		    'D' , '-' ,		/*  d0  */
		    'N' , NULL,		/*  d1  */
		    'O' , NULL,		/*  d2  */
		    'O' , NULL,		/*  d3  */
		    'O' , NULL,		/*  d4  */
		    'O' , NULL,		/*  d5  */
		    'O' , NULL,		/*  d6  */
		    'x', NULL,		/*  d7  multiply */
		    'O' , '/' ,		/*  d8  */
		    'U' , NULL,		/*  d9  */
		    'U' , NULL,		/*  da  */
		    'U' , NULL,		/*  db  */
		    'U' , NULL,		/*  dc  */
		    'Y' , NULL,		/*  dd  */
		    'p' , 'b' ,		/*  de  */
		    HP_DF_CH, NULL,	/*  df  */
		    'a' , '`' ,		/*  e0  */
		    'a' , '\'',         /*  e1  */
                    'a' , '^' ,         /*  e2  */
                    'a' , NULL,         /*  e3 - cannot overstrike ~ */
                    'a' , '"' ,         /*  e4  */
                    'a' , NULL,         /*  e5  */
                    'a' , NULL,         /*  e6  */
                    'c' , ',' ,         /*  e7  */
                    'e' , '`' ,         /*  e8  */
                    'e' , '\'',         /*  e9  */
                    'e' , '^' ,         /*  ea  */
                    'e' , '"' ,         /*  eb  */
                    '`' , 'i' ,         /*  ec  */
                    '\'', 'i' ,		/*  ed  */
		    '^' , 'i' ,		/*  ee  */
		    '"' , 'i' ,		/*  ef  */
		    'd' , '-' ,		/*  f0  */
		    'n' , NULL,		/*  f1  */
		    'o' , '`' ,		/*  f2  */
		    'o' , '\'',         /*  f3  */
                    'o' , '^' ,         /*  f4  */
                    'o' , NULL,         /*  f5  */
                    'o' , '"' ,         /*  f6  */
                    '-', ':',     	/*  f7  divide */
                    'o' , '/' ,         /*  f8  */
                    'u' , '`' ,         /*  f9  */
                    'u' , '\'',         /*  fa  */
                    'u' , '^' ,         /*  fb  */
                    'u' , '"' ,         /*  fc  */
                    'y' , '\'',		/*  fd  */
		    'p' , 'b' ,		/*  fe  */
		    'y' , '"'	};	 /*  ff  */

/* translation table from Extended Roman character set */

unsigned char Roman8_Trans[] = {
		    HP_DF_CH, NULL,	/*  80  */
		    HP_DF_CH, NULL,	/*  81  */
		    HP_DF_CH, NULL,	/*  82  */
		    HP_DF_CH, NULL,	/*  83  */
		    HP_DF_CH, NULL,	/*  84  */
		    HP_DF_CH, NULL,	/*  85  */
		    HP_DF_CH, NULL,	/*  86  */
		    HP_DF_CH, NULL,	/*  87  */
		    HP_DF_CH, NULL,	/*  88  */
		    HP_DF_CH, NULL,	/*  89  */
		    HP_DF_CH, NULL,	/*  8a  */
		    HP_DF_CH, NULL,	/*  8b  */
		    HP_DF_CH, NULL,	/*  8c  */
		    HP_DF_CH, NULL,	/*  8d  */
		    HP_DF_CH, NULL,	/*  8e  */
		    HP_DF_CH, NULL,	/*  8f  */
		    HP_DF_CH, NULL,	/*  90  */
		    0x60, NULL,		/*  91  open single quote */
		    0x27, NULL,		/*  92  close single quote */
		    HP_DF_CH, NULL,	/*  93  */
		    HP_DF_CH, NULL,	/*  94  */
		    HP_DF_CH, NULL,	/*  95  */
		    HP_DF_CH, NULL,	/*  96  */
		    HP_DF_CH, NULL,	/*  97  */
		    HP_DF_CH, NULL,	/*  98  */
		    HP_DF_CH, NULL,	/*  99  */
		    HP_DF_CH, NULL,	/*  9a  */
		    HP_DF_CH, NULL,	/*  9b  */
		    HP_DF_CH, NULL,	/*  9c  */
		    HP_DF_CH, NULL,	/*  9d  */
		    HP_DF_CH, NULL,	/*  9e  */
		    HP_DF_CH, NULL,	/*  9f  */
		    0xa0, NULL,		/*  a0  */
		    0xb8, NULL,		/*  a1  */
		    0xbf, NULL,		/*  a2  */
		    0xbb, NULL,		/*  a3  */
		    0xba, NULL,		/*  a4  */
		    0xbc, NULL,		/*  a5  */
		    '|' , NULL,		/*  a6  */
		    0xbd, NULL,		/*  a7  */
		    0xab, NULL,		/*  a8  */
		    'C' , NULL,		/*  a9  Copyright */
		    0xf9, NULL,		/*  aa  */
		    0xfb, NULL,		/*  ab  */
		    '-', NULL,		/*  ac  logical not */
		    '-' , NULL,		/*  ad  special dash */
		    'R', NULL,		/*  ae  registered trademark */
		    0xb0, NULL,		/*  af  */
		    0xb3, NULL,		/*  b0  */
		    0xfe, NULL,		/*  b1  */
		    '2', NULL,		/*  b2  '2' superscript */
		    '3', NULL,		/*  b3  '3' superscript */
		    0xa8, NULL,		/*  b4  */
		    'u' , NULL,		/*  b5  */
		    189, NULL,		/*  b6  paragraph -> section sign */
		    242, NULL,		/*  b7  raised dot (was FC block) */
		    ',', NULL,		/*  b8  cedilla (deadkey) -> comma */
		    '1', NULL,		/*  b9  1-super */
		    0xfa, NULL,		/*  ba  */
		    0xfd, NULL,		/*  bb  */
		    0xf7, NULL,		/*  bc  */
		    0xf8, NULL,		/*  bd  */
		    245, NULL,		/*  be  3/4 */
		    0xb9, NULL,		/*  bf  */
		    0xa1, NULL,		/*  c0  */
		    0xe0, NULL,		/*  c1  */
		    0xa2, NULL,		/*  c2  */
		    0xe1, NULL,		/*  c3  */
		    0xd8, NULL,		/*  c4  */
		    0xd0, NULL,		/*  c5  */
		    0xd3, NULL,		/*  c6  */
		    0xb4, NULL,		/*  c7  */
		    0xa3, NULL,		/*  c8  */
		    0xdc, NULL,		/*  c9  */
		    0xa4, NULL,		/*  ca  */
		    0xa5, NULL,		/*  cb  */
		    0xe6, NULL,		/*  cc  */
		    0xe5, NULL,		/*  cd  */
		    0xa6, NULL,		/*  ce  */
		    0xa7, NULL,		/*  cf  */
		    0xe3, NULL,		/*  d0  */
		    0xb6, NULL,		/*  d1  */
		    0xe8, NULL,		/*  d2  */
		    0xe7, NULL,		/*  d3  */
		    0xdf, NULL,		/*  d4  */
		    0xe9, NULL,		/*  d5  */
		    0xda, NULL,		/*  d6  */
		    'x', NULL,		/*  d7  multiply sign = 'x' */
		    0xd2, NULL,		/*  d8  */
		    0xad, NULL,		/*  d9  */
		    0xed, NULL,		/*  da  */
		    0xae, NULL,		/*  db  */
		    0xdb, NULL,		/*  dc  */
		    'Y' , 0xa8,		/*  dd  */
		    0xf0, NULL,		/*  de  */
		    0xde, NULL,		/*  df  */
		    0xc8, NULL,		/*  e0  */
		    0xc4, NULL,		/*  e1  */
		    0xc0, NULL,		/*  e2  */
		    0xe2, NULL,		/*  e3  */
		    0xcc, NULL,		/*  e4  */
		    0xd4, NULL,		/*  e5  */
		    0xd7, NULL,		/*  e6  */
		    0xb5, NULL,		/*  e7  */
		    0xc9, NULL,		/*  e8  */
		    0xc5, NULL,		/*  e9  */
		    0xc1, NULL,		/*  ea  */
		    0xcd, NULL,		/*  eb  */
		    0xd9, NULL,		/*  ec  */
		    0xd5, NULL,		/*  ed  */
		    0xd1, NULL,		/*  ee  */
		    0xdd, NULL,		/*  ef  */
		    0xe4, NULL,		/*  f0  */
		    0xb7, NULL,		/*  f1  */
		    0xca, NULL,		/*  f2  */
		    0xc6, NULL,		/*  f3  */
		    0xc2, NULL,		/*  f4  */
		    0xea, NULL,		/*  f5  */
		    0xce, NULL,		/*  f6  */
		    '-', ':',		/*  f7  divide sign */
		    0xd6, NULL,		/*  f8  */
		    0xcb, NULL,		/*  f9  */
		    0xc7, NULL,		/*  fa  */
		    0xc3, NULL,		/*  fb  */
		    0xcf, NULL,		/*  fc  */
		    'y',  0xa8,		/*  fd  */
		    0xf1, NULL,		/*  fe  */
		    0xef, NULL	};	 /*  ff  */

/* generic 7-bit translation table */
unsigned char GENERIC7_Trans[] = {
		    HP_DF_CH, NULL,	/*  80  */
		    HP_DF_CH, NULL,	/*  81  */
		    HP_DF_CH, NULL,	/*  82  */
		    HP_DF_CH, NULL,	/*  83  */
		    HP_DF_CH, NULL,	/*  84  */
		    HP_DF_CH, NULL,	/*  85  */
		    HP_DF_CH, NULL,	/*  86  */
		    HP_DF_CH, NULL,	/*  87  */
		    HP_DF_CH, NULL,	/*  88  */
		    HP_DF_CH, NULL,	/*  89  */
		    HP_DF_CH, NULL,	/*  8a  */
		    HP_DF_CH, NULL,	/*  8b  */
		    HP_DF_CH, NULL,	/*  8c  */
		    HP_DF_CH, NULL,	/*  8d  */
		    HP_DF_CH, NULL,	/*  8e  */
		    HP_DF_CH, NULL,	/*  8f  */
		    HP_DF_CH, NULL,	/*  90  */
		    HP_DF_CH, NULL,	/*  91  */
		    HP_DF_CH, NULL,	/*  92  */
		    HP_DF_CH, NULL,	/*  93  */
		    HP_DF_CH, NULL,	/*  94  */
		    HP_DF_CH, NULL,	/*  95  */
		    HP_DF_CH, NULL,	/*  96  */
		    HP_DF_CH, NULL,	/*  97  */
		    HP_DF_CH, NULL,	/*  98  */
		    HP_DF_CH, NULL,	/*  99  */
		    HP_DF_CH, NULL,	/*  9a  */
		    HP_DF_CH, NULL,	/*  9b  */
		    HP_DF_CH, NULL,	/*  9c  */
		    HP_DF_CH, NULL,	/*  9d  */
		    HP_DF_CH, NULL,	/*  9e  */
		    HP_DF_CH, NULL,	/*  9f  */
		    HP_DF_CH, NULL,	/*  a0  */
		    HP_DF_CH, NULL,	/*  a1  */
		    HP_DF_CH, NULL,	/*  a2  */
		    HP_DF_CH, NULL,	/*  a3  */
		    HP_DF_CH, NULL,	/*  a4  */
		    HP_DF_CH, NULL,	/*  a5  */
		    HP_DF_CH, NULL,	/*  a6  */
		    HP_DF_CH, NULL,	/*  a7  */
		    HP_DF_CH, NULL,	/*  a8  */
		    HP_DF_CH, NULL,	/*  a9  */
		    HP_DF_CH, NULL,	/*  aa  */
		    HP_DF_CH, NULL,	/*  ab  */
		    HP_DF_CH, NULL,	/*  ac  */
		    HP_DF_CH, NULL,	/*  ad  */
		    HP_DF_CH, NULL,	/*  ae  */
		    HP_DF_CH, NULL,	/*  af  */
		    HP_DF_CH, NULL,	/*  b0  */
		    HP_DF_CH, NULL,	/*  b1  */
		    HP_DF_CH, NULL,	/*  b2  */
		    HP_DF_CH, NULL,	/*  b3  */
		    HP_DF_CH, NULL,	/*  b4  */
		    HP_DF_CH, NULL,	/*  b5  */
		    HP_DF_CH, NULL,	/*  b6  */
		    HP_DF_CH, NULL,	/*  b7  */
		    HP_DF_CH, NULL,	/*  b8  */
		    HP_DF_CH, NULL,	/*  b9  */
		    HP_DF_CH, NULL,	/*  ba  */
		    HP_DF_CH, NULL,	/*  bb  */
		    HP_DF_CH, NULL,	/*  bc  */
		    HP_DF_CH, NULL,	/*  bd  */
		    HP_DF_CH, NULL,	/*  be  */
		    HP_DF_CH, NULL,	/*  bf  */
		    HP_DF_CH, NULL,	/*  c0  */
		    HP_DF_CH, NULL,	/*  c1  */
		    HP_DF_CH, NULL,	/*  c2  */
		    HP_DF_CH, NULL,	/*  c3  */
		    HP_DF_CH, NULL,	/*  c4  */
		    HP_DF_CH, NULL,	/*  c5  */
		    HP_DF_CH, NULL,	/*  c6  */
		    HP_DF_CH, NULL,	/*  c7  */
		    HP_DF_CH, NULL,	/*  c8  */
		    HP_DF_CH, NULL,	/*  c9  */
		    HP_DF_CH, NULL,	/*  ca  */
		    HP_DF_CH, NULL,	/*  cb  */
		    HP_DF_CH, NULL,	/*  cc  */
		    HP_DF_CH, NULL,	/*  cd  */
		    HP_DF_CH, NULL,	/*  ce  */
		    HP_DF_CH, NULL,	/*  cf  */
		    HP_DF_CH, NULL,	/*  d0  */
		    HP_DF_CH, NULL,	/*  d1  */
		    HP_DF_CH, NULL,	/*  d2  */
		    HP_DF_CH, NULL,	/*  d3  */
		    HP_DF_CH, NULL,	/*  d4  */
		    HP_DF_CH, NULL,	/*  d5  */
		    HP_DF_CH, NULL,	/*  d6  */
		    HP_DF_CH, NULL,	/*  d7  */
		    HP_DF_CH, NULL,	/*  d8  */
		    HP_DF_CH, NULL,	/*  d9  */
		    HP_DF_CH, NULL,	/*  da  */
		    HP_DF_CH, NULL,	/*  db  */
		    HP_DF_CH, NULL,	/*  dc  */
		    HP_DF_CH, NULL,	/*  dd  */
		    HP_DF_CH, NULL,	/*  de  */
		    HP_DF_CH, NULL,	/*  df  */
		    HP_DF_CH, NULL,	/*  e0  */
		    HP_DF_CH, NULL,	/*  e1  */
		    HP_DF_CH, NULL,	/*  e2  */
		    HP_DF_CH, NULL,	/*  e3  */
		    HP_DF_CH, NULL,	/*  e4  */
		    HP_DF_CH, NULL,	/*  e5  */
		    HP_DF_CH, NULL,	/*  e6  */
		    HP_DF_CH, NULL,	/*  e7  */
		    HP_DF_CH, NULL,	/*  e8  */
		    HP_DF_CH, NULL,	/*  e9  */
		    HP_DF_CH, NULL,	/*  ea  */
		    HP_DF_CH, NULL,	/*  eb  */
		    HP_DF_CH, NULL,	/*  ec  */
		    HP_DF_CH, NULL,	/*  ed  */
		    HP_DF_CH, NULL,	/*  ee  */
		    HP_DF_CH, NULL,	/*  ef  */
		    HP_DF_CH, NULL,	/*  f0  */
		    HP_DF_CH, NULL,	/*  f1  */
		    HP_DF_CH, NULL,	/*  f2  */
		    HP_DF_CH, NULL,	/*  f3  */
		    HP_DF_CH, NULL,	/*  f4  */
		    HP_DF_CH, NULL,	/*  f5  */
		    HP_DF_CH, NULL,	/*  f6  */
		    HP_DF_CH, NULL,	/*  f7  */
		    HP_DF_CH, NULL,	/*  f8  */
		    HP_DF_CH, NULL,	/*  f9  */
		    HP_DF_CH, NULL,	/*  fa  */
		    HP_DF_CH, NULL,	/*  fb  */
		    HP_DF_CH, NULL,	/*  fc  */
		    HP_DF_CH, NULL,	/*  fd  */
		    HP_DF_CH, NULL,	/*  fe  */
		    HP_DF_CH, NULL};	/*  ff  */

/* generic 8-bit translation table */
unsigned char GENERIC8_Trans[] = {
		    0x80, NULL,		/*  80  */
		    0x81, NULL,		/*  81  */
		    0x82, NULL,		/*  82  */
		    0x83, NULL,		/*  83  */
		    0x84, NULL,		/*  84  */
		    0x85, NULL,		/*  85  */
		    0x86, NULL,		/*  86  */
		    0x87, NULL,		/*  87  */
		    0x88, NULL,		/*  88  */
		    0x89, NULL,		/*  89  */
		    0x8a, NULL,		/*  8a  */
		    0x8b, NULL,		/*  8b  */
		    0x8c, NULL,		/*  8c  */
		    0x8d, NULL,		/*  8d  */
		    0x8e, NULL,		/*  8e  */
		    0x8f, NULL,		/*  8f  */
		    0x90, NULL,		/*  90  */
		    0x91, NULL,		/*  91  */
		    0x92, NULL,		/*  92  */
		    0x93, NULL,		/*  93  */
		    0x94, NULL,		/*  94  */
		    0x95, NULL,		/*  95  */
		    0x96, NULL,		/*  96  */
		    0x97, NULL,		/*  97  */
		    0x98, NULL,		/*  98  */
		    0x99, NULL,		/*  99  */
		    0x9a, NULL,		/*  9a  */
		    0x9b, NULL,		/*  9b  */
		    0x9c, NULL,		/*  9c  */
		    0x9d, NULL,		/*  9d  */
		    0x9e, NULL,		/*  9e  */
		    0x9f, NULL,		/*  9f  */
		    0xa0, NULL,		/*  a0  */
		    0xa1, NULL,		/*  a1  */
		    0xa2, NULL,		/*  a2  */
		    0xa3, NULL,		/*  a3  */
		    0xa4, NULL,		/*  a4  */
		    0xa5, NULL,		/*  a5  */
		    0xa6, NULL,		/*  a6  */
		    0xa7, NULL,		/*  a7  */
		    0xa8, NULL,		/*  a8  */
		    0xa9, NULL,		/*  a9  */
		    0xaa, NULL,		/*  aa  */
		    0xab, NULL,		/*  ab  */
		    0xac, NULL,		/*  ac  */
		    0xad, NULL,		/*  ad  */
		    0xae, NULL,		/*  ae  */
		    0xaf, NULL,		/*  af  */
		    0xb0, NULL,		/*  b0  */
		    0xb1, NULL,		/*  b1  */
		    0xb2, NULL,		/*  b2  */
		    0xb3, NULL,		/*  b3  */
		    0xb4, NULL,		/*  b4  */
		    0xb5, NULL,		/*  b5  */
		    0xb6, NULL,		/*  b6  */
		    0xb7, NULL,		/*  b7  */
		    0xb8, NULL,		/*  b8  */
		    0xb9, NULL,		/*  b9  */
		    0xba, NULL,		/*  ba  */
		    0xbb, NULL,		/*  bb  */
		    0xbc, NULL,		/*  bc  */
		    0xbd, NULL,		/*  bd  */
		    0xbe, NULL,		/*  be  */
		    0xbf, NULL,		/*  bf  */
		    0xc0, NULL,		/*  c0  */
		    0xc1, NULL,		/*  c1  */
		    0xc2, NULL,		/*  c2  */
		    0xc3, NULL,		/*  c3  */
		    0xc4, NULL,		/*  c4  */
		    0xc5, NULL,		/*  c5  */
		    0xc6, NULL,		/*  c6  */
		    0xc7, NULL,		/*  c7  */
		    0xc8, NULL,		/*  c8  */
		    0xc9, NULL,		/*  c9  */
		    0xca, NULL,		/*  ca  */
		    0xcb, NULL,		/*  cb  */
		    0xcc, NULL,		/*  cc  */
		    0xcd, NULL,		/*  cd  */
		    0xce, NULL,		/*  ce  */
		    0xcf, NULL,		/*  cf  */
		    0xd0, NULL,		/*  d0  */
		    0xd1, NULL,		/*  d1  */
		    0xd2, NULL,		/*  d2  */
		    0xd3, NULL,		/*  d3  */
		    0xd4, NULL,		/*  d4  */
		    0xd5, NULL,		/*  d5  */
		    0xd6, NULL,		/*  d6  */
		    0xd7, NULL,		/*  d7  */
		    0xd8, NULL,		/*  d8  */
		    0xd9, NULL,		/*  d9  */
		    0xda, NULL,		/*  da  */
		    0xdb, NULL,		/*  db  */
		    0xdc, NULL,		/*  dc  */
		    0xdd, NULL,		/*  dd  */
		    0xde, NULL,		/*  de  */
		    0xdf, NULL,		/*  df  */
		    0xe0, NULL,		/*  e0  */
		    0xe1, NULL,		/*  e1  */
		    0xe2, NULL,		/*  e2  */
		    0xe3, NULL,		/*  e3  */
		    0xe4, NULL,		/*  e4  */
		    0xe5, NULL,		/*  e5  */
		    0xe6, NULL,		/*  e6  */
		    0xe7, NULL,		/*  e7  */
		    0xe8, NULL,		/*  e8  */
		    0xe9, NULL,		/*  e9  */
		    0xea, NULL,		/*  ea  */
		    0xeb, NULL,		/*  eb  */
		    0xec, NULL,		/*  ec  */
		    0xed, NULL,		/*  ed  */
		    0xee, NULL,		/*  ee  */
		    0xef, NULL,		/*  ef  */
		    0xf0, NULL,		/*  f0  */
		    0xf1, NULL,		/*  f1  */
		    0xf2, NULL,		/*  f2  */
		    0xf3, NULL,		/*  f3  */
		    0xf4, NULL,		/*  f4  */
		    0xf5, NULL,		/*  f5  */
		    0xf6, NULL,		/*  f6  */
		    0xf7, NULL,		/*  f7  */
		    0xf8, NULL,		/*  f8  */
		    0xf9, NULL,		/*  f9  */
		    0xfa, NULL,		/*  fa  */
		    0xfb, NULL,		/*  fb  */
		    0xfc, NULL,		/*  fc  */
		    0xfd, NULL,		/*  fd  */
		    0xfe, NULL,		/*  fe  */
		    0xff, NULL};	/*  ff  */

/* ECMA-94 translation table */
unsigned char ECMA94_Trans[] = {
		    HP_DF_CH, NULL,	/*  80  */
		    HP_DF_CH, NULL,	/*  81  */
		    HP_DF_CH, NULL,	/*  82  */
		    HP_DF_CH, NULL,	/*  83  */
		    HP_DF_CH, NULL,	/*  84  */
		    HP_DF_CH, NULL,	/*  85  */
		    HP_DF_CH, NULL,	/*  86  */
		    HP_DF_CH, NULL,	/*  87  */
		    HP_DF_CH, NULL,	/*  88  */
		    HP_DF_CH, NULL,	/*  89  */
		    HP_DF_CH, NULL,	/*  8a  */
		    HP_DF_CH, NULL,	/*  8b  */
		    HP_DF_CH, NULL,	/*  8c  */
		    HP_DF_CH, NULL,	/*  8d  */
		    HP_DF_CH, NULL,	/*  8e  */
		    HP_DF_CH, NULL,	/*  8f  */
		    HP_DF_CH, NULL,	/*  90  */
		    0x60, NULL,		/*  91  */
		    0x27, NULL,		/*  92  */
		    HP_DF_CH, NULL,	/*  93  */
		    HP_DF_CH, NULL,	/*  94  */
		    HP_DF_CH, NULL,	/*  95  */
		    HP_DF_CH, NULL,	/*  96  */
		    HP_DF_CH, NULL,	/*  97  */
		    HP_DF_CH, NULL,	/*  98  */
		    HP_DF_CH, NULL,	/*  99  */
		    HP_DF_CH, NULL,	/*  9a  */
		    HP_DF_CH, NULL,	/*  9b  */
		    HP_DF_CH, NULL,	/*  9c  */
		    HP_DF_CH, NULL,	/*  9d  */
		    HP_DF_CH, NULL,	/*  9e  */
		    HP_DF_CH, NULL,	/*  9f  */
		    0xa0, NULL,		/*  a0  */
		    0xa1, NULL,		/*  a1  */
		    0xa2, NULL,		/*  a2  */
		    0xa3, NULL,		/*  a3  */
		    0xa4, NULL,		/*  a4  */
		    0xa5, NULL,		/*  a5  */
		    0xa6, NULL,		/*  a6  */
		    0xa7, NULL,		/*  a7  */
		    0xa8, NULL,		/*  a8  */
		    0xa9, NULL,		/*  a9  */
		    0xaa, NULL,		/*  aa  */
		    0xab, NULL,		/*  ab  */
		    0xac, NULL,		/*  ac  */
		    0xad, NULL,		/*  ad  */
		    0xae, NULL,		/*  ae  */
		    0xaf, NULL,		/*  af  */
		    0xb0, NULL,		/*  b0  */
		    0xb1, NULL,		/*  b1  */
		    0xb2, NULL,		/*  b2  */
		    0xb3, NULL,		/*  b3  */
		    0xb4, NULL,		/*  b4  */
		    0xb5, NULL,		/*  b5  */
		    0xb6, NULL,		/*  b6  */
		    0xb7, NULL,		/*  b7  */
		    0xb8, NULL,		/*  b8  */
		    0xb9, NULL,		/*  b9  */
		    0xba, NULL,		/*  ba  */
		    0xbb, NULL,		/*  bb  */
		    0xbc, NULL,		/*  bc  */
		    0xbd, NULL,		/*  bd  */
		    0xbe, NULL,		/*  be  */
		    0xbf, NULL,		/*  bf  */
		    0xc0, NULL,		/*  c0  */
		    0xc1, NULL,		/*  c1  */
		    0xc2, NULL,		/*  c2  */
		    0xc3, NULL,		/*  c3  */
		    0xc4, NULL,		/*  c4  */
		    0xc5, NULL,		/*  c5  */
		    0xc6, NULL,		/*  c6  */
		    0xc7, NULL,		/*  c7  */
		    0xc8, NULL,		/*  c8  */
		    0xc9, NULL,		/*  c9  */
		    0xca, NULL,		/*  ca  */
		    0xcb, NULL,		/*  cb  */
		    0xcc, NULL,		/*  cc  */
		    0xcd, NULL,		/*  cd  */
		    0xce, NULL,		/*  ce  */
		    0xcf, NULL,		/*  cf  */
		    0xd0, NULL,		/*  d0  */
		    0xd1, NULL,		/*  d1  */
		    0xd2, NULL,		/*  d2  */
		    0xd3, NULL,		/*  d3  */
		    0xd4, NULL,		/*  d4  */
		    0xd5, NULL,		/*  d5  */
		    0xd6, NULL,		/*  d6  */
		    0xd7, NULL,		/*  d7  */
		    0xd8, NULL,		/*  d8  */
		    0xd9, NULL,		/*  d9  */
		    0xda, NULL,		/*  da  */
		    0xdb, NULL,		/*  db  */
		    0xdc, NULL,		/*  dc  */
		    0xdd, NULL,		/*  dd  */
		    0xde, NULL,		/*  de  */
		    0xdf, NULL,		/*  df  */
		    0xe0, NULL,		/*  e0  */
		    0xe1, NULL,		/*  e1  */
		    0xe2, NULL,		/*  e2  */
		    0xe3, NULL,		/*  e3  */
		    0xe4, NULL,		/*  e4  */
		    0xe5, NULL,		/*  e5  */
		    0xe6, NULL,		/*  e6  */
		    0xe7, NULL,		/*  e7  */
		    0xe8, NULL,		/*  e8  */
		    0xe9, NULL,		/*  e9  */
		    0xea, NULL,		/*  ea  */
		    0xeb, NULL,		/*  eb  */
		    0xec, NULL,		/*  ec  */
		    0xed, NULL,		/*  ed  */
		    0xee, NULL,		/*  ee  */
		    0xef, NULL,		/*  ef  */
		    0xf0, NULL,		/*  f0  */
		    0xf1, NULL,		/*  f1  */
		    0xf2, NULL,		/*  f2  */
		    0xf3, NULL,		/*  f3  */
		    0xf4, NULL,		/*  f4  */
		    0xf5, NULL,		/*  f5  */
		    0xf6, NULL,		/*  f6  */
		    0xf7, NULL,		/*  f7  */
		    0xf8, NULL,		/*  f8  */
		    0xf9, NULL,		/*  f9  */
		    0xfa, NULL,		/*  fa  */
		    0xfb, NULL,		/*  fb  */
		    0xfc, NULL,		/*  fc  */
		    0xfd, NULL,		/*  fd  */
		    0xfe, NULL,		/*  fe  */
		    0xff, NULL};	/*  ff	*/

/* this table is different in that it translates the whole character
 * set, but allows only one byte translations.	Same length.
 */
unsigned char MATH8_Trans[] = {
/*	    x0	    1	    2	    3	    4	    5	    6	    7 */
/* 000 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 010 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 020 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 030 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 040 */   32,     127,    177,    127,    178,    127,    127,    212,
/* 050 */   40,     41,     238,    43,     44,     45,     46,     47,
/* 060 */   48,     49,     50,     51,     52,     53,     54,     55,
/* 070 */   56,     57,     127,    127,    60,     61,     62,     127,
/* 100 */   239,    65,     66,     86,     68,     69,     85,     67,
/* 110 */   71,     73,     121,    74,     75,     76,     77,     79,
/* 120 */   80,     72,     81,     82,     83,     127,    91,     88,
/* 130 */   78,     87,     70,     127,    64,     127,    180,    95,
/* 140 */   176,    97,     98,     118,    100,    101,    117,    99,
/* 150 */   103,    105,    122,    106,    107,    108,    109,    111,
/* 160 */   112,    104,    113,    114,    115,    116,    123,    120,
/* 170 */   110,    119,    102,    127,    246,    127,    127,    127,
/* 200 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 210 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 220 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 230 */   127,    127,    127,    127,    127,    127,    127,    127,
/* 240 */   160,    84,     39,     92,     235,    36,     127,    127,
/* 250 */   127,    127,    127,    170,    164,    161,    102,    163,
/* 260 */   35,     254,    34,     94,     42,     38,     90,     203,
/* 270 */   37,     93,     125,    63,     127,    246,    236,    127,
/* 300 */   217,    221,    222,    127,    194,    192,    216,    182,
/* 310 */   181,    187,    191,    188,    186,    190,    183,    185,
/* 320 */   215,    89,     127,    127,    127,    180,    33,     202,
/* 330 */   200,    197,    198,    172,    168,    165,    166,    167,
/* 340 */   127,    252,    127,    127,    127,    82,     225,    245,
/* 350 */   228,    224,    246,    225,    226,    227,    228,    245,
/* 360 */   32,     249,    213,    229,    245,    231,    242,    245,
/* 370 */   244,    240,    246,    241,    242,    243,    244,    127  };
