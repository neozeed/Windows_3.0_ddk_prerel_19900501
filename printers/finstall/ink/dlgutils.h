/**[f******************************************************************
 * dlgutils.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 *  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

int FAR PASCAL MyDialogBox(HANDLE,int,HWND,FARPROC);

void FAR PASCAL CenterDlg (HWND);
int FAR PASCAL GenericWndProc(HWND, unsigned, WORD, long);
