/**[f******************************************************************
 * options.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.  
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   options.c   ******************************/
//
//  Procs for handling options dialog.
//
//  01 dec 89	peterbe	Declarations in ifdef also.
//
//  30 nov 89	peterbe	Visual edge calls in ifdef now.
//
//  07 jun 89	peterbe	Changed duplex groupbox to have only one
//			icon showing at a time.
//
//  31 mar 89	peterbe	Added code for determining whether landscape port.
//			icons are to appear.  Implemented icon
//			enable/disable code -- works now.
//
//  30 mar 89	peterbe	Disable laser port control if there's no hardware..
//
//   2-22-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
//

#include "nocrap.h"
#undef NOCTLMGR
#undef NOWINMESSAGES
#undef NOSHOWWINDOW
#include "windows.h"
#include "hppcl.h"
#include "resource.h"
#include "strings.h"
#include "dlgutils.h"
#include "debug.h"

// laser port enable function.  Determine if it exists.

#ifdef VISUALEDGE
int FAR PASCAL lp_enbl(void);
#endif


/*  DEBUG switches
 */
#define DBGdlgfn(msg)       DBMSG(msg)

#define LOCAL static

LOCAL LPPCLDEVMODE glpDevmode;	    /* we are not reentrant, so this is okay */


/*  Forward references
 */
BOOL  FAR PASCAL OPdlgFn(HWND, unsigned, WORD, LONG);
short FAR PASCAL OptionsDlg(HANDLE, HWND, LPPCLDEVMODE);
LOCAL void	 UpdateDuplex(HWND, LPPCLDEVMODE);
void NEAR PASCAL SetDuplexIcon(HWND, LPPCLDEVMODE);

/*  OptionsDlg
 */
short FAR PASCAL
OptionsDlg(HANDLE hMd, HWND hWndParent, LPPCLDEVMODE lpDevmode) {

    FARPROC lpDlgFunc;
    short response, OldOptions, OldDuplex;

    DBGdlgfn(("OptionsDlg(%d,%d,%lp)\n",(WORD)hMd,(WORD)hWndParent,lpDevmode));

    glpDevmode = lpDevmode;
    OldOptions = lpDevmode->options;
    OldDuplex  = lpDevmode->dm.dmDuplex;

    lpDlgFunc = MakeProcInstance(OPdlgFn, hMd);
    response = DialogBox(hMd, MAKEINTRESOURCE(OPTIONS), hWndParent, lpDlgFunc);
    FreeProcInstance(lpDlgFunc);

    /* restore old options if user canceled out */

    if (response == IDCANCEL) {
	lpDevmode->options = OldOptions;
	lpDevmode->dm.dmDuplex = OldDuplex;
    }

    DBMSG(("...end, response=%d %ls\n", response, (response == IDOK) ?
	 (LPSTR)"IDOK" : (LPSTR)"IDCANCEL"));

    return(response);
}

/*  OPdlgFn
 */
BOOL FAR PASCAL
OPdlgFn(HWND hDB, unsigned wMsg, WORD wParam, LONG lParam) {

    switch (wMsg) {

        case WM_INITDIALOG:

	    DBGdlgfn(("OPdlgFn(%d,%d,%d,%ld): WM_INITDIALOG\n",
		      hDB, wMsg, wParam, lParam));

#ifdef VISUALEDGE
	    // if there's a LaserPort installed (or Intel Visual Edge),
	    if (lp_enbl())
		// display LaserPort button checked,
		CheckDlgButton(hDB, OPTN_IDDPTEK,
			   (glpDevmode->options) & OPTIONS_DPTEKCARD);
	    else
		// otherwise disable this control:
		EnableWindow(GetDlgItem(hDB, OPTN_IDDPTEK), FALSE);
#endif

	    UpdateDuplex(hDB,glpDevmode);

            CenterDlg(hDB);
            break;


        case WM_COMMAND:

	    switch (wParam) {

                case NODUPLEX:
                case VDUPLEX:
		case HDUPLEX:

		    CheckRadioButton(hDB, NODUPLEX, HDUPLEX, wParam);
		    glpDevmode->dm.dmDuplex = (wParam == NODUPLEX) ?
					 DMDUP_SIMPLEX : ((wParam == VDUPLEX) ?
					 DMDUP_VERTICAL : DMDUP_HORIZONTAL);
		    SetDuplexIcon(hDB, glpDevmode);
		    break;

#ifdef VISUALEDGE
                case OPTN_IDDPTEK:
                    {
                    short flag = wParam - OPTN_DLG_BASE;

		    DBGdlgfn(("OPdlgFn(%d,%d,%d,%ld): OPTION %d\n",
			      hDB, wMsg, wParam, lParam, flag));

		    glpDevmode->options ^= flag;

		    CheckDlgButton(hDB, wParam, (glpDevmode->options & flag));
                    }
                    break;
#endif

                case IDOK:
                case IDCANCEL:

                    EndDialog(hDB, wParam);
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

/*  UpdateDuplex
 */
LOCAL void
UpdateDuplex(HWND hDB, LPPCLDEVMODE lpDevmode) {

    char buf[32];
    extern HANDLE hLibInst;
    HDC hDC;
    WORD wShow;

    if (!(lpDevmode->prtCaps & ANYDUPLEX)) {

	// This case will occur if there's a laserport installed
	// the printer doesn't handle duplex.

        CheckRadioButton(hDB,NODUPLEX,HDUPLEX,0);
        EnableWindow(GetDlgItem(hDB,NODUPLEX),FALSE);
        EnableWindow(GetDlgItem(hDB,VDUPLEX),FALSE);
	EnableWindow(GetDlgItem(hDB,HDUPLEX),FALSE);

	lpDevmode->dm.dmDuplex = 0;

    } else {

	// We enable the radio buttons and icons for selection of
	// duplex printing.
	// The terminology shown depends on the model of printer.
	//
	if (lpDevmode->prtCaps & HPIIDDUPLEX) {

	    // For this printer, the terms are 'short' and 'long' bind.
	    LoadString(hLibInst,IDS_DUPLBIND,buf,sizeof(buf));
            SetDlgItemText(hDB,VDUPLEX,buf);
	    LoadString(hLibInst,IDS_DUPSBIND,buf,sizeof(buf));
            SetDlgItemText(hDB,HDUPLEX,buf);

	} else {

	    // For this printer, the terms are 'Vertical' and 'Horizontal' bind.
	    LoadString(hLibInst,IDS_DUPVBIND,buf,sizeof(buf));
            SetDlgItemText(hDB,VDUPLEX,buf);
	    LoadString(hLibInst,IDS_DUPHBIND,buf,sizeof(buf));
            SetDlgItemText(hDB,HDUPLEX,buf);
	}

	// Now show the icons.. which depend on whether we're in
	// landscape or portrait mode, and what the duplex mode is.

	SetDuplexIcon(hDB, lpDevmode);

	// check the proper radio button.

	CheckRadioButton(hDB,NODUPLEX,HDUPLEX,
		    (lpDevmode->dm.dmDuplex == DMDUP_SIMPLEX) ?
		    NODUPLEX : ((lpDevmode->dm.dmDuplex == DMDUP_VERTICAL) ?
		    VDUPLEX : HDUPLEX));
    }
}	// UpdateDuplex()

void NEAR PASCAL SetDuplexIcon(HWND hDB, LPPCLDEVMODE lpDevmode)
{
    LPSTR lpIconName;
    extern HANDLE hLibInst;

    // first, get the name of the icon
    switch(lpDevmode->dm.dmDuplex)
	{
	case DMDUP_SIMPLEX:

		lpIconName =
		    (lpDevmode->dm.dmOrientation == DMORIENT_PORTRAIT) ?
			(LPSTR) "ICO_NONEPORT" : (LPSTR) "ICO_NONELAND";
		break;

	case DMDUP_HORIZONTAL:

		lpIconName =
		    (lpDevmode->dm.dmOrientation == DMORIENT_PORTRAIT) ?
			(LPSTR) "ICO_HORPORT" : (LPSTR) "ICO_HORLAND";
		break;

	case DMDUP_VERTICAL:
	default:

		lpIconName =
		    (lpDevmode->dm.dmOrientation == DMORIENT_PORTRAIT) ?
			(LPSTR) "ICO_VERTPORT" : (LPSTR) "ICO_VERTLAND";
		break;
	}

    // Now, load the icon and display it

    SetDlgItemText(hDB, OPT_ICON,
	MAKEINTRESOURCE(LoadIcon(hLibInst, lpIconName)));

}	// SetDuplexIcon()

