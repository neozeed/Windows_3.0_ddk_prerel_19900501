/**********************************************************************
 * devcap.c -
 *
 * Copyright (C) 1989-1990 Microsoft Corporation.  All rights reserved.
 *
 *********************************************************************/

//
// 07 aug 89	peterbe	Changed lstrcmp() to lstrcmpi().
//   2-14-89	jimmat	Created as part of the Driver Initialization
//			changes.
//

#include "nocrap.h"
#undef	NOMEMMGR
#undef	NOGDI
#include "windows.h"
#include "hppcl.h"
#include "resource.h"
#include "debug.h"
#include "environ.h"
#define NO_OUTUTIL
#include "utils.h"
#define NO_PAPERBANDCRAP
#include "paperfmt.h"
#include "paper.h"


LPSTR   FAR PASCAL lstrcpy(LPSTR, LPSTR);
int	FAR PASCAL lstrcmpi(LPSTR, LPSTR);

#ifdef DEBUG
#define DBGDevCap(msg)	    DBMSG(msg)
#else
#define DBGDevCap(msg)	  /*null*/
#endif


extern	HANDLE hLibInst;	    /* driver library instance handle */


/*  DeviceCapabilities -
 *
 *  This function returns the capabilities of the device driver to
 *  the caller.
 */

DWORD FAR PASCAL
DeviceCapabilities(LPSTR lpDevice, LPSTR lpPort, WORD nIndex,
		   LPSTR lpOutput, LPPCLDEVMODE lpDevmode) {

    DWORD rc;
    LPPOINT pp;
    int i, pcap;
    short FAR *wp;
    PAPERFORMAT pf;
    PCLDEVMODE CurEnv;
    LPPCLDEVMODE lpDm;
    WORD paperBits[MAX_PAPERLIST];

    LockSegment(-1);		    /* nail down our data segment */

    DBGDevCap(("DeviceCapabilities(%lp,%lp,%d,%lp,%lp)\n",
	       lpDevice,lpPort,nIndex,lpOutput,lpDevmode));

    /*	If the caller didn't pass in a DEVMODE pointer, then get/build
     *	a current one
     */

    if (!lpDevmode) {
	lstrcpy(CurEnv.dm.dmDeviceName, lpDevice);

	if (!GetEnvironment(lpPort,(LPSTR)&CurEnv,sizeof(PCLDEVMODE)) ||
	    lstrcmpi(lpDevice, CurEnv.dm.dmDeviceName))

	    MakeEnvironment(&CurEnv, lpDevice, lpPort, NULL);

	lpDm = &CurEnv; 		/* use current/constructed DEVMODE */

    } else

	lpDm = lpDevmode;		/* use caller's DEVMODE struct */


    /*	Return capability value(s) to caller based on requested index
     */

    switch (nIndex) {

	case DC_FIELDS:

	    rc = lpDm->dm.dmFields;
	    break;


	case DC_PAPERS: 		/* return value is # supported paper */
	case DC_PAPERSIZE:		/* sizes, and (maybe) list of papers */
					/* or sizes in 10ths of a mm	     */

	    if (nIndex == DC_PAPERS) {
		wp = (short FAR *)lpOutput;
		pp = NULL;
	    } else {
		pp = (LPPOINT)lpOutput;
		wp = NULL;
	    }

	    GetPaperBits(hLibInst,paperBits); /* supported papers by printer */

	    for (i = DMPAPER_FIRST, rc = 0; i <= DMPAPER_LAST; i++)
		if (paperBits[lpDm->paperInd] & Paper2Bit(i)) {
		    rc++;
		    if (wp)		/* DC_PAPERS index */
			*wp++ = i;
		    if (pp) {		/* DC_PAPERSIZE index */

			if (GetPaperFormat(&pf,hLibInst,lpDm->paperInd,i,
					   lpDm->dm.dmOrientation)) {
			    pp->x = (int) labdivc((long)pf.xPhys,
						  (long)HDPI,(long)254);
			    pp->y = (int) labdivc((long)pf.yPhys,
						  (long)VDPI,(long)254);
			    pp++;
			} else
			    rc--;	/* shouldn't happen, but... */
		    }
		}
	    break;


	case DC_BINS:			/* return value is # supported paper */
					/* bins, and (maybe) the list of bins*/
	    pcap = lpDm->prtCaps;
	    wp = (short FAR *)lpOutput;

	    /* following check _very_ similar to code in MergeEnvironment() */

	    for (i = DMBIN_FIRST, rc = 0; i <= DMBIN_LAST; i++)
		if (i == DMBIN_UPPER ||
		    ((i == DMBIN_LOWER) && (pcap & LOTRAY))    ||
		    ((i == DMBIN_MANUAL) && !(pcap & NOMAN))   ||
		    ((i == DMBIN_AUTO) && (pcap & AUTOSELECT)) ||
		    ((i == DMBIN_ENVELOPE) && (pcap & ANYENVFEED))) {
		    rc++;
		    if (wp)
			*wp++ = i;
		}
	    break;


	case DC_DUPLEX: 		/* return 0 for no duplex, 1 for yes */

	    rc = (lpDm->prtCaps & ANYDUPLEX) != 0;
	    break;


	case DC_SIZE:

	    rc = lpDm->dm.dmSize;
	    break;


	case DC_EXTRA:

	    rc = lpDm->dm.dmDriverExtra;
	    break;


	case DC_VERSION:

	    rc = lpDm->dm.dmSpecVersion;
	    break;


	case DC_DRIVER:

	    rc = lpDm->dm.dmDriverVersion;
	    break;

	default:

	    rc = SP_ERROR;
	    break;

    }

    DBGDevCap(("DeviceCapabilities() returning %ld\n",rc));

    UnlockSegment(-1);		    /* DS now free to be... somewhere else */

    return(rc);
}
