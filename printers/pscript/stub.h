/**[f******************************************************************
 * stub.h - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

#define	LOCAL	static
#define	STDERR	2
#define	FAR	far

typedef	short int	WORD;
typedef	char	FAR	*LPSTR;

extern int far pascal dos_write(short, LPSTR, WORD, WORD far *);

