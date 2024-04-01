/**[f******************************************************************
 * utils.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

extern long FAR PASCAL lmul(long, long);
extern long FAR PASCAL ldiv(long, long);
extern long FAR PASCAL FontMEM(int, long, long);
extern int FAR PASCAL itoa(int, LPSTR);
extern int FAR PASCAL atoi(LPSTR);
extern int FAR PASCAL _lopenp(LPSTR, WORD);
extern void FAR PASCAL MakeAppName(LPSTR, LPSTR, LPSTR, int);
