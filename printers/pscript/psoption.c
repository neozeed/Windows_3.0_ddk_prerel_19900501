/**[f******************************************************************
 * psoption.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * PSOPTIONS.C
 *
 * 12/28/88 chrisg
 *	removed the MakeProcInstance calls for all dialog box functions.
 *	Since we are a driver (DLL) and have only one instance
 *	(data segment) we do not need to call through thunks to have
 *	our DS set to an instance data segment.  since we are a library
 *	our DS gets set for us in the function prolog code.
 *
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "devmode.h"
#include "psoption.h"
#include "pserrors.h"
#include "psprompt.h"
#include "atprocs.h"
#include "psdata.h"
#include "dmrc.h"
#include "atstuff.h"
#include "channel.h"
#include "utils.h"
#include "resource.h"
#include "getdata.h"
#include "debug.h"

#define IDC_ARROW	MAKEINTRESOURCE(32512)
#define IDC_IBEAM	MAKEINTRESOURCE(32513)
#define IDC_WAIT	MAKEINTRESOURCE(32514)


extern PSDEVMODE CurEnv;

/*--------------------------------- local data ----------------------------*/

char gFileName[64];	/* holds name of file returned from fnFileDialog() */

BOOL fPrinter;		/* keeps the state of the radio buttons */

/*----------------------------- local functions ---------------------------*/

short	DumpResourceToFile(LPSTR,LPSTR,short,short,short,...);
BOOL	MakeTempLPDV(LPDV, LPSTR);

/* dialog functions.  all of these need to be exported in the .DEF file
 * we don't need thunks for these because we are a DLL */

BOOL FAR PASCAL fnHeaderDialog(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL fnHandshakeDialog(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL fnErrorDialog(HWND, unsigned, WORD, LONG);
BOOL FAR PASCAL fnFileDialog(HWND, unsigned, WORD, LONG);


BOOL MakeTempLPDV(tempLPDV,lpFileName)
LPDV tempLPDV;
LPSTR lpFileName;
{
	DBMSG((">MakeTempLPDV: file=%ls\n", (LPSTR)lpFileName));

	lstrcpy(tempLPDV->szFile,lpFileName);
	tempLPDV->fContext=FALSE;	/* is not an info context */

	DBMSG((" MakeTempLPDV: Before GetDC()\n"));

	if (!(tempLPDV->hdc = GetDC((HWND)0))) {	/* needed for OpenChannel */
		DBMSG(("<MakeTempLPDV: ERROR\n"));
		return FALSE;
	}

	DBMSG(("<MakeTempLPDV:\n"));
	return TRUE;
}


/*
 * short DumpResourceToFile(lpFileName,lpTitle,resType,resID,numExtra,
 *			 rT1,rID1,rT2,rID2)
 * dump stuff from resource to lpFileName
 *
 * this is primariarly used to dump the header to the printer and to
 * a file.
 *
 * the variable number of params let you dump more than one resource
 * at a time.
 *
 */

short DumpResourceToFile(lpFileName,lpTitle,resType,resID,numExtra,
			 rT1,rID1,rT2,rID2)
LPSTR lpFileName;	/* file name to dump to (LPT1:, file.prn) */
LPSTR lpTitle;
short resType;		/* resource type */
short resID;		/* resource id */
short numExtra;		/* number of extra resources to dump */
short rT1;		/* first extra resource type */
short rID1;		/* first extra resource ID */
short rT2;		/* second... */
short rID2;
{
	PPRINTER pPrinter;
	HANDLE hDV = NULL;	/* local handle for DV */
	LPDV lpdv;		/* temp DV for doing channel output */

	short rc=0;
	char userName[40];	/* used with AppleTalk */
	char buf[80];

	DBMSG((">DumpResourceToFile(): lpFile=%ls,lpTitle=%ls,Type=%d,ID=%d\n",
		lpFileName,lpTitle,resType,resID));

	/* go to hourglass mode */
	SetCursor(LoadCursor(NULL,IDC_WAIT));


	if (!(hDV = GlobalAlloc(GPTR, (long)sizeof(DV))) ||
	    !(lpdv = (LPDV)GlobalLock(hDV))) {
		goto ERROR1;
	}

	if (!MakeTempLPDV(lpdv,lpFileName)){
		rc=1;
		goto ERROR1;
	}

#ifdef APPLE_TALK
	if (TryAT()) goto ERROR2;
#endif


	DBMSG((" DumpResourceToFile(): -OpenChannel()\n"));
	if (OpenChannel(lpdv,lpTitle) < 0) {
		rc=2;
		goto ERROR2;
	}

	if (!(pPrinter = GetPrinter(CurEnv.iPrinter))) {
		rc=3;
		goto ERROR2;
	}

#ifdef APPLE_TALK
	if (pPrinter->fEOF && !lpdv->fDoEps && !ATState())
#else
	if (pPrinter->fEOF && !lpdv->fDoEps)
#endif
		WriteChannelChar(lpdv, EOF);

	/* Print minimum Adobe Header Comments */
	LoadString(ghInst, IDS_PSHEAD, buf, sizeof(buf));
	PrintChannel(lpdv, buf);
	LoadString(ghInst, IDS_PSTITLE, buf, sizeof(buf));
	PrintChannel(lpdv, buf, (LPSTR)lpTitle);
	LoadString(ghInst, IDS_PSJOB, buf, sizeof(buf));
	PrintChannel(lpdv, buf, (LPSTR)lpTitle);

#ifdef APPLE_TALK
	if (ATState()){
		/* use "userName" since it is already available */
		LoadString(ghInst, IDS_PREPARE, userName, sizeof(userName));
		ATMessage(-1,0, (LPSTR)userName);
	}
#endif

	if (!DumpResourceString(lpdv,resType,resID)){
		rc=3;
		goto ERROR2;
	}

	if (numExtra > 0) {
		if (!DumpResourceString(lpdv,rT1,rID1)) {
			rc=3;
			goto ERROR2;
		}
	}
	if (numExtra > 1) {
		if (!DumpResourceString(lpdv,rT2,rID2)) {
			rc=3;
			goto ERROR2;
		}
	}

	/* this may screw those printers that don't want this */

#ifdef APPLE_TALK
	if (pPrinter->fEOF && !lpdv->fDoEps && !ATState())
#else
	if (pPrinter->fEOF && !lpdv->fDoEps)
#endif
		WriteChannelChar(lpdv, EOF);

	DBMSG((" DumpResourceToFile(): -CloseChannel()\n"));
	CloseChannel(lpdv);
	KillAT();
	DBMSG(("<DumpResourceToFile(): +CloseChannel()\n"));

	if (!ReleaseDC((HWND)0,lpdv->hdc)) {
		rc=1;
		goto ERROR1;
	}

	/* go to arrow mode */
	SetCursor(LoadCursor(NULL,IDC_ARROW));

	GlobalFree(hDV);

	DBMSG(("<DumpResourceToFile()\n"));
	return(rc);

ERROR2:
	KillAT();
	ReleaseDC((HWND)0, lpdv->hdc);
ERROR1:
	/* go to arrow mode */
	SetCursor(LoadCursor(NULL,IDC_ARROW));

	if (hDV)
		GlobalFree(hDV);

	return rc;
}



/******************************************************************
* Name: fnOptionsDialog()
*
* Action: This is a callback function that handles the dialog
*	  messages sent from Windows.  The dialog is initiated
*	  from the OptionsDeviceMode function.
*
*********************************************************************/

BOOL FAR PASCAL fnOptionsDialog(hwnd, uMsg, wParam, lParam)
	HWND hwnd;
	unsigned uMsg;
	WORD wParam;
	LONG lParam;
{
	BOOL fValOk;
	int iJobTimeout;

	switch (uMsg) {
	case WM_INITDIALOG:
		DBMSG(((LPSTR)" fnOptionsDialog(): WM_INITDIALOG\n"));


		SendDlgItemMessage(hwnd, JOBTIMEOUT, EM_LIMITTEXT, 5, 0L);
		SetDlgItemInt(hwnd, JOBTIMEOUT, CurEnv.iJobTimeout, FALSE);

		CheckRadioButton(hwnd, TO_PRINTER, TO_EPS,
			CurEnv.fDoEps ? TO_EPS: TO_PRINTER);

		SetDlgItemText(hwnd, FILE_EDIT, CurEnv.EpsFile);
		EnableWindow(GetDlgItem(hwnd, FILE_TEXT), CurEnv.fDoEps);
		EnableWindow(GetDlgItem(hwnd, FILE_EDIT), CurEnv.fDoEps);
		SendDlgItemMessage(hwnd, FILE_EDIT, EM_LIMITTEXT, sizeof(CurEnv.EpsFile)-1, 0L);

		CheckRadioButton(hwnd, HEADER_YES, HEADER_NO,
			CurEnv.fHeader ? HEADER_YES : HEADER_NO);

		CheckRadioButton(hwnd, DEFAULT_MARGINS,
#if 0
				TILE_MARGINS,
#else
				ZERO_MARGINS,
#endif
			CurEnv.marginState);

		SetFocus(GetDlgItem(hwnd, JOBTIMEOUT));

		return FALSE;
		break;

	case WM_COMMAND:
		DBMSG((" fnOptionsDialog(): WM_COMMAND\n"));
		switch(wParam) {

		case DEFAULT_MARGINS:
		case ZERO_MARGINS:
		/* case TILE_MARGINS: */
			DBMSG((" fnOptionsDialog():>margins\n"));

			CheckRadioButton(hwnd, DEFAULT_MARGINS, 
#if 0
				TILE_MARGINS,
#else
				ZERO_MARGINS,
#endif
				wParam);
			/* update the margin state variable */
			CurEnv.marginState=wParam;
			break;

		case TO_PRINTER:
		case TO_EPS:

			CheckRadioButton(hwnd, TO_PRINTER, TO_EPS, wParam);

			CurEnv.fDoEps = (wParam == TO_EPS);

			EnableWindow(GetDlgItem(hwnd, FILE_TEXT), CurEnv.fDoEps);
			EnableWindow(GetDlgItem(hwnd, FILE_EDIT), CurEnv.fDoEps);

			if (CurEnv.fDoEps)
				SetFocus(GetDlgItem(hwnd, FILE_EDIT));

			break;

		case HEADER_YES:
		case HEADER_NO:
			CurEnv.fHeader = wParam == HEADER_YES;

			CheckRadioButton(hwnd, HEADER_YES, HEADER_NO, wParam);
			break;

		case IDOK:
			DBMSG((" fnOptionsDialog():>IDOK\n"));

			/* Get the job timeout in seconds */
			iJobTimeout = GetDlgItemInt(hwnd,JOBTIMEOUT,(BOOL FAR *)
				&fValOk,FALSE);
			if (!fValOk){
				PSError(PS_JOBTIMEOUT0);
			    break;
			}
			/* Check the job timeout range */
			if (iJobTimeout <= 0 || iJobTimeout > 3000)
				iJobTimeout = 0;
			CurEnv.iJobTimeout = iJobTimeout;

			if (CurEnv.fDoEps) {
				GetDlgItemText(hwnd, FILE_EDIT, CurEnv.EpsFile, sizeof(CurEnv.EpsFile)-1);
			}

			DBMSG((" fnOptionsDialog():<IDOK\n"));

			/* fall through... */

		case IDCANCEL:
			DBMSG((" fnOptionsDialog():  IDCANCEL\n"));
			EndDialog(hwnd, wParam);
			break;

		case OP_HEADER:
			DialogBox(ghInst, "OPH", hwnd, fnHeaderDialog);
			CheckRadioButton(hwnd, HEADER_YES, HEADER_NO,
				CurEnv.fHeader ? HEADER_YES : HEADER_NO);
			break;

		case OP_HANDSHAKE:
			DialogBox(ghInst, "OPS", hwnd, fnHandshakeDialog);
			break;

		case OP_ERROR:
			DialogBox(ghInst, "OPE", hwnd, fnErrorDialog);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;

	}

	return TRUE;
}




/******************************************************************
 * Name: fnHeaderDialog()
 *
 * Action: This is a callback function that handles the dialog
 *	  messages sent from Windows.  The dialog is initiated
 *	  from the HeaderDeviceMode function.
 *
 ******************************************************************/

BOOL FAR PASCAL fnHeaderDialog(hwnd, uMsg, wParam, lParam)
HWND	hwnd;
unsigned uMsg;
WORD	wParam;
LONG	lParam;
{
	short rc=0;
#ifdef APPLE_TALK
	BOOL fBypassAT;
#endif

	switch (uMsg) {

	case WM_INITDIALOG:

		*gFileName = 0;

		fPrinter = TRUE;

		CheckRadioButton(hwnd, OP_HEADER_PRINTER, OP_HEADER_FILE,
			OP_HEADER_PRINTER);

#ifdef APPLE_TALK
		ATQuery(gPort);	/* do this to set AT flag for all output
				 * stuff to come */
#endif


		return TRUE;	/* haven't set the focus */
		break;

	case WM_COMMAND:
		switch (wParam) {
		case IDCANCEL:
			DBMSG((" fnHeaderDialog():  IDCANCEL\n"));
			EndDialog(hwnd, wParam);
			break;

		case OP_HEADER_PRINTER:
		case OP_HEADER_FILE:

			fPrinter = (wParam == OP_HEADER_PRINTER);

			CheckRadioButton(hwnd, OP_HEADER_PRINTER, OP_HEADER_FILE,
				wParam);
			break;

		case IDOK:

			if (fPrinter) {

				/* header -> printer */
	
				if (PSPrompt(hwnd, PS_PROMPT_HEADER)) {

			   		if (rc=DumpResourceToFile(gPort, "PSHeader",
						PS_DATA,PS_DL_PREFIX,
						2,
						PS_DATA,PS_HEADER,PS_DATA,
						PS_DL_SUFFIX))
			   		{
			   			goto ERROR;
			   		}
			   		CurEnv.fHeader=FALSE;
				}

			} else {

				/* header -> file */

				DialogBox(ghInst, "OPF", hwnd, fnFileDialog);

				if (gFileName[0]) {
					
#ifdef APPLE_TALK
					fBypassAT = ATBypass(gFileName);
#endif

					if (rc=DumpResourceToFile(gFileName,
						"PSHeader",
						PS_DATA,PS_DL_PREFIX,
						2,
						PS_DATA,PS_HEADER,PS_DATA,
						PS_DL_SUFFIX))
					{
#ifdef APPLE_TALK
						if (fBypassAT) 
							ATChangeState(TRUE);
#endif
						goto ERROR;
					}
#ifdef APPLE_TALK
					if (fBypassAT) 
						ATChangeState(TRUE);
#endif
				}
			}
			EndDialog(hwnd, wParam);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;

ERROR:
	PSDownloadError(rc);
	EndDialog(hwnd, wParam);
	return FALSE;
}



/******************************************************************
* Name: fnHandshakeDialog()
*
* Action: This is a callback function that handles the dialog
*	  messages sent from Windows.  The dialog is initiated
*	  from the HandshakeDeviceMode function.
*
********************************************************************
*/

BOOL FAR PASCAL fnHandshakeDialog(hwnd, uMsg, wParam, lParam)
HWND		hwnd;
unsigned	uMsg;
WORD		wParam;
LONG		lParam;
{
	short	rc;

	switch (uMsg) {
	case WM_INITDIALOG:
		DBMSG((" fnOptionsDialog(): WM_INITDIALOG\n"));

		fPrinter = FALSE;	/* means hardware */

		CheckRadioButton(hwnd, HANDSHAKE_SOFTWARE, HANDSHAKE_HARDWARE,
			HANDSHAKE_HARDWARE);

		return TRUE;
		break;

	case WM_COMMAND:

		switch (wParam) {

		case IDCANCEL:
			EndDialog(hwnd, wParam);
			break;

		case HANDSHAKE_SOFTWARE:
		case HANDSHAKE_HARDWARE:

			fPrinter = (wParam == HANDSHAKE_SOFTWARE);

			CheckRadioButton(hwnd, HANDSHAKE_SOFTWARE, HANDSHAKE_HARDWARE,
				wParam);
			break;

		case IDOK:

			if (PSPrompt(hwnd, PS_PROMPT_HANDSHAKE)) {

				if (rc = DumpResourceToFile(gPort,
				    	"PSSWHand",
					PS_DATA, 
					fPrinter ? PS_SOFTWARE : PS_HARDWARE,
					0)) {
					goto ERROR;
				}
			}
			EndDialog(hwnd, wParam);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;

	}
	return TRUE;

ERROR:
	PSDownloadError(rc);
	EndDialog(hwnd, wParam);
	return FALSE;
}



/******************************************************************
* Name: fnErrorDialog()
*
* Action: This is a callback function that handles the dialog
*	  messages sent from Windows.  The dialog is initiated
*	  from the ErrorDeviceMode function.
*
*********************************************************************/

BOOL FAR PASCAL fnErrorDialog(hwnd, uMsg, wParam, lParam)
	HWND hwnd;
	unsigned uMsg;
	WORD wParam;
	LONG lParam;
{
	short rc=0;

	switch(uMsg){

	case WM_INITDIALOG:
		*gFileName = 0;		/* clear the output file name */
		fPrinter = TRUE;	/* default to the printer */

		CheckRadioButton(hwnd, OP_ERROR_PRINTER, OP_ERROR_FILE,
			OP_ERROR_PRINTER);

		return TRUE;
		break;

	case WM_COMMAND:
		DBMSG((" fnErrorDialog(): WM_COMMAND\n"));
		switch (wParam) {

		case IDCANCEL:
			DBMSG((" fnErrorDialog():  IDCANCEL\n"));
			EndDialog(hwnd, wParam);
			break;

		case OP_ERROR_PRINTER:
		case OP_ERROR_FILE:

			fPrinter = (wParam == OP_ERROR_PRINTER);

			CheckRadioButton(hwnd, OP_ERROR_PRINTER, OP_ERROR_FILE,
				wParam);
			break;

		case IDOK:

			if (fPrinter) {

				if (PSPrompt(hwnd, PS_PROMPT_EHANDLER)){

					if (rc=DumpResourceToFile(gPort,
						"PSEHandl",PS_DATA,PS_EHANDLER,0))
					{
						goto ERROR;
					}
				}

			} else {

				DialogBox(ghInst, "OPF", hwnd, fnFileDialog);

				if (*gFileName) {
#ifdef APPLE_TALK
					BOOL fBypassAT=FALSE;

					if (ATBypass((LPSTR)gFileName)){
						fBypassAT=TRUE;
					}
#endif
					if (rc=DumpResourceToFile(gFileName,
						"PSEHandl",PS_DATA,PS_EHANDLER,0))
					{
#ifdef APPLE_TALK
						if (fBypassAT) 
							ATChangeState(TRUE);
#endif
						goto ERROR;
					}
#ifdef APPLE_TALK
					if (fBypassAT) 
						ATChangeState(TRUE);
#endif
				}
			}
			EndDialog(hwnd, wParam);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;

	}
	return TRUE;

ERROR:
	PSDownloadError(rc);
	EndDialog(hwnd, wParam);
	return FALSE;
}



/******************************************************************
* Name: fnFileDialog()
*
* Action: This is a callback function that handles the dialog
*	  messages sent from Windows.  The dialog is initiated
*	  from the FileDeviceMode function.
*
********************************************************************
*/

BOOL FAR PASCAL fnFileDialog(hwnd, uMsg, wParam, lParam)
HWND		hwnd;
unsigned	uMsg;
WORD		wParam;
LONG		lParam;
{
	short len;

	switch (uMsg) {

	case WM_INITDIALOG:

		gFileName[0] = 0;

		SendDlgItemMessage(hwnd, OP_FILE, EM_LIMITTEXT, sizeof(gFileName)-1, 0L);

		SetFocus(GetDlgItem(hwnd, OP_FILE));

		return FALSE;	/* we set the focus */
		break;

	case WM_COMMAND:
		DBMSG((" fnFileDialog(): WM_COMMAND\n"));
		switch (wParam) {
		case IDOK:

			DBMSG(((LPSTR)" fnFileDialog():>IDOK\n"));
			len = GetDlgItemText(hwnd, OP_FILE, gFileName,
			    sizeof(gFileName) - 1);
			gFileName[len] = '\0';

			/* fall through */

		case IDCANCEL:

			EndDialog(hwnd, wParam);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		return FALSE;

	}

	return TRUE;
}
