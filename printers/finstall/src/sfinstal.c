/**[f******************************************************************
 * sfinstal.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   sfinstal.c   *****************************/
/*
 *  SFInstall:  Main program module for soft font installer.
 *
 * 08 jan 89	peterbe	Now preload strings for MyDialogBox()
 *				in InstallSoftFont().
 * 02 oct 89	peterbe	Help button calls WinHelp(..HELP_INDEX..) now.
 *
 * 27 sep 89	peterbe	Added InitStatusLine() to indicate whether soft fonts
 *			are handled.
 *
 * 13 sep 89	peterbe	Moved WEP() to WEP.A
 *
 * 23 aug 89	peterbe	Added HELP support (see HelpWasCalled, SF_HELP,etc.)
 *
 * 22 aug 89	peterbe	Remember to unlock segment on error returns.
 *
 * 08 aug 89	peterbe	Add neg. return codes for lack of or bad LZ module.
 *
 * 01 aug 89	peterbe	Add lpOpenFile for LZOpenFile() call.
 *			Add WEP().
 *
 * 28 jul 89	peterbe	Load LZEXPAND.DLL and get addresses for the 4
 *			main functions used here.
 *
 * 17 jul 89	peterbe	Add global gPrintClass, set from InstallSoftFont()
 *			parameter (for LaserJet/DeskJet/DJ Plus selection)
 *
 * 28 jun 89	peterbe	(1.29 next). Renamed SoftFontInstall() to
 *			InstallSoftFont() and added parameter to indicate
 *			what driver's calling this, to identify printer class.
 *
 * 11 may 89	peterbe	(1.23 build next).  Making Edit button always
 *			enabled.
 *
 * 05 may 89	peterbe	Hide SF_ADD_RIGHT before calling GetPort() to fix
 *			redraw problem if port dialog moves.
 *
 * 04 may 89	peterbe	If DrawItem() returns FALSE, call DefWindowProc(),
 *			to redraw/clear the focus caret for a selection.
 *
 * 15 apr 89	peterbe	About box evoked by button instead of sys menu now.
 *
 * 27 mar 89	peterbe	Add call to GetListboxBitmaps().  Passes hDb now.
 *
 * 25 mar 89	peterbe	DrawItem() returns BOOL value now.
 *
 * 24 mar 89	peterbe	Moved routines created yesterday to SFOWNER.C,
 *			new module.  Just have calls to them here.
 * 23 mar 89	peterbe	Adding code for WM_MEASUREITEM and WM_DRAWITEM,
 *			for user-draw listbox items.
 * 21 mar 89	peterbe Removed copyright string SF_COPYRIGHT from main
 *			dialog box.
 * 07 mar 89	peterbe	Making SF_EDIT pushbutton permanently enabled.
 *
 * 02 mar 89	peterbe	Changed tabs to 8 spaces.  Removed SF_YESEDIT and
 *			SF_YESPORT sys. menu items, and related code.
 *   1-25-89    jimmat  Changed SoftFontInstall() parameters to not pass
 *          the module Instance handle--use hLibInst instead.
 *   1-26-89    jimmat  Adjustments do to changes in the resource file.
 *   2-20-89    jimmat  Font Installer/Driver use same WIN.INI section (again)!
 */

#ifdef SFDEBUG
#define	DEBUG
#endif

#include "nocrap.h"
#undef NOCTLMGR
#undef NOWINMESSAGES
#undef NOSHOWWINDOW
#undef NOMEMMGR
#undef NOVIRTUALKEYCODES
#undef NOMENUS
#undef NOSYSCOMMANDS
#undef NOMSG
#undef NOMB
#include "windows.h"
#include "sfinstal.h"
#include "dlgutils.h"
#include "neededh.h"
#include "resource.h"
#include "sfdir.h"
#define	NOBLDDESCSTR
#include "sfutils.h"
#include "strings.h"
#include "sffile.h"
#include "sfadd.h"
#include "sferase.h"
#include "sfcopy.h"
#include "sfedit.h"
#include "sfdownld.h"


/*  DEBUG switches
 */
#define	DBGdlgfn(msg)	    DBMSG(msg)

#define	LOCAL static


/*  This seems to prevent keyboard shortcuts from getting
 *  confused after updating the status line.
 */
#define	KERPLUNK(hDB) SetFocus(GetDlgItem(hDB, SF_STATUS))


/*  Forward references
 */

/* main dialog function */
BOOL FAR PASCAL	SFdlgFn(HWND, unsigned,	WORD, LONG);

/* Functions in SFOWNER.C for dialog function */
void FAR PASCAL FillMeasureItem(LPMEASUREITEMSTRUCT);
BOOL FAR PASCAL DrawItem(HWND, LPDRAWITEMSTRUCT);
void FAR PASCAL GetListboxBitmaps(HWND);

int gSF_FLAGS =	0;
int gFSvers = 0;

LOCAL WORD gDlgState = 0;
LOCAL HANDLE gHLBleft =	0;
LOCAL HANDLE gHLBright = 0;
LOCAL BOOL gIgnoreMessages = FALSE;
LOCAL char gLPortNm[32];
LOCAL char gRPortNm[32];
LOCAL char gModNm[32];
LOCAL BOOL bCopyPort = FALSE;
LOCAL BOOL HelpWasCalled = FALSE;

extern HANDLE hLibInst;
int gPrintClass;		// 0: CLASS_LASERJET
				// 1: CLASS_DESKJET
				// 2: CLASS_DESJET_PLUS

BOOL gNoSoftFonts = FALSE;	// if TRUE, printer has only cartridges.

// strings for MyDialogBox() (in DLGUTILS.C).  These must be preloaded.
// Initialized with English text here, overwritten with localized text.
char szFontInstall[35] = "Printer Font Installer";
char szNotEnough[60] = "Not enough memory to bring up dialog box.";

// Handle and FAR function pointers for the LZEXPAND.DLL library.
HANDLE hLZ;
FARPROC lpOpenFile;
FARPROC lpInit;
FARPROC lpSeek;
FARPROC lpRead;
FARPROC lpClose;

// local function.
void NEAR PASCAL InitStatusLine(HWND);

/**************************************************************************/
/****************************   Global Procs   ****************************/


/*  InstallSoftFont
 *
 *  Soft font installer startup procedure.
 */
int FAR	PASCAL
InstallSoftFont(hWndParent, lpModNm, lpPortNm, smartMode, fsvers, nPrintClass)
    HWND hWndParent;
    LPSTR lpModNm;
    LPSTR lpPortNm;
    BOOL smartMode;
    int	fsvers;
    int nPrintClass;		// 0 for PCL, 1 for DeskJet, etc.
    {
    FARPROC lpDlgFunc;
    HANDLE hWinSF = 0;
    LPSTR dlFile = 0L;

    LockSegment(-1);	/* lock the soft font installer's data seg */

    DBGdlgfn(("InstallSoftFont(%d,%lp,%lp,%d,%d): %ls\n",(WORD)hWndParent,
	 lpAppNm, lpPortNm, (WORD)smartMode, fsvers, lpPortNm));

    /*  Pick up the global information.
     */
    gDlgState =	SFDLG_INACTIVE;
    gSF_FLAGS =	0;
    gFSvers = fsvers;
    gHLBleft = 0;
    gHLBright =	0;
    gIgnoreMessages = FALSE;
    lmemcpy((LPSTR)gModNm, lpModNm, sizeof(gModNm));
    gModNm[sizeof(gModNm)-1] = '\0';
    lmemcpy((LPSTR)gLPortNm, lpPortNm, sizeof(gLPortNm));
    gLPortNm[sizeof(gLPortNm)-1] = '\0';
    gRPortNm[0]	= '\0';
    gPrintClass = nPrintClass;

    // handle gPrintClass bit for 'no soft fonts'
    if (256 & gPrintClass)
	{
	gPrintClass &= !(WORD)256;
	gNoSoftFonts= TRUE;
	}

    // Load the LZ decompression library and get the addresses
    // of its functions.
    if ( 32 > (WORD) (hLZ = LoadLibrary((LPSTR) "lzexpand.dll")))
	{
	UnlockSegment(-1);	// unlock segment
	return(-1);
	}

    if (	// get proc. addresses and check them
	(NULL == (lpOpenFile = GetProcAddress(hLZ, (LPSTR) "LZOpenFile"))) ||
	(NULL == (lpInit = GetProcAddress(hLZ, (LPSTR) "LZInit"))) ||
	(NULL == (lpRead = GetProcAddress(hLZ, (LPSTR) "LZRead"))) ||
	(NULL == (lpSeek = GetProcAddress(hLZ, (LPSTR) "LZSeek"))) ||
	(NULL == (lpClose = GetProcAddress(hLZ, (LPSTR) "LZClose"))) )
	    {	// oops, something wasn't there..
	    //MessageBox(hWndParent,
	    //	(LPSTR)"A function is missing in LZEXPAND.DLL",
	    //	(LPSTR)"Font Installer",
	    //	MB_OK | MB_ICONEXCLAMATION );
	    FreeLibrary(hLZ);
	    UnlockSegment(-1);	// unlock segment
	    return(-2);
	    }

    // this isn't enough, somehow .. see WM_INITDIALOG code
    if (smartMode)
	gDlgState |= SFDLG_ALLOWEDIT | (SFDLG_ALLOWEDIT << 4);

    GetListboxBitmaps(hWndParent);	// for user-draw listboxes

    // Load strings needed for not-enough-memory-to-load dialog message box.
    // If we can't load them, we're REALLY low on memory.
    LoadString(hLibInst, SFINSTAL_NM, (LPSTR)szFontInstall,
	sizeof(szFontInstall));
    LoadString(hLibInst, SF_NOMEMDLG, (LPSTR)szNotEnough, sizeof(szNotEnough));

    MyDialogBox(hLibInst,SFINSTALL, hWndParent, SFdlgFn);

    FreeLibrary(hLZ);

    UnlockSegment(-1);	/* about to exit, unlock soft font installers seg */

    return ((gSF_FLAGS & SF_CHANGES) ? ((gFSvers > 0) ?	gFSvers	: 1) : 0);

} // InstallSoftFont()

/*  SFdlgFn
 */
BOOL FAR PASCAL	SFdlgFn(hDB, wMsg, wParam, lParam)
    HWND hDB;
    unsigned wMsg;
    WORD wParam;
    LONG lParam;
    {
    switch (wMsg)
	{
	case WM_INITDIALOG:
	    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): WM_INITDIALOG\n",
				hDB, wMsg, wParam, lParam));

	    CenterDlg(hDB);

	    InitLBstrings(hLibInst, 0L,	0L, 0L);

	    // if this is a LaserJet (no soft fonts) tell the user.
	    InitStatusLine(hDB);

	    EnableWindow(GetDlgItem(hDB, SF_PERM_LEFT),	FALSE);
	    EnableWindow(GetDlgItem(hDB, SF_TEMP_LEFT),	FALSE);

	    EnableWindow(GetDlgItem(hDB, SF_LB_LEFT), FALSE);
	    EnableWindow(GetDlgItem(hDB, SF_LB_RIGHT), FALSE);

	    ShowWindow(GetDlgItem(hDB, SF_PERM_RIGHT), HIDE_WINDOW);
	    ShowWindow(GetDlgItem(hDB, SF_TEMP_RIGHT), HIDE_WINDOW);

	    /*  Kill the two active controls because the message
             *  SF_INITLB_LEFT enables them after filling in the
             *  left listbox.
             */
	    EnableWindow(GetDlgItem(hDB, SF_ADD_RIGHT),	FALSE);
	    EnableWindow(GetDlgItem(hDB, SF_EXIT), FALSE);

	    //  Make edit button always enabled now.... (11 may 89)
	    gDlgState |= (SFDLG_ALLOWEDIT) | (SFDLG_ALLOWEDIT << 4);

	    gIgnoreMessages = FALSE;

	    UpdateControls(hDB,	gDlgState);

	    /*  Show the dialog now and send a message to fill
             *  in the left listbox after the dialog is up.
             */
	    ShowWindow(hDB, SHOW_OPENWINDOW);
	    UpdateWindow(hDB);
	    PostMessage(hDB, WM_COMMAND, SF_INITLB_LEFT, 0L);
	    break;

	case WM_COMMAND:
	    switch (wParam)
		{
		case SF_INITLB_LEFT:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_INITLB_LEFT\n",
			hDB, wMsg, wParam, lParam));

		    if (gHLBleft)
			{
			GlobalFree(gHLBleft);
			gHLBleft = 0;
			SendDlgItemMessage(hDB,SF_LB_LEFT,LB_RESETCONTENT,0,0L);
			}

		    if (gHLBleft=FillListBox(
			    hDB,hLibInst,SF_LB_LEFT,gModNm,gLPortNm))
			gDlgState |= (SFDLG_RESFONTS <<	4);

		    EnableWindow(GetDlgItem(hDB,SF_ADD_RIGHT),TRUE);
		    EnableWindow(GetDlgItem(hDB,SF_EXIT),TRUE);
		    UpdateControls(hDB,	gDlgState);
		    break;

		case SF_IGNORMESSAGES:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_IGNORMESSAGES\n",
				hDB, wMsg, wParam, lParam));

		    /*  Enable/disable processing of messages to the perm/temp
                     *  buttons (see UpdatePermTemp in sfutils.c).
                     */
		    if (lParam)
			gIgnoreMessages	= TRUE;
		    else
			gIgnoreMessages	= FALSE;
		    break;

		case SF_LB_LEFT:
		case SF_LB_RIGHT:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_LB\n",
			hDB, wMsg, wParam, lParam));

		    if (HIWORD(lParam) == LBN_ERRSPACE)
			EndDialog(hDB,-1);

		    gDlgState =	UpdateStatusLine(hDB, wParam, gDlgState,
			(wParam	== SF_LB_LEFT) ? gHLBleft : gHLBright,
			(GetKeyState(VK_SHIFT) < 0 &&
			GetKeyState(VK_CONTROL)	< 0));

		    UpdateControls(hDB,	gDlgState);
		    break;

		case SF_PERM_LEFT:
		case SF_PERM_RIGHT:
		case SF_TEMP_LEFT:
		case SF_TEMP_RIGHT:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_PERM\TEMP\n",
			hDB, wMsg, wParam, lParam));

		    /*  Ignore this message if it comes while we're already
                     *  doing something in UpdatePermTemp.
                     */
		    if (gIgnoreMessages)
			{
			DBGdlgfn(("...ignoring\n"));
			break;
			}

		    gDlgState =	UpdatePermTemp(hDB, hLibInst, wParam, gDlgState,
			(wParam	== SF_PERM_LEFT	|| wParam == SF_TEMP_LEFT) ?
			gHLBleft : gHLBright);

		    UpdateControls(hDB,	gDlgState);
		    break;

		case SF_ADD_RIGHT:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_ADD\n",
			hDB, wMsg, wParam, lParam));
		    if (gHLBright)
			{
			gHLBright = EndAddFontsMode(
				hDB, hLibInst, gHLBright, SF_LB_RIGHT);
			gDlgState &= ~(SFDLG_DSKFONTS);

			if (gDlgState &	SFDLG_SELECTED)
			    {
			    gDlgState &= ~(SFDLG_SELECTED);
			    //SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");
			    InitStatusLine(hDB);
			    UpdateControls(hDB,	gDlgState);
			    }
			else if	(!(gDlgState & (SFDLG_SELECTED << 4)))
			    {
			    //SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");
			    InitStatusLine(hDB);
			    }
			}
		    else
			{
			WORD n;

			InitStatusLine(hDB);
			gHLBright = AddFontsMode(hDB, hLibInst,	SF_LB_RIGHT, &n,
			    (GetKeyState(VK_SHIFT) < 0 &&
			    GetKeyState(VK_CONTROL) < 0));

			if (gHLBright)
			    {
			    gDlgState |= SFDLG_DSKFONTS;
			    resetLB(hDB,hLibInst,SF_LB_RIGHT,gHLBright,n,
				SF_ADDREADY,FALSE);
			    }
			}
		    KERPLUNK(hDB);
		    break;

		case SF_MOVE:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_MOVE\n",
			hDB, wMsg, wParam, lParam));

		    if (gDlgState & (SFDLG_SELECTED << 4))
			{
			if (gDlgState &	SFDLG_RESFONTS)
			    {
			    WORD n;

			    gHLBright =	CopyFonts(hDB,hLibInst,SF_LB_RIGHT,
				gHLBright,gRPortNm,SF_LB_LEFT,gHLBleft,
				gLPortNm,TRUE,&n,gModNm);
			    resetLB(hDB,hLibInst,SF_LB_LEFT,gHLBleft,n,
				SF_MOVSUCCESS,TRUE);
			    gDlgState &= ~(SFDLG_SELECTED << 4);
			    UpdateControls(hDB,	gDlgState);
			    }
			}
		    else if (gDlgState & SFDLG_SELECTED)
			{
			if (gDlgState &	SFDLG_DSKFONTS)
			    {
			    WORD n;

			    if (gHLBleft=AddFonts(hDB, hLibInst, SF_LB_LEFT,
				gHLBleft,SF_LB_RIGHT,gHLBright,
				gModNm,gLPortNm,&n))
				{
				gDlgState |= (SFDLG_RESFONTS <<	4);
				gDlgState &= ~(SFDLG_SELECTED);

				if (resetLB(hDB,hLibInst,SF_LB_RIGHT,gHLBright,
				    n, SF_ADDSUCCESS,FALSE))
				    {
				    /*  Listbox is empty, end add fonts mode.
                                     */
				    gHLBright =	EndAddFontsMode(hDB, hLibInst,
					gHLBright, SF_LB_RIGHT);
				    gDlgState &= ~(SFDLG_DSKFONTS);
				    }
				}
			    else
				gDlgState &= ~(SFDLG_SELECTED);

			    UpdateControls(hDB,	gDlgState);
			    }
			else if	(gDlgState & SFDLG_RESFONTS)
			    {
			    WORD n;

			    gHLBleft = CopyFonts(hDB,hLibInst,SF_LB_LEFT,
				gHLBleft,gLPortNm,SF_LB_RIGHT,gHLBright,
				gRPortNm,TRUE,&n,gModNm);
			    resetLB(hDB,hLibInst,SF_LB_RIGHT,gHLBright,n,
				SF_MOVSUCCESS,TRUE);
			    gDlgState &= ~(SFDLG_SELECTED);
			    }
			}
		    KERPLUNK(hDB);
		    break;

		case SF_COPY:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_COPY\n",
			hDB, wMsg, wParam, lParam));

		    if (gDlgState & SFDLG_RESFONTS)
			{
			if (gDlgState &	(SFDLG_SELECTED	<< 4))
			    {
			    WORD n;

			    gHLBright =	CopyFonts(hDB,hLibInst,SF_LB_RIGHT,
				gHLBright,gRPortNm,SF_LB_LEFT,gHLBleft,
				gLPortNm,FALSE,&n,gModNm);
			    resetLB(hDB,hLibInst,SF_LB_LEFT,gHLBleft,n,
				SF_CPYSUCCESS,TRUE);
			    gDlgState &= ~(SFDLG_SELECTED << 4);
			    UpdateControls(hDB,	gDlgState);
			    }
			else if	(gDlgState & SFDLG_SELECTED)
			    {
			    WORD n;

			    gHLBleft = CopyFonts(hDB,hLibInst,SF_LB_LEFT,
				gHLBleft,gLPortNm,SF_LB_RIGHT,gHLBright,
				gRPortNm,FALSE,&n,gModNm);
			    resetLB(hDB,hLibInst,SF_LB_RIGHT,gHLBright,n,
				SF_CPYSUCCESS,TRUE);
			    gDlgState &= ~(SFDLG_SELECTED);
			    UpdateControls(hDB,	gDlgState);

			    if (n > 0)
				gDlgState |= (SFDLG_RESFONTS <<	4);
			    }
			}
		    KERPLUNK(hDB);
		    break;

		case SF_ERASE:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_ERASE\n",
			hDB, wMsg, wParam, lParam));

		    if (gDlgState & (SFDLG_SELECTED << 4))
			{
			WORD n;

			if (RemoveFonts(hDB, hLibInst, SF_LB_LEFT,
			    gHLBleft, gModNm, gLPortNm,	&n))
			    {
			    gDlgState &= ~(SFDLG_SELECTED << 4);
			    resetLB(hDB, hLibInst, SF_LB_LEFT, gHLBleft,
				n, SF_RMVSUCCESS, TRUE);
			    UpdateControls(hDB,	gDlgState);
			    }
			}
		    else if (gDlgState & SFDLG_SELECTED)
			{
			WORD n;

			if (RemoveFonts(hDB, hLibInst, SF_LB_RIGHT,
			    gHLBright, gModNm, gRPortNm, &n))
			    {
			    gDlgState &= ~(SFDLG_SELECTED);
			    resetLB(hDB,hLibInst,SF_LB_RIGHT,gHLBright,
				n,SF_RMVSUCCESS,TRUE);
			    UpdateControls(hDB,	gDlgState);
			    }
			}
		    KERPLUNK(hDB);
		    break;

		case SF_EDIT:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_EDIT\n",
			hDB, wMsg, wParam, lParam));
		    if (gDlgState & (SFDLG_SELECTED << 4))
			{
			WORD n;

			if (EditFonts(hDB, hLibInst, SF_LB_LEFT, gHLBleft,
			    ((gDlgState	& SFDLG_RESFONTS) ? gHLBright :	0),
			    gModNm, gLPortNm, &n))
			    {
			    resetLB(hDB, hLibInst, SF_LB_LEFT, gHLBleft, n,
				SF_EDSUCCESS, FALSE);
			    }
			gDlgState &= ~(SFDLG_SELECTED << 4);
			UpdateControls(hDB, gDlgState);
			}
		    else if ((gDlgState	& SFDLG_SELECTED) &&
			(gDlgState & SFDLG_RESFONTS))
			{
			WORD n;

			if (EditFonts(hDB, hLibInst, SF_LB_RIGHT, gHLBright,
			    gHLBleft, gModNm, gRPortNm, &n))
			    {
			    resetLB(hDB, hLibInst, SF_LB_RIGHT,	gHLBright, n,
				SF_EDSUCCESS, FALSE);
			    }
			gDlgState &= ~(SFDLG_SELECTED);
			UpdateControls(hDB, gDlgState);
			}
		    KERPLUNK(hDB);
		    break;

		case SF_COPYPORT:
		/* turn on/off port copy */

		if(!bCopyPort)
		{
		    DBGdlgfn((
		     "SFdlgFn(%d,%d,%d,%ld): SF_COPYPORT, (bCopyPort = FALSE\n",
			hDB, wMsg, wParam, lParam));

		    // hide Add Fonts pushbutton before displaying Ports
		    // dialog, since it may move, causing strange repainting
		    // of the buttons (because it will be replaced by
		    // SF_PERM_RIGHT and SF_TEMP_RIGHT if a port is selected).

		    ShowWindow(GetDlgItem(hDB, SF_ADD_RIGHT), HIDE_WINDOW);

		    if (GetPort(hDB,hLibInst,gLPortNm,gRPortNm,
				sizeof(gRPortNm)))
			{
			char buf[24];

			bCopyPort = TRUE;

			/*  Clear add fonts mode if the right listbox
                         *  is currently active.
                         */
			if (gHLBright)
			    {
			    gHLBright =	EndAddFontsMode(hDB,
					hLibInst, gHLBright, SF_LB_RIGHT);
			    gDlgState &= ~(SFDLG_DSKFONTS);

			    if (gDlgState & SFDLG_SELECTED)
				{
				gDlgState &= ~(SFDLG_SELECTED);
				SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");
				UpdateControls(hDB, gDlgState);
				}
			    else if (!(gDlgState & (SFDLG_SELECTED << 4)))
				{
				SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");
				}
			    }

			/*  Fill right listbox.
                         */
			gHLBright = FillListBox(hDB, hLibInst,
					SF_LB_RIGHT, gModNm, gRPortNm);

			/*  Invert text in pushbutton.
                         */
			if (LoadString(hLibInst, SF_NOPORT, buf, sizeof(buf)))
			    SetDlgItemText(hDB, SF_COPYPORT, (LPSTR) buf);

			gDlgState |= SFDLG_RESFONTS;

			/*  Change the dialog contols.
                         */
			ShowWindow(GetDlgItem(hDB, SF_PERM_RIGHT),
					SHOW_OPENWINDOW);
			ShowWindow(GetDlgItem(hDB, SF_TEMP_RIGHT),
					SHOW_OPENWINDOW);
			EnableWindow(GetDlgItem(hDB, SF_PERM_RIGHT), FALSE);
			EnableWindow(GetDlgItem(hDB, SF_TEMP_RIGHT), FALSE);
			UpdateControls(hDB, gDlgState);
			} /* if (GetPort(..)) */
		    else
			{
			// Didn't select a port, so restore the
			// ADD FONTS pushbutton
			ShowWindow(GetDlgItem(hDB, SF_ADD_RIGHT),
					SHOW_OPENWINDOW);
			}


		    KERPLUNK(hDB);
		  }

		else
		  {
		    DBGdlgfn((
		      "SFdlgFn(%d,%d,%d,%ld): SF_COPYPORT, bCopyPort = TRUE\n",
				hDB, wMsg, wParam, lParam));
		    {
		    char buf[24];

		    bCopyPort = FALSE;

		    if (gHLBright && (gDlgState	& SFDLG_RESFONTS))
			{
			EndPort(hDB,hLibInst,gHLBright,gModNm,gRPortNm,FALSE);
			gRPortNm[0] = '\0';
			GlobalFree(gHLBright);
			gHLBright = 0;
			}

		    /*  Invert text in pushbutton.
		     */
		    if (LoadString(hLibInst, SF_YESPORT, buf, sizeof(buf)))
			SetDlgItemText(hDB, SF_COPYPORT, (LPSTR) buf);

		    gDlgState &= ~(SFDLG_RESFONTS);
		    gDlgState &= ~(SFDLG_SELECTED);

		    /*  Change the dialog contols.
                     */
		    SetDlgItemText(hDB,	SF_PRINTER_RIGHT, (LPSTR)"");
		    SendDlgItemMessage(hDB,SF_LB_RIGHT,LB_RESETCONTENT,0,0L);
		    CheckRadioButton(hDB, SF_PERM_RIGHT, SF_TEMP_RIGHT,	0);
		    EnableWindow(GetDlgItem(hDB, SF_LB_RIGHT), FALSE);
		    EnableWindow(GetDlgItem(hDB, SF_PERM_RIGHT), FALSE);
		    EnableWindow(GetDlgItem(hDB, SF_TEMP_RIGHT), FALSE);
		    ShowWindow(GetDlgItem(hDB, SF_PERM_RIGHT), HIDE_WINDOW);
		    ShowWindow(GetDlgItem(hDB, SF_TEMP_RIGHT), HIDE_WINDOW);
		    ShowWindow(GetDlgItem(hDB,SF_ADD_RIGHT),SHOW_OPENWINDOW);
		    UpdateControls(hDB,	gDlgState);
		    }
		    KERPLUNK(hDB);
		  }
		  break;
		
		case SF_ABOUT:	// ABOUT pushbutton -- display About box.
		    MyDialogBox(hLibInst,SFABOUT,hDB, GenericWndProc);
		    KERPLUNK(hDB);
		    break;

		case SF_HELP:	// HELP pushbutton -- run help.
		    HelpWasCalled = WinHelp(hDB, (LPSTR) "finstall.hlp",
						(WORD) HELP_INDEX,
						(DWORD) 0L);
		    break;

		case SF_EXIT:
sysclose:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SF_EXIT\n",
				hDB, wMsg, wParam, lParam));

		    // End Help if it was called.
		    if (HelpWasCalled)
			WinHelp(hDB, (LPSTR) "finstall.hlp",
				    (WORD) HELP_QUIT, (DWORD) NULL);

		    if (gSF_FLAGS & SF_NOABORT)
			{
			/*  The exit button has been changed to a cancel
                         *  button -- indicate it has been clicked and
                         *  continue without actually exiting the dialog.
                         *  Flush extra exit messages from the queue so
                         *  we won't exit right away, this also prevents
                         *  the right LB struct from getting freed in the
                         *  middle of AddFonts, where it is still locked.
                         */
			MSG msg;

			while (PeekMessage(&msg, hDB, NULL, NULL, TRUE))
			    {
			    /*  We have to process paint messages.
                             */
			    if (msg.message == WM_PAINT)
				IsDialogMessage(hDB, &msg);
			    }

			gSF_FLAGS &= ~(SF_NOABORT);
			break;
			}

		    /*  Place the installer dialog in a "null" state by
                     *  unselecting any selected fonts and sending messages
                     *  to their processing procs to clean up.  This seems
                     *  to be the proper thing to do plus it fixes an obscure
                     *  bug where the perm/temp buttons get some extraneous
                     *  messages when we don't want them.
                     */
		    if (gDlgState & (SFDLG_SELECTED << 4))
			{
			SendDlgItemMessage(hDB,	SF_LB_LEFT, LB_SETSEL,
				FALSE, (long)(-1));
			gDlgState = UpdateStatusLine(hDB, wParam, gDlgState,
			    gHLBleft, FALSE);
			UpdateControls(hDB, gDlgState);
			}
		    else if (gDlgState & SFDLG_SELECTED)
			{
			SendDlgItemMessage(hDB,	SF_LB_RIGHT, LB_SETSEL,
				FALSE, (long)(-1));
			gDlgState = UpdateStatusLine(hDB, wParam, gDlgState,
			    gHLBright, FALSE);
			UpdateControls(hDB, gDlgState);
			}

		    if (gHLBleft && !EndPort(hDB,hLibInst,gHLBleft,gModNm,
				gLPortNm,
			(GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_CONTROL))))
			{
			break;
			}

		    if (gHLBright && (gDlgState	& SFDLG_RESFONTS) &&
			!EndPort(hDB,hLibInst,gHLBright,gModNm,gRPortNm,FALSE))
			{
			break;
			}

		    EndDialog(hDB, wParam);

		    endSFdir(0L);

		    if (gHLBleft)
			{
			GlobalFree(gHLBleft);
			gHLBleft = 0;
			}
		    if (gHLBright)
			{
			GlobalFree(gHLBright);
			gHLBright = 0;
			}
		    break;
		}
	    break;

	case WM_SYSCOMMAND:
	    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): WM_SYSCOMMAND\n",
			hDB, wMsg, wParam, lParam));

	    switch (wParam)
		{
		case SC_CLOSE:
		    DBGdlgfn(("SFdlgFn(%d,%d,%d,%ld): SC_CLOSE\n",
				hDB, wMsg, wParam, lParam));
		    goto sysclose;
		    break;

		default:
		    return FALSE;

		}
	    break;	/* end of case WM_SYSCOMMAND */

	case WM_MEASUREITEM:
	    // An owner-draw control is being created, and this message
	    // is sent to obtain its dimensions.
	    // So far, only the main dialog listboxes fit in this category.
	    // lParam points to a MEASUREITEMSTRUCT containing this info.

	    FillMeasureItem( (LPMEASUREITEMSTRUCT) lParam);

	    break;

	case WM_DRAWITEM:
	    // This message causes an owner-draw control to be drawn
	    // or updated.
	    // In this program, the control will be an item in one of
	    // the two main listboxes.
	    // lParam points to a DRAWITEMSTRUCT indicating what's to
	    // be done, and what it's to be done to or with.

	    if (!DrawItem(hDB,  (LPDRAWITEMSTRUCT) lParam))
		DefWindowProc(hDB, wMsg, wParam, lParam) ;
	    break;

	default:
	    return FALSE;

	}	/* switch (wMsg) */

    return TRUE;
    }	// SFdlgFn()

// Local function
// If this is a basic laserJet, indicate that only cartridges can be installed,
// Otherwise, clear status line.

void NEAR PASCAL InitStatusLine(hDB)
HWND hDB;
{
char buf[80];

// init to blank status line
buf[0] = '\0';

// if this is a LaserJet (no soft fonts) tell the user.
if (gNoSoftFonts)
    LoadString(hLibInst, SF_NOSOFT, buf, sizeof(buf));

// display blank or warning message
SetDlgItemText(hDB, SF_STATUS, (LPSTR) buf);

} // InitStatusLine()
