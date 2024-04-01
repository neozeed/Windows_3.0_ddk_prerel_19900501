/**********************************************************************
 * devcap.c -
 *
 * Copyright (C) 1989 Microsoft Corporation.  All rights reserved.
 *
 *********************************************************************/

/*
 *   2-14-89	jimmat	Created as part of the Driver Initialization
 *			changes.
 *
 *   3-13-89	chrisg	cloned. adding to the PS driver.
 */

#include "pscript.h"
#include "driver.h"
#include "psdata.h"
#include "profile.h"
#include "defaults.h"
#include "getdata.h"
#include "utils.h"
#include "control2.h"
#include "debug.h"


/*  DeviceCapabilities -
 *
 *  This function returns the capabilities of the device driver to
 *  the caller.
 */

DWORD FAR PASCAL DeviceCapabilities(
	LPSTR lpDevice, 
	LPSTR lpPort, 
	WORD nIndex,
	LPSTR lpOutput, 
	LPPSDEVMODE lpDevmode)
{

	DWORD rc;
	LPPOINT pp;
	int	i;
	short	FAR *wp;
	PSDEVMODE CurEnv;
	LPPSDEVMODE lpdm;
	PPRINTER pPrinter;
	LPPAPER pPaper;

	DBMSG(("DeviceCapabilities(%lp,%lp,%d,%lp,%lp)\n",
	    lpDevice, lpPort, nIndex, lpOutput, lpDevmode));

	/* If the caller didn't pass in a PSDEVMODE pointer, then get/build
	 * a current one
	 */

	if (!lpDevmode) {

		MakeEnvironment(lpDevice, lpPort, &CurEnv, NULL);

		lpdm = &CurEnv;		/* use current/constructed PSDEVMODE */

	} else

		lpdm = lpDevmode;	/* use caller's PSDEVMODE struct */


	/* Return capability value(s) to caller based on requested index
	 */

	pPrinter = GetPrinter(lpdm->iPrinter);


	switch (nIndex) {

	case DC_FIELDS:

		rc = lpdm->dm.dmFields;
		break;


	case DC_PAPERS:   	/* return value is # supported paper */
	case DC_PAPERSIZE:	/* sizes, and (maybe) list of papers */
				/* or sizes in 10ths of a mm	     */

		pPaper = GetPaperMetrics(pPrinter);

		if (!pPaper)
			break;

		if (nIndex == DC_PAPERS) {
			wp = (short FAR * )lpOutput;
		  	pp = NULL;
		} else {

			pp = (LPPOINT)lpOutput;
			wp = NULL;
		}

		rc = (DWORD)pPrinter->iNumPapers;
		for (i = 0; i < (int)rc; i++) {
			int iPaper;

			iPaper = pPrinter->Paper[i].iPaperType;
			if (wp)		/* DC_PAPERS index */
				*wp++ = iPaper;
			if (pp) {	/* DC_PAPERSIZE index */
				pp->x = Scale(pPaper[iPaper].cxPaper, 254, DEFAULTRES);
				pp->y = Scale(pPaper[iPaper].cxPaper, 254, DEFAULTRES);
				pp++;
			}
		}

		LocalFree((HANDLE)pPaper);
		break;


	case DC_BINS:		/* return value is # supported paper */
				/* bins, and (maybe) the list of bins*/
		wp = (short FAR * )lpOutput;

		/* following check _very_ similar to code in MergeEnvironment() */

		for (i = 0, rc = 0; i < NUMFEEDS; i++)
			if (pPrinter->feed[i]) {
				rc++;
				if (wp)
					*wp++ = i + DMBIN_FIRST;
			}
		break;


	case DC_SIZE:

		rc = lpdm->dm.dmSize;
		break;


	case DC_EXTRA:

		rc = lpdm->dm.dmDriverExtra;
		break;


	case DC_VERSION:

		rc = lpdm->dm.dmSpecVersion;
		break;


	case DC_DRIVER:

		rc = lpdm->dm.dmDriverVersion;
		break;

	default:

		rc = -1;
		break;

	}

	FreePrinter(pPrinter);

	DBMSG(("DeviceCapabilities() returning %ld\n", rc));

	return rc;
}


