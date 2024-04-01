/**[f******************************************************************
 * transtbl.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   transtbl.c   *****************************/
/*
 *  TransTbl:  Utility for reading character translation table from resources.
 */

// history
// 25 aug 89	craigc		Added MATH8 symbol font support.
// 27 apr 89	peterbe		Tabs @ 8 spaces, add a FF.

#include "nocrap.h"
#include "windows.h"
#include "pfm.h"
#include "transtbl.h"
#define NO_PRINTER_STUFF
#include "hppcl.h"
#include "resource.h"
#include "debug.h"


#define DBGtrans(msg) DBMSG(msg)


/*  GetTransTable
 *
 *  Read the translation table from the resources, the caller is
 *  responsible for unlocking/removing the table.
 */
HANDLE FAR PASCAL GetTransTable(hModule, lpLPTransTbl, symbolSet)
    HANDLE hModule;
    LPSTR FAR *lpLPTransTbl;
    SYMBOLSET symbolSet;
    {
    LPSTR transResID = 0L;
    HANDLE hTransInfo = 0;
    HANDLE hTransData = 0;

    *lpLPTransTbl = 0L;

    switch (symbolSet)
	{
	case epsymRoman8:
	    DBGtrans(("GetTransTable(): Roman8 translation\n"));
	    transResID = (LPSTR)XTBL_ROMAN8;
	    break;
	case epsymUSASCII:
	    DBGtrans(("GetTransTable(): USASCII translation\n"));
	    transResID = (LPSTR)XTBL_USASCII;
	    break;
	case epsymECMA94:
	    DBGtrans(("GetTransTable(): ECMA94 translation\n"));
	    transResID = (LPSTR)XTBL_ECMA94;
	    break;
	case epsymGENERIC8:
	case epsymKana8:
	    DBGtrans(("GetTransTable(): GENERIC8 translation\n"));
	    transResID = (LPSTR)XTBL_GENERIC8;
	    break;
	case epsymMath8:
	    DBGtrans(("GetTransTable(): Symbol->MATH8 translation\n"));
	    transResID = (LPSTR)XTBL_MATH8;
	    break;
	case epsymGENERIC7:
	case epsymUSLegal:
	default:
	    DBGtrans(("GetTransTable(): GENERIC7 translation\n"));
	    transResID = (LPSTR)XTBL_GENERIC7;
	    break;
	}

    if ((hTransInfo = FindResource(hModule, transResID, (LPSTR)TRANSTBL)) &&
	(hTransData = LoadResource(hModule, hTransInfo)))
	{
	if (!(*lpLPTransTbl = LockResource(hTransData)))
	    {
	    FreeResource(hTransData);
	    hTransData = 0;
	    hTransInfo = 0;
	    }
	}

    DBGtrans(("GetTransTable(): ...return %d\n", hTransData));

    return (hTransData);
    }
