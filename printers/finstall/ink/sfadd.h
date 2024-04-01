/**[f******************************************************************
 * sfadd.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 *  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

HANDLE FAR PASCAL AddFontsMode(HWND, HANDLE, WORD, WORD FAR *, BOOL);
HANDLE FAR PASCAL EndAddFontsMode(HWND, HANDLE, HANDLE, WORD);
HANDLE FAR PASCAL AddFonts(HWND, HANDLE, WORD, HANDLE, WORD, HANDLE, LPSTR, LPSTR, WORD FAR *);
BOOL FAR PASCAL MergePath(LPSTR, LPSTR, int, BOOL);
BOOL FAR PASCAL GetTargDir(HWND, HANDLE, LPSTR, int, LPSTR);
BOOL FAR PASCAL existLBL(HANDLE, int, LPSTR, int);
