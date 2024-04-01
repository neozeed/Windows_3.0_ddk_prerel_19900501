/**[f******************************************************************
 * profile.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * PROFILE.C
 *
 * 14Aug87 sjp	Moved MapProfile(), GetPaperType() and ReadProfile()
 *		from segment RESET.
 *
 * 88Jan13 chrisg	established MakeEnv and SaveEnv as means to
 *			change load/save envs and got rid of global
 *			devmode usage.
 *
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "devmode.h"
#include "utils.h"
#include "debug.h"
#include "resource.h"
#include "defaults.h"
#include "psdata.h"
#include "profile.h"
#include "pserrors.h"
#include "getdata.h"
#include "dmrc.h"
#include "version.h"
#include "control2.h"
		     
/* temporary until windows.h gets real */
int  PASCAL GetInt( LPSTR, LPSTR, int, LPSTR );
int  PASCAL GetString( LPSTR, LPSTR, LPSTR, LPSTR, int, LPSTR );
BOOL PASCAL WriteString( LPSTR, LPSTR, LPSTR, LPSTR );


/* these should go in win.h */
int     FAR PASCAL GetPrivateProfileInt( LPSTR, LPSTR, int, LPSTR );
int     FAR PASCAL GetPrivateProfileString( LPSTR, LPSTR, LPSTR, LPSTR, int, LPSTR );
BOOL    FAR PASCAL WritePrivateProfileString( LPSTR, LPSTR, LPSTR, LPSTR );


/*--------------------------- local functions ------------------------*/

int	PASCAL GetPaperType(LPSTR, int, LPSTR);
void	PASCAL WriteProfileInt(LPSTR, LPSTR, int, LPSTR);
void	PASCAL WriteProfile(LPSTR, LPPSDEVMODE, LPPSDEVMODE, LPSTR);
void	PASCAL ReadProfile(LPSTR, LPPSDEVMODE, LPSTR, LPSTR);


/* use these until private profile gets fixed */

int PASCAL WriteString(LPSTR sec, LPSTR key, LPSTR buf, LPSTR pro )
{
	if (pro)
		return WritePrivateProfileString(sec, key, buf, pro);
	else
		return WriteProfileString(sec, key, buf);
}

int PASCAL GetString(LPSTR sec, LPSTR key, LPSTR def, LPSTR buf, int size, LPSTR pro )
{
	if (pro)
		return GetPrivateProfileString(sec, key, def, buf, size, pro);
	else
		return GetProfileString(sec, key, def, buf, size);
}


int PASCAL GetInt(LPSTR sec, LPSTR key, int def, LPSTR pro )
{
	if (pro)
		return GetPrivateProfileInt(sec, key, def, pro);
	else
		return GetProfileInt(sec, key, def);
}



int FAR PASCAL MatchPrinter(LPSTR lpName)
{
	PPRINTER pPrinter;
	char buf[40];
	int i;
	int num_ext;

	LoadString (ghInst, IDS_EXTPRINTERS, buf, sizeof(buf));
	num_ext = GetProfileInt(szModule, buf, 0);

	for (i = INT_PRINTER_MIN; i <= (INT_PRINTER_MAX + num_ext); i++) {

		if (pPrinter = GetPrinter(i)) {
		
			if (!lstrcmpi(pPrinter->Name, lpName)) {
				FreePrinter(pPrinter);
				return i;
			}

			FreePrinter(pPrinter);
		}
	}

	return 0;	/* failure, did not match */
}


/****************************************************************************
 * void FAR PASCAL MakeEnvironment(lszDevType, lszFile, lpdm, lpProfile)
 *
 * fill the PSDEVMODE struct with a copy of the enviornment for *lszFile
 * and *lszDevType.  Then init this struct with the values from win.ini
 * 
 * in:
 *	hinst		driver instance handle
 *	lszDevType	device name "PostScript Printer" or
 *			"IBM Personal PP II", etc.
 *	lszFile		file or port name ("COM1:")
 *	lpProfile	file to read to (win.ini if NULL)
 *
 * out:
 *	lpdm	filled with initialized devmode struct
 *
 * returns:
 *	TRUE		if new env read from win ini
 *	FALSE		PSCRIPT env already existed
 *
 ****************************************************************************/

#define STR_EQ(s1, s2) (lstrcmpi((s1),(s2)) == 0)

BOOL FAR PASCAL MakeEnvironment(lszDevType, lszFile, lpdm, lpProfile)
LPSTR	lszDevType;
LPSTR	lszFile;
LPPSDEVMODE lpdm;
LPSTR	lpProfile;
{
	BOOL result;

	DBMSG(("\n>MakeEnv(%ls %ls %lp %ls)\n",
	    lszDevType, lszFile, lpdm, lpProfile));

	/* copy the module name into the first part of the env incase
	 * lszFile is the null port.  in this case GetEnv uses the first
	 * string (our module name) for the env search */

	lstrcpy(lpdm->dm.dmDeviceName, szModule);

	/* if (env doesn't exists || does exists and doesn't belong to us) */

	if ((GetEnvironment(lszFile, (LPSTR)lpdm, sizeof(PSDEVMODE)) == 0) ||
	    (lstrcmpi(lszDevType, lpdm->dm.dmDeviceName) != 0))
	{
	     	/* read env from win.ini */
		DBMSG(("\n MakeEnv read win.ini\n"));

		ReadProfile(lszFile, lpdm, lszDevType, lpProfile);

		/* tag as ours */

		lstrcpy(lpdm->dm.dmDeviceName, lszDevType);

		result = TRUE;		/* was created */

	} else

		result = FALSE;		/* alread existed */

	lpdm->dm.dmSize = sizeof(DEVMODE);
	lpdm->dm.dmSpecVersion = GDI_VERSION;
	lpdm->dm.dmDriverVersion = DRIVER_VERSION;
	lpdm->dm.dmDriverExtra = sizeof(PSDEVMODE) - sizeof(DEVMODE);

	lpdm->dm.dmFields = DM_ORIENTATION	|
			    DM_PAPERSIZE	|
			    DM_PAPERLENGTH	|
			    DM_PAPERWIDTH	|
			    DM_SCALE		|
			    DM_COPIES		|
			    DM_DEFAULTSOURCE	|
			    DM_COLOR;

	DBMSG(("\n<MakeEnv() lszFile:%ls\n", lszFile));
	return result;
}


/****************************************************************************
 * void FAR PASCAL SaveEnvironment(lszDevType, lszFile, lpdm,
 *				   lpOrigDM, lpProfilefWriteProfile, fSendMsg)
 *
 * save the enviornemnt for lszDevType and lszFile defined by lpdm.
 * the new enviornemnt is saved for the current session by GDI with
 * SetEnviornment and optionally saved to win.ini. 
 * 
 * in:
 *	hinst		driver instance handle
 *	lszDevType	device type ("PostScript Printer")
 *	lszFile		file or port name ("COM1:")
 *	lpdm		enviornment to save
 *	lpProfile	profile to write to
 *	lpOrigDM	original devmode used to minimize writing to win.ini
 *			this may be NULL if fWriteProfile is FALSE
 *	fWriteProfile	write the new env to win.ini or not
 *	fSendMsg	set a message to all apps indicating devmode change
 *
 ****************************************************************************/

void FAR PASCAL SaveEnvironment(lszDevType, lszFile, lpdm, lpOrigDM,
				lpProfile, fWriteProfile, fSendMsg)
LPSTR		lszDevType;
LPSTR		lszFile;
LPPSDEVMODE	lpdm;
LPPSDEVMODE	lpOrigDM;
LPSTR		lpProfile;
BOOL		fWriteProfile;	/* TRUE write profile to win.ini */
BOOL		fSendMsg;	/* TRUE send message to all windows */
{

	SetEnvironment(lszFile, (LPSTR)lpdm, sizeof(PSDEVMODE));

	if (fWriteProfile)
		WriteProfile(lszFile, lpdm, lpOrigDM, lpProfile);

	if (fSendMsg)
		SendMessage(-1, WM_PSDEVMODECHANGE, 0, (LONG)(LPSTR)lszDevType);
}



/**********************************************************************
 * Name: ReadProfile()
 *
 * Action:
 *	Read the device mode parameters from win.ini and fill lpdm
 *	structure with the results.  the win.ini section used is 
 *	"Post Script," concatenated with lszFile.  lszFile is a port or an
 *	output file name.  This info gets saved in win.ini when the user
 *	makes changes as well. the first time this is called we try
 *	to match the printer name (lszDevType) to those we know
 *	to auto configure to a certain printer.
 * 
 *	things not read from win.ini are set to resonable defaults.
 * 
 * note:
 *	some error checking code is to ensure that user changes to
 *	win.ini are not bogus.
 *
 **********************************************************************/

void PASCAL ReadProfile(lszFile, lpdm, lszDevType, lpProfile)
LPSTR		lszFile;	/* port used to form win.ini key */
LPPSDEVMODE	lpdm;		/* Far ptr to the device mode info */
LPSTR		lszDevType;
LPSTR		lpProfile;
{
	int	i;
	char	idsBuf[40];
	char	szKey[60];
	PPRINTER pPrinter;
	PPAPER pPaper, pP;
	BOOL rc = TRUE;

	lpdm->dm.dmCopies = 1;
	lpdm->dm.dmScale = 100;
	lpdm->EpsFile[0] = 0;	/* start off with no EPS output file name */
	lpdm->fDoEps = FALSE;

	SetKey(szKey, lszFile);

	DBMSG((">ReadProfile(): file=%ls\n", (LPSTR)lszFile));


	/* get the printer number from the "device=printer name"
	 * section in win.ini */

	/* "device" is a resource string */
	LoadString (ghInst, IDS_DEVICE, idsBuf, sizeof(idsBuf));

	/* see if we know what printer this is */

	lpdm->iPrinter = GetInt(szKey, idsBuf, 0, lpProfile);

	if (lpdm->iPrinter == 0) {

		/* this is the first time for this printer on this
		 * port.  so we check the device name to see if it is
		 * one we know. */

		lpdm->iPrinter = MatchPrinter(lszDevType);
	}

	pPrinter = NULL;

	if ((lpdm->iPrinter < INT_PRINTER_MIN) || 
	    (lpdm->iPrinter > EXT_PRINTER_MAX) ||
	    !(pPrinter = GetPrinter(lpdm->iPrinter)))
		lpdm->iPrinter = DEFAULT_PRINTER;

	if (pPrinter)
		FreePrinter(pPrinter);

	DBMSG((" Printer # %d\n", lpdm->iPrinter));

	/* now get the basic printer caps for this printer */

	if (!(pPrinter = GetPrinter(lpdm->iPrinter))) {
		DBMSG(("GetPrinter() FAILED!\n"));
		return;
	}

	lpdm->iRes = pPrinter->defRes;


	LoadString(ghInst, IDS_PAPERSOURCE, idsBuf, sizeof(idsBuf));

	lpdm->dm.dmDefaultSource = GetInt(szKey, idsBuf, pPrinter->defFeed, lpProfile);

	if (lpdm->dm.dmDefaultSource < DMBIN_FIRST || 
		lpdm->dm.dmDefaultSource > DMBIN_LAST)
		lpdm->dm.dmDefaultSource = pPrinter->defFeed;

	lpdm->dm.dmPaperSize = GetPaperType(szKey, lpdm->dm.dmDefaultSource, lpProfile);

	/* in case user messed up WIN.INI value */
	if (!PaperSupported(pPrinter, lpdm->dm.dmPaperSize))
		lpdm->dm.dmPaperSize = GetDefaultPaper();


	/* search for the paper metrics for the current printer and
	 * set the devinit fields accordingly */

	pPaper = GetPaperMetrics(pPrinter);

	if (pPaper) {

		for (pP = pPaper; pP->iPaper; pP++)
			if (pP->iPaper == lpdm->dm.dmPaperSize)
				break;

		if (pP->iPaper) {
			lpdm->dm.dmPaperWidth  = Scale(pP->cxPaper, 254, 100);
			lpdm->dm.dmPaperLength = Scale(pP->cyPaper, 254, 100);

			DBMSG(("Paper size (in mm) %d %d\n", 
				lpdm->dm.dmPaperWidth, lpdm->dm.dmPaperLength)); 
		}

		LocalFree((HANDLE)pPaper);
	} else {
		DBMSG(("GetPaperMetrics() failed!!\n"));
	}

	LoadString (ghInst, IDS_JOBTIMEOUT, idsBuf, sizeof(idsBuf));
	lpdm->iJobTimeout = GetInt(szKey, idsBuf, DEFAULTJOBTIMEOUT, lpProfile);

	/* keep positive */
	if (lpdm->iJobTimeout < 0)
		lpdm->iJobTimeout = DEFAULTJOBTIMEOUT;

	LoadString (ghInst, IDS_ORIENTATION, idsBuf, sizeof(idsBuf));
	lpdm->dm.dmOrientation = GetInt(szKey, idsBuf, DEFAULTORIENTATION, lpProfile);

#if 0
	// binary stuff not supported anymore.  we now compress bitmaps
	// if we want to do this we should redefine readhexstring and readstring
	// in the header.  and then do all binary output in bitblt and strchblt

	LoadString (ghInst, IDS_BINARYIMAGE, idsBuf, sizeof(idsBuf));
	lpdm->fBinary = GetInt(szKey, idsBuf, FALSE, lpProfile);
#endif

	if (pPrinter->fColor) {
		LoadString (ghInst, IDS_COLOR, idsBuf, sizeof(idsBuf));
		lpdm->dm.dmColor = GetInt(szKey, idsBuf, DEFAULT_COLOR, lpProfile);
	} else
		lpdm->dm.dmColor = DMCOLOR_MONOCHROME;


	for (i = DMBIN_FIRST; i <= DMBIN_LAST; i++) {

		lpdm->rgiPaper[i] = GetPaperType(szKey, i, lpProfile);

		DBMSG((" ReadProfile(): [%d]%d\n", i, lpdm->rgiPaper[i]));
	}

	/* get the header flag */

	LoadString (ghInst, IDS_HEADER, idsBuf, sizeof(idsBuf));
	lpdm->fHeader = GetInt(szKey, idsBuf, TRUE, lpProfile);

	LoadString (ghInst, IDS_MARGINS, idsBuf, sizeof(idsBuf));
	lpdm->marginState = GetInt(szKey, idsBuf, DEFAULT_MARGINS, lpProfile);

	if (lpdm->marginState < DEFAULT_MARGINS || lpdm->marginState > ZERO_MARGINS)
		lpdm->marginState = DEFAULT_MARGINS;

	DBMSG((" ReadProfile(): iPrinter=%d dmDefaultSource=%d dmPaperSize=%d iRes=%d iJT=%d dmOrient=%d\n",
	    lpdm->iPrinter, lpdm->dm.dmDefaultSource, lpdm->dm.dmPaperSize, lpdm->iRes,
	    lpdm->iJobTimeout, lpdm->dm.dmOrientation));

	FreePrinter(pPrinter);

	DBMSG(("<ReadProfile() \n"));
}


void PASCAL WriteProfileInt(lszApp, lszKey, iVal, lpProfile)
LPSTR	lszApp;
LPSTR	lszKey;
int	iVal;
LPSTR	lpProfile;
{
	char	sz[10];

	wsprintf(sz, "%d", iVal);

	WriteString(lszApp, lszKey, sz, lpProfile);
}


/*******************************************************************
 * Name: WriteProfile()
 *
 * Action: Write the device mode parameters out to win.ini.
 *
 *******************************************************************/

void PASCAL WriteProfile(lszFile, lpdm, lpOrigDM, lpProfile)
LPSTR		lszFile;	/* Ptr to the com port's file name */
LPPSDEVMODE	lpdm;		/* new PSDEVMODE to write */
LPPSDEVMODE	lpOrigDM;	/* old copy to save unnecessary writes */
LPSTR		lpProfile;
{
	char	szKey[64];
	char	sz[64];
	char	idsBuf[40];
	char	buf[10];	/* used for wsprintf */
	int	iFeed;
	PPRINTER pPrinter;
	PPRINTER pOrigPrinter;

	DBMSG((">WriteProfile()\n"));

	SetKey(szKey, lszFile);

	if (!(pPrinter = GetPrinter(lpdm->iPrinter)))
		return;

	if (!(pOrigPrinter = GetPrinter(lpOrigDM->iPrinter))) {
		FreePrinter(pPrinter);
		return;
	}

	if (lpdm->iPrinter != lpOrigDM->iPrinter) {

		LoadString(ghInst, IDS_DEVICE, idsBuf, sizeof(idsBuf));
		DBMSG_WP((" WriteProfile(): %ls %ls\n",
			(LPSTR)idsBuf, (LPSTR)sz));
		WriteProfileInt(szKey, idsBuf, lpdm->iPrinter, lpProfile);
	}

	if (lpdm->dm.dmOrientation != lpOrigDM->dm.dmOrientation) {

		DBMSG_WP((" WriteProfile(): %ls %d\n",
		    (LPSTR)idsBuf, lpdm->dm.dmOrientation == DMORIENT_LANDSCAPE));

		LoadString(ghInst, IDS_ORIENTATION, idsBuf, sizeof(idsBuf));
		WriteProfileInt(szKey, idsBuf, lpdm->dm.dmOrientation, lpProfile);
	}

	if (lpdm->dm.dmColor != lpOrigDM->dm.dmColor) {
		LoadString(ghInst, IDS_COLOR, idsBuf, sizeof(idsBuf));
		WriteProfileInt(szKey, idsBuf, lpdm->dm.dmColor, lpProfile);
	}


	if (lpdm->iJobTimeout != lpOrigDM->iJobTimeout) {
		LoadString(ghInst, IDS_JOBTIMEOUT, idsBuf, sizeof(idsBuf));
		WriteProfileInt(szKey, idsBuf, lpdm->iJobTimeout, lpProfile);
	}

	/* save the paper source, i.e. upper, lower or ... */
	if (lpdm->dm.dmDefaultSource != lpOrigDM->dm.dmDefaultSource) {
		DBMSG_WP((" WriteProfile(): %ls %d\n", (LPSTR)idsBuf, lpdm->iFeed));

		LoadString(ghInst, IDS_PAPERSOURCE, idsBuf, sizeof(idsBuf));
		WriteProfileInt(szKey, idsBuf, lpdm->dm.dmDefaultSource, lpProfile);
	}

	if (lpdm->fHeader != lpOrigDM->fHeader) {

		LoadString(ghInst, IDS_HEADER, sz, sizeof(sz));
		WriteProfileInt(szKey, sz, lpdm->fHeader, lpProfile);

		DBMSG_WP((" WriteProfile(): %ls %ls\n", (LPSTR)idsBuf, (LPSTR)sz));
	}

	if (lpdm->marginState != lpOrigDM->marginState) {

		LoadString(ghInst, IDS_MARGINS, sz, sizeof(sz));
		WriteProfileInt(szKey, sz, lpdm->marginState, lpProfile);

		DBMSG_WP((" WriteProfile(): marginState %d\n", lpdm->marginState));
	}

	/* For each feed available to the printer save the paper
	 * associated with it.
	 */
	LoadString(ghInst, IDS_PAPERX, buf, sizeof(buf));

	for (iFeed = DMBIN_FIRST; iFeed <= DMBIN_LAST; iFeed++) {

		if (pPrinter->feed[iFeed - DMBIN_FIRST]) {
			wsprintf(idsBuf, buf, iFeed);
	
			DBMSG_WP((" WriteProfile(): [%d]%d %ls\n",
			    iFeed, lpdm->rgiPaper[iFeed], (LPSTR)idsBuf));

			DBMSG_WP((" WriteProfile(): %ls %ls\n",
			    (LPSTR)idsBuf, (LPSTR)sz));

			WriteProfileInt(szKey, idsBuf, lpdm->rgiPaper[iFeed], lpProfile);

		} else if (pOrigPrinter->feed[iFeed - DMBIN_FIRST] && !pPrinter->feed[iFeed - DMBIN_FIRST]) {

			/* null it out in case it isn't used by the new printer */

			WriteString(szKey, idsBuf, szNull, lpProfile);
		}
	}

	FreePrinter(pOrigPrinter);
	FreePrinter(pPrinter);

	DBMSG_WP(("<WriteProfile()\n"));
	return;
}


/********************************************************************/

short	FAR PASCAL GetDefaultPaper()
{
	if (isUSA())
		return(DMPAPER_LETTER);	/* US LETTER */
	else
		return(DMPAPER_A4);	/* DIN A4 */
}



/***********************************************************************
* Name: GetPaperType()
*
* Action: Get the paper type from win.ini for feeder iFeed
*
* return the default
*
************************************************************************/

int PASCAL GetPaperType(szKey, iFeed, lpProfile)
LPSTR	szKey;
int	iFeed;      /* The tray from which the paper is being fed */
LPSTR	lpProfile;
{
	char	idsBuf[10];
	char	feed[10];
	int	val;

	LoadString (ghInst, IDS_PAPERX, feed, sizeof(feed));

	wsprintf(idsBuf, feed, iFeed);

	val = GetInt(szKey, idsBuf, GetDefaultPaper(), lpProfile);

	return val;
}



/*
 * int FAR PASCAL GetExternPrinter(int iExtPrinterNum)
 *
 * returns file handel for external printer (-1 if it doesn't exist)
 * 
 * uses the section in win.in that looks like this:
 *
 *	[PSCRIPT]
 *	External Printers=1
 *	printer1=FILENAME
 *
 * in:
 *	i	external printer index (1 - N)
 *
 * return:
 *	fh	of external printer (.WPD) file
 */

int FAR PASCAL GetExternPrinter(int i)
{
	OFSTRUCT os;
	char	temp[40];
	char	idsBuf[12];
	char	szPrinter[20];

	DBMSG(("GetExternPrinter() %d\n", i));

	LoadString (ghInst, IDS_PRINTER, idsBuf, sizeof(idsBuf));
	wsprintf(temp, idsBuf, i);

	GetProfileString(szModule, temp, szNull, szPrinter, sizeof(szPrinter));

	DBMSG(("GetExternPrinter %d %ls\n", i, (LPSTR)szPrinter));

	lstrcpy(temp, szPrinter);
 	lstrcat(temp, szRes);

 	return OpenFile(temp, &os, OF_READ);
}


