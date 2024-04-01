/* Misc.c
 *
 */

#include "windows.h"
#include "winnet.h"
#include "nettest.h"
#include "string.h"
#include "stdlib.h"
#include "netdlg.h"

/* BrowseDialog vars */
char szPath[];	  /* The path passed back from WNetBrowseDialog. It is entered by the user and verified */

/* GetUserName vars */
char szUser[79];    /* user name string */
WORD nBuffSize;   /* size of user name buffer */
LPSTR lpBiggerUser; /* buffer to be used in case of WN_MORE_DATA */

/* NetCapsDlgProc vars */
char szBuffer[128];
char szBuffer2[79];

/****************************************************************************

    FUNCTION: BrowseDialog()

    PURPOSE:  Calls WNetBrowseDialog function to display the dialog box.
	      It then gets input from the user or analyzes any errors
	      encountered in the call.

    MESSAGES:


    COMMENTS:


****************************************************************************/

BOOL FAR PASCAL BrowseDialog(HWND hWnd)
{
    int i;
    BOOL bSuc;

    bSuc = FALSE;
    /* szPath = (char *) malloc(128); */

    /* Write log file title */
    lstrcpy(szBuffer2, "\n\nB R O W S E   D I A L O G");
    WriteLog(szBuffer2,"\n");

   /* Test each WNGC_Constant value relavent to resources:
    *	  WNGC_UNKNOWN	- unknown or other
    *	  WNGC_DISKTREE - disk tree
    *	  WNGC_PRINTQ	- print queue
    *	  WNGC_DEVICE	- serial device
    *	  WNGC_IPC	- interprocess communication */

   /*	  WNGC_UNKNOWN	- unknown or other */

    switch (WNetBrowseDialog(hWnd, WNBD_CONN_UNKNOWN, (LPSTR) szPath)) {
	case WN_SUCCESS:
	    bSuc = TRUE;
	    break;
	case WN_NOT_SUPPORTED:
	    lstrcpy(szPath, "Not supported");
	    break;
	case WN_NET_ERROR:
	    lstrcpy(szPath, "Net Error");
	    break;
	case WN_BAD_VALUE:
	    lstrcpy(szPath, "Bad Value");
	    break;
	case WN_CANCEL:
	    lstrcpy(szPath, "Cancelled");
	    break;
    }
    if (bSuc) {
	MessageBox(NULL, szPath, (LPSTR) "Unknown Dialog", MB_YESNO);
	bSuc = FALSE;
    }
    else
	MessageBox(NULL, szPath, (LPSTR) "Unknown Dialog Error", MB_OK | MB_ICONEXCLAMATION);

    /* write log file */
    lstrcpy(szBuffer2, " Unknown Dialog: ");
    WriteLog(szBuffer2, szPath);

   /*	  WNGC_DISKTREE - disk tree */

    switch (WNetBrowseDialog(hWnd, WNBD_CONN_DISKTREE, (LPSTR) szPath, (LPWORD) lstrlen(szPath))) {
	case WN_SUCCESS:
	    bSuc = TRUE;
	    break;
	case WN_NOT_SUPPORTED:
	    lstrcpy(szPath, "Not supported");
	    break;
	case WN_NET_ERROR:
	    lstrcpy(szPath, "Net Error");
	    break;
	case WN_BAD_VALUE:
	    lstrcpy(szPath, "Bad Value");
	    break;
	case WN_CANCEL:
	    lstrcpy(szPath, "Cancelled");
	    break;
    }
    if (bSuc) {
	MessageBox(NULL, szPath, (LPSTR) "Disk Tree Dialog", MB_YESNO);
	bSuc = FALSE;
    }
    else
	MessageBox(NULL, szPath, (LPSTR) "Disk Tree Dialog Error", MB_OK | MB_ICONEXCLAMATION);


    /* write log file */
    lstrcpy(szBuffer2, " Disk Tree Dialog: ");
    WriteLog(szBuffer2, szPath);

   /*	  WNGC_PRINTQ	- print queue */

    switch (WNetBrowseDialog(hWnd, WNBD_CONN_PRINTQ, (LPSTR) szPath, (LPWORD) lstrlen(szPath))) {
	case WN_SUCCESS:
	    bSuc = TRUE;
	    break;
	case WN_NOT_SUPPORTED:
	    lstrcpy(szPath, "Not supported");
	    break;
	case WN_NET_ERROR:
	    lstrcpy(szPath, "Net Error");
	    break;
	case WN_BAD_VALUE:
	    lstrcpy(szPath, "Bad Value");
	    break;
	case WN_CANCEL:
	    lstrcpy(szPath, "Cancelled");
	    break;
    }
    if (bSuc) {
	MessageBox(NULL, szPath, (LPSTR) "Print Queue Dialog", MB_YESNO);
	bSuc = FALSE;
    }
    else
	MessageBox(NULL, szPath, (LPSTR) "Print Queue Dialog Error", MB_OK | MB_ICONEXCLAMATION);

    /* write log file */
    lstrcpy(szBuffer2, " Print Queue Dialog: ");
    WriteLog(szBuffer2, szPath);

    /*	 WNGC_DEVICE   - serial device [removed] */

    /*	   WNGC_IPC	 - interprocess communication [removed] */


    return(TRUE);
}

/****************************************************************************

    FUNCTION: GetUserName(HWND)

    PURPOSE:  Calls WNetGetUser function to get the user name.
	      Then it will display the user name or any errors
	      encountered in the call.

    MESSAGES:


    COMMENTS:


****************************************************************************/

BOOL FAR PASCAL GetUserName(HWND hWnd)
{

    szUser[0] = 0;
    nBuffSize = sizeof(szUser);

    /* Write section title to log file */
    lstrcpy(szBuffer2, "\n\nU S E R   N A M E");
    WriteLog(szBuffer2,"\n");

    switch (WNetGetUser(szUser, (LPWORD) &nBuffSize)) {
	case WN_SUCCESS:
	    break;
	case WN_NOT_SUPPORTED:
	    lstrcpy(szUser, "Not Supported");
	    break;
	case WN_NET_ERROR:
	    lstrcpy(szUser, "Network Error");
	    break;
	case WN_BAD_POINTER:
	    lstrcpy(szUser, "Bad Pointer");
	    break;
	case WN_BAD_USER:
	    lstrcpy(szUser, "Bad User Name");
	    break;
	case WN_MORE_DATA:
	   /* try to allocate a larger buffer */

	    lpBiggerUser = (LPSTR)(PSTR) LocalAlloc(LPTR, 128 * sizeof(char)); /* Alloc memory for string */

	    /* see if alloc and re-call is successful */
	    switch (WNetGetUser(lpBiggerUser, (LPWORD) &nBuffSize)) {
		case WN_SUCCESS:
		    break;
		case WN_NOT_SUPPORTED:
		    lstrcpy(lpBiggerUser, "Not Supported");
		    break;
		case WN_NET_ERROR:
		    lstrcpy(lpBiggerUser, "Network Error");
		    break;
		case WN_BAD_POINTER:
		    lstrcpy(lpBiggerUser, "Bad Pointer");
		    break;
		case WN_BAD_USER:
		    lstrcpy(lpBiggerUser, "Bad User Name");
		    break;
		case WN_MORE_DATA:
		    lstrcpy(lpBiggerUser, "More Data - Realloc Unsuccessful");
		    break;
	    }
	    MessageBox(NULL, lpBiggerUser, (LPSTR) "User Name", MB_OK);
	    lstrcpy(lpBiggerUser, " User Name: ");
	    WriteLog(lpBiggerUser, szUser);
	    LocalFree((LOCALHANDLE) lpBiggerUser); /* free previous handle before another alloc */
	    return(TRUE);
	    break;
    }
    MessageBox(NULL, (LPSTR) szUser, (LPSTR) "User Name", MB_OK);
    lstrcpy(szBuffer2, " User Name: ");
    WriteLog(szBuffer2, szUser);

    return(TRUE);
}

/****************************************************************************

    FUNCTION: NetCapsDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "NetCaps" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL NetCapsDlgProc(HWND hDlg, unsigned message, WORD wParam,
							    LONG lParam)
{
    WORD nIndex;

    switch (message) {

	case WM_INITDIALOG:		   /* message: initialize dialog box */

	    lstrcpy(szBuffer2, "\n\nN E T W O R K   C A P A B I L I T I E S");
	    WriteLog(szBuffer2, "\n");

	   /* query net version number */
	    lstrcpy(szBuffer2, "*DRIVER INFORMATION*");
	    WriteLog(szBuffer2,"");
	    nIndex = WNetGetCaps(WNNC_SPEC_VERSION);
	    wsprintf((LPSTR) szBuffer, "%u.%u",  HIBYTE(nIndex), LOBYTE(nIndex));
	    SendDlgItemMessage(hDlg, IDD_SPECVER, WM_SETTEXT, NULL, (LONG)(LPSTR) szBuffer);
	    lstrcpy(szBuffer2, " Spec Version: ");
	    WriteLog(szBuffer2, szBuffer);

	   /* query driver version number */
	    nIndex =  WNetGetCaps(WNNC_DRIVER_VERSION);
	    wsprintf((LPSTR) szBuffer, "%u.%u",  HIBYTE(nIndex), LOBYTE(nIndex));
	    SendDlgItemMessage(hDlg, IDD_DRVVER, WM_SETTEXT, NULL, (LONG)(LPSTR) szBuffer);
	    lstrcpy(szBuffer2, " Driver Version: ");
	    WriteLog(szBuffer2, szBuffer);

	   /* query net version type */
	    switch ((WNetGetCaps(WNNC_NET_TYPE)) & 0x0f00) {
		case WNNC_NET_NONE:
		    lstrcpy(szBuffer, "None");
		    break;
		case WNNC_NET_MSNet:
		    lstrcpy(szBuffer, "MSNet");
		    break;
		case WNNC_NET_LanMan:
		    lstrcpy(szBuffer, "LanMan");
		    break;
		case WNNC_NET_NetWare:
		    lstrcpy(szBuffer, "NetWare");
		    break;
		case WNNC_NET_Vines:
		    lstrcpy(szBuffer, "Vines");
		    break;
		default:
		    lstrcpy(szBuffer, "Unsupt'd");
		    MessageBox(hDlg, (LPSTR) "This Net Type Not Supported", (LPSTR) "Net Type", IDOK | MB_ICONEXCLAMATION);
		    break;

	    }
	    SendDlgItemMessage(hDlg, IDD_NETTYPE, WM_SETTEXT, NULL, (LONG)(LPSTR) szBuffer);
	    lstrcpy(szBuffer2," Network Type: " );
	    WriteLog(szBuffer2, szBuffer);

	   /* query for User support */
	    if ((WNetGetCaps(WNNC_USER)) & WNNC_USR_GetUser)
		lstrcpy(szBuffer, "Supported");
	    else {
		lstrcpy(szBuffer, "Unsupported");
		MessageBox(hDlg, (LPSTR) "User Query Not Supported", (LPSTR) "User Query", MB_OK | MB_ICONEXCLAMATION);
	    }
	    SendDlgItemMessage(hDlg, IDD_USER, WM_SETTEXT, NULL, (LONG)(LPSTR) szBuffer);
	    lstrcpy(szBuffer2, " Get User Name Functionality: ");
	    WriteLog(szBuffer2, szBuffer);

	   /* query connection support */
	    lstrcpy(szBuffer2, "\n*DRIVER'S NETWORK CONNECTION SUPPORT*");
	    WriteLog(szBuffer2,"");

	    if (WNetGetCaps(WNNC_CONNECTION) & WNNC_CON_AddConnection) {
		lstrcpy(szBuffer, "Add");
		SendDlgItemMessage(hDlg, IDD_CONNECTIONS, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_CONNECTION) &	WNNC_CON_CancelConnection) {
		lstrcpy(szBuffer, "Cancel");
		SendDlgItemMessage(hDlg, IDD_CONNECTIONS, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_CONNECTION) &	WNNC_CON_GetConnections) {
		lstrcpy(szBuffer, "Get");
		SendDlgItemMessage(hDlg, IDD_CONNECTIONS, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }

	   /* query printing support */
	    lstrcpy(szBuffer2, "\n*DRIVER'S NETWORK PRINTING SUPPORT*");
	    WriteLog(szBuffer2,"");

	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_OpenJob) {
		lstrcpy(szBuffer, "Open Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_CloseJob) {
		lstrcpy(szBuffer, "Close Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_HoldJob) {
		lstrcpy(szBuffer, "Hold Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_ReleaseJob) {
		lstrcpy(szBuffer, "Release Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_CancelJob) {
		lstrcpy(szBuffer, "Cancel Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_SetJobCopies) {
		lstrcpy(szBuffer, "Set Job Copies");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_WatchQueue) {
		lstrcpy(szBuffer, "Watch Queue");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_UnwatchQueue) {
		lstrcpy(szBuffer, "Unwatch Queue");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_LockQueueData) {
		lstrcpy(szBuffer, "Lock Queue Data");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_UnlockQueueData) {
		lstrcpy(szBuffer, "Unlock Queue Data");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_ChangeMsg) {
		lstrcpy(szBuffer, "Change Message");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_PRINTING) & WNNC_PRT_AbortJob) {
		lstrcpy(szBuffer, "Abort Job");
		SendDlgItemMessage(hDlg, IDD_PRINTING, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }


	   /* query error support */
	    lstrcpy(szBuffer2, "\n*DRIVER'S NETWORK ERROR SUPPORT*");
	    WriteLog(szBuffer2,"");

	    if (WNetGetCaps(WNNC_ERROR) & WNNC_ERR_GetError) {
		lstrcpy(szBuffer, "Get Error");
		SendDlgItemMessage(hDlg, IDD_ERROR, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    if (WNetGetCaps(WNNC_ERROR) & WNNC_ERR_GetErrorText) {
		lstrcpy(szBuffer, "Get Error Info");
		SendDlgItemMessage(hDlg, IDD_ERROR, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer);
		lstrcpy(szBuffer2, " ");
		WriteLog(szBuffer2, szBuffer);
	    }
	    return (TRUE);
	    break;

	case WM_COMMAND:		      /* message: received a command */
	    if (wParam == IDOK) {	      /* "OK" box selected?	     */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
		return (TRUE);
	    }
	    break;


    }
    return (FALSE);			      /* Didn't process a message    */
}
