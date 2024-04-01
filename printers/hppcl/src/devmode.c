/**[f******************************************************************
 * devmode.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   devmode.c   ******************************/
//
//  Procs for handling driver-specific dialog.
//
//  05 mar 90   clarkc  75 dpi default instead of 300 (make up your mind).
//
//  25 feb 90   clarkc  300 dpi default instead of 75.
//
//  01 dec 89	peterbe	Declarations in ifdef also.
//
//  30 nov 89	peterbe	Visual edge calls in ifdef now.
//
//  13 nov 89	CLARKC	Limit # of copies to < 10,000 
//
//  08 nov 89	peterbe	In UpdateCartridges(), changed position of
//			LB_SETTOPINDEX and changed some message calls.
//
//  01 nov 89	peterbe	Change code for moving 1st cart to top of listbox.
//			Display message box if there're too many cartridges
//			in win.ini, instead of crashing!
//
//  31 oct 89	peterbe	When init. cartridge listbox, scroll to first item
//			independent of order in list.
//
//  25 oct 89	peterbe Now gray system menu SC_CLOSE when Cancel button is
//			disabled.
//			Corrected comment on 'fake the Cancel button'
//
//  02 oct 89	peterbe	Help button calls WinHelp(..HELP_INDEX..) now.
//
//  29 sep 89	peterbe	Force write of paper size in WriteWinIniEnv() if
//			letter or A4 is current paper size.
//
//  27 sep 89	peterbe	Deselect other cartridge selections if None is sel'ed.
//			-- see 'bNone' in UpdateCartridges()
//			Also, 'None' string is loaded from .RC now.
//
//  26 sep 89	clarkc 	Added HourGlass cursor at initialization
//
//  19 sep 89	peterbe	Removed .. PrivateProfile..() declarations.  It's
//			'some day' today!
//
//  22 aug 89	peterbe	Changed lpFins declaration back, and changed check
//			on range of return from InstallSoftFont().
//
//  17 aug 89	peterbe	Change decl: FARPROC lpFins
//
//  16 aug 89	peterbe	Fixed code for cat-ing "PCL/Las..." and port name
//			in DialogFn(), and also init of port name in
//			ExtDeviceMode().
//
//  15 aug 89	peterbe	Don't FreeLibrary() when LoadLibrary() returns value
//			between 1 and 31 inclusive!  .. caused RIP.
//			Also, displays message box (DLLErrorMsg()) when
//			GetProcAddress() fails.
//
//  08 aug 89	peterbe	Actually removed NO_PRIVATE_HACK stuff and other old
//			commented-out stuff.
//
//  07 aug 89	peterbe	Changed lstrcmp() to lstrcmpi().
//
//  03 aug 89	peterbe	Commented out NO_PRIVATE_HACK stuff; delete later.
//			Also, SOFTFONTS_ENABLED no longer needed.
//			The font installer is now called if numCartridges
//			is nonzero; for this case, the NOSOFT bit is passed
//			in the last param. of InstallSoftFont() for printers
//			which have cartridges but no soft fonts (LASERJET).
//
//  01 aug 89	peterbe	Removed declarations of _llseek() etc.
//
//  28 jun 89	peterbe	Renamed SoftFontInstall() to InstallSoftFont() and
//			added parameter to indicate driver type (0 == PCL,
//			1 == DeskJet).(see declaration of lpFIns).
//
//  07 jun 89	peterbe	Single icon in orientation groupbox now, changes
//			when orientation changes.
//			Added case IDHELP:, and code for it.
//
//  15 apr 89	peterbe	Changed about-box init. to pushbutton instead of sys.
//			menu item.
//
//  01 apr 89	peterbe	Added About-box code. Removed old icon-changing hack.
//			Closing with system menu goes to 'closeprogram:'
//
//  30 mar 89	peterbe	IDOPTION is (in)activated with EnableWindow() instead of
//			of ShowWindow.
//   1-12-89	jimmat	Most DeviceMode local data is now allocated via
//			GlobalAlloc() instead of statically allocated in DGROUP.
//   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
//   1-17-89	jimmat	Added PCL_* entry points to lock/unlock data seg.
//   1-25-89	jimmat	Use global hLibInst instead of GetModuleHandle().
//   1-26-89	jimmat	Added dynamic link to Font Installer library.
//   2-07-89	jimmat	Driver Initialization changes.
//   2-20-89	jimmat	Driver/Font Installer use same WIN.INI section (again)!
//   2-21-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
//   2-24-89	jimmat	Removed parameters from lp_enbl() & lp_disable().
//

#include "nocrap.h"
#undef NOGDI
#undef NOCTLMGR
#undef NOWINMESSAGES
#undef NOMB
#undef NOMENUS
#undef NOSYSCOMMANDS
#undef NOMEMMGR
#undef NOSCROLL
#undef NOVIRTUALKEYCODES
#undef NOSHOWWINDOW
#undef NOSYSMETRICS
#undef NODRAWTEXT	    /***** HACK HACK HACK ******/
#include "windows.h"
#define NO_PRINTER_STUFF
#include "hppcl.h"
#include "pfm.h"
#include "resource.h"
#include "debug.h"
#define FONTMAN_ENABLE
#include "fontman.h"
#include "strings.h"
#include "dlgutils.h"
#define PRTCARTITEMS
#include "environ.h"
#define NO_OUTUTIL
#include "utils.h"
#define NO_PAPERBANDCRAP
#include "paperfmt.h"
#include "paper.h"
#include "fntutils.h"
#include "lclstr.h"


// Removed *PrivateProfile*() declarations here 19 sep 89

/*  LaserPort
 *
 *  These procs are declared in dump.h but it requires device.h
 *  to include the file here -- that ends up requiring all kinds
 *  of include files just to get these definitions.
 */
#ifdef VISUALEDGE
int FAR PASCAL lp_enbl(void);
int FAR PASCAL lp_disable(void);
#endif


LPSTR   FAR PASCAL lstrcpy(LPSTR, LPSTR);
LPSTR	FAR PASCAL lstrcat(LPSTR, LPSTR);
int	FAR PASCAL lstrlen(LPSTR);
int	FAR PASCAL lstrcmpi(LPSTR, LPSTR);
LPSTR	FAR PASCAL lmemcpy(LPSTR, LPSTR, WORD);
LPSTR	FAR PASCAL lmemset(LPSTR, BYTE, WORD);

/*  Utilities
 */
#include "getint.c"

#define LOCAL static 

//#ifdef DEBUG
//    #define LOCAL_DEBUG
//#endif

#ifdef DEBUG
#define DBGerr(msg)	     /*DBMSG(msg)*/
#define DBGDevMode(msg)      /*DBMSG(msg)*/
#define DBGpaper(msg)	     /* DBMSG(msg) */
#define DBGdispmemop(msg)  /*DBMSG(msg)*/
#define DBGPrtInfo(msg)    /*DBMSG(msg)*/
#define DBGCartInfo(msg)   /* DBMSG(msg) */
#define DBGupdatecart(msg) DBMSG(msg)
#define DBGgetcartlist(msg) DBMSG(msg)
#define DBGupdatenumcarts(msg) DBMSG(msg)
#else
#define DBGerr(msg)	   /*null*/
#define DBGDevMode(msg)    /*null*/
#define DBGpaper(msg)	   /*null*/
#define DBGdispmemop(msg)  /*null*/
#define DBGPrtInfo(msg)    /*null*/
#define DBGCartInfo(msg)   /*null*/
#define DBGupdatecart(msg) /*null*/
#define DBGgetcartlist(msg) /*null*/
#define DBGupdatenumcarts(msg) /*null*/
#endif

/*  Data structures.
 */

extern HANDLE hLibInst; 	/* Driver library Instance handle */

static HANDLE hDMdata;		/* only used if DevMode Dialog box used */

static BOOL DevModeBusy = FALSE;

static BOOL HelpWasCalled = FALSE;


typedef struct tagDMData {	/* for "large" DeviceMode() "local" data */

    PCLDEVMODE CurEnv, OldEnv;
    PRTINFO    PrtStuff[MAX_PRINTERS];
    CARTINFO   CartStuff[MAX_CARTRIDGES];

    WORD totalCarts;
    WORD totalPrinters;
    WORD paperbits[MAX_PAPERLIST];
    WORD paperStrs[MAX_PAPERSIZES];
    WORD sourceStrs[MAX_PAPERSOURCES];

    BOOL fontsOK;
    BOOL cartsVisible;
    BOOL LaserPortOn;

    char portName[32];

} FAR * LPDMDATA;


/*  Forward refs
 */

int  FAR PASCAL DeviceMode(HANDLE,HANDLE,LPSTR,LPSTR);
BOOL FAR PASCAL DialogFn(HWND, unsigned, WORD, LONG);

LOCAL void  MergeEnvironment(LPDMDATA,LPPCLDEVMODE,LPPCLDEVMODE);
LOCAL WORD  GetFillPrtList(HWND,PRTINFO FAR *);
LOCAL void  GetCartList(HWND, LPDMDATA);
LOCAL VOID  DisplayMemoryOptions(HWND, PRTINFO FAR *, short, short, WORD);
LOCAL void  CheckDuplex(LPPCLDEVMODE);
LOCAL BOOL  UpdateCartridges(HWND, HWND, LPDMDATA, WORD);
LOCAL void  shiftCarts(LPPCLDEVMODE, WORD, WORD);
LOCAL void  UpdateNumCarts(HWND, WORD);
LOCAL short UpdatePaperSource(HWND,WORD, short, BOOL, WORD FAR *);
LOCAL short UpdatePaperSize(HWND, WORD, WORD, WORD FAR *);
LOCAL void  WriteWinIniEnv(LPPCLDEVMODE, LPPCLDEVMODE, LPSTR, LPSTR);
LOCAL short LBCartIndex(LPDMDATA, short);
LOCAL short PaperBit2Str(short);

LOCAL void DLLErrorMsg(int);

#if defined(LOCAL_DEBUG)
LOCAL void  dumpDevMode(LPPCLDEVMODE);
LOCAL void  dumpPrtInfo(PRTINFO FAR *);
      void  dumpCartInfo(CARTINFO FAR *);
#endif

extern short FAR PASCAL OptionsDlg(HANDLE, HWND, LPPCLDEVMODE);
void NEAR PASCAL SetOrientIcon(HWND, LPPCLDEVMODE);

#if 0
void far pascal OutputDebugString(LPSTR);
int far pascal wvsprintf(LPSTR,LPSTR,LPSTR);
void near cdecl dpf(LPSTR lp,...)
{
    char sz[160];

    wvsprintf(sz,lp,(LPSTR)(&lp+1));
    OutputDebugString(sz);
}
#endif

void HourGlass(bOn)
BOOL bOn;                       /* Turn hourglass on or off */
{
  /* change cursor to hourglass */
  if (!GetSystemMetrics(SM_MOUSEPRESENT))
      ShowCursor(bOn);
  SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
}


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
  */

short FAR PASCAL
ExtDeviceMode(HWND hWnd, HANDLE hInst, LPPCLDEVMODE lpdmOutput,
	      LPSTR lpDeviceName, LPSTR lpPort, LPPCLDEVMODE lpdmInput,
	      LPSTR lpProfile, WORD Mode) {

    LPDMDATA lpDM;
    HANDLE hDynData;
    short rc, exclusive;

    DBGDevMode(("ExtDeviceMode(%d,%d,%lp,%lp,%lp,%lp,%lp,%d)\n",
		hWnd,hInst,lpdmOutput,lpDeviceName,lpPort,lpdmInput,
		lpProfile,Mode));
    DBGDevMode(("     DeviceName = ->%ls<-   Port = ->%ls<-\n",
		lpDeviceName ? lpDeviceName : (LPSTR) "NULL",
		lpPort ? lpPort : (LPSTR) "NULL"));
    DBGDevMode(("     Profile    = ->%ls<-\n",
		lpProfile ? lpProfile : (LPSTR) "NULL"));
    DBGDevMode(("     Mode = %s %s %s %s\n",Mode & DM_UPDATE ? "UPDATE" : "",
		Mode & DM_COPY ? "COPY" : "", Mode & DM_PROMPT ? "PROMPT" :
		"", Mode & DM_MODIFY ? "MODIFY" : ""));


    /* Mode == 0 is a request for the full size of the DEVMODE structure */

    if (!Mode) {
	DBGDevMode(("ExtDeviceMode returning size: %d\n",sizeof(PCLDEVMODE)));
	return sizeof(PCLDEVMODE);
    }


    /* Okay, there is some real work to do.  Make sure we haven't been
       (re)entered more than once to UPDATE or PROMPT (possibly by two or
       more applications), then allocate and lock down our data areas */

    exclusive = Mode & (DM_UPDATE | DM_PROMPT);

    if (DevModeBusy) {
	if (exclusive)
	    return(SP_ERROR);
    } else
	DevModeBusy = exclusive;

    LockSegment(-1);		/* lock our own data segment */

    rc = SP_OUTOFMEMORY;

    if (hDynData = GlobalAlloc(GMEM_MOVEABLE,(DWORD)sizeof(struct tagDMData))) {
	if (!(lpDM = (LPDMDATA) GlobalLock(hDynData)))
	    goto ExtFree;
    } else
	goto ExtExit;

    /* Initialize a few items in the DevMode data area */

    lpDM->fontsOK = (Mode & DM_UPDATE);
    lpDM->totalCarts = lpDM->totalPrinters = 0;
    // copy port name, taking a little care if it's really long.
    {
    int siz;

    siz = sizeof(lpDM->portName);
    if(lstrlen(lpPort) < siz)
	lstrcpy(lpDM->portName, lpPort);
    else
	{ // it's really long for some #$@*&#$ reason, so use lmemcpy()
	lmemcpy(lpDM->portName, lpPort, sizeof(lpDM->portName));
	// and 0-terminate it.
	lpDM->portName[siz - 1] = '\0';
	}
    }


    /* Get a copy of the environment--build one if the user gave us a
       private .INI file, or there is no current environment, or it doesn't
       match our device */

    lstrcpy(lpDM->CurEnv.dm.dmDeviceName, lpDeviceName);

    if (lpProfile ||
	!GetEnvironment(lpPort,(LPSTR)&lpDM->CurEnv,sizeof(PCLDEVMODE)) ||
	lstrcmpi(lpDeviceName, lpDM->CurEnv.dm.dmDeviceName))
      {
	MakeEnvironment(&lpDM->CurEnv, lpDeviceName, lpPort, lpProfile);
        SetEnvironment(lpPort,(LPSTR)&lpDM->CurEnv, sizeof(PCLDEVMODE));
      } 

    /* Keep a copy of the current environment, changes may get written to
       a .INI file before we're finished. */

    lmemcpy((LPSTR)&lpDM->OldEnv, (LPSTR)&lpDM->CurEnv, sizeof(PCLDEVMODE));


    /* If the user passed in a DEVMODE structure, merge it with the current
       environment before going futher */

    if ((Mode & DM_MODIFY) && lpdmInput)
	MergeEnvironment(lpDM,&lpDM->CurEnv,lpdmInput);

    /* Throw-up the device mode dialog box if the caller wants us to
       prompt the user for any changes */

    if (Mode & DM_PROMPT) {

	GlobalUnlock(hDynData);  /* let the data area move around if needed */

	hDMdata = hDynData;	 /* static for DialogFn() */

	rc = DialogBox(hInst,MAKEINTRESOURCE(DTMODE),hWnd,(FARPROC)DialogFn);

	if (!(lpDM = (LPDMDATA) GlobalLock(hDynData))) {
	    rc = SP_OUTOFMEMORY;
	    goto ExtFree;
	}

    } else

	rc = IDOK;	/* didn't prompt, but we still give the okay return */


    /* If the caller wants a copy of the resulting environment,
       give it to 'em */

    if ((Mode & DM_COPY) && lpdmOutput)
	lmemcpy((LPSTR)lpdmOutput, (LPSTR)&lpDM->CurEnv, sizeof(PCLDEVMODE));


#ifdef LOCAL_DEBUG
    dumpDevMode(&lpDM->CurEnv);
#endif


    /* Finally, update the default environment if everything is okay so far
       (and the user didn't Cancel the dialog box), and the caller wants us
       to do so */

    if ((Mode & DM_UPDATE) && rc == IDOK) {

	SetEnvironment(lpPort, (LPSTR) &lpDM->CurEnv, sizeof(PCLDEVMODE));
	WriteWinIniEnv(&lpDM->CurEnv,&lpDM->OldEnv, lpPort, lpProfile);
	SendMessage(0xffff, WM_DEVMODECHANGE, 0, (LONG)(LPSTR)lpDeviceName);
    }

    GlobalUnlock(hDynData);

ExtFree:
    GlobalFree(hDynData);

ExtExit:
    UnlockSegment(-1);

    if (exclusive)		/* since there can only be 1 exclusive	   */
	DevModeBusy = FALSE;	/* invocation, no longer "busy" if this it */

    DBGDevMode(("ExtDeviceMode() returning %d\n",rc));

    return (rc);
}

/***********************************************************************
			 D E V I C E M O D E
 ***********************************************************************/

int FAR PASCAL
DeviceMode(HANDLE hWnd, HANDLE hInst, LPSTR lpDevice, LPSTR lpPort) {

    return (ExtDeviceMode(hWnd,hInst,NULL,lpDevice,lpPort,NULL,NULL,
			  DM_PROMPT | DM_UPDATE) == IDOK);
}

/***********************************************************************
			  D I A L O G  F N
 ***********************************************************************/

/*  The Device Mode Dialog function */

BOOL FAR PASCAL
DialogFn(HWND hDB, unsigned message, WORD wParam, LONG lParam) {

    short temp;
    LPDMDATA lpDM;
    PRTINFO FAR *lpPI;
    LPPCLDEVMODE lpDevmode;
    char buf[30];
    // HANDLE gHSysMenu; // for about box system menu item

    /* lock down the DevMode data--terminate DialogBox if we can't */

    if (!(lpDM = (LPDMDATA) GlobalLock(hDMdata))) {
	EndDialog(hDB,SP_OUTOFMEMORY);
	return TRUE;
    }


    lpDevmode = &lpDM->CurEnv;			    /* used a bunch */
    lpPI = &lpDM->PrtStuff[lpDevmode->prtIndex];    /*	   ditto    */


    switch (message) {

	/*  Initialize the dialog box values --------------------------- */

        case WM_INITDIALOG:

            HourGlass(TRUE);
            CenterDlg(hDB);

            //  Set the caption of the dialog to "PCL/LaserJet on <port>"
            {
		short printlen;
		short portlen;
		char captionText[64];

		// get "PCL/LaserJet on ".. (intial caption from .RC file)
		// this will return 0-terminated string.
	 	printlen = GetWindowText(hDB, captionText, sizeof(captionText));
		portlen = lstrlen(lpDM->portName);

		// if we have enough space, append <port>
		if (sizeof(captionText) > printlen + portlen)
		    {
		    lstrcat((LPSTR)captionText, lpDM->portName);
		    SetWindowText(hDB, captionText);
		    }
            }


	    /*	Get PrtStuff and fill in printer listbox.
             */
	    lpDM->totalPrinters = GetFillPrtList(hDB, lpDM->PrtStuff);

	    if (lpDevmode->prtIndex > lpDM->totalPrinters)
		lpPI = &lpDM->PrtStuff[lpDevmode->prtIndex = 0];

	    SendDlgItemMessage(hDB, PRTBOX, CB_SETCURSEL,
			       lpPI->indlistbox,0L);

            /*  Fill in memory options.
             */
            {
		short i, refind = lpPI->indlistbox;

		/* find first occurrance of this printer */

		for (i = 0; i < lpDM->totalPrinters &&
		    lpDM->PrtStuff[i].indlistbox < refind; ++i)
		    ;

		DBGdispmemop(("CurEnv.prtIndex=%d, refind=%d,"
			      " first occurance=%d\n", lpDevmode->prtIndex,
			      refind, i));

		DisplayMemoryOptions(hDB, lpDM->PrtStuff, i, lpPI->availmem,
				     lpDM->totalPrinters);
            }


	    CheckDuplex(lpDevmode);	/*  Make sure Duplex is setup okay */


	    /*	Get CartStuff
             */
	    GetCartList(hDB, lpDM);


	    /*	Display or disable cartridges (depending upon printer). */

	    lpDM->cartsVisible = FALSE;
	    UpdateNumCarts(hDB, lpPI->numcart);

	    if (lpPI->numcart > 0)

		lpDM->cartsVisible =

		    UpdateCartridges(hDB,GetDlgItem(hDB,CARTBOX),lpDM,
				     lpPI->numcart);


	    /*	Put allowed paper trays (sources) into paper source combobox */

	    lpDevmode->dm.dmDefaultSource =

		UpdatePaperSource(hDB,lpDevmode->prtCaps,
				  lpDevmode->dm.dmDefaultSource,0,
				  lpDM->sourceStrs);


	    /*	Put allowed paper sizes into combobox */

	    GetPaperBits(hLibInst, lpDM->paperbits);

	    lpDevmode->dm.dmPaperSize =

		UpdatePaperSize(hDB,lpDM->paperbits[lpDevmode->paperInd],
				lpDevmode->dm.dmPaperSize,lpDM->paperStrs);


	    // disable font installer if the printer doesn't have
	    // soft fonts or cartridges.
	    if (((lpDevmode->prtCaps & NOSOFT) &&
		 (0 == lpDevmode->numCartridges)) ||
			(!lpDM->fontsOK))
                EnableWindow(GetDlgItem(hDB,IDSOFTFONT),FALSE);

	    /*	Set the current orientation */

	    CheckRadioButton(hDB, PORTRAIT, LANDSCAPE,
			    (lpDevmode->dm.dmOrientation == DMORIENT_PORTRAIT) ?
			    PORTRAIT : LANDSCAPE);

	    SetOrientIcon(hDB, lpDevmode);

	    /*	Set the current graphics resolution */

	    if (lpDevmode->prtResFac == SF300)
                CheckRadioButton(hDB, DPI75, DPI300, DPI300);
	    else if (lpDevmode->prtResFac == SF150)
                CheckRadioButton(hDB, DPI75, DPI300, DPI150);
            else
                CheckRadioButton(hDB, DPI75, DPI300, DPI75);


	    /*	Disable the Options... button if the printer doesn't support
	     *	duplex and the DP-TEK LaserPort isn't available.
	     */

#ifdef VISUALEDGE
	    if (lpDM->LaserPortOn = lp_enbl())	    /* LaserPort available? */
		lp_disable();
#endif

#ifdef VISUALEDGE
	    if (!(lpDevmode->prtCaps & ANYDUPLEX) && !lpDM->LaserPortOn)
		EnableWindow(GetDlgItem(hDB, IDOPTION), FALSE);
#else
	    // There's no Visual Edge, so disable options if no duplex.
	    if (!(lpDevmode->prtCaps & ANYDUPLEX))
		EnableWindow(GetDlgItem(hDB, IDOPTION), FALSE);
#endif


        // Limit # of copies to < 10,000
	    SendDlgItemMessage(hDB, COPYBOX, EM_LIMITTEXT, 4, 0L);
	    SetDlgItemInt(hDB,COPYBOX,lpDevmode->dm.dmCopies,0);


	    /* Necessary due to a Windows 3.0 bug?!? ************************/

	    SetFocus(GetDlgItem(hDB, PRTBOX));
            HourGlass(FALSE);

            break;


	// Process System commands (make About dialog appear, in this case....

	case WM_SYSCOMMAND:

		switch (wParam)
		    {
		    case SC_CLOSE:		// close with System menu
			wParam = IDCANCEL;	// fake the Cancel button..
		    	goto closeprogram;

		    default:
			GlobalUnlock(hDMdata);
			return FALSE;
		    }
		break;	/* end of case WM_SYSCOMMAND */


	/* Process messages from the dialog controls ------------------- */

	case WM_COMMAND:

	    switch (wParam) {

                case PORTRAIT:
		case LANDSCAPE:

		    CheckRadioButton(hDB, PORTRAIT, LANDSCAPE, wParam);
		    lpDevmode->dm.dmOrientation = (wParam == PORTRAIT) ?
			       DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE;
		    SetOrientIcon(hDB, lpDevmode);

                    break;


                case DPI75:
                case DPI150:
		case DPI300:

                    CheckRadioButton(hDB, DPI75, DPI300, wParam);
		    if (wParam==DPI300) {
			lpDevmode->prtResFac=SF300;
			lpDevmode->dm.dmPrintQuality = DMRES_HIGH;
		    } else if (wParam==DPI150) {
			lpDevmode->prtResFac=SF150;
			lpDevmode->dm.dmPrintQuality = DMRES_MEDIUM;
		    } else {
			lpDevmode->prtResFac=SF75;
			lpDevmode->dm.dmPrintQuality = DMRES_LOW;
		    }
                    break;


                case MEMBOX:
                    {
		    short i, refind;

                    DBGdispmemop(("MEMBOX...\n"));

		    temp = (short)SendDlgItemMessage(hDB, MEMBOX, CB_GETCURSEL,
						     0, 0L);
		    if (temp == CB_ERR) {
			SendDlgItemMessage(hDB, MEMBOX, CB_SETCURSEL, 0, 0L);
                        temp = 0;
		    }

                    /*  Locate the first occurrance of this printer
                     *  in the array of prtinfo structs.
		     */

		    refind = lpPI->indlistbox;
		    for (i = 0; i < lpDM->totalPrinters &&
			lpDM->PrtStuff[i].indlistbox < refind; ++i)
                        ;
                    DBGdispmemop(("temp=%d, refind=%d, first occurance=%d\n",
                        temp, refind, i));

                    /*  The currently selected printer is the first
                     *  occurrance of the printer in the prtinfo array
                     *  plus the index of the selected memory option
                     *  (this works because we displayed the memory
                     *  options in the order they appear in the prtinfo
                     *  list and we've required the operator to list
                     *  the same printer with different memory in order
                     *  in the resource file).
                     */
		    lpPI = &lpDM->PrtStuff[lpDevmode->prtIndex = i + temp];

		    lpDevmode->availmem = lpPI->availmem;

                    DBGdispmemop(("prtIndex=%d, availmem=%d\n",
				  lpDevmode->prtIndex, lpDevmode->availmem));
                    break;
                    }


                case PRTBOX:
                    {
		    short i, j;

                    /*  Pick up the selected printer from the printer
		     *	combobox.
		     */
		    temp = (short)SendDlgItemMessage(hDB, PRTBOX, CB_GETCURSEL,
						     0, 0L);

		    if (temp == CB_ERR) {
			SendDlgItemMessage(hDB, PRTBOX, CB_SETCURSEL, 0, 0L);
                        temp = 0;
		    }

                    /*  Locate the range of printers (same printer,
                     *  different memory options) for which this
                     *  printer exists.
                     */
		    for (i = 0; i < lpDM->totalPrinters &&
			lpDM->PrtStuff[i].indlistbox < temp; ++i)
                        ;
		    for (j = i; j < lpDM->totalPrinters &&
			lpDM->PrtStuff[j].indlistbox == temp; ++j)
                        ;

                    /*  Change all the printer data only if the printer
                     *  selection has really changed.
                     */
		    if (lpDevmode->prtIndex < i || lpDevmode->prtIndex >= j) {

			BOOL prevWasAuto = (lpDevmode->prtCaps & AUTOSELECT);

			lpPI = &lpDM->PrtStuff[lpDevmode->prtIndex = i];
			lpDevmode->availmem = lpPI->availmem;

                        DBGdispmemop(("PRTBOX: prtIndex=%d, availmem=%d\n",
				     lpDevmode->prtIndex,lpDevmode->availmem));

			lpDevmode->prtCaps   = lpPI->caps;
			lpDevmode->romind    = lpPI->romind;
			lpDevmode->romcount  = lpPI->romcount;
			lpDevmode->maxPgSoft = lpPI->maxpgsoft;
			lpDevmode->maxSoft   = lpPI->maxsoft;
			lpDevmode->paperInd  = lpPI->indpaperlist;

			if (lpDevmode->options & OPTIONS_FORCESOFT)
			    lpDevmode->prtCaps &= ~(NOSOFT);

                        /*  Display memory options.
                         */
			DisplayMemoryOptions(hDB,lpDM->PrtStuff,i,
					    lpPI->availmem,lpDM->totalPrinters);

			CheckDuplex(lpDevmode);     /* verify duplex fields */

                        /*  Update tray selection.
			 */
			lpDevmode->dm.dmDefaultSource =

			    UpdatePaperSource(hDB,lpDevmode->prtCaps,
					      lpDevmode->dm.dmDefaultSource,
					      prevWasAuto,lpDM->sourceStrs);

                        /*  Update paper sizes.
                         */
			lpDevmode->dm.dmPaperSize =

			    UpdatePaperSize(hDB,
					   lpDM->paperbits[lpDevmode->paperInd],
					   lpDevmode->dm.dmPaperSize,
					   lpDM->paperStrs);


			/* A change in printer may enable or disable the
			   selection of dmDefaultSource--make sure the
			   DEVMODE struct is correct.  NOTE: very similar
			   code exists in environ.c, if you update this
			   you may need to update that also */

			if ((lpDevmode->prtCaps &
			    (AUTOSELECT|LOTRAY|ANYENVFEED|NOMAN)) != NOMAN)
			    lpDevmode->dm.dmFields |= DM_DEFAULTSOURCE;
			else
			    lpDevmode->dm.dmFields &= ~DM_DEFAULTSOURCE;


                        /*  Update cartridge listbox to reflect printer change.
                         */
			UpdateNumCarts(hDB, lpPI->numcart);

			if (lpPI->numcart > 0)

			    lpDM->cartsVisible =

				UpdateCartridges(hDB,GetDlgItem(hDB,CARTBOX),
						 lpDM,lpPI->numcart);
			else {

			    if (lpDM->cartsVisible)
				SendDlgItemMessage(hDB,CARTBOX,LB_RESETCONTENT,
						   0,0L);

			    lpDM->cartsVisible = FALSE;
			    lpDevmode->numCartridges = 0;
			    lmemset((LPSTR)lpDevmode->cartIndex, 0,
				    DEVMODE_MAXCART*2);
			    lmemset((LPSTR)lpDevmode->cartind, 0,
				    DEVMODE_MAXCART*2);
			    lmemset((LPSTR)lpDevmode->cartcount, 0,
				    DEVMODE_MAXCART*2);
			}


			// enable or disable the Options...
			// button as necessary:

#ifdef VISUALEDGE
			EnableWindow(GetDlgItem(hDB, IDOPTION),
					    (lpDevmode->prtCaps & ANYDUPLEX) ||
					      lpDM->LaserPortOn);
#else
			EnableWindow(GetDlgItem(hDB, IDOPTION),
					    (lpDevmode->prtCaps & ANYDUPLEX));
#endif

			// was in #ifdef SOFTFONTS_ENABLED..
			// Enable or disable the Fonts button.

			if (((lpDevmode->prtCaps & NOSOFT) &&
			     (0 == lpDevmode->numCartridges)) ||
				    (!lpDM->fontsOK))
                            EnableWindow(GetDlgItem(hDB,IDSOFTFONT),FALSE);
                        else
                            EnableWindow(GetDlgItem(hDB,IDSOFTFONT),TRUE);

		    }
                    }
		    break;


		case TRAYBOX:

		    /* Pick up selected paper source from combobox */

		    temp = (short)SendDlgItemMessage(hDB, TRAYBOX,
						     CB_GETCURSEL, 0, 0L);

		    DBGpaper(("TRAYBOX: Current paper source = %d, ",temp));

		    if (temp == CB_ERR) {
			SendDlgItemMessage(hDB,SIZEBOX,CB_SETCURSEL,0,0L);
			temp = 0;
		    }

		    lpDevmode->dm.dmDefaultSource = lpDM->sourceStrs[temp];

		    break;


		case SIZEBOX:

		    /*	Pick up the selected paper size from the combobox. */

		    temp = (short)SendDlgItemMessage(hDB, SIZEBOX,
						     CB_GETCURSEL, 0, 0L);

		    DBGpaper(("SIZEBOX: Current paper size = %d, ",temp));

		    if (temp == CB_ERR) {
			SendDlgItemMessage(hDB,SIZEBOX,CB_SETCURSEL,0,0L);
			temp = 0;
		    }

		    lpDevmode->dm.dmPaperSize = lpDM->paperStrs[temp];

		    break;


		case CARTBOX:

		    if (lpDM->cartsVisible)

			lpDM->cartsVisible =

			    UpdateCartridges(hDB,GetDlgItem(hDB,CARTBOX), lpDM,
					     lpPI->numcart);
		    break;


                case COPYBOX:
                    {
                    BOOL flag;
		    int value;

		    value = GetDlgItemInt(hDB,COPYBOX,(BOOL FAR *)&flag,FALSE);

                    if (flag)
			lpDevmode->dm.dmCopies = value;
		    }
                    break;

                case IDOK:
		case IDCANCEL:
closeprogram:					// come here if SC_CLOSE

		    if (lpDM->cartsVisible && lpPI->numcart > 0)

                        UpdateCartridges(hDB, GetDlgItem(hDB,CARTBOX),
			   lpDM,lpPI->numcart);

		    if (HelpWasCalled)
			WinHelp(hDB, (LPSTR) "hppcl.hlp",
					(WORD) HELP_QUIT,
					(DWORD) NULL);
		    EndDialog(hDB, wParam);
		    GlobalUnlock(hDMdata);

                    return TRUE;


                case IDSOFTFONT:

		    DBMSG(("DevMode...DialogFn(): CALLING FONT INSTALLER\n"));

                    {
                    int fsvers;
		    HANDLE hFIlib;
		    // declare far ptr to InstallSoftFont()
		    int (FAR * PASCAL lpFIns)(HWND,LPSTR,LPSTR,BOOL,int,int);
		    // FARPROC lpFIns;

		    if ((hFIlib = LoadLibrary(FontInstallerStr)) < 32 ||
			!(lpFIns = GetProcAddress(hFIlib,"InstallSoftFont")))
			{
			if (hFIlib >= 32)
			    {
			    FreeLibrary(hFIlib);
			    }
			DBGerr(
			("Can't load FINSTALL library or find entry point!\n"));
			DLLErrorMsg(ERROR_BASE+2);
			break;
			}

		    // FINSTALL.DLL was loaded properly. Now call
		    // InstallSoftFont()

		    fsvers = (*lpFIns)(hDB, ModuleNameStr, lpDM->portName,
				      (GetKeyState(VK_SHIFT) < 0 &&
				       GetKeyState(VK_CONTROL) < 0),
				      lpDevmode->fsvers,
				      // 0 = PCL class fonts 
				      // bit 7 indicates 'no soft fonts'.
				      (lpDevmode->prtCaps & NOSOFT) ? 256 : 0
				      );

		    FreeLibrary(hFIlib);

		    // InstallSoftFont() returns positive non-zero value
		    // when it does something good.
		    // This value is 1 or the version number.

		    if (fsvers > 0)
			{	// Fonts changed
			HMENU hSysMenu;

			// disable the cancel button, and also disable
			//  Exit in the system menu.
                        EnableWindow(GetDlgItem(hDB,IDCANCEL),FALSE);
			if (hSysMenu = GetSystemMenu(hDB, FALSE))
			    {
			    BYTE szClose[21];

			    GetMenuString(hSysMenu, SC_CLOSE, (LPSTR)szClose,
				20, MF_BYCOMMAND);
			    ModifyMenu(hSysMenu, SC_CLOSE,
				MF_BYCOMMAND | MF_GRAYED,
				SC_CLOSE, (LPSTR)szClose);
			    }

			lpDevmode->fsvers = fsvers;

			/*  Installed cartridges may have been changed */

			GetCartList(hDB, lpDM);
            
			/*  Display or disable cartridges */

			SendDlgItemMessage(hDB, CARTBOX, LB_RESETCONTENT,0,0L);

			lpDM->cartsVisible = FALSE;
			UpdateNumCarts(hDB, lpPI->numcart);

			if (lpPI->numcart > 0)

			    lpDM->cartsVisible =

				UpdateCartridges(hDB,GetDlgItem(hDB,CARTBOX),
						 lpDM,lpPI->numcart);

			}	// end.. Fonts changed

		    /* SetFocus seems to be needed in protect mode ?! */

		    SetFocus(GetDlgItem(hDB, PRTBOX));
                    }
                    break;

		case IDOPTION:

                    DBMSG(("DevMode...DialogFn(): CALLING OPTIONS DIALOG\n"));

		    OptionsDlg(hLibInst, hDB, lpDevmode);

		    if (lpDevmode->options & OPTIONS_FORCESOFT)
			lpDevmode->prtCaps &= ~(NOSOFT);
                    break;

		case IDABOUT:
		    {
			FARPROC lpDlgFunc;

			lpDlgFunc =
				MakeProcInstance(GenericWndProc, hLibInst);
			DialogBox(hLibInst,MAKEINTRESOURCE(SFABOUT),
				    hDB, lpDlgFunc);
			FreeProcInstance(lpDlgFunc);
		    }
		    SetFocus(GetDlgItem(hDB, PRTBOX));	// ???
                    break;

		case IDHELP:
			// We must call WinHelp(.... HELP_QUIT...) when
			// we exit from the dialog now..
			HelpWasCalled = WinHelp(hDB, (LPSTR) "hppcl.hlp",
					(WORD) HELP_INDEX,
					(DWORD) 0L);
			break;

		default:    /* A message we don't process */

		    GlobalUnlock(hDMdata);
                    return FALSE;

	    }

            break;
    }

    GlobalUnlock(hDMdata);
    return FALSE;
}


/***********************************************************************
		    M E R G E  E N V I R O N M E N T
 ***********************************************************************/

/*  Merge source and destination environments into the destination. */

LOCAL void
MergeEnvironment(LPDMDATA lpDM, LPPCLDEVMODE lpDest, LPPCLDEVMODE lpSrc) {

    short pcap, value, res;
    long Fields = lpSrc->dm.dmFields;

    /* All the Laserjets and compatibiles allow portrait/landscape */

    if (Fields & DM_ORIENTATION)
	if ((value = lpSrc->dm.dmOrientation) == DMORIENT_PORTRAIT ||
	    value == DMORIENT_LANDSCAPE)
	    lpDest->dm.dmOrientation = value;

    /* Copies?	We can do that! */

    if (Fields & DM_COPIES)
	lpDest->dm.dmCopies = lpSrc->dm.dmCopies;

    /* PrintQuality?  No problem! */

    if (Fields & DM_PRINTQUALITY) {

	/* map dots per inch values to low/med/high */

	if ((value = lpSrc->dm.dmPrintQuality) >= 0) {
	    if (value <= 75)
		value = DMRES_LOW;
	    else if (value <= 150)
		value = DMRES_MEDIUM;
	    else
		value = DMRES_HIGH;
	}

	/* map low/med/high to resolution shift factor */

	switch (value) {
	    case DMRES_HIGH:	res = 0;		    break;
	    case DMRES_MEDIUM:	res = 1;		    break;
	    case DMRES_LOW:	res = 2;		    break;
	    case DMRES_DRAFT:	res = lpDest->prtResFac;    break;
	    default:		res = -1;		    break;
	}

	if (res != -1) {		/* assign values if valid */
	    lpDest->dm.dmPrintQuality = value;
	    lpDest->prtResFac = res;
	}
    }

    /* Duplex depends on the type of printer */

    if ((Fields & DM_DUPLEX) && (lpDest->dm.dmFields & DM_DUPLEX))
	if ((value = lpSrc->dm.dmDuplex) == DMDUP_SIMPLEX ||
	    value == DMDUP_VERTICAL || value == DMDUP_HORIZONTAL)
	    lpDest->dm.dmDuplex = value;

    /* The allowed range of paper sizes also depends of printer type */

    if (Fields & DM_PAPERSIZE) {
	value = lpSrc->dm.dmPaperSize;
	if (lpDM->paperbits[lpDest->paperInd] & Paper2Bit(value))
	    lpDest->dm.dmPaperSize = value;
    }

    /* Last, but not least, the source/tray/bin depends on the printer --
       BTW, the following code is similar to code in DeviceCapabilities() */

    if ((Fields & DM_DEFAULTSOURCE)&&(lpDest->dm.dmFields & DM_DEFAULTSOURCE)) {
	pcap = lpDest->prtCaps;
	value = lpSrc->dm.dmDefaultSource;
	if (value == DMBIN_UPPER ||
	    ((value == DMBIN_LOWER) && (pcap & LOTRAY))    ||
	    ((value == DMBIN_MANUAL) && !(pcap & NOMAN))   ||
	    ((value == DMBIN_AUTO) && (pcap & AUTOSELECT)) ||
	    ((value == DMBIN_ENVELOPE) && (pcap & ANYENVFEED)))
	    lpDest->dm.dmDefaultSource = value;
    }

#ifdef LOCAL_DEBUG
    DBMSG(("MergeEnvironment: merged PCLDEVMODE follows:\n"));
    dumpDevMode(lpDest);
#endif

}


/***********************************************************************
		    G E T  F I L L  P R T  L I S T
 ***********************************************************************/

/*  Read the list of available printers from the resource file. */

LOCAL WORD GetFillPrtList(hDB, printers)
    HWND hDB;
    PRTINFO FAR *printers;
    {
    BOOL infoLoaded;
    PRTINFO FAR *p, FAR *top = printers;
    short numPrinters, ind, i, numlisted;

    for (ind = 0, numlisted = 0, numPrinters = 0, infoLoaded=TRUE;
        (ind < MAX_PRINTERS) && infoLoaded;
        ++ind, ++printers)
        {
	if (infoLoaded = GetPrtItem(printers, ind, hLibInst))
            {
	    ++numPrinters;

            for (i = 0, p = top; i < ind &&
                lstrcmpi(p->devname, printers->devname); ++i, ++p)
                ;

            if (i < ind)
                {
                /*  This is a true kludge:  it comes from a history of
                 *  handling the same printer with different memory options.
                 *  Enough excuses, when we encounter a printer listed in
                 *  the resource file which matches a printer we have
                 *  already displayed in the printer list box, then we 
                 *  assume (actually, depend) it matches exactly but only
                 *  differs by the available memory.  We do not display
                 *  the printer again, and we remember the index to what
                 *  was already displayed -- this index will be used in
                 *  DialogFn() to derive the correct memory options and
                 *  correct printer.
                 */
                printers->indlistbox = p->indlistbox;
                }
            else
                {
                /*  Add unique printer to list box
                 */
		SendDlgItemMessage(hDB,PRTBOX,CB_INSERTSTRING,(WORD)-1,
                    (LONG)(LPSTR)printers->devname);

                printers->indlistbox = numlisted++;
                }

            #ifdef LOCAL_DEBUG
            dumpPrtInfo(printers);
            #endif
            }
	}
    return numPrinters;
    }


/***********************************************************************
		  U P D A T E  P A P E R  S O U R C E
 ***********************************************************************/

/*  Update the TRAYBOX combobox with paper sources supported by this
 *  printer.
 */

LOCAL short
UpdatePaperSource(HWND hDB, WORD prtCaps, short source, BOOL prevWasAuto,
		  WORD FAR *indList) {

    char buf[64];
    short i, comboInd, selectInd, strid, autoind, upperind;

    DBGpaper(("UpdatePaperSource(%d,%2x)\n",hDB,prtCaps));

    /*	Erase contents of combobox. */

    SendDlgItemMessage(hDB, TRAYBOX, CB_RESETCONTENT, 0, 0L);


    comboInd = 0;
    selectInd = -1;

    for (i = DMBIN_FIRST; i <= DMBIN_LAST; i++) {

	strid = 0;

	/* Map DMBIN_* value to string ID if printer supports it */

	switch (i) {
	    case DMBIN_UPPER:
		strid = IDS_UPPER;
		upperind = comboInd;
		break;

	    case DMBIN_LOWER:
		if (prtCaps & LOTRAY)
		    strid = IDS_LOWER;
		break;

	    case DMBIN_MANUAL:
		if (!(prtCaps & NOMAN))
		    strid = IDS_MANUAL;
		break;

	    case DMBIN_ENVELOPE:
		if (prtCaps & ANYENVFEED)
		    strid = IDS_ENVELOPE;
		break;

	    case DMBIN_AUTO:
		if (prtCaps & AUTOSELECT) {
		    strid = IDS_AUTO;
		    autoind = comboInd;
		}
		break;
	}

	if (strid && comboInd < MAX_PAPERSOURCES &&
	    LoadString(hLibInst,strid,buf,sizeof(buf))) {

	    /* keep list of paper sources sent to combobox */

	    indList[comboInd] = i;
	    if (i == source)		/* was this one already selected? */
		selectInd = comboInd;
	    comboInd++;

	    DBGpaper(("  adding %ls, comboInd=%d, i=%d\n",(LPSTR)buf,
		      comboInd,i));

	    SendDlgItemMessage(hDB, TRAYBOX, CB_INSERTSTRING,(WORD)-1,
			       (LONG)(LPSTR)buf);
	}
    }

    /* Pick a new bin if the last one selected is no longer supported */

    if (selectInd < 0)
	if (prtCaps & AUTOSELECT) {
	    source = DMBIN_AUTO;
	    selectInd = autoind;
	} else {
	    source = DMBIN_UPPER;
	    selectInd = upperind;
	}

    /* Pick autofeed if this printer supports if and last printer didn't and
       we would be using the upper tray (default) */

    if (!prevWasAuto && source == DMBIN_UPPER && (prtCaps & AUTOSELECT)) {
	source = DMBIN_AUTO;
	selectInd = autoind;
    }

    /* select desired paper size in combobox */

    SendDlgItemMessage(hDB, TRAYBOX, CB_SETCURSEL, selectInd, 0L);

    DBGpaper(("UpdatePaperSource: returning %d\n",source));

    return(source);
}


/***********************************************************************
		    U P D A T E  P A P E R  S I Z E
 ***********************************************************************/

/*  Update the combobox with the paper sizes supported by this printer.
 *  Also, select LETTER by default if the currently selected paper isn't
 *  supported.
 */

LOCAL short
UpdatePaperSize(HWND hDB, WORD bits, WORD paper, WORD FAR *indList) {

    char buf[64];
    WORD i, chkbit, comboInd, selectInd;

    DBGpaper(("UpdatePaperSize(%2x,%2x,%d,%lp)\n",hDB,bits,paper,indList));

    /*	Erase contents of combobox. */

    SendDlgItemMessage(hDB, SIZEBOX, CB_RESETCONTENT, 0, 0L);


    /*	Add paper size strings */

    comboInd = selectInd = 0;

    for (i = DMPAPER_FIRST; i <= DMPAPER_LAST; i++) {

	if (!(chkbit = Paper2Bit(i)))	    /* 0 if driver doesn't support */
	    continue;			    /*	 this paper size at all    */

	/* add to combobox if printer supports this size */

	if ((bits & chkbit) && comboInd < MAX_PAPERSIZES &&
	    LoadString(hLibInst,PaperBit2Str(chkbit),buf,sizeof(buf))) {

	    /* keep list of paper sizes sent to combobox */

	    indList[comboInd] = i;
	    if (i == paper)		/* was this one already selected? */
		selectInd = comboInd;
	    comboInd++;

	    DBGpaper(("   adding %ls, comboInd=%d, i=%d\n",(LPSTR)buf,
		      comboInd,i));

	    SendDlgItemMessage(hDB, SIZEBOX, CB_INSERTSTRING,(WORD)-1,
			       (LONG)(LPSTR)buf);

	} else	    /* assumes all printers support letter & letter is first */

	    if (paper == i)
		paper = DMPAPER_LETTER;
    }


    /*	Select item with matching size */

    SendDlgItemMessage(hDB, SIZEBOX, CB_SETCURSEL, selectInd, 0L);

    DBGpaper(("UpdatePaperSize: returning %d\n",paper));

    return (paper);
}


/***********************************************************************
			G E T  C A R T	L I S T
 ***********************************************************************/

/*  Read the list of available cartridges from the WIN.INI file. */

LOCAL void
GetCartList(HWND hDB, LPDMDATA lpDM) {

    int ind;
    BOOL infoLoaded;
    HANDLE hWinCart;
    char szBuf[64];
    CARTINFO FAR *carts = lpDM->CartStuff;
    HANDLE hRes;
    LPINT lpResList;

    DBGgetcartlist(("GetCartList()..\n"));

    /* define the "None" cartridge (index 0) */

    carts->iPCM = carts->cartind = carts->cartcount = 0;
    if (!LoadString(hLibInst,NULL_CART,carts->cartname,sizeof(carts->cartname)))
	lstrcpy(carts->cartname,"No Cartr.");
    lpDM->totalCarts = 1;
    carts++;

    if ((hRes = FindResource(hLibInst,"CLIST",MAKEINTRESOURCE(PCMFILE)))
	&& (hRes = LoadResource(hLibInst,hRes)))
      {

	for (lpResList = (LPINT)LockResource(hRes);*lpResList;lpResList++)
	  {
	    if (!GetCartName(MAKEINTRESOURCE(*lpResList),
			     carts->cartname,sizeof(carts->cartname)))
		continue;

	    carts->iPCM = -*lpResList;
	    carts->cartind = 1;
	    carts->cartcount = 0;
	    ++carts;
	    ++(lpDM->totalCarts);
	  }

	DBGgetcartlist(("GCL: Cartridge resources=%d\n", lpDM->totalCarts));

	UnlockResource(hRes);
	FreeResource(hRes);
      }

    /* access win.ini looking for cartridge info */

    MakeAppName(ModuleNameStr,lpDM->portName,szBuf,sizeof(szBuf));

    if (!(hWinCart=InitWinSF(szBuf)))
        return;

    while ((ind=NextWinCart(hWinCart,szBuf,sizeof(szBuf)))>0)
	{
	if (!GetCartName(szBuf,carts->cartname,sizeof(carts->cartname)))
	    continue;
	if ((lpDM->totalCarts) >= MAX_CARTRIDGES - 1)
	    {
	    // we just stop if there are too many cartridges listed.
	    DLLErrorMsg(ERROR_BASE+4);
	    break;
	    }
	carts->iPCM=-ind;
	carts->cartind=1;
	carts->cartcount=0;
	++carts;
	++(lpDM->totalCarts);
	}

    DBGgetcartlist(("GCL: lastcart:'%lp' totalCarts=%d\n",
	carts->cartname,
	lpDM->totalCarts));

    EndWinSF(hWinCart);

}


/***********************************************************************
	      D I S P L A Y  M E M O R Y  O P T I O N S
 ***********************************************************************/

/*  Update the memory options combobox -- this happens at startup
 *  and at a printer change.  Note that the array of prtinfo contains
 *  multiple copies of the printer where only the available memory
 *  is different, they are tied together by sharing the same devname
 *  and the same indlistbox.
 */

LOCAL VOID
DisplayMemoryOptions(hDB, printers, ind, availmem, numPrinters)
HWND hDB;
PRTINFO FAR *printers;
short ind;
short availmem;
WORD  numPrinters;
{
    short currentind, selectind, numlisted;
    PRTINFO FAR *p;

    DBGdispmemop(("DisplayMemoryOptions(%d,%lp,%d,%d): numPrinters=%d\n",
		 (HWND)hDB, printers, ind, availmem, numPrinters));

    /*	Erase contents of combobox.
     */
    SendDlgItemMessage(hDB, MEMBOX, CB_RESETCONTENT, 0, 0L);

    /*  Display all the available memory options.
     */
    for (p = &printers[ind], currentind = p->indlistbox, numlisted = 0;
	ind < numPrinters && p->indlistbox == currentind;
	++ind, ++p, ++numlisted) {

        DBGdispmemop(("...ind=%d, indlistbox=%d, currentind=%d, numlisted=%d\n",
		     ind, p[ind].indlistbox, currentind, numlisted));
        DBGdispmemop(("   availmem=%d, realmem=%ls, refmem=%d\n",
		     p->availmem, (LPSTR)p->realmem, availmem));

	SendDlgItemMessage(hDB,MEMBOX,CB_INSERTSTRING,(WORD)-1,
			  (LONG)(LPSTR)p->realmem);

        if (p->availmem == availmem)
            selectind = numlisted;
    }

    DBGdispmemop(("END...ind=%d, indlistbox=%d, currentind=%d, numlisted=%d\n",
		  ind, p[ind].indlistbox, currentind, numlisted));
    DBGdispmemop(("   availmem=%d, realmem=%ls, refmem=%d\n",
		  p->availmem, (LPSTR)p->realmem, availmem));

    /*	Select item with matching availmem.
     */
    SendDlgItemMessage(hDB, MEMBOX, CB_SETCURSEL, selectind, 0L);
}


/***********************************************************************
		      C H E C K  D U P L E X
 ***********************************************************************/

/*  Make sure the DEVMODE structure contains valid duplex info for the
 *  current printer.
 */

LOCAL void
CheckDuplex(LPPCLDEVMODE lpDevmode) {

    if (lpDevmode->prtCaps & ANYDUPLEX) {
	if (!(lpDevmode->dm.dmFields & DM_DUPLEX)) {
	    lpDevmode->dm.dmFields |= DM_DUPLEX;
	    lpDevmode->dm.dmDuplex = DMDUP_SIMPLEX;
	}
    } else {
	lpDevmode->dm.dmFields &= ~(DM_DUPLEX);
	lpDevmode->dm.dmDuplex = 0;
    }
}

// This function selects and displays the icon for the orientation groupbox.

void NEAR PASCAL SetOrientIcon(HWND hDB, LPPCLDEVMODE lpDevmode)
{
    LPSTR lpIconName;
    extern HANDLE hLibInst;

    // select the icon resource name
    lpIconName = (lpDevmode->dm.dmOrientation == DMORIENT_PORTRAIT) ?
	(LPSTR) "ICO_PORTRAIT": (LPSTR) "ICO_LANDSCAPE";

    // load the icon and display it in the dialog
    SetDlgItemText(hDB, IDPORTLAND,
	MAKEINTRESOURCE(LoadIcon(hLibInst, lpIconName)));

}	// SetOrientIcon()

/***********************************************************************
		    U P D A T E  C A R T R I D G E S
 ***********************************************************************/

/*  Update the cartridge information in the cartridge listbox.	This proc
 *  can be called at startup, when a printer changes, or when the cartridge
 *  list box is changed.
 */

LOCAL BOOL
UpdateCartridges(hDB, hLB, lpDM, limit)
HWND hDB;
HWND hLB;
LPDMDATA lpDM;
WORD limit;
{
    WORD ind, j;
    LPPCLDEVMODE lpDevmode = &lpDM->CurEnv;
    CARTINFO FAR *lpCarts = lpDM->CartStuff;

    DBGupdatecart(("UpdateCartridges(%d,%d,%lp,%lp,%d) vis=%ls, numcart=%d\n",
        (HWND)hDB, (HWND)hLB, lpDevmode, lpCarts, limit,
	(lpDM->cartsVisible) ? (LPSTR)"TRUE" : (LPSTR)"FALSE",
        lpDevmode->numCartridges));

    if (lpDevmode->numCartridges > DEVMODE_MAXCART)
        {
        DBGupdatecart(
		("sanity check: numCartridges=%d, bigger than max (%d)!\n",
            lpDevmode->numCartridges, DEVMODE_MAXCART));
        lpDevmode->numCartridges = DEVMODE_MAXCART;
        }

    if (!lpDM->cartsVisible)
        {
        WORD lastCart = 0;
        WORD firstCart = 0;

	DBGupdatecart(("UC(): %d selected out of %d\n",
	    lpDevmode->numCartridges,lpDM->totalCarts));

        /*  Disable drawing of listbox while we fill it in.
         */
        SendMessage(hLB, WM_SETREDRAW, FALSE, 0L);

        /*  The cartridge listbox is empty, fill it in.
         */
	for (ind = 0; ind < lpDM->totalCarts; ++ind, ++lpCarts)
            {
            SendMessage(hLB, LB_INSERTSTRING, (WORD)(-1),
                (LONG)(LPSTR)lpCarts->cartname);
            }

        SendMessage(hLB, LB_SETSEL, FALSE, (long)(-1));

	/*  Highlight the cartridges listed in CurEnv.
         */
        for (ind = 0; ind < lpDevmode->numCartridges; )
            {
            short iT;

	    if (((iT=LBCartIndex(lpDM,ind)) == 0) &&
                (lpDevmode->numCartridges > 1))
                {
                DBGupdatecart(
    ("suppressing highlight of 'none' because other cartridges selected\n"));
                shiftCarts(lpDevmode, ind, lpDevmode->numCartridges--);
                }
            else if (iT>=0)
                {
                DBGupdatecart(("highlighting cartridge %d\n",iT));
                SendMessage(hLB, LB_SETSEL, TRUE, (long)iT);
                lastCart = iT;
		// firstCart is smallest non-zero cartridge no.
		if ((!firstCart) || (iT < firstCart))
		    firstCart = iT;
                ++ind;
                }
            else
                {
                DBGupdatecart(("no cartridge %d\n",iT));
                shiftCarts(lpDevmode, ind, lpDevmode->numCartridges--);
                }
            }

	DBGupdatecart(("first cartridge is %d\n",firstCart));

	// enable redraw .. do this before LB_SETTOPINDEX
        SendMessage(hLB, WM_SETREDRAW, TRUE, 0L);

	// move first cartridge to top of listbox
	if (firstCart)
	    SendMessage(hLB, LB_SETTOPINDEX, firstCart, 0L);

        /*  invalidate listbox it so
         *  it will be drawn immediately.
         */
        InvalidateRect(hLB, (LPRECT)0L, FALSE);

        }
    else
        {
	// see if None (first item) has been selected.
	BOOL bNone = (BOOL)SendDlgItemMessage(hDB, CARTBOX, LB_GETSEL, 0, 0L);

	#ifdef DEBUG
	DBGupdatecart(("'None' has been selected\n"));
	#endif

        /*  For each currently selected cartridge...
         */
        for (ind = 0; ind < lpDevmode->numCartridges; )
            {

	    int iT=LBCartIndex(lpDM,ind);

            /*  Verify that the cartridge is still selected.
             */
            DBGupdatecart(("verify cartridge %d\n",iT));

            if (!SendDlgItemMessage(hDB, CARTBOX, LB_GETSEL,iT,0L))
                {
                /*  Cartridge is not selected, remove it from the list.
                 */
                DBGupdatecart(("...not selected anymore\n"));
                shiftCarts(lpDevmode, ind, lpDevmode->numCartridges--);
                }
	    else if (bNone && (iT > 0))
		{
		// Cartridge IS selected, but 'None' was selected, too
		DBGupdatecart(("'None' selected, deselect %d\n", iT));
                shiftCarts(lpDevmode, ind, lpDevmode->numCartridges--);
		SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, FALSE, (long)iT);
		}
            else
                ++ind;
            }

	bNone = FALSE;

        /*  For each cartridge in the listbox.
         */
	for (ind = 0; ind < lpDM->totalCarts; ++ind)
            {
            /*  Continue if we already know cartridge is selected.
             */
            for (j = 0; j < lpDevmode->numCartridges; ++j)
                {
		if (LBCartIndex(lpDM,j) == ind)
                    break;
                }
            if (j < lpDevmode->numCartridges)
                continue;

            /*  Check to see if cartridge is selected.
             */
            if (SendDlgItemMessage(hDB, CARTBOX, LB_GETSEL, ind, 0L))
                {
                /*  Cartridge IS selected.
                 */
                DBGupdatecart(("%d NOW selected\n", ind));

                /*  Don't allow the user to select 'none' if there
                 *  is more than one cartridge selected.
                 */
                if ((ind == 0) && (lpDevmode->numCartridges > 1))
                    {
                    DBGupdatecart(
		("(1)...0 unselected because other cartridges are selected"));
                    DBGupdatecart((", numCartridges = %d\n",
				lpDevmode->numCartridges));
                    SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, FALSE, 0L);
                    continue;
                    }

                /*  Make room for cartridge if necessary.
                 */
                if (lpDevmode->numCartridges >= DEVMODE_MAXCART)
                    {
                    short iT;

                    /*  Overflow, knock out the first cartridge or 'none'.
                     */
                    for (j = 0; j < DEVMODE_MAXCART; ++j)
                        {
                        if (lpDevmode->cartIndex[j] == 0)
                            break;
                        }
                    if (j == DEVMODE_MAXCART)
                        j = 0;

		    iT=LBCartIndex(lpDM,j);

                    DBGupdatecart(("...overflow, remove %d\n",iT));
                    SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, FALSE,
                        (long)iT);

                    shiftCarts(lpDevmode, j, DEVMODE_MAXCART);
                    j = lpDevmode->numCartridges - 1;
                    }
                else
                    {
                    j = lpDevmode->numCartridges++;
                    }

                /*  Add to list of cartridges.
                 */
                lpDevmode->cartIndex[j] = lpCarts[ind].iPCM;
                lpDevmode->cartind[j] = lpCarts[ind].cartind;
                lpDevmode->cartcount[j] = lpCarts[ind].cartcount;
                }
            }
        }

    /*  No cartridges selected -- select "none" (first item).
     */
    if (!lpDevmode->numCartridges)
        {
        DBGupdatecart(("no cartridges, selecting 'None'\n"));

        lpDevmode->numCartridges = 1;
        lpDevmode->cartIndex[0] = 0;
        lpDevmode->cartind[0] = lpCarts[0].cartind;
        lpDevmode->cartcount[0] = lpCarts[0].cartcount;
        SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, TRUE, 0L);
        }

    /*  More than one cartridge selected, look for 'none'
     *  (first item) and unselect it.
     */
    if (lpDevmode->numCartridges > 1)
        {
        for (ind = 0; ind < lpDevmode->numCartridges; )
            {
            if (lpDevmode->cartIndex[ind] == 0)
                {
                DBGupdatecart(
		("(2)...0 unselected because other cartridges are selected"));
		DBGupdatecart((", numCartridges = %d\n",
			    lpDevmode->numCartridges));
                SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, FALSE, 0L);
                shiftCarts(lpDevmode, ind, lpDevmode->numCartridges--);
                }
            else
                ++ind;
            }
        }

    /*  Deselect some cartridges if the user has selected more than
     *  is allowed for this printer.
     */
    while (lpDevmode->numCartridges > limit)
        {
	int iT=LBCartIndex(lpDM,0);
        DBGupdatecart(("deselecting %d\n", iT));

        SendDlgItemMessage(hDB, CARTBOX, LB_SETSEL, FALSE, (long)iT);
        shiftCarts(lpDevmode, 0, lpDevmode->numCartridges--);
        }


    #ifdef LOCAL_DEBUG
    DBGupdatecart(("numCartridges=%d\n", lpDevmode->numCartridges));

    for (ind = 0; ind < DEVMODE_MAXCART; ++ind)
        {
        DBGupdatecart(("%d:  Index=%d, cartind=%d, cartcount=%d\n",
            ind, lpDevmode->cartIndex[ind], lpDevmode->cartind[ind],
            lpDevmode->cartcount[ind]));
        }
    #endif

    return TRUE;
    }

/***********************************************************************
			   S H I F T  C A R T S
 ***********************************************************************/

/*  Remove the cartridge entry at ind and shift all cartridge entries
 *  after it up one.
 */

LOCAL void
shiftCarts(lpDevmode, ind, last)
LPPCLDEVMODE lpDevmode;
WORD ind;
WORD last;
{
    DBGupdatecart(("shiftCarts(%lp,%d,%d)\n", lpDevmode, ind, last));

    for (++ind; ind < last; ++ind)
	lpDevmode->cartIndex[ind-1] = lpDevmode->cartIndex[ind];

    lpDevmode->cartIndex[--last] = 0;
    lpDevmode->cartind[last] = 0;
    lpDevmode->cartcount[last] = 0;
}

/***********************************************************************
		       U P D A T E  N U M  C A R T S
 ***********************************************************************/

/*  Update number of cartridges in string above listbox.
 */

LOCAL void UpdateNumCarts(hDB, limit)
    HWND hDB;
    WORD limit;
    {
    char numcarts[64];

    DBGupdatenumcarts(("UpdateNumCarts(.. limit=%d)", limit));

    if (limit > 0)
        {
        /*  Load resource string, search for the '%' sign and replace
         *  it with the maximum allowable cartridges.
         */
	if (LoadString(hLibInst,IDS_NUMCARTS,numcarts,sizeof(numcarts)))
            {
            LPSTR s;

            for (s=numcarts ; *s && *s!='%' ; s++ )
                ;

            if (lstrlen(numcarts) < sizeof(numcarts) - 20)
                {
                char temp[10];
                lmemcpy(temp, &s[1], sizeof(temp));
                s += itoa(limit, s);
                lstrcpy(s, temp);
                }

            SetDlgItemText(hDB, NUMCARTS, numcarts);
            }
        }
    else
        {
        /*  No cartridges may be selected.
         */
	if (LoadString(hLibInst,IDS_NOCARTS,numcarts,sizeof(numcarts)))
            SetDlgItemText(hDB, NUMCARTS, numcarts);
        }
	DBGupdatenumcarts((" .. \n"));
    }

/***********************************************************************
			 L B  C A R T  I N D E X
 ***********************************************************************/

/*  Given an index of a cartridge from the lpDevmode, returns its position
 *  in the list box...
 */

LOCAL short LBCartIndex(LPDMDATA lpDM, short ind)
    {
    short i;

    for (i=0; i < lpDM->totalCarts; i++)
	if (lpDM->CartStuff[i].iPCM==lpDM->CurEnv.cartIndex[ind])
            return i;
    return -1;
    }

/***********************************************************************
		      W R I T E  W I N	I N I  E N V
 ***********************************************************************/

/*  Write the environment information to the win.ini file.
 */

LOCAL void WriteWinIniEnv(lpDevmode, lpOldDevmode, lpPortName, lpProfile)
    LPPCLDEVMODE lpDevmode;
    LPPCLDEVMODE lpOldDevmode;
    LPSTR lpPortName;
    LPSTR lpProfile;
    {
    char appName[64];
    char name[16];
    char str_data[16];
    short ind, data, oldData, tmp;

    MakeAppName(ModuleNameStr,lpPortName,appName,sizeof(appName));

    DBMSG(("WriteWinIniEnv(%lp,%lp), appName=%ls\n",
	  lpDevmode, lpPortName, (LPSTR)appName));

    /*  For each dialog item.
     */
    for (ind = WININI_BASE; ind < WININI_LAST; ++ind)
        {
        tmp = ind;

        /*  Pick up the data
         */
        switch (ind)
            {
            case WININI_PAPER:
		oldData = lpOldDevmode->dm.dmPaperSize;
		// force write of paper= if this is a default value,
		// Letter or A4.
		if ((oldData == DMPAPER_LETTER) || (oldData == DMPAPER_A4))
			oldData = -1;
		data = lpDevmode->dm.dmPaperSize;
                break;

            case WININI_COPIES:
		oldData = lpOldDevmode->dm.dmCopies;

                /* ALWAYS SAY ONE COPY
		 * data = lpDevmode->dm.dmCopies;
                 */
                data = 1;
                break;

            case WININI_ORIENT:
		oldData = lpOldDevmode->dm.dmOrientation;
		data = lpDevmode->dm.dmOrientation;
                break;
                
            case WININI_PRTRESFAC:
                oldData = lpOldDevmode->prtResFac;
                data = lpDevmode->prtResFac;
                break;

            case WININI_TRAY:
		oldData = lpOldDevmode->dm.dmDefaultSource;

                /*  Do not write manual or envelope feed.
                 */
		if (lpDevmode->dm.dmDefaultSource == DMBIN_MANUAL ||
		    lpDevmode->dm.dmDefaultSource == DMBIN_ENVELOPE)

		    lpDevmode->dm.dmDefaultSource =
					  (lpDevmode->prtCaps & AUTOSELECT) ?
					   DMBIN_AUTO  :  DMBIN_UPPER;

		data = lpDevmode->dm.dmDefaultSource;
                break;

            case WININI_PRTINDEX:
                oldData = lpOldDevmode->prtIndex;
                data = lpDevmode->prtIndex;
                break;

            case WININI_NUMCART:
                oldData = lpOldDevmode->numCartridges;
                data = lpDevmode->numCartridges;
                break;

            case WININI_DUPLEX:
		oldData = lpOldDevmode->dm.dmDuplex;
		data = lpDevmode->dm.dmDuplex;
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
                    oldData = lpOldDevmode->cartIndex[tmp];
                    data = lpDevmode->cartIndex[tmp];
                    if (data<0)
                        data=-data;
                    }
                else
                    {
                    /*  If entry exists, erase it.
                     */
                    name[0] = '\0';
                    if (tmp < lpOldDevmode->numCartridges &&
			LoadString(hLibInst,ind,(LPSTR)name,sizeof(name)))

			if (lpProfile) {
			    if (GetPrivateProfileInt(
					appName,name,-1,lpProfile) > 0)
				WritePrivateProfileString(appName,
					name,NullStr,lpProfile);
			} else
			    if (GetProfileInt(appName,name,-1) > 0)
				WriteProfileString(appName,name,NullStr);

                    data = oldData = 0;
                    }
                break;

            case WININI_TXWHITE:
                oldData = lpOldDevmode->txwhite;
                data = lpDevmode->txwhite;
                break;

            case WININI_OPTIONS:
                oldData = lpOldDevmode->options;
                data = lpDevmode->options;
                break;

            case WININI_FSVERS:
                oldData = lpOldDevmode->fsvers;
                data = lpDevmode->fsvers;
                break;

            case WININI_PRTCAPS:
                /*  The driver does not rely on getting this information
                 *  from the win.ini file.  It uses the prtCaps field for
                 *  the printer from the resource file.  This field is
                 *  written to the win.ini file so other apps can use it.
                 */
                oldData = lpOldDevmode->prtCaps;
                data = lpDevmode->prtCaps;
                break;

            case WININI_PAPERIND:
                oldData = lpOldDevmode->paperInd;
                data = lpDevmode->paperInd;
                break;

            default:
                oldData = 0;
                data = 0;
                break;
            }

        /*  Write data to win.ini
         */
        if (data != oldData)
            {
            name[0] = '\0';
            str_data[0] = '\0';
	    if (LoadString(hLibInst, ind, (LPSTR)name, sizeof(name)) &&
                itoa(data, (LPSTR)str_data))
		{
		if (lpProfile)
		    WritePrivateProfileString(appName,name,str_data,lpProfile);
		else
		    WriteProfileString(appName,name,str_data);
                }
            }
	}

    }

/***********************************************************************
		  P A P E R  B I T  2  S T R
 ***********************************************************************/

/*  Map paper bits to name string. */

LOCAL short
PaperBit2Str(short paper) {

    switch(paper) {

	case PAPERID_LETTER:	    return IDS_LETTER;
	case PAPERID_LEGAL:	    return IDS_LEGAL;
	case PAPERID_LEDGER:	    return IDS_LEDGER;
	case PAPERID_EXEC:	    return IDS_EXEC;
	case PAPERID_A3:	    return IDS_A3;
	case PAPERID_A4:	    return IDS_A4;
	case PAPERID_B5:	    return IDS_B5;
    }

    return IDS_LETTER;
}

/***********************************************************************
		     D E B U G	  R O U T I N E S
 ***********************************************************************/

#ifdef LOCAL_DEBUG
LOCAL void
dumpDevMode(LPPCLDEVMODE lpEnv) {

#ifdef FLARP
    short ind;
#endif

    DBGDevMode(("     dmDeviceName: %ls\n",(LPSTR)lpEnv->dm.dmDeviceName));
    DBGDevMode(("     dmSpecVersion: %4xh\n",lpEnv->dm.dmSpecVersion));
    DBGDevMode(("     dmDriverVersion: %4xh\n",lpEnv->dm.dmDriverVersion));
    DBGDevMode(("     dmSize: %d\n",lpEnv->dm.dmSize));
    DBGDevMode(("     dmDriverExtra: %d\n",lpEnv->dm.dmDriverExtra));
    DBGDevMode(("     dmFields: %8lxh\n",lpEnv->dm.dmFields));
    DBGDevMode(("     dmOrientation: %d\n",lpEnv->dm.dmOrientation));
    DBGDevMode(("     dmPaperSize: %d\n",lpEnv->dm.dmPaperSize));
    DBGDevMode(("     dmCopies: %d\n",lpEnv->dm.dmCopies));
    DBGDevMode(("     dmDefaultSource: %d\n",lpEnv->dm.dmDefaultSource));
    DBGDevMode(("     dmPrintQuality: %d\n",lpEnv->dm.dmPrintQuality));
    DBGDevMode(("     dmColor: %d\n",lpEnv->dm.dmColor));
    DBGDevMode(("     dmDuplex: %d\n",lpEnv->dm.dmDuplex));
    DBGDevMode(("     prtIndex: %d\n",lpEnv->prtIndex));
    DBGDevMode(("     romind: %d\n",lpEnv->romind));
    DBGDevMode(("     romcount: %d\n",lpEnv->romcount));
    DBGDevMode(("     prtCaps: %4xh\n",lpEnv->prtCaps));
    DBGDevMode(("     prtResFac: %d\n",lpEnv->prtResFac));
    DBGDevMode(("     availmem: %d KB\n",lpEnv->availmem));
    DBGDevMode(("     paperInd: %d\n",lpEnv->paperInd));
    DBGDevMode(("     maxPgSoft: %d KB\n",lpEnv->maxPgSoft));
    DBGDevMode(("     maxSoft: %d KB\n",lpEnv->maxSoft));
    DBGDevMode(("     numCartridges: %d\n", lpEnv->numCartridges));

#ifdef FLARP
    for (ind = 0; ind < DEVMODE_MAXCART; ++ind) {
        DBGDevMode(("%d:  cartIndex=%d, cartind=%d, cartcount=%d\n",
	    ind, lpEnv->cartIndex[ind], lpEnv->cartind[ind],
	    lpEnv->cartcount[ind]));
    }
#endif
}


LOCAL void
dumpPrtInfo(PRTINFO FAR *pinfo) {

    DBGPrtInfo(("PrtInfo is  devname: %ls\n",pinfo->devname));
    DBGPrtInfo(("            availmem: %d\n",pinfo->availmem));
    DBGPrtInfo(("            realmem: %ls\n",pinfo->realmem));
    DBGPrtInfo(("            caps: %d\n",pinfo->caps));
    DBGPrtInfo(("            romind: %d\n",pinfo->romind));
    DBGPrtInfo(("            romcount: %d\n",pinfo->romcount));
    DBGPrtInfo(("            maxpgsoft: %d\n",pinfo->maxpgsoft));
    DBGPrtInfo(("            maxsoft: %d\n",pinfo->maxsoft));
    DBGPrtInfo(("            numcart: %d\n",pinfo->numcart));
    DBGPrtInfo(("            indlistbox: %d\n",pinfo->indlistbox));
    DBGPrtInfo(("            indpaperlist: %d\n",pinfo->indpaperlist));
}   


void
dumpCartInfo(CARTINFO FAR *cinfo) {

    DBGCartInfo(("CartInfo is cartname: %ls\n",cinfo->cartname));
    DBGCartInfo(("            cartind: %d\n",cinfo->cartind));
    DBGCartInfo(("            cartcount: %d\n",cinfo->cartcount));
}   
#endif

// DLLErrorMsg()

#define BUF_LEN 64


/*	DLLErrorMsg
 *
 *	Dump out 'wrong version of FINSTALL' error message to the user.
 */
LOCAL void DLLErrorMsg(mesno)
int mesno;
    {
    char	capbuf[BUF_LEN];
    char	textbuf[BUF_LEN];
    extern	HANDLE hLibInst;

    if (LoadString(hLibInst, ERROR_BASE+3, (LPSTR) capbuf, BUF_LEN))
	if(LoadString(hLibInst, mesno, (LPSTR) textbuf, BUF_LEN))
	    MessageBox(NULL,textbuf,capbuf,MB_OK);
    }
