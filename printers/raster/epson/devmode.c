/*
 *  DEVMODE.C
 *
 *  Contains code which implements the device mode portion of the driver:
 *  ExtDeviceMode, DeviceCapabilities, DeviceMode, and the related user
 *  interface.
 *
 */

#include <generic.h>
#include <resource.h>
#include <drivinit.h>

#define DMPAPER_FANFOLD (DMPAPER_USER+1)

#ifndef DEVICEEXTRA
#define DEVICEEXTRA 0
#endif

LPDEVMODE lpdmGlobal = NULL;
LPSTR lpDevGlobal;

char szDialogTitle[160];

char szWinIni[] = "Win.Ini";

HANDLE hInstance;

BOOL FAR PASCAL isUSA();
BOOL FAR PASCAL DevModeDlgProc(HWND, WORD, WORD, DWORD);

/*****************************************************
 *  GetProfileEntry() -
 *
 */

int near pascal GetProfileEntry(LPSTR lpSection, WORD iTag, LPSTR lpFile)
{
    char sz[32];

    if (!HIWORD(lpFile))
	lpFile = szWinIni;

    if (!LoadString(hInstance, iTag, sz, sizeof(sz)))
	return 0;

    return GetPrivateProfileInt(lpSection,sz,0,lpFile);
}


/******************************************************
 *  WriteProfileEntry() -
 *
 *  Writes an integer to an .ini file
 */

void near pascal WriteProfileEntry(LPSTR lpSection, WORD iTag, WORD wInt,
    LPSTR lpFile)
{
    char sz1[32];
    char sz2[32];

    if (!HIWORD(lpFile))
	lpFile = szWinIni;

    if (!LoadString(hInstance,iTag,sz1,sizeof(sz1)))
	return;

    wsprintf(sz2,"%d",wInt);

    WritePrivateProfileString(lpSection,sz1,sz2,lpFile);
}

#if DEVMODE_WIDEPAPER

/******************************************************
 *  IsWideCarriagePrinter() -
 *
 *  Returns true if the printer has a wide carriage
 */

BOOL NEAR PASCAL IsWideCarriagePrinter(LPSTR lpDevice)
{
#ifdef EPSON
    if (!lstrcmpi("Epson FX-80",lpDevice))
	return FALSE;
#endif

#ifdef IBMGRX
    if (!lstrcmpi("Okidata 92/192 (IBM)",lpDevice))
	return FALSE;
#endif

    /* all other printers are wide carriage
     */
    return TRUE;
}

/******************************************************
 *  SetPaperWidth() -
 *
 *  Sets the dmPaperWidth according to the paper index and
 *  whether we are using a wide carriage or not
 */

void NEAR PASCAL SetPaperWidth(
    LPDEVMODE lpdm,
    BOOL fWide)
{
    if (fWide)
	lpdm->dmPaperWidth = 3683; /* 14.5 in */
    else
	switch (lpdm->dmPaperSize)
	  {
	case DMPAPER_LETTER:
	    lpdm->dmPaperWidth = 2159;	/* 8.5 in */
	    break;

	case DMPAPER_A4:
	    lpdm->dmPaperWidth = 2100;	/* 210 mm */
	    break;

	case DMPAPER_FANFOLD:
	    lpdm->dmPaperWidth = 2038;	/* 8 1/42 in */
	    break;
	  }
}
#endif

/*****************************************************
 *  ExtDeviceMode() -
 *
 *  General routine for manipulating DEVMODE... read, write, prompt, and
 *  do Win.Ini things.	Exported as part of the support for 3.0 application
 *  initialization stuff, and used internally in a couple spots.
 */

WORD FAR PASCAL ExtDeviceMode(
    HWND hWndParent,		    /* parent for DM_PROMPT dialog box */
    HANDLE hModule,		    /* handle from LoadLibrary() */
    LPDEVMODE lpdmOut,		    /* output DEVMODE for DM_COPY */
    LPSTR lpDevName,		    /* device name */
    LPSTR lpPortName,		    /* port name */
    LPDEVMODE lpdmIn,		    /* input DEVMODE for DM_MODIFY */
    LPSTR lpProfile,		    /* alternate .INI file */
    WORD wMode) 		    /* operation(s) to carry out */
{
    LPDEVMODE lpdm;
    HANDLE hDM=NULL;
    LPSTR lpT;
    WORD wResult;
#if DEVMODE_WIDEPAPER
    BOOL fWidePrinter;
#endif

    if (wMode == 0)
	return sizeof(DEVMODE) + DEVICEEXTRA;

    /* allow only one dialog box at a time
     */
    if (wMode & DM_PROMPT && lpdmGlobal)
	return FALSE;

    if (!(hDM=GlobalAlloc(GHND,(DWORD)sizeof(DEVMODE)+DEVICEEXTRA)))
	return FALSE;

#if DEVMODE_WIDEPAPER
    fWidePrinter = IsWideCarriagePrinter(lpDevName);
#endif

    lpdm = (LPDEVMODE)GlobalLock(hDM);

    /* set the defaults and zero out the unused fields */
    if (GetEnvironment(lpPortName,(LPSTR)lpdm,sizeof(DEVMODE)+DEVICEEXTRA)
	!= sizeof(DEVMODE)+DEVICEEXTRA
	|| lstrcmpi(lpdm->dmDeviceName,lpDevName)
	|| lpdm->dmSpecVersion != DM_SPECVERSION
	|| lpdm->dmDriverVersion != 0x300
	|| lpdm->dmSize != sizeof(DEVMODE)
	|| lpdm->dmDriverExtra != DEVICEEXTRA)
      {
	/* we didn't get a valid environment... set up a new default
	 */

	/* these fields always have the same values...
	 */
	lstrcpy(lpdm->dmDeviceName,lpDevName);
	lpdm->dmSpecVersion = DM_SPECVERSION;
	lpdm->dmDriverVersion = 0x300;
	lpdm->dmSize = sizeof(DEVMODE);
	lpdm->dmDriverExtra = DEVICEEXTRA;
	lpdm->dmFields = DM_ORIENTATION
		       | DM_PAPERSIZE
#if DEVMODE_NO_PRINT_QUALITY
#else
		       | DM_PRINTQUALITY
#endif
#if COLOR
		       | DM_COLOR
#endif
#if DEVMODE_WIDEPAPER
		       | DM_PAPERWIDTH
#endif
		       ;

	/* always unused
	 */
	lpdm->dmPaperLength =

#if DEVMODE_WIDEPAPER
#else
	lpdm->dmPaperWidth =
#endif

	lpdm->dmCopies =
	lpdm->dmDefaultSource =

#if DEVMODE_NO_PRINT_QUALITY
	lpdm->dmPrintQuality =
#endif

#if COLOR
#else
	lpdm->dmColor =
#endif

	lpdm->dmDuplex = 0;

	/* now set the remaining fields according to the settings in
	 * win.ini.  keep ds from moving while we do this.
	 */
	LockData(0);

	/* get the orientation
	 */
	if (GetProfileEntry(lpDevName,IDS_ORIENT,lpProfile)
	    == DMORIENT_LANDSCAPE)
	  {
	    lpdm->dmOrientation = DMORIENT_LANDSCAPE;
	  }
	else
	  {
	    lpdm->dmOrientation = DMORIENT_PORTRAIT;
	  }

	/* get the paper size
	 */
	switch (GetProfileEntry(lpDevName,IDS_PAPER,lpProfile))
	  {
	case DMPAPER_FANFOLD:
	    lpdm->dmPaperSize = DMPAPER_FANFOLD;
	    break;

	case DMPAPER_A4:
	    lpdm->dmPaperSize = DMPAPER_A4;
	    break;

	case DMPAPER_LETTER:
	    lpdm->dmPaperSize = DMPAPER_LETTER;
	    break;

	default:
	    if (isUSA())
		lpdm->dmPaperSize = DMPAPER_LETTER;
	    else
		lpdm->dmPaperSize = DMPAPER_A4;
	    break;
	  }

#if DEVMODE_NO_PRINT_QUALITY
#else
	if (GetProfileEntry(lpDevName,IDS_DRAFTMODE,lpProfile))
	    lpdm->dmPrintQuality = DMRES_DRAFT;
	else
	    lpdm->dmPrintQuality = DMRES_HIGH;
#endif

#if COLOR
	switch (GetProfileEntry(lpDevName,IDS_COLOR,lpProfile))
	  {
	case 1:
	    /* black only --- monochrome
	     */
	    lpdm->dmColor = DMCOLOR_MONOCHROME;
	    break;

#ifdef IBMCOLOR
	case 2:
	    /* RGBK ribbon. There is an additional flag word after the
	     * devmode indicating the ribbon type.  It is true for
	     * the rgb ribbon, false for cmyk.
	     */
	    lpdm->dmColor = DMCOLOR_COLOR;
	    *(LPINT)(lpdm+1) = TRUE;
	    break;
#endif

	default:
	    /* CMYK.
	     */
	    lpdm->dmColor = DMCOLOR_COLOR;
#ifdef IBMCOLOR
	    *(LPINT)(lpdm+1) = FALSE;
#endif
	    break;
	  }
#endif

#if DEVMODE_WIDEPAPER
	/* the units of dmPaperSize are tenths of a millimeter
	 */
	SetPaperWidth(lpdm,
	    fWidePrinter
	      ? GetProfileEntry(lpDevName,IDS_WIDE,lpProfile)
	      : FALSE);
#endif

	UnlockData(0);

	/* we now have a valid environment.  Since it is the default
	 * (unmodified) devmode, place it back in GDI's default environment
	 * so we don't have to go through this each time we need it.
	 */
	if (!HIWORD(lpProfile))
	    SetEnvironment(lpPortName,(LPSTR)lpdm,sizeof(DEVMODE)+DEVICEEXTRA);
      }

    if (wMode & DM_MODIFY && lpdmIn)
      {
	/* validate the user's modifications to the devmode structure
	 */

	if (lpdmIn->dmFields & DM_ORIENTATION)
	  {
	    if (lpdmIn->dmOrientation == DMORIENT_PORTRAIT
		|| lpdmIn->dmOrientation == DMORIENT_LANDSCAPE)
	      {
		lpdm->dmOrientation = lpdmIn->dmOrientation;
	      }
	  }

	if (lpdmIn->dmFields & DM_PAPERSIZE)
	  {
	    switch (lpdmIn->dmPaperSize)
	      {
	    case DMPAPER_LETTER:
	    case DMPAPER_A4:
	    case DMPAPER_FANFOLD:
		lpdm->dmPaperSize = lpdmIn->dmPaperSize;
	      }
	  }

#if DEVMODE_WIDEPAPER
	if (fWidePrinter && lpdmIn->dmFields & DM_PAPERWIDTH)
	  {
	    SetPaperWidth(lpdm, lpdmIn->dmPaperWidth > 3000);
	  }
#endif

#if DEVMODE_NO_PRINT_QUALITY
#else
	if (lpdmIn->dmFields & DM_PRINTQUALITY)
	  {
	    if (lpdmIn->dmPrintQuality == DMRES_DRAFT)
		lpdm->dmPrintQuality = DMRES_DRAFT;
	    else
		lpdm->dmPrintQuality = DMRES_HIGH;
	  }
#endif
#if COLOR
	if (lpdmIn->dmFields & DM_COLOR)
	  {
	    if (lpdmIn->dmColor == DMCOLOR_MONOCHROME)
		lpdm->dmColor = DMCOLOR_MONOCHROME;
	    else
		lpdm->dmColor = DMCOLOR_COLOR;
	  }

#ifdef IBMCOLOR
	/* if we got device extra data of the correct length and the driver
	 * version number is correct, get the ribbon type from the input
	 * data.
	 */
	if (lpdmIn->dmDriverExtra == DEVICEEXTRA
	    && lpdmIn->dmDriverVersion == 0x300)
	  {
	    /* set the ribbon type according to the flag in the
	     * driver extra information after the devmode structure
	     */
	    *(LPINT)(lpdm+1) = *(LPINT)(lpdmIn+1);
	  }
#endif
#endif
      }

    if (wMode & DM_PROMPT)
      {

	lpdmGlobal = lpdm;
	lpDevGlobal = lpDevName;

	/* if prompt specified, allow the user to modify the environment
	 * we've got so far
	 */
	wResult = DialogBoxParam(hInstance,
		       MAKEINTRESOURCE(IDD_DEVMODE),
		       hWndParent,
		       (FARPROC)DevModeDlgProc,
		       (LONG)0L);

	lpdmGlobal = (LPDEVMODE)NULL;
      }
    else
	wResult = IDOK;

    if (wMode & DM_UPDATE && wResult == IDOK)
      {
	LockData(0);

	/* changing the defaults: write profile strings and environment
	 */
	WriteProfileEntry(lpDevName,IDS_ORIENT,lpdm->dmOrientation,lpProfile);
	WriteProfileEntry(lpDevName,IDS_ORIENT,lpdm->dmOrientation,lpProfile);
	WriteProfileEntry(lpDevName,IDS_PAPER,lpdm->dmPaperSize,lpProfile);

#if DEVMODE_WIDEPAPER
	WriteProfileEntry(lpDevName,IDS_WIDE,
		lpdm->dmPaperWidth > 3000,
		lpProfile);
#endif

#if COLOR
#ifdef IBMCOLOR
	WriteProfileEntry(lpDevName,IDS_COLOR,
		       (lpdm->dmColor==DMCOLOR_MONOCHROME ? 1 :
			 ((BOOL)*(LPINT)(lpdm+1) ? 2 : 0)),
		       lpProfile);
#else
	WriteProfileEntry(lpDevName,IDS_COLOR,
		     lpdm->dmColor==DMCOLOR_COLOR ? 0 : 1,
		     lpProfile);

#endif /* IBMCOLOR */

#endif /* COLOR */

#if DEVMODE_NO_PRINT_QUALITY
#else
	WriteProfileEntry(lpDevName,IDS_DRAFTMODE,
	  lpdm->dmPrintQuality == DMRES_DRAFT ? 1 : 0 ,
	  lpProfile);
#endif

	UnlockData(0);

	/* put the new default environment into a GDI buffer
	 */
	SetEnvironment(lpPortName,(LPSTR)lpdm,sizeof(DEVMODE)+DEVICEEXTRA);

	/* let apps know we played with this device's defaults
	 */
	SendMessage((HWND)0xFFFF, WM_DEVMODECHANGE, 0,
		    (DWORD)(LPSTR)lpDevName);
      }

    if (wMode & DM_COPY && lpdmOut && wResult == IDOK)
      {
	Copy((LPSTR)lpdmOut, (LPSTR)lpdm, sizeof(DEVMODE)+DEVICEEXTRA);
      }

    if (hDM)
      {
	/* free up temporary buffer
	 */
	GlobalUnlock(hDM);
	GlobalFree(hDM);
      }

    /* hand back the result
     */
    return wResult;
}

/*******************************************************************
 *  DeviceMode() -
 *
 *  Simulates 2.X device mode by calling ExtDeviceMode().
 *
 */

WORD FAR PASCAL DeviceMode(
    HWND hWnd,
    HANDLE hLib,
    LPSTR lpDev,
    LPSTR lpPort)
{
    return (ExtDeviceMode(hWnd,hLib,NULL,lpDev,lpPort,NULL,NULL,
		DM_PROMPT|DM_UPDATE) == IDOK);
}

/********************************************************************
 *  DeviceCapabilities() -
 *
 *  Exported so apps can get info about our support of extended device mode
 */

DWORD DeviceCapabilities(
    LPSTR lpDevName,
    LPSTR lpPort,
    WORD wIndex,
    LPSTR lpOutput,
    LPDEVMODE lpdm)
{
    switch (wIndex)
      {
    case DC_FIELDS:
	return DM_ORIENTATION
		| DM_PAPERSIZE
#if DEVMODE_WIDEPAPER
		| DM_PAPERWIDTH
#endif
#if COLOR
		| DM_COLOR
#endif
#if DEVMODE_NO_PRINT_QUALITY
#else
		| DM_PRINTQUALITY
#endif
		;

    case DC_PAPERS:
	if (lpOutput)
	  {
	    ((WORD FAR *)lpOutput)[0] = DMPAPER_LETTER;
	    ((WORD FAR *)lpOutput)[1] = DMPAPER_A4;
	    ((WORD FAR *)lpOutput)[2] = DMPAPER_FANFOLD;
	  }
	return 3L;

    case DC_SIZE:
	return sizeof(DEVMODE);

    case DC_VERSION:
	return DM_SPECVERSION;

    case DC_DRIVER:
	return 0x300;

    case DC_EXTRA:
	return DEVICEEXTRA;

    default:
	return 0;
      }
}

/**************************************************************
 *  AboutDlgProc() -
 *
 *  callback function for about... dialog box
 */

BOOL FAR PASCAL AboutDlgProc(
    HWND hwnd,
    WORD wMsg,
    WORD wParam,
    DWORD lParam)
{
    switch (wMsg)
      {
    case WM_INITDIALOG:
	SetDlgItemText(hwnd,IDD_ABOUT,lpDevGlobal);
	break;

    case WM_COMMAND:
	if (wParam == IDOK || wParam == IDCANCEL)
	  {
	    EndDialog(hwnd,0);
	    break;
	  }

	/*** else FALL THRU ***/

    default:
	return FALSE;
      }
    return TRUE;
}

/************************************************************
 *  SetOrientation() -
 *
 *  Sets up the orientation icon
 */

void near pascal SetOrientation(
    HWND hwnd,
    WORD idIcon)
{
    HICON hIcon;

    CheckRadioButton(hwnd,IDD_PORTRAIT,IDD_LANDSCAPE,idIcon);

    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(idIcon));
    SetDlgItemText(hwnd,IDD_ORIENTICON,MAKEINTRESOURCE(hIcon));
}


/**************************************************************
 *  DevModeDlgProc() -
 *
 *  Dialog callback for devicemode dialog box
 */

BOOL FAR PASCAL DevModeDlgProc(
    register HWND hwnd,
    WORD wMsg,
    register WORD wParam,
    DWORD lParam)
{
    char sz[40];

    switch (wMsg)
      {
    case WM_INITDIALOG:
	/* select the appropriate orientation
	 */
	if (lpdmGlobal->dmOrientation == DMORIENT_PORTRAIT)
	  {
	    SetOrientation(hwnd,IDD_PORTRAIT);
	  }
	else
	  {
	    SetOrientation(hwnd,IDD_LANDSCAPE);
	  }

	/* select the appropriate paper
	 */
	wParam = GetDlgItem(hwnd,IDD_PAPER);
	LoadString(hInstance,PS_LETTER,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 0, (LONG)(LPSTR)sz);
	LoadString(hInstance,PS_A4,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 1, (LONG)(LPSTR)sz);
	LoadString(hInstance,PS_FANFOLD,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 2, (LONG)(LPSTR)sz);

	wParam = 0;
	switch (lpdmGlobal->dmPaperSize)
	  {
	case DMPAPER_FANFOLD:
	    wParam++;
	case DMPAPER_A4:
	    wParam++;
	  }
	SendDlgItemMessage(hwnd, IDD_PAPER, CB_SETCURSEL, wParam, 0L);

#if (defined(IBMCOLOR) && COLOR)
	/* do that ribbon setup
	 */
	wParam = GetDlgItem(hwnd,IDD_RIBBON);
	LoadString(hInstance,RS_BK,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 0, (LONG)(LPSTR)sz);
	LoadString(hInstance,RS_CMYK,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 1, (LONG)(LPSTR)sz);
	LoadString(hInstance,RS_RGBK,sz,40);
	SendMessage(wParam, CB_INSERTSTRING, 2, (LONG)(LPSTR)sz);

	if (lpdmGlobal->dmColor == DMCOLOR_MONOCHROME)
	    wParam = 0;
	else if (*(LPINT)(lpdmGlobal+1))
	    wParam = 2;
	else
	    wParam = 1;
	SendDlgItemMessage(hwnd,IDD_RIBBON,CB_SETCURSEL, wParam, 0L);
#endif

#if DEVMODE_WIDEPAPER
	if (!IsWideCarriagePrinter(lpDevGlobal))
	    EnableWindow(GetDlgItem(hwnd,IDD_WIDE),FALSE);
	else if (lpdmGlobal->dmPaperWidth > 3000)
	    CheckDlgButton(hwnd, IDD_WIDE, TRUE);
#endif

#if DEVMODE_NO_PRINT_QUALITY
#else
	/* initialize the quality radios
	 */
	CheckDlgButton(hwnd,
	    (lpdmGlobal->dmPrintQuality==DMRES_DRAFT) ? IDD_LOW : IDD_HIGH,
	    TRUE);
#endif

	/* lastly, set the title of the dialog box to the device name
	 */
	SetWindowText(hwnd,lpDevGlobal);
	break;

    case WM_COMMAND:
	switch (wParam)
	  {
	case IDD_ABOUT:
	    DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hwnd,AboutDlgProc);
	    break;

	case IDD_HELP:
	    break;

	case IDD_PORTRAIT:
	case IDD_LANDSCAPE:
	    SetOrientation(hwnd,wParam);
	    break;

#if DEVMODE_NO_PRINT_QUALITY
#else
	case IDD_LOW:
	case IDD_HIGH:
	    CheckRadioButton(hwnd,IDD_LOW,IDD_HIGH,wParam);
	    break;
#endif

	case IDOK:
	    /* fill in the DEVMODE fields
	     */
	    if (IsDlgButtonChecked(hwnd,IDD_PORTRAIT))
		lpdmGlobal->dmOrientation = DMORIENT_PORTRAIT;
	    else
		lpdmGlobal->dmOrientation = DMORIENT_LANDSCAPE;

	    switch ((WORD)SendDlgItemMessage(hwnd,IDD_PAPER,CB_GETCURSEL,0,0L))
	      {
	    case 1:
		lpdmGlobal->dmPaperSize = DMPAPER_A4;
		break;

	    case 2:
		lpdmGlobal->dmPaperSize = DMPAPER_FANFOLD;
		break;

	    default:
		lpdmGlobal->dmPaperSize = DMPAPER_LETTER;
	      }

#if DEVMODE_WIDEPAPER
	    SetPaperWidth(lpdmGlobal,IsDlgButtonChecked(hwnd,IDD_WIDE));
#endif

#if DEVMODE_NO_PRINT_QUALITY
#else
	    if (IsDlgButtonChecked(hwnd,IDD_LOW))
		lpdmGlobal->dmPrintQuality = DMRES_DRAFT;
	    else
		lpdmGlobal->dmPrintQuality = DMRES_HIGH;
#endif

#if (defined(IBMCOLOR) && COLOR)
	    switch ((WORD)SendDlgItemMessage(hwnd,IDD_RIBBON,CB_GETCURSEL,0,0L))
	      {
	    case 0:
		lpdmGlobal->dmColor = DMCOLOR_MONOCHROME;
		break;

	    case 2:
		lpdmGlobal->dmColor = DMCOLOR_COLOR;
		*(LPINT)(lpdmGlobal+1) = TRUE;
		break;

	    default:
		lpdmGlobal->dmColor = DMCOLOR_COLOR;
		*(LPINT)(lpdmGlobal+1) = FALSE;
	      }
#endif

	    /*** FALL THRU ***/

	case IDCANCEL:
	    EndDialog(hwnd,wParam);
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
