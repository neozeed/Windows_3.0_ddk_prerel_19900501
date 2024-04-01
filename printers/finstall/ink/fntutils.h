/**[f******************************************************************
 * fntutils.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 *  All rights reserved.
 *
 **f]*****************************************************************/


HANDLE FAR PASCAL InitWinSF(LPSTR);
int FAR PASCAL NextWinSF(HANDLE, LPSTR, WORD);
int FAR PASCAL EndWinSF(HANDLE);
int FAR PASCAL NextWinCart(HANDLE, LPSTR, WORD);
int FAR PASCAL GetCartName(LPSTR,LPSTR, WORD);
