/**[f******************************************************************
 * devmode.c - 
 *
 * Copyright (C) 1988-1989 Aldus Corporation, Microsoft Corporation.
 * Copyright (C) 1989 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   devmode.c   ******************************/
/*
 *  Procs for handling driver-specific dialog.
 *
 *   1-12-89	jimmat	Most DeviceMode local data is now allocated via
 *			GlobalAlloc() instead of statically allocated in DGROUP.
 *   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
 *   1-17-89	jimmat	Added PCL_* entry points to lock/unlock data seg.
 *   1-25-89	jimmat	Use global hLibInst instead of GetModuleHandle().
 *   1-26-89	jimmat	Added dynamic link to Font Installer library.
 *   2-07-89	jimmat	Driver Initialization changes.
 *   2-20-89	jimmat	Driver/Font Installer use same WIN.INI section (again)!
 *   2-21-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
 *   2-24-89	jimmat	Removed parameters from lp_enbl() & lp_disable().
 *   3-13-89	chrisg	cloning over to the PS driver.
 *   3-16-89	chrisg	seems to be working...
 */

#include "pscript.h"
#include <winexp.h>
#include "driver.h"
#include "profile.h"
#include "utils.h"
#include "debug.h"
#include "getdata.h"
#include "dmrc.h"
#include "control2.h"
#include "version.h"

#define MMPERIN 254	/* mm per inch */

BOOL DevModeBusy = FALSE;

PSDEVMODE CurEnv, OldEnv;
char gPort[20];

extern HANDLE ghInst;


BOOL FAR PASCAL fnDialog(HWND, unsigned, WORD, LONG);


void MergeEnvironment(LPPSDEVMODE, LPPSDEVMODE);


/***********************************************************************
		       E X T D E V I C E M O D E
 ***********************************************************************/

/*
 * NOTE: be very careful of any static data used/set by this function
 * (and the functions it calls) because this routine may be reentered
 * by different applications.	For example, one app may have caused the
 * device mode dialog to appear (DM_PROMPT), and another app might
 * request a copy of the current DEVMODE settings (DM_COPY).  Multiple
 * calls for DM_PROMPT and/or DM_UPDATE processing are not allowed.
 *
 * whatever lpProfile points to better be locked because things
 * may move around while the dialog box is up.
 */

short FAR PASCAL ExtDeviceMode(
	HWND hWnd, 
	HANDLE hInst, 
	LPPSDEVMODE lpdmOutput,
	LPSTR lpDeviceName, 
	LPSTR lpPort, 
	LPPSDEVMODE lpdmInput,
	LPSTR lpProfile, 
	WORD Mode)
{
	short	rc, exclusive;
	char DeviceName[80];		/* keep copy if callers DS moves */

	DBMSG(("ExtDeviceMode(%d,%d,%lp,%lp,%lp,%lp,%lp,%d)\n",
	    hWnd, hInst, lpdmOutput, lpDeviceName, lpPort, lpdmInput,
	    lpProfile, Mode));
	DBMSG(("     DeviceName = ->%ls<-   Port = ->%ls<-\n",
	    lpDeviceName ? lpDeviceName : (LPSTR) "NULL",
	    lpPort ? lpPort : (LPSTR) "NULL"));
	DBMSG(("     Profile    = ->%ls<-\n",
	    lpProfile ? lpProfile : (LPSTR) "NULL"));
	DBMSG(("     Mode = %ls %ls %ls %ls\n", Mode & DM_UPDATE ? (LPSTR)"UPDATE" : (LPSTR)"",
	    Mode & DM_COPY ? (LPSTR)"COPY" : (LPSTR)"", Mode & DM_PROMPT ? (LPSTR)"PROMPT" : 
	    (LPSTR)"", Mode & DM_MODIFY ? (LPSTR)"MODIFY" : (LPSTR)""));

	lstrcpy(DeviceName, lpDeviceName);


	/* Mode == 0 is a request for the full size of the DEVMODE structure */

	if (!Mode) {
		DBMSG(("ExtDeviceMode returning size: %d\n", sizeof(PSDEVMODE)));
		return sizeof(PSDEVMODE);
	}


	/* Okay, there is some real work to do.  Make sure we haven't been
           (re)entered more than once to UPDATE or PROMPT (possibly by two or
           more applications), then allocate and lock down our data areas */

	exclusive = Mode & (DM_UPDATE | DM_PROMPT);

	if (DevModeBusy) {
		if (exclusive)
			return -1;
	} else
		DevModeBusy = exclusive;

	/* Initialize a few items in the DevMode data area */

	lstrcpy(gPort, lpPort);

	/* Get a copy of the environment--build one if the user gave us a
           private .INI file, or there is no current environment, or it doesn't
           match our device */

#if 0
	lstrcpy(CurEnv.dm.dmDeviceName, lpDeviceName);

	if (lpProfile || 
	    !GetEnvironment(lpPort, (LPSTR)&CurEnv, sizeof(PSDEVMODE)) || 
	    lstrcmpi(lpDeviceName, CurEnv.dm.dmDeviceName))
#endif

		MakeEnvironment(DeviceName, gPort, &CurEnv, lpProfile);


	/* Keep a copy of the current environment, changes may get written to
           a .INI file before we're finished. */

	OldEnv = CurEnv;


	/* If the user passed in a DEVMODE structure, merge it with the current
           environment before going futher */

	if ((Mode & DM_MODIFY) && lpdmInput) {

		/* if this is one of ours we need to get some stuff */

		if (lpdmInput->dm.dmSize == sizeof(DEVMODE) &&
		    lpdmInput->dm.dmDriverVersion == DRIVER_VERSION &&
		    lpdmInput->dm.dmSpecVersion == GDI_VERSION &&
		    lpdmInput->dm.dmDriverExtra == sizeof(PSDEVMODE)-sizeof(DEVMODE) &&
		    !lstrcmpi(lpDeviceName, lpdmInput->dm.dmDeviceName)) {

			DBMSG((" Input Env is one of ours, using it's extra data\n"));

			lmemcpy((LPSTR)&CurEnv   + sizeof(DEVMODE),
		    		(LPSTR)lpdmInput + sizeof(DEVMODE),
			    	sizeof(PSDEVMODE) - sizeof(DEVMODE));
		} else {
			DBMSG((" Input Env not one of ours\n"));
		}

		MergeEnvironment(&CurEnv, lpdmInput);
	}


	/* Throw-up the device mode dialog box if the caller wants us to
           prompt the user for any changes */

	if (Mode & DM_PROMPT) {

		rc = DialogBox(ghInst, "DM", hWnd, fnDialog);
	} else
		rc = IDOK;	/* didn't prompt, but we still give the okay return */


	/* If the caller wants a copy of the resulting environment,
           give it to 'em */

	if ((Mode & DM_COPY) && lpdmOutput) {

		*(LPPSDEVMODE)lpdmOutput = CurEnv;	/* check rc == IDOK ? */
	}

#ifdef LOCAL_DEBUG
	dumpDevMode(&CurEnv);
#endif

	/* Finally, update the default environment if everything is okay so far
           (and the user didn't Cancel the dialog box), and the caller wants us
           to do so */

	if ((Mode & DM_UPDATE) && rc == IDOK) {

		SaveEnvironment(DeviceName, gPort, &CurEnv, &OldEnv, lpProfile, TRUE, TRUE);
	}


	if (exclusive)			/* since there can only be 1 exclusive	   */
		DevModeBusy = FALSE;	/* invocation, no longer "busy" if this it */

	DBMSG(("ExtDeviceMode() returning %d\n", rc));

	return rc;
}


/***********************************************************************
			 D E V I C E M O D E
 ***********************************************************************/

int FAR PASCAL DeviceMode(HANDLE hWnd, HANDLE hInst, LPSTR lpDevice, LPSTR lpPort)
{

	return (ExtDeviceMode(hWnd, hInst, NULL, lpDevice, lpPort, NULL, NULL,
	    DM_PROMPT | DM_UPDATE) == IDOK);
}



/***********************************************************************
		    M E R G E  E N V I R O N M E N T
 ***********************************************************************/

/*  Merge source and destination environments into the destination. */

void MergeEnvironment(LPPSDEVMODE lpDest, LPPSDEVMODE lpSrc)
{
	PPRINTER pPrinter;
	short	value;
	long	Fields = lpSrc->dm.dmFields;
	PPAPER pPaper, pP;


	/* portrait/landscape */

	if (Fields & DM_ORIENTATION)
		if ((value = lpSrc->dm.dmOrientation) == DMORIENT_PORTRAIT || 
		    value == DMORIENT_LANDSCAPE)
			lpDest->dm.dmOrientation = value;

	/* Copies?	We can do that! */

	if (Fields & DM_COPIES)
		lpDest->dm.dmCopies = lpSrc->dm.dmCopies;

	/* PrintQuality?  No problem! */

	/* The allowed range of paper sizes also depends of printer type */

	pPrinter = GetPrinter(lpDest->iPrinter);

	/* if they specify a paper width and length search for one that
	 * might match and set the dmPaperSize if that size is found */

	if (Fields & (DM_PAPERLENGTH | DM_PAPERWIDTH)) {

		for (pP = pPaper = GetPaperMetrics(pPrinter); pP->iPaper; pP++)

			if (pP->cxPaper == Scale(lpSrc->dm.dmPaperWidth, MMPERIN, 100) &&
			    pP->cyPaper == Scale(lpSrc->dm.dmPaperLength, MMPERIN, 100))
				break;

		if (pP->iPaper)
			lpDest->dm.dmPaperSize = pP->iPaper;

		LocalFree((HANDLE)pPaper);
	}


	if (Fields & DM_PAPERSIZE) {

		if (pPrinter->feed[lpDest->dm.dmDefaultSource - DMBIN_FIRST])
			lpDest->dm.dmPaperSize = lpSrc->dm.dmPaperSize;
	}

	if ((Fields & DM_DEFAULTSOURCE) && (lpDest->dm.dmFields & DM_DEFAULTSOURCE)) {

		if (pPrinter->feed[lpDest->dm.dmDefaultSource - DMBIN_FIRST])
			lpDest->dm.dmDefaultSource = lpSrc->dm.dmDefaultSource;
	}

	FreePrinter(pPrinter);

	/* copy over the scale value */

	if (Fields & DM_SCALE)
		lpDest->dm.dmScale = lpSrc->dm.dmScale;

	if (Fields & DM_COLOR)
		lpDest->dm.dmColor = lpSrc->dm.dmColor;

#ifdef DEBUG
	DBMSG(("MergeEnvironment: merged PSDEVMODE follows:\n"));
	dumpDevMode(lpDest);
#endif

}


#ifdef DEBUG

/***********************************************************************
		     D E B U G	  R O U T I N E S
 ***********************************************************************/

LOCAL void dumpDevMode(LPPSDEVMODE lpEnv) 
{

	DBMSG(("     dmDeviceName: %ls\n", (LPSTR)lpEnv->dm.dmDeviceName));
	DBMSG(("     dmSpecVersion: %4xh\n", lpEnv->dm.dmSpecVersion));
	DBMSG(("     dmDriverVersion: %4xh\n", lpEnv->dm.dmDriverVersion));
	DBMSG(("     dmSize: %d\n", lpEnv->dm.dmSize));
	DBMSG(("     dmDriverExtra: %d\n", lpEnv->dm.dmDriverExtra));
	DBMSG(("     dmFields: %8lxh\n", lpEnv->dm.dmFields));
	DBMSG(("     dmOrientation: %d\n", lpEnv->dm.dmOrientation));
	DBMSG(("     dmPaperSize: %d\n", lpEnv->dm.dmPaperSize));
	DBMSG(("     dmCopies: %d\n", lpEnv->dm.dmCopies));
	DBMSG(("     dmDefaultSource: %d\n", lpEnv->dm.dmDefaultSource));
	DBMSG(("     dmPrintQuality: %d\n", lpEnv->dm.dmPrintQuality));
	DBMSG(("     dmColor: %d\n", lpEnv->dm.dmColor));
	DBMSG(("     dmDuplex: %d\n", lpEnv->dm.dmDuplex));
	DBMSG(("     iScale: %d\n", lpEnv->iScale));

}


#endif
