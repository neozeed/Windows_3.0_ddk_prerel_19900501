/**[f******************************************************************
 * dlgutils.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

// History
//	27 apr 89	peterbe		Tabs are 8 spaces
/***************************************************************************/
/******************************   dlgutils.c   *****************************/
/*
 *  DlgUtils:  Dialog utilities.
 */

#include "nocrap.h"
#undef NOPOINT
#undef NORECT
#undef NOSYSMETRICS
#undef NOWINMESSAGES
#undef NOCTLMGR
#include "windows.h"
#include "debug.h"
#include "dlgutils.h"

static HWND GetBiggestParent(HWND);


/*************************************************************************/
/****************************   Global procs   ***************************/


/*  CenterDlg
 *
 *  Center dialog over parent window.
 */
void FAR PASCAL CenterDlg (hDlg)
    HWND hDlg;
    {
    HWND hParentWnd;
    RECT DlgWndRect;
    RECT ParentWndRect;
    POINT DlgLoc;
    short DlgHeight;
    short DlgWidth;
    short ScrTop;
    short ScrLeft;
    short ScrBot;
    short ScrRight;

    /*  Locate the biggest parent window we can find.
     */
    hParentWnd = GetBiggestParent(hDlg);

    /*  Get bounding rectangles of dialog and parent window.
     */
    GetWindowRect(hDlg, (LPRECT) &DlgWndRect);
    GetClientRect(hParentWnd, (LPRECT) &ParentWndRect);

    /*  Calculate upper left corner of dialog so it will
     *  be centered over the parent window.
     */
    DlgWidth = DlgWndRect.right - DlgWndRect.left;
    DlgHeight = DlgWndRect.bottom - DlgWndRect.top;

    DlgLoc.x = ParentWndRect.left;
    DlgLoc.y = ParentWndRect.top;
    DlgLoc.x +=
	(ParentWndRect.right - ParentWndRect.left - DlgWidth) / 2;
    DlgLoc.y +=
	((ParentWndRect.bottom - ParentWndRect.top - DlgHeight) * 2) / 5;

    /*  Convert to equivalent screen coordinates.
     */
    ClientToScreen(hParentWnd, (LPPOINT) &DlgLoc);

    /*  Limit the dialog from going off screen.
     */
    ScrLeft = 0;
    ScrTop = 0;
    ScrRight = (short)GetSystemMetrics(SM_CXSCREEN);
    ScrBot = (short)GetSystemMetrics(SM_CYSCREEN);

    if (DlgLoc.x < ScrLeft)
	{
	DlgLoc.x = ScrLeft;
	}
    else if (DlgLoc.x > (ScrRight - DlgWidth))
	{
	DlgLoc.x = ScrRight - DlgWidth;
	}

    if (DlgLoc.y < ScrTop)
	{
	DlgLoc.y = ScrTop;
	}
    else if (DlgLoc.y > (ScrBot - DlgHeight))
	{
	DlgLoc.y = ScrBot - DlgHeight;
	}

    /*  Center dialog.
     */
    MoveWindow(hDlg, DlgLoc.x, DlgLoc.y, DlgWidth, DlgHeight,
	IsWindowVisible(hDlg));
    }

/*  GenericWndProc
 *
 *  Generic window proc for handling informative non-modal dialogs.
 */
int FAR PASCAL GenericWndProc(hWnd, wMsg, wParam, lParam)
    HWND hWnd;
    unsigned wMsg;
    WORD wParam;
    long lParam;
    {
    DBMSG(("GenericWndProc(%d,%d,%d,%ld)\n",
	(HWND)hWnd, (unsigned)wMsg, (WORD)wParam, lParam));

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    CenterDlg(hWnd);
	    break;

	case WM_COMMAND:
	    EndDialog(hWnd, wParam);
	    break;

	default:
	    return FALSE;
	}

    return TRUE;
    }


/*************************************************************************/
/****************************   Local procs   ****************************/


/*  GetBiggestParent
 *
 *  Keep calling GetParent() until we've gotten the biggest window
 *  we can find.
 */
static HWND GetBiggestParent(hCurrentWnd)
    HWND hCurrentWnd;
    {
    HWND hSaveWnd = hCurrentWnd;
    HWND hBiggestWnd = hCurrentWnd;
    RECT currentRect;
    RECT prevRect;
    short currentWidth;
    short biggestWidth = 0;
    WORD redundantLoop = 0;

    while ((++redundantLoop < 32) &&
	(hCurrentWnd = GetParent(hCurrentWnd)))
	{
	GetWindowRect(hCurrentWnd, (LPRECT) &currentRect);

	currentWidth = currentRect.right - currentRect.left;

	if (currentWidth > biggestWidth)
	    {
	    hBiggestWnd = hCurrentWnd;
	    biggestWidth = currentWidth;
	    }
DBMSG(("...inside GetBiggestParent(%d): current width=%d, biggest=%d\n",
    redundantLoop, currentWidth, biggestWidth));
	}

    if (redundantLoop > 31)
	{
	hBiggestWnd = GetParent(hSaveWnd);
	}

    return (hBiggestWnd);
    }

