/**[f******************************************************************
 * environ.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   Environ.c   ******************************/
//
//  Environ:    Procs for reading/creating devmode
//
//  05 mar 90   clarkc  DMRES_LOW default instead of DMRES_LOW, to match 75DPI
//                      (mind changes are fun, aren't they?)
//  25 feb 90   clarkc  DMRES_HIGH default instead of DMRES_LOW, to match 300DPI
//  09 jan 90   clarkc  duplex setting from win.ini no longer overruled.
//  09 oct 89	peterbe	isUSA() algorithm changed.  TRUE for all countries in
//			the Americas.
//  29 sep 89	peterbe	isUSA() returns TRUE now for default country (0) and
//			most countries in the Americas.  Also, now call isUSA()
//			in GetWinIniEnv().
//  28 sep 89	craigc	fixed cartridge not being selected.
//  03 aug 89	peterbe	Commented out NO_PRIVATE_HACK stuff -- remove later.
//   2-06-89    jimmat  Changes related to externalizing HP cartridge info.
//   2-07-89	jimmat	Driver Initialization changes.
//   2-20-89	jimmat	Driver/Font Installer use same WIN.INI section (again)
//


#include "generic.h" 
#include "resource.h"
#include "strings.h"
#define PRTCARTITEMS
#include "environ.h"
#include "utils.h"
#include "version.h"
#include "lclstr.h"
#include "country.h"

LPSTR   FAR PASCAL lstrcpy(LPSTR, LPSTR);
LPSTR   FAR PASCAL lstrcat( LPSTR, LPSTR );
int FAR PASCAL lstrlen( LPSTR );


/*  Utilities
 */
#include "getint.c"

#define LOCAL static


#ifdef DEBUG
#define LOCAL_DEBUG
#endif

#ifdef LOCAL_DEBUG
#define DBGentry(msg) /*DBMSG(msg)*/
#define DBGEnv(msg)	DBMSG(msg)
#else
#define DBGentry(msg) /*null*/
#define DBGEnv(msg)   /*null*/
#endif


extern HANDLE hLibInst; 	/* driver's instance handel */

/*  Forward refs
 */
LOCAL void DefaultEnvironment(LPPCLDEVMODE, LPSTR, HANDLE);
LOCAL void GetWinIniEnv(LPPCLDEVMODE, LPSTR, LPSTR, HANDLE);
LOCAL BOOL isUSA(void);


#ifdef LOCAL_DEBUG
LOCAL void
dumpDevMode(LPPCLDEVMODE lpEnv) {

    DBMSG(("   dmDeviceName: %ls\n",(LPSTR)lpEnv->dm.dmDeviceName));
    DBMSG(("   dmSpecVersion: %4xh\n",lpEnv->dm.dmSpecVersion));
    DBMSG(("   dmDriverVersion: %4xh\n",lpEnv->dm.dmDriverVersion));
    DBMSG(("   dmSize: %d\n",lpEnv->dm.dmSize));
    DBMSG(("   dmDriverExtra: %d\n",lpEnv->dm.dmDriverExtra));
    DBMSG(("   dmFields: %8lxh\n",lpEnv->dm.dmFields));
    DBMSG(("   dmOrientation: %d\n",lpEnv->dm.dmOrientation));
    DBMSG(("   dmPaperSize: %d\n",lpEnv->dm.dmPaperSize));
#ifdef FLARP
    DBMSG(("   dmPaperLength: %d\n",lpEnv->dm.dmPaperLength));
    DBMSG(("   dmPaperWidth: %d\n",lpEnv->dm.dmPaperWidth));
    DBMSG(("   dmXMargin: %d\n",lpEnv->dm.dmXMargin));
    DBMSG(("   dmYMargin: %d\n",lpEnv->dm.dmYMargin));
    DBMSG(("   dmLogicalLength: %d\n",lpEnv->dm.dmLogicalLength));
    DBMSG(("   dmLogicalWidth: %d\n",lpEnv->dm.dmLogicalWidth));
#endif
    DBMSG(("   dmCopies: %d\n",lpEnv->dm.dmCopies));
    DBMSG(("   dmDefaultSource: %d\n",lpEnv->dm.dmDefaultSource));
    DBMSG(("   dmPrintQuality: %d\n",lpEnv->dm.dmPrintQuality));
    DBMSG(("   dmColor: %d\n",lpEnv->dm.dmColor));
    DBMSG(("   dmDuplex: %d\n",lpEnv->dm.dmDuplex));
}
#endif


/***************************************************************************/
/**************************   Global Procedures   **************************/


/*  MakeEnvironment
 *
 *  Initialize the devmode data structure.  First try to get info from
 *  win.ini -- if that fails, use information from first printer and
 *  first cartridge listed in resources -- if that fails, use constants.
 */
void FAR PASCAL
MakeEnvironment(lpDevmode, lpDeviceName, lpPortName, lpProfile)
    LPPCLDEVMODE lpDevmode;
    LPSTR lpDeviceName;
    LPSTR lpPortName;
    LPSTR lpProfile;
    {
    PRTINFO prtInfo;
#if defined(CARTS_IN_RESOURCE)  /*--------------------------------------*/
    CARTINFO cartInfo;
#endif  /*-- defined(CARTS_IN_RESOURCE) --------------------------------*/

    DBGEnv(("MakeEnvironment(%lp,%lp,%lp,%lp)\n",
	   lpDevmode, lpDeviceName, lpPortName, lpProfile));
    DBGEnv(("   %ls on %ls\n", lpDeviceName, lpPortName));

    /*  Start off with defaults
     */
    DefaultEnvironment(lpDevmode, lpDeviceName, hLibInst);

    /*  Read what we can from win.ini
     */
    GetWinIniEnv(lpDevmode, lpPortName, lpProfile, hLibInst);

    /*  Read printer and cartridge information from the resources.
     */
    if (GetPrtItem((PRTINFO FAR*)&prtInfo, lpDevmode->prtIndex, hLibInst))
        {
        /*  Info read, fill in devmode.
         */
        short ind;

        lpDevmode->availmem = prtInfo.availmem;
        lpDevmode->romind = prtInfo.romind;
        lpDevmode->romcount = prtInfo.romcount;
        lpDevmode->prtCaps= prtInfo.caps;
        lpDevmode->maxPgSoft = prtInfo.maxpgsoft;
	lpDevmode->maxSoft = prtInfo.maxsoft;

	/* CraigC: This fixes PC LOAD EXEC bug on LJII!
	 * NOTE: We shouldn't be saving PAPERIND in WIN.INI!!!!!!!
	 * Fix THAT for Win 3.1.
	 */
	lpDevmode->paperInd = prtInfo.indpaperlist;

        if (lpDevmode->numCartridges > prtInfo.numcart)
            lpDevmode->numCartridges = prtInfo.numcart;

        /*  Get cartridge info for each cartridge selected.
         */

#if defined(CARTS_IN_RESOURCE)  /*--------------------------------------*/

        for (ind = 0;
            (ind < DEVMODE_MAXCART) && (ind < lpDevmode->numCartridges) &&
	    GetCartItem(&cartInfo, lpDevmode->cartIndex[ind], hLibInst);
            ++ind)
            {
            lpDevmode->cartind[ind] = cartInfo.cartind;
            lpDevmode->cartcount[ind] = cartInfo.cartcount;
            }

#else   /*-- defined(CARTS_IN_RESOURCE) --------------------------------*/

        /* Now that cartridges are externalized, cartIndex[ind] can
           only be > 0 if the user edited his win.ini file by hand,
           which he/she had best not do.  At some point in time, we
           should go through the code and make cartridge indexes
           >= 0 again--now that there is only one type (external),
	   there is no need to have indexes above and below 0. */

	/* craigc-sort of did this.  cartridges now back in but in the
	 * more convenient form of resource PCM's.
	 */

        for (ind = 0; (ind < DEVMODE_MAXCART) &&
	    (ind < lpDevmode->numCartridges); ++ind) {

            lpDevmode->cartind[ind] = 1;
            lpDevmode->cartcount[ind] = 0;
        }

#endif  /*-- defined(CARTS_IN_RESOURCE) --------------------------------*/

        lpDevmode->numCartridges = ind;

        DBMSG(("environ: numcart=%d\n",ind));
        }
    else
        {
        /*  Use defaults.
         */
	DefaultEnvironment(lpDevmode, lpDeviceName, hLibInst);
        lpDevmode->availmem = JETMEM;
        lpDevmode->romind = 0;
        lpDevmode->romcount = 1;
        lpDevmode->numCartridges = 0;
        lpDevmode->prtCaps= JETCAPS;
        lpDevmode->maxPgSoft = 0;
        lpDevmode->maxSoft = 0;
        }

    /*  Force loading of soft fonts if the user requested it.
     */
    if (lpDevmode->options & OPTIONS_FORCESOFT)
        lpDevmode->prtCaps &= ~(NOSOFT);

    /*	Make adjustments to DEVMODE based on printer capabilities --
	NOTE: very similar code exists in devmode.c, if you update this
	you may need to update that also */

    if ((lpDevmode->prtCaps & (AUTOSELECT|LOTRAY|ANYENVFEED|NOMAN)) == NOMAN) {
	lpDevmode->dm.dmFields |= DM_DEFAULTSOURCE;
	lpDevmode->dm.dmDefaultSource = DMBIN_UPPER;
    }

    if (lpDevmode->prtCaps & ANYDUPLEX) {
	lpDevmode->dm.dmFields |= DM_DUPLEX;
/* This wipes out setting in win.ini, shouldn't be here.  9 Jan 1990  clarkc  */
//	lpDevmode->dm.dmDuplex = DMDUP_SIMPLEX;
    } else
	lpDevmode->dm.dmDuplex = DMDUP_SIMPLEX;

#ifdef LOCAL_DEBUG
    DBMSG(("MakeEnvironment returning:\n"));
    dumpDevMode(lpDevmode);
#endif
}


/*  GetPrtItem
 *
 *  Get the printer information for one string in the resource file.
 */
BOOL FAR PASCAL GetPrtItem(lpPrtInfo, ind, hModule)
    PRTINFO FAR *lpPrtInfo;
    short ind;
    HANDLE hModule;
    {
    char tempbuf[STR_LEN];
    LPSTR infoptr;
    LPSTR bufptr;
    short i;
    char sepchar;

    DBGentry(("GetPrtItem(%lp,%d,%d)\n", lpPrtInfo, ind, (HANDLE)hModule));

    if (!LoadString(hModule, DEVNAME_BASE+ind, (LPSTR)tempbuf, STR_LEN))
        return FALSE;

    bufptr = (LPSTR)tempbuf;

    /*  separator character is the first char in string
     */
    sepchar = *bufptr++;

    /*  parse string to get devname
     */
    for (i = 0, infoptr = (LPSTR)lpPrtInfo->devname;
        *bufptr && (*bufptr != sepchar); i++)
        {
        if (i < DEV_NAME_LEN - 1)
            *infoptr++ = *bufptr;
        ++bufptr;
        }
    *infoptr = '\0';

    /*  parse for other values
     */
    if (*bufptr && (i > 0))
        {
        /*  increment after each field to move past sepchar
         */
        bufptr++;

        lpPrtInfo->availmem = GetInt((LPSTR FAR *)&bufptr, sepchar);

        if (*bufptr && (*bufptr == sepchar))
            {
            bufptr++;
            for (infoptr = (LPSTR)lpPrtInfo->realmem;
                *bufptr && (*bufptr != sepchar);
                *infoptr++ = *bufptr++)
                ;
            *infoptr='\0';

            if (*bufptr && (*bufptr == sepchar))
                {
                bufptr++;
                lpPrtInfo->caps = GetInt((LPSTR FAR *)&bufptr, sepchar);
                bufptr++;
                lpPrtInfo->romind=GetInt((LPSTR FAR *)&bufptr,sepchar);
                bufptr++;
                lpPrtInfo->romcount=GetInt((LPSTR FAR *)&bufptr,sepchar);
                bufptr++;
                lpPrtInfo->maxpgsoft=GetInt((LPSTR FAR *)&bufptr,sepchar);
                bufptr++;
                lpPrtInfo->maxsoft=GetInt((LPSTR FAR *)&bufptr,sepchar);
                bufptr++;
                lpPrtInfo->numcart=GetInt((LPSTR FAR *)&bufptr,sepchar);
                bufptr++;
                lpPrtInfo->indpaperlist=GetInt((LPSTR FAR *)&bufptr,sepchar);

                return TRUE;
                }
            }
        }

    return FALSE;
    }


#if defined(CARTS_IN_RESOURCE)  /*--------------------------------------*/

/*  GetCartItem
 *
 *  Get the cartridge information for one string in the resource file.
 */
BOOL FAR PASCAL GetCartItem(lpCartInfo, ind, hModule)
    CARTINFO FAR *lpCartInfo;
    short ind;
    HANDLE hModule;
    {
    char tempbuf[STR_LEN];
    LPSTR infoptr;
    LPSTR bufptr;
    short i;
    char sepchar;

    DBGentry(("GetCartItem(%lp,%d,%d)\n",
        lpCartInfo, ind, (HANDLE)hModule));

    /* if cartridge comes from a PCM, there's no resource information,
     * but we'll assume the cartridge is ok ...craigc
     */
    if (ind<0)
        {
        lpCartInfo->cartind=1;
        lpCartInfo->cartcount=0;
        return TRUE;
        }

    if (!LoadString(hModule, CART_BASE+ind, (LPSTR)tempbuf, STR_LEN))
        return FALSE;

    bufptr = (LPSTR)tempbuf;

    /*  get separator character which is first char in string
     */
    sepchar = *bufptr++;

    /*  parse string to get devname
     */
    for (i = 0, infoptr = (LPSTR)lpCartInfo->cartname;
        *bufptr && (*bufptr != sepchar); i++)
        {
        if (i < DEV_NAME_LEN - 1)
            *infoptr++ = *bufptr;
        ++bufptr;
        }
    *infoptr='\0';

    /*  parse for other values
     */
    if (*bufptr && (i > 0))
        {
        /*  increment past sepchar and extract values
         */
        bufptr++;
        lpCartInfo->cartind = GetInt((LPSTR FAR *)&bufptr, sepchar);
        bufptr++;
        lpCartInfo->cartcount = GetInt((LPSTR FAR *)&bufptr, sepchar);

        #if 0
        dumpCartInfo(lpCartInfo);
        #endif

        return TRUE;
        }

    return FALSE;

    }

#endif  /*-- defined(CARTS_IN_RESOURCE) --------------------------------*/


/***************************************************************************/
/**************************   Local Procedures   ***************************/


/*  DefaultEnvironment
 *
 *  Set up the default environment for the printer.
 */

LOCAL void
DefaultEnvironment(LPPCLDEVMODE lpDevmode, LPSTR lpDeviceName, HANDLE hModule) {

    DBGEnv(("DefaultEnvironment(%lp,%lp,%d)\n",
	    lpDevmode, lpDeviceName, hModule));

    lmemset((LPSTR)lpDevmode, 0, sizeof(PCLDEVMODE));
    lstrcpy((LPSTR)lpDevmode->dm.dmDeviceName, (LPSTR)lpDeviceName);

    lpDevmode->dm.dmSpecVersion = DM_SPECVERSION;
    lpDevmode->dm.dmDriverVersion = VNUMint;
    lpDevmode->dm.dmSize = sizeof(DEVMODE);
    lpDevmode->dm.dmDriverExtra = sizeof(PCLDEVMODE) - sizeof(DEVMODE);
    lpDevmode->dm.dmFields = (DM_ORIENTATION | DM_PAPERSIZE | DM_COPIES |
			      DM_PRINTQUALITY);

    lpDevmode->dm.dmOrientation = DMORIENT_PORTRAIT;

    lpDevmode->dm.dmPaperSize = (isUSA()) ? DMPAPER_LETTER : DMPAPER_A4;
    lpDevmode->dm.dmDefaultSource = DMBIN_UPPER;

    lpDevmode->dm.dmCopies = 1;
    lpDevmode->dm.dmPrintQuality = DMRES_LOW;
    lpDevmode->prtResFac=SF75;

    /*  Index to first printer in resources, and no cartridge selected.
     */
    lpDevmode->prtIndex = 0;
    lpDevmode->numCartridges = 1;
    lpDevmode->txwhite = 255;
    lpDevmode->options = JETOPTS;
    lpDevmode->fsvers = 0;
}


/*  GetWinIniEnv
 *
 *  Get the environment information from a .INI file.
 */
LOCAL void GetWinIniEnv(lpDevmode, lpPortName, lpProfile, hModule)
    LPPCLDEVMODE lpDevmode;
    LPSTR lpPortName;
    LPSTR lpProfile;
    HANDLE hModule;
    {
    char appName[64];
    char name[16];
    short ind, data, tmp;

//#ifndef NO_PRIVATE_HACK
//    HANDLE hKernel;
//    FARPROC GetPint;
//    HANDLE FAR PASCAL LoadLibrary(LPSTR);
//    HANDLE FAR PASCAL FreeLibrary(HANDLE);
//#define MAKEINTRESOURCE(n) ((LPSTR)((DWORD)((WORD)n)))
//
//    if ((hKernel = LoadLibrary("KERNEL.EXE")) >= 32)
//	GetPint = GetProcAddress(hKernel,MAKEINTRESOURCE(127));
//#endif

    MakeAppName(ModuleNameStr,lpPortName,appName,sizeof(appName));

    DBGEnv(("GetWinIniEnv(%lp,%lp,%lp), appName=%ls\n",
	   lpDevmode, lpPortName, lpProfile, (LPSTR)appName));

    /*  For each dialog item.
     */
    for (ind = WININI_BASE; ind < WININI_LAST; ++ind)
        {
        /*  Load key name of item from resources and get its
         *  corresponding info field from win.ini.
         */
        name[0] = '\0';
        if (LoadString(hModule, ind, (LPSTR)name, sizeof(name)))
            {
	    /*	Get data from .INI, use -1 to indicate failure.
	     */
//#ifndef NO_PRIVATE_HACK
//	    data = (lpProfile && GetPint) ?
//		    (int) GetPint((LPSTR)appName,(LPSTR)name,(int)-1,
//				  (LPSTR)lpProfile) :
//#else
	    data = lpProfile ? GetPrivateProfileInt(appName,name,-1,lpProfile) :
//#endif
			       GetProfileInt(appName,name,-1);


	    // force paper to default to 1 (letter)
	    // but return -1 for other values
	    //if ((data < 0) && (ind != WININI_PAPER))
	    if (data < 0)
                continue;

            tmp = ind;

            /*  Transfer info to devmode.
             */
            switch (ind)
                {
		case WININI_PAPER:
		    switch(data)
			{		// we read some normal paper size
			case DMPAPER_LETTER:
			case DMPAPER_LEGAL:
			case DMPAPER_LEDGER:
			case DMPAPER_A3:
			case DMPAPER_A4:
			case DMPAPER_B5:
			case DMPAPER_EXECUTIVE:
			    lpDevmode->dm.dmPaperSize = data;
			    break;

			default:	// 0 or -1, so set country default
			    lpDevmode->dm.dmPaperSize = (isUSA()) ?
					DMPAPER_LETTER : DMPAPER_A4;
			}
                    break;

                case WININI_COPIES:
                    /*  Size of edit box for copies limits to
                     *  9999 copies.
                     */
                    if (data < 1)
                        data = 1;
                    else if (data > MAX_COPIES)
                        data = MAX_COPIES;
		    lpDevmode->dm.dmCopies = data;
                    break;

                case WININI_ORIENT:
		    if ((data == DMORIENT_PORTRAIT) ||
			(data == DMORIENT_LANDSCAPE))
			lpDevmode->dm.dmOrientation = data;
                    else
			lpDevmode->dm.dmOrientation = DMORIENT_PORTRAIT;
                    break;
                    
		case WININI_PRTRESFAC:
		    if (data == SF300)
			lpDevmode->dm.dmPrintQuality = DMRES_HIGH;
		    else if (data == SF150)
			lpDevmode->dm.dmPrintQuality = DMRES_MEDIUM;
		    else {
			data = SF75;
			lpDevmode->dm.dmPrintQuality = DMRES_LOW;
		    }
		    lpDevmode->prtResFac = data;
                    break;

                case WININI_TRAY:
		    if ((data == DMBIN_UPPER)  || (data == DMBIN_LOWER) ||
			(data == DMBIN_MANUAL) || (data == DMBIN_AUTO)	||
			(data == DMBIN_ENVELOPE))
			lpDevmode->dm.dmDefaultSource = data;
                    else
			lpDevmode->dm.dmDefaultSource = DMBIN_UPPER;
                    break;

                case WININI_PRTINDEX:
                    if (data < 0)
                        data = 0;
                    else if (data > MAX_PRINTERS - 1)
                        data = MAX_PRINTERS - 1;
                    lpDevmode->prtIndex = data;
                    break;

                case WININI_NUMCART:
                    DBMSG(("requested numcart is %d",data));
                    if (data < 0)
                        data = 0;
                    else if (data > DEVMODE_MAXCART)
                        data = DEVMODE_MAXCART;
                    lpDevmode->numCartridges = data;
                    DBMSG((", using %d.\n",data));
                    break;

		case WININI_DUPLEX:
/* There's no need to check for DMDUP_SIMPLEX.  9 Jan 1990  clarkc  */
		    if (data == DMDUP_VERTICAL || data == DMDUP_HORIZONTAL)
			lpDevmode->dm.dmDuplex = data;
		    else
			lpDevmode->dm.dmDuplex = DMDUP_SIMPLEX;
                    break;

                case WININI_CARTINDEX:
                case WININI_CARTINDEX1:
                case WININI_CARTINDEX2:
                case WININI_CARTINDEX3:
                case WININI_CARTINDEX4:
                case WININI_CARTINDEX5:
                case WININI_CARTINDEX6:
                case WININI_CARTINDEX7:
                    tmp -= WININI_CARTINDEX;

                    if (tmp < lpDevmode->numCartridges)
                        {
                        if (data < 0)
                            data = 0;
			lpDevmode->cartIndex[tmp] = - data;
                        DBMSG(("lpdevmode->cartindex[%d]=%d\n",
                            tmp,lpDevmode->cartIndex[tmp]));
                        }
                    break;

                case WININI_TXWHITE:
                    lpDevmode->txwhite = data;
                    break;

                case WININI_OPTIONS:
                    lpDevmode->options = data;
                    break;

                case WININI_FSVERS:
                    lpDevmode->fsvers = data;
                    break;

                case WININI_PRTCAPS:
                    /*  Note: this is superseded by whatever exists
                     * in the internal resources.  We write this
                     *  field to the win.ini so other apps can use
                     *  the information.
                     */
                    lpDevmode->prtCaps = data;
                    break;

                case WININI_PAPERIND:
                    lpDevmode->paperInd = data;
                    break;

                default:
                    break;
                }
            }
	}
//#ifndef NO_PRIVATE_HACK
//    if (hKernel >= 32)
//	FreeLibrary(hKernel);
//#endif
    }

//  isUSA
//
//  Read win.ini and return country code
//
//  Note that if a country was set up, it will have a valid nonzero country
//  code.
//
//  This version of the function returns TRUE for ANY Western-hemisphere
//  country  (USA, CANADA, any area with dial code beginning with 5:
//  5n, 5nn)

LOCAL BOOL isUSA()
    {
    int iCountry;

    iCountry = GetProfileInt((LPSTR)"intl", (LPSTR)"icountry", USA_COUNTRYCODE);
    switch(iCountry)
	{
	case 0:				// string was there but 0
	case USA_COUNTRYCODE:		// DEFINED in COUNTRY.H
	case FRCAN_COUNTRYCODE:
		return TRUE;

	default:
		if (((iCountry >= 50) && (iCountry < 60)) ||
		    ((iCountry >= 500) && (iCountry < 600)))
		    return TRUE;
		else
		    return FALSE;
	}

    }	// isUSA()
