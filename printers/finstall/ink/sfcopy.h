/**[f******************************************************************
 * sfcopy.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 *  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

BOOL FAR PASCAL GetPort(HWND, HANDLE, LPSTR, LPSTR, int);
HANDLE FAR PASCAL CopyFonts(HWND, HANDLE, WORD, HANDLE, LPSTR, WORD, HANDLE, LPSTR, BOOL, WORD FAR *, LPSTR);
