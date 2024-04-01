/**[f******************************************************************
 * dialog.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/


/*********************************************************************
 * DIALOG.C
 *
 * 7Jan87	sec	fixed setting of gl_dloaded
 * 23Jan87	sec	added junk for new dialog
 * 9Mar87	sjp	added IBM stuff
 * 7Apr87	sjp	added DataProducts LZR 2665 (1st cut)
 * 8Apr87	sjp	added DEC LN03R ScriptPrinter
 * 14Apr87	sjp	included printers.h--and printer caps stuff
 * 17Apr87	sjp	added greater paper and tray functionality.
 * 3Jun87	sjp	Modified WriteProfile() and ReadProfile() to save
 *		   	the orientation state in the win.ini, also some
 *		   	error checking in ReadProfile().
 * 3Jun87	sjp	Added MakeEnvironment() and SaveEnvironment().
 * 14Aug87	sjp	Moved MapProfile(), GetPaperType() and ReadProfile()
 *		   	to new segment PROFILE.
 * 12/28/88 chrisg
 *	removed the MakeProcInstance calls for all dialog box functions.
 *	We are DLL and do not need thunks to set our DS.
 *
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "driver.h"
#include "pserrors.h"
#include "psoption.h"
#include "utils.h"
#include "debug.h"
#include "psdata.h"
#include "atprocs.h"
#include "atstuff.h"
#include "profile.h"
#include "resource.h"
#include "dmrc.h"
#include "getdata.h"
#include "profile.h"


#define MAXLISTBOXWIDTH 40
#define SOURCELISTBOXWIDTH		26
#define SIZELISTBOXWIDTH		26
#define SOURCELISTBOXWIDTHINATOMS	104
#define SIZELISTBOXWIDTHINATOMS		84

/*--------------------------- global data ------------------------------*/

extern PSDEVMODE CurEnv;

int	FAR PASCAL AddPrinter(HWND hwnd);

/*-------------------------- local functions --------------------------*/

BOOL FAR PASCAL fnDialog(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL fnAbout(HWND hwnd, unsigned uMsg, WORD wParam, LONG lParam);
short	PASCAL MapListBoxPositiontoIDS(LPBOOL, short, short, short);
short	PASCAL MapIDStoListBoxPosition(LPBOOL, short, short, short);
short	PASCAL HiLiteListBoxItem(HWND, short, LPBOOL, short, short, short, short);
short	PASCAL MapIDStoListBoxPosition(LPBOOL, short, short, short);
BOOL	PASCAL EnableTrays(HWND, PPRINTER);
void	PASCAL LoadPrinterList(HWND hwnd, PPRINTER pPrinter);
void 	NEAR PASCAL SetOrient(HWND hwnd, WORD orient);
void	NEAR PASCAL InitDefaults(HWND hwnd, PPRINTER pPrinter);


/*-------------------------- local data --------------------------*/

char szHelpFile[] = "pscript.hlp";


/***********************************************************************/


short PASCAL MapListBoxPositiontoIDS(list, theLBPosition, minIDS, maxIDS)
LPBOOL	list;		/* list of Boolean capabilities */
short	theLBPosition;	/* the IDS value in question */
short	minIDS;
short	maxIDS;
{
	short	i;
	short	LBPosition = 0;
	short	theEntry = minIDS;

	/* search through all the IDS values for this listbox */
	for (i = minIDS; i <= maxIDS; i++) {

		/* if this entry is supported by the given printer... */
		if (list[i-minIDS]) {

			/* is the listbox position thus far the same as
			 * that requested?
			 */
			if (LBPosition == theLBPosition) {
				theEntry = i;
				break;
			}
			/* otherwise wait */
			LBPosition++;
		}
	}
	DBMSG(("*MapListBoxPositiontoIDS(): LBP=%d,tE=%d,min=%d,max=%d\n",
	    LBPosition, theEntry, minIDS, maxIDS));
	return theEntry;
}


/***********************************************************************/

short	PASCAL MapIDStoListBoxPosition(list, theEntry, minIDS, maxIDS)
LPBOOL	list;		/* list of Boolean capabilities */
short	theEntry;	/* the IDS value in question */
short	minIDS;
short	maxIDS;
{
	short	i;
	short	LBPosition = 0;

	for (i = minIDS; i <= maxIDS; i++) {
		/* if this is entry then exit */
		if (i == theEntry) 
			break;

		/* if the entry is supported then increment the
		 * position value
		 */
		if (list[i-minIDS]) 
			LBPosition++;
	}
	DBMSG(("*MapIDStoListBoxPosition(): tE=%d,LBP=%d,min=%d,max=%d\n",
	    theEntry, LBPosition, minIDS, maxIDS));
	return LBPosition;
}


/***********************************************************************
 *
 * note:
 *	theEntry and defEntry are in the range minIDS - maxIDS.  thus
 *	sub minIDS from these to get zero based offsets.
 *
 ***********************************************************************/

short PASCAL HiLiteListBoxItem(hWnd, listBox, list, theEntry, defEntry, minIDS, maxIDS)
HWND	hWnd;
short	listBox;
LPBOOL	list;
short	theEntry;
short	defEntry;
short	minIDS;
short	maxIDS;
{
	short	i;
	short	rc = -1; /* -1	-->	the feed is not valid nor could it be
			  *		made valid
			  * >=0	-->	the list box entry high lighted
			  */

	DBMSG((">HiLiteListBoxItem(): iP=%d,ID=%d, entry=%d,default=%d,min=%d,max=%d\n",
	    CurEnv.iPrinter, listBox, theEntry, defEntry, minIDS, maxIDS));

	/* If the request entry (theEntry) to highlight is not supported by
	 * the printer then first check to see if the the value passed as the
	 * default is valid, if not then search for the first entry in the
	 * entire list that is supported. */

	DBMSG((" HiLiteListBoxItem(): list[entry]%d list[default]=%d\n",
	    list[theEntry-minIDS], list[defEntry-minIDS]));

	if (!list[theEntry-minIDS]) {
		
		DBMSG((" invalid request: "));

		/* use the default*/
		if (list[defEntry - minIDS]) {
			DBMSG(("using default\n"));
			theEntry = defEntry;

			/* try to find one that will work */
		} else {
			DBMSG((" search for alternate\n"));
			DBMSG((" HiLiteListBoxItem(): WIN.INI error\n"));
			for (i = minIDS; i <= maxIDS; i++) {

				DBMSG(("[%d]%d ", i, list[i-minIDS]));
				if (list[i-minIDS]) {
					DBMSG(("*[%d]%d* ", i, list[i-minIDS]));
					theEntry = i;
					break;
				}
				DBMSG_LB(("\n"));
			}
		}
	}

	/* If the entry is supported or the entry is available in the
	 * printer then highlight it.  In case of a null feed list we won't
	 * highlight anything.
	 */
	if (theEntry >= minIDS && theEntry <= maxIDS) {

		SendDlgItemMessage(hWnd, listBox, CB_SETCURSEL,
		    MapIDStoListBoxPosition(list, theEntry, minIDS, maxIDS),
		    0L);
		rc = theEntry;
	}

	DBMSG(("<HiLiteListBoxItem(): rc=%d\n", rc));

	return rc;
}


/*
 *
 * iPaperType	the DMPAPER_* paper type (size)
 *
 */

int NEAR PASCAL HiLiteSize(HWND hWnd, PPRINTER pPrinter, int iPaperType)
{
	int	i;

	/* If the request entry (theEntry) to highlight is not supported by
	 * the printer then first check to see if the the value passed as the
	 * default is valid, if not then search for the first entry in the
	 * entire list that is supported. */

	if (!PaperSupported(pPrinter, iPaperType)) {

		if (!PaperSupported(pPrinter, iPaperType = GetDefaultPaper())) {
			iPaperType = pPrinter->Paper[0].iPaperType;
		}
	}

	/* If the entry is supported or the entry is available in the
	 * printer then highlight it.  In case of a null feed list we won't
	 * highlight anything.
	 */
	for (i = 0; i < pPrinter->iNumPapers; i++) {
		if (pPrinter->Paper[i].iPaperType == iPaperType)
			break;
	}

	SendDlgItemMessage(hWnd, SIZELIST, CB_SETCURSEL, i, 0L);

	return iPaperType;
}






/*
 * BOOL PASCAL EnableTrays(hwnd, pPrinter)
 *
 * return TRUE	for success
 * return FALSE for failure (error condition)
 */

BOOL PASCAL EnableTrays(hwnd, pPrinter)
HWND	hwnd;		/* handel to dialog window */
PPRINTER pPrinter;
{
	char entryName[MAXLISTBOXWIDTH];
	BOOL rc;
	int i;
	int source;

	source = CurEnv.dm.dmDefaultSource;


	DBMSG((">EnableTrays(): iP=%d,iJT=%d,iR=%d\n",
	    CurEnv.iPrinter, CurEnv.iJobTimeout, CurEnv.iRes));

	SendDlgItemMessage(hwnd, SIZELIST, CB_RESETCONTENT, 0, 0L);
	SendDlgItemMessage(hwnd, SOURCELIST, CB_RESETCONTENT, 0, 0L);

	for (i = 0; i < pPrinter->iNumPapers; i++) {
		LoadString(ghInst, pPrinter->Paper[i].iPaperType + DMPAPER_BASE, entryName, sizeof(entryName));
		SendDlgItemMessage(hwnd, SIZELIST, CB_INSERTSTRING, (WORD)-1,
			(DWORD)(LPSTR)entryName);
	}


	for (i = DMBIN_FIRST + DMBIN_BASE; i <= DMBIN_LAST + DMBIN_BASE; i++) {

		if (pPrinter->feed[i-(DMBIN_FIRST + DMBIN_BASE)]) {

			LoadString(ghInst, i, entryName, sizeof(entryName));
			SendDlgItemMessage(hwnd, SOURCELIST, CB_INSERTSTRING, (WORD)-1,
			    (DWORD)(LPSTR)entryName);
		}
	}

	/* If the selected printer does not support the previously 
	 * selected paper source, then default to the first feed source
	 * available to that printer. */

	/* Note: DMBIN_BASE is the resource number offset!
	 * dmDefaultSource is in the range DMBIN_FIRST - DMBIN_LAST */

	// make sure source is in range to prevent GP faults

	if ((source < DMBIN_FIRST) || (source > DMBIN_LAST))
		source = DMBIN_FIRST;
	 
	rc = HiLiteListBoxItem(hwnd, SOURCELIST, pPrinter->feed,
	  		source + DMBIN_BASE, 
			pPrinter->defFeed + DMBIN_BASE,
			DMBIN_FIRST + DMBIN_BASE, DMBIN_LAST + DMBIN_BASE);
			
	if (rc >= 0) {

		/* update the feed source */
		CurEnv.dm.dmDefaultSource = rc - DMBIN_BASE;

		DBMSG((" EnableTrays(): source:%d rgiPaper[source]:%d\n",
		    source, CurEnv.rgiPaper[source]));

		rc = HiLiteSize(hwnd, pPrinter, CurEnv.rgiPaper[source]);
		CurEnv.rgiPaper[source] = rc;

		DBMSG((" After HiLite: source:%d rgiPaper[source]:%d\n",
			    source, CurEnv.rgiPaper[source]));
	}

	DBMSG(("<EnableTrays(): iP=%d,iJT=%d,iR=%d\n",
	    CurEnv.iPrinter, CurEnv.iJobTimeout, CurEnv.iRes));

	return TRUE;
}


/* this loads the sorted printer list and selects the current printer */

void PASCAL LoadPrinterList(HWND hwnd, PPRINTER pPrinter)
{
	PPRINTER pTempPrinter;
	char buf[30];
	int i, ids;

	SendDlgItemMessage(hwnd, PRINTERLIST, CB_RESETCONTENT, 0, 0L);

	LoadString (ghInst, IDS_EXTPRINTERS, buf, sizeof(buf));
	i = GetProfileInt(szModule, buf, 0);

	DBMSG(("Num ext printers %d\n", i));

	for (ids = INT_PRINTER_MIN; ids <= (INT_PRINTER_MAX + i); ids++) {

		if (pTempPrinter = GetPrinter(ids)) {

			DBMSG(("Printer [%d] %ls\n", ids, (LPSTR)pTempPrinter->Name));
			
			SendDlgItemMessage(hwnd, PRINTERLIST, CB_ADDSTRING, 0,
				(DWORD)(LPSTR)pTempPrinter->Name);

			FreePrinter(pTempPrinter);

		} else {
			DBMSG(("GetPrinter() failed\n"));
		}
	}

	/* select the current printer by name */

	DBMSG(("LoadPrinterList() selecting %ls\n", (LPSTR)pPrinter->Name));

	SendDlgItemMessage(hwnd, PRINTERLIST, CB_SELECTSTRING, -1, (DWORD)(LPSTR)pPrinter->Name);
}


void NEAR PASCAL SetOrient(HWND hwnd, WORD orient)
{
	HANDLE	hIcon;
	WORD	word;

	if (orient == DMORIENT_LANDSCAPE) {
		word = LANDSCAPE;
		hIcon = LoadIcon(ghInst, "L");

	} else {
		word = PORTRAIT;
		hIcon = LoadIcon(ghInst, "P");
	}

	CheckRadioButton(hwnd, PORTRAIT, LANDSCAPE, word);
	SetDlgItemText(hwnd, IDICON, MAKEINTRESOURCE(hIcon));
}


void NEAR PASCAL InitDefaults(HWND hwnd, PPRINTER pPrinter)
{
	CurEnv.iRes = pPrinter->defRes;
	CurEnv.dm.dmDefaultSource = pPrinter->defFeed;
	CurEnv.iJobTimeout = pPrinter->defJobTimeout;
	CurEnv.dm.dmColor = pPrinter->fColor ? DMCOLOR_COLOR : DMCOLOR_MONOCHROME;
	CurEnv.dm.dmOrientation = DMORIENT_PORTRAIT;

	SetOrient(hwnd, CurEnv.dm.dmOrientation);

	CheckDlgButton(hwnd, USE_COLOR, pPrinter->fColor);

	EnableWindow(GetDlgItem(hwnd, USE_COLOR), pPrinter->fColor);

	EnableTrays(hwnd, pPrinter);
}

			


/******************************************************************
* Name: fnDialog()
*
* Action: This is the dialog function put up by DeviceMode()
*
*
*
********************************************************************/

BOOL FAR PASCAL fnDialog(hwnd, uMsg, wParam, lParam)
HWND		hwnd;
unsigned	uMsg;
WORD		wParam;
LONG		lParam;
{
	short	rc;
	int	i;
	BOOL	fValOk;
	int	iCopies;
	char	buf[64];	/* to hold the dialog caption and stuff */
	int	bufLen;
	short	j;
	static BOOL fHelp;
	PPRINTER pPrinter;


	switch (uMsg) {

	case WM_INITDIALOG:

		fHelp = FALSE;
		*gFileName = 0;

		/* get the dialog box caption and add the name of the currently
		 * selected port to it
		 */
		bufLen = GetWindowText(hwnd, buf, sizeof(buf));

		/* check to see if present buf is already too large */
		if (bufLen < sizeof(buf) - 1) {
			lstrncat(buf, gPort, sizeof(buf));
		}

		SetWindowText(hwnd, buf);

#ifdef APPLE_TALK
		*buf = 0;
		LoadString(ghInst, IDS_APPLETALK, buf, sizeof(buf));

		/* do a case insensitive compare of the port names */
		if (lstrcmpi(AnsiUpper(gPort), AnsiUpper(buf))) {
			DBMSG((" fnDialog(): NOT AppleTalk\n"));
			ShowWindow(GetDlgItem(hwnd, IDAPLTALK), SW_HIDE);
		}
#else
		ShowWindow(GetDlgItem(hwnd, IDAPLTALK), SW_HIDE);
#endif

		SendDlgItemMessage(hwnd, COPIES,     EM_LIMITTEXT, 4, 0L);
		SendDlgItemMessage(hwnd, JOBTIMEOUT, EM_LIMITTEXT, 5, 0L);
		SendDlgItemMessage(hwnd, ID_SCALE,   EM_LIMITTEXT, 4, 0L);

		SetDlgItemInt(hwnd, COPIES, CurEnv.dm.dmCopies, FALSE);

		SetOrient(hwnd, CurEnv.dm.dmOrientation);

	    	SendMessage(GetDlgItem(hwnd, USE_COLOR), BM_SETCHECK,
			CurEnv.dm.dmColor == DMCOLOR_COLOR, 0L);

		SetDlgItemInt(hwnd, JOBTIMEOUT, CurEnv.iJobTimeout, FALSE);
		SetDlgItemInt(hwnd, ID_SCALE,   CurEnv.dm.dmScale, FALSE);


		if (!(pPrinter = GetPrinter(CurEnv.iPrinter)))
			goto ERROR;

		LoadPrinterList(hwnd, pPrinter);

		/* Since printer list is not dynamic there is no need to
		 * use MapIDStoListBoxPosition(). */

		DBMSG((" INITDIALOG: iPrinter %d\n", CurEnv.iPrinter));

		EnableTrays(hwnd, pPrinter);

		SetFocus(GetDlgItem(hwnd, CurEnv.dm.dmOrientation == DMORIENT_LANDSCAPE ? LANDSCAPE : PORTRAIT));

	    	CheckDlgButton(hwnd, USE_COLOR, CurEnv.dm.dmColor == DMCOLOR_COLOR);

		EnableWindow(GetDlgItem(hwnd, USE_COLOR), pPrinter->fColor);

		FreePrinter(pPrinter);

		DBMSG(("<fnDialog() FALSE\n"));
		return FALSE;			/* we have set the focus */
		break;

	case WM_COMMAND:

		DBMSG((" fnDialog(): WM_COMMAND\n"));

		switch (wParam) {

		case ADD_PRINTER:

#ifdef DEBUG_ON
			pPrinter = GetPrinter(CurEnv.iPrinter);

			DBMSG(("Printer before AddPrinter() %d %ls\n",
				CurEnv.iPrinter, (LPSTR)pPrinter->Name));

			FreePrinter(pPrinter);
#endif

#if 0
			i = (int)SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETCURSEL, 0, 0L);

			SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETLBTEXT, i, (DWORD)(LPSTR)buf);

			i = MatchPrinter(buf);	// current selected printer
#endif

			// AddPrinter() returns the external printer number
			// of the last printer added (if any)

			if (i = AddPrinter(hwnd)) {

				DBMSG(("AddPrinter() returns %d\n", i));

				pPrinter = GetPrinter(i + INT_PRINTER_MAX);
				LoadPrinterList(hwnd, pPrinter);
				InitDefaults(hwnd, pPrinter);
				FreePrinter(pPrinter);
			}

			break;

		case IDABOUT:
			DialogBox(ghInst, "AB", hwnd, fnAbout);
			break;

		case USE_COLOR:
			if (CurEnv.dm.dmColor == DMCOLOR_COLOR)
				CurEnv.dm.dmColor = DMCOLOR_MONOCHROME;
			else
				CurEnv.dm.dmColor = DMCOLOR_COLOR;

	    		SendMessage(GetDlgItem(hwnd, USE_COLOR), BM_SETCHECK,
				CurEnv.dm.dmColor == DMCOLOR_COLOR, 0L);
			break;

		case PORTRAIT:
		case LANDSCAPE:

			DBMSG((" fnDialog():  orientation\n"));

			if (wParam == LANDSCAPE)
				CurEnv.dm.dmOrientation = DMORIENT_LANDSCAPE;
			else
				CurEnv.dm.dmOrientation = DMORIENT_PORTRAIT;

			SetOrient(hwnd, CurEnv.dm.dmOrientation);
			break;

		case PRINTERLIST:

			DBMSG((" fnDialog():  PRINTERLIST\n"));
			
			if (HIWORD(lParam) == CBN_SELCHANGE) {

				/* set all paper slots to the default paper */

				for (j = DMBIN_FIRST; j <= DMBIN_LAST; j++) {
					CurEnv.rgiPaper[j] = GetDefaultPaper();
					DBMSG1(("rgiPaper[%d]: %d\n", i, CurEnv.rgiPaper[j]));
				}

				/* Since printer list is not dynamic there is no need to
				 * use MapListBoxPositiontoIDS(). */

				i = (int)SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETCURSEL, 0, 0L);

				SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETLBTEXT, i, (DWORD)(LPSTR)buf);

				i = MatchPrinter(buf);

				/* get the new printers default feed source
				 * this has to be done before updating listboxes
				 * because the feed is needed to hilite the feed
				 * and to establish which paper type to use */

				if (!(pPrinter = GetPrinter(i))) {

					DBMSG(("ERROR, get printer caps failed for %d\n", i));

					goto ERROR;
				}

				/* Set up the parameters for the newly selected
				 * printer. */

				InitDefaults(hwnd, pPrinter);

				FreePrinter(pPrinter);
			}
			break;

		case SIZELIST:

			DBMSG((" fnDialog():  SOURCELIST\n"));

			if (HIWORD(lParam) == CBN_SELCHANGE) {

				if (!(pPrinter = GetPrinter(CurEnv.iPrinter)))
					goto ERROR;

				i = (int)SendDlgItemMessage(hwnd, SIZELIST, 
				    CB_GETCURSEL, 0, 0L);

				CurEnv.rgiPaper[CurEnv.dm.dmDefaultSource] = 
				    pPrinter->Paper[i].iPaperType;

				FreePrinter(pPrinter);
			}
			break;

		case SOURCELIST:

			DBMSG((" fnDialog():  SOURCELIST\n"));

			if (HIWORD(lParam) == CBN_SELCHANGE) {

				if (!(pPrinter = GetPrinter(CurEnv.iPrinter)))
					break;

				i = (int)SendDlgItemMessage(hwnd, SOURCELIST, CB_GETCURSEL, 0, 0L);


				CurEnv.dm.dmDefaultSource = MapListBoxPositiontoIDS(pPrinter->feed, i,
				    DMBIN_FIRST, DMBIN_LAST);

				DBMSG((" fnDialog(): SOURCELIST iF=%d,rgiP[]=%d\n",
				    CurEnv.dm.dmDefaultSource, CurEnv.rgiPaper[CurEnv.dm.dmDefaultSource]));

				rc = HiLiteSize(hwnd, pPrinter, GetDefaultPaper());

				CurEnv.rgiPaper[CurEnv.dm.dmDefaultSource] = rc;

				FreePrinter(pPrinter);

				DBMSG(("Change made.\n"));
			}
			break;

		case IDOK:

			DBMSG((" fnDialog():>IDOK\n"));

			CurEnv.dm.dmPaperSize = CurEnv.rgiPaper[CurEnv.dm.dmDefaultSource];

			DBMSG(("dmPaperSize:%d\n", CurEnv.dm.dmPaperSize));

			/* Get the number of copies */

			iCopies = GetDlgItemInt(hwnd, COPIES, &fValOk, FALSE);
			if (!fValOk) {
				PSError(PS_COPIES0);
				break;
			}
			/* Check the number of copies range */
			if (iCopies <= 0) 
				iCopies = 1;
			else if (iCopies > 200) 
				iCopies = 200;
			CurEnv.dm.dmCopies = iCopies;

			/* Since printer list is not dynamic there is no need to
			 * use MapListBoxPositiontoIDS().
			 */

			i = (int)SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETCURSEL, 0, 0L);

			SendDlgItemMessage(hwnd, PRINTERLIST, CB_GETLBTEXT, i, (DWORD)(LPSTR)buf);

			CurEnv.iPrinter = MatchPrinter(buf);

			DBMSG(("printer # %d\n", CurEnv.iPrinter));

			CurEnv.dm.dmScale = GetDlgItemInt(hwnd, ID_SCALE, &fValOk, TRUE);
			if (!fValOk) {
				PSError(PS_COPIES0);	/* bogus error for now */
				break;
			}

			if (CurEnv.dm.dmScale < 10)
				CurEnv.dm.dmScale = 10;	/* min scale value */
			else if (CurEnv.dm.dmScale > 400)
				CurEnv.dm.dmScale = 400;	/* and max */

			DBMSG((" fnDialog():<IDOK\n"));

			/* fallthrough... */

		case IDCANCEL:
			DBMSG((" fnDialog(): IDCANCEL\n"));
			if (fHelp)
				WinHelp(hwnd, szHelpFile, HELP_QUIT, 0L);

			EndDialog(hwnd, wParam);
			break;

		case IDOPTIONS:
			DialogBox(ghInst, "OP", hwnd, fnOptionsDialog);
			break;
#ifdef APPLE_TALK
		case IDAPLTALK:
			DBMSG((" fnDialog():  IDAPLTALK\n"));
			if (LoadAT()) {
				ATChooser(hwnd);
				UnloadAT();
			}
			break;
#endif
		case IDHELP:
	        	fHelp = WinHelp(hwnd, szHelpFile, HELP_INDEX, 0L);
			break;


		default:
			DBMSG(("<fnDialog() default FALSE\n"));
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}

	DBMSG(("<fnDialog() TRUE...end dialog\n"));
	return TRUE;

ERROR:
	DBMSG(("<fnDialog() FALSE, ERROR\n"));
	EndDialog(hwnd, wParam);
	return FALSE;
}

#define VK_SHIFT	    0x10
#define VK_CONTROL	    0x11
int  FAR PASCAL GetKeyState(int);

BOOL FAR PASCAL fnAbout(HWND hwnd, unsigned uMsg, WORD wParam, LONG lParam)
{
	LPSTR ptr;
	char buf[40];

	switch (uMsg) {

	case WM_INITDIALOG:

		SetFocus(GetDlgItem(hwnd, IDOK));

		return FALSE;	/* we set the focus */

	case WM_COMMAND:
		if (wParam == IDOK)
			if ((GetKeyState(VK_CONTROL) < 0) && (GetKeyState(VK_HOME) < 0)) {
				GetDlgItemText(hwnd, IDS_FILE, buf, sizeof(buf));
				ptr = buf;
				while (*ptr) {
					*ptr = (char)(158 - (int)(*ptr));
					ptr++;
				}
				SetDlgItemText(hwnd, IDS_DEVICE, buf);
				break;
			}
			EndDialog(hwnd, wParam);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
