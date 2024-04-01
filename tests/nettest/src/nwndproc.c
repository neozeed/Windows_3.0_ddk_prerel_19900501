/*
 * Nwndproc.c
 */

#include "windows.h"
#include "winnet.h"
#include "nettest.h"


/****************************************************************************

    FUNCTION: NetTestWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

	WM_CREATE     - create window
	WM_DESTROY    - destroy window

    COMMENTS:


****************************************************************************/

long FAR PASCAL NetTestWndProc(HWND hWnd, unsigned message, WORD wParam,
							    LONG lParam)
{

    switch (message) {

	case WM_COMMAND:
	    ProcessWMCommand(hWnd, message, wParam, lParam);
	    break;


	case WM_DESTROY:		  /* message: window being destroyed */
	    PostQuitMessage(0);
	    break;

	default:			  /* Passes it on if unproccessed    */
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
}

/***************************************************************************


ProcessWMCommand(HWND, unsigned, WORD, LONG)

Function to process the WM_COMMAND in the NetTestWndProc function.


*****************************************************************************/

BOOL FAR PASCAL ProcessWMCommand(HWND hWnd, unsigned message, WORD wParam, LONG lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;

   /* define dialog proc pointers */
    FARPROC lpfnAbout;		/* Pointer to the About function */
    FARPROC lpfnHelp;		/* Pointer to the Help function */
    FARPROC lpfnBatchFile;	/* Pointer to the Batch function */
    FARPROC lpfnListConns;	/* Pointer to the List Net Connections function */
    FARPROC lpfnNetCaps;	/* Pointer to the NetCaps function */
    FARPROC lpfnAddConn;	/* Pointer to the AddConnection function */
    FARPROC lpfnRemoveConn;	/* Pointer to the RemoveConnection function */
    FARPROC lpfnErrorText;	/* Pointer to the ErrorText function */

	    switch (wParam) {

		case WM_CREATE:
		    break;		   /* message: window being created */

		case WM_PAINT:
		    hDC = BeginPaint(hParWnd, &ps);
		    EndPaint(hParWnd, &ps);
		    break;

		case IDM_HELP:
		    if (lpfnHelp = MakeProcInstance(HelpDlgProc, hInst)) {
			DialogBox(hInst, "HelpBox", hParWnd, lpfnHelp);
			FreeProcInstance(lpfnHelp);
		    }
		    break;

		case IDM_EXIT:
		    DestroyWindow(hParWnd);
		    break;
    
		case IDM_BATCH:
		    if (lpfnBatchFile = MakeProcInstance(BatchFileDlgProc, hInst)) {
			DialogBox(hInst, "BatchFileBox", hParWnd, lpfnBatchFile);
			FreeProcInstance(lpfnBatchFile);
		    }
		    break;

		case IDM_ABOUT:
		    if (lpfnAbout = MakeProcInstance(AboutDlgProc, hInst)) {
			DialogBox(hInst, "AboutBox", hParWnd, lpfnAbout);
			FreeProcInstance(lpfnAbout);
		    }
		    break;

		case IDM_LISTNET: /* list all network connections */
		    if (lpfnListConns = MakeProcInstance(ListConnsDlgProc, hInst)) {
			DialogBox(hInst, "ListConnsBox", hParWnd, lpfnListConns);
			FreeProcInstance(lpfnListConns);
		    }
		    break;

		case IDM_ADDNET:
		    if (lpfnAddConn = MakeProcInstance(AddConnectionDlgProc, hInst)) {
			DialogBox(hInst, "AddConnectionBox", hParWnd, lpfnAddConn);
			FreeProcInstance(lpfnAddConn);
		    }
		    break;

		case IDM_REMOVENET:
		    if (lpfnRemoveConn = MakeProcInstance(RemoveConnectionDlgProc, hInst)) {
			DialogBox(hInst, "RemoveConBox", hParWnd, lpfnRemoveConn);
			FreeProcInstance(lpfnRemoveConn);
		    }
		    break;

		case IDM_OPENJOB:
		    break;

		case IDM_CLOSEJOB:
		    break;

		case IDM_CANCELJOB:
		    break;

		case IDM_RELEASEJOB:
		    break;

		case IDM_ABORTJOB:
		    break;

		case IDM_HOLDJOB:
		    break;

		case IDM_LOCKQ:
		    break;

		case IDM_SETCOPIES:
		    break;

		case IDM_STOPWATCHQ:
		    break;

		case IDM_UNLOCKQ:
		    break;

		case IDM_WATCHQ:
		    break;

		case IDM_BROWSE: /* Display the Browse dialog box */
		    BrowseDialog(hParWnd);
		    break;

		case IDM_DRIVEDLG: /* Display the Driver-Specific dialog box */
		    WNetDeviceMode(hParWnd); /* Currently no return value */
		    break;

		case IDM_USERNAME: /* Get current user */
		    GetUserName(hParWnd);
		    break;

		case IDM_NETCAPS: /* Get Network Capabilities */
		    if (lpfnNetCaps = MakeProcInstance(NetCapsDlgProc, hInst)) {
			DialogBox(hInst, "NetCapsDlg", hParWnd, lpfnNetCaps);
			FreeProcInstance(lpfnNetCaps);
		    }
		    break;

		case IDM_GETERRCODE:
		    GetErrorCode();
		    break;

		case IDM_GETERRTEXT:
		    if (lpfnErrorText = MakeProcInstance(GetErrorTextDlgProc, hInst)) {
			DialogBox(hInst, "ErrorTextBox", hParWnd, lpfnErrorText);
			FreeProcInstance(lpfnErrorText);
		    }
		    break;

		case IDM_NETBIOS:
		    NetBiosTest(hParWnd);
		    break;

		case IDM_FILECOUNT:
		    FileCount();
		    break;


	    }

	return(TRUE);

}
