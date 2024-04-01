/**[f******************************************************************
 * sfinstal.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 *
 *   1-25-89	jimmat	Changed SoftFontInstall() parameters.
 *   2-20-89	jimmat	Font Installer/Driver use same WIN.INI section (again)!
 */

int FAR PASCAL SoftFontInstall(HWND, LPSTR, LPSTR, BOOL, int);

#define SF_NOABORT		0x0001
#define SF_CHANGES		0x0002
#define SF_PERMALRT		0x0004

extern int gSF_FLAGS;
extern int gFSvers;
