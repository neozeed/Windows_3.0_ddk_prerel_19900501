/**[f******************************************************************
 * sfpfm.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 *  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

BOOL FAR PASCAL DLtoPFM(int, HANDLE, BOOL, LPPFMHEADER, WORD FAR *, LPEXTTEXTMETRIC, LPDRIVERINFO, LPSTR, int, LPSTR, int);
long FAR PASCAL writePFM(int, LPPFMHEADER, WORD FAR *, LPPFMEXTENSION, LPEXTTEXTMETRIC, LPDRIVERINFO, LPSTR, LPSTR);
void FAR PASCAL useDupPFM(LPSTR, LPSTR, int);

