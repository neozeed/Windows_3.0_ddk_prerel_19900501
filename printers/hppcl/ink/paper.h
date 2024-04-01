/**[f******************************************************************
 * paper.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

// 06 sep 89	peterbe	Added ComputeLineBufSize()

BOOL FAR PASCAL GetPaperFormat(PAPERFORMAT FAR *, HANDLE, short, short, short);
BOOL FAR PASCAL GetPaperBits(HANDLE, WORD FAR *);
WORD FAR PASCAL Paper2Bit(short);
#ifndef NO_PAPERBANDCRAP
WORD FAR PASCAL ComputeLineBufSize(PAPERFORMAT FAR *, LPPCLDEVMODE);
WORD FAR PASCAL ComputeBandBitmapSize(PAPERFORMAT FAR *, LPPCLDEVMODE);
void FAR PASCAL ComputeBandingParameters (LPDEVICE, short);
void FAR PASCAL ComputeBandStartPosition(LPPOINT, LPDEVICE, short);
BOOL FAR PASCAL ComputeNextBandRect(LPDEVICE, short, LPRECT);
#endif
