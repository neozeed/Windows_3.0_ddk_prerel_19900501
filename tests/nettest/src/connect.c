/* Connect.c
 *
 */
#include "windows.h"
#include "winnet.h"
#include "nettest.h"
#include "Spool.h"		/* Spooler core include file */
#include "netdlg.h"

char szBuffer[79];
char szBuffer2[128];
char szPassword[10];
char szLocal[10];
char szPath[79];
char szRemoteName[79];
LPSTR szBigRemoteName;
BOOL bForce, bSuccess, bConnected;
LPWORD nBufferSize;
int i, j;

/****************************************************************************

    FUNCTION: AddConnectionDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "AddConnectionBox" dialog box
	      This procedure calls the WNetAddConnection() function and
	      lists all the current connections and their status.
    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL AddConnectionDlgProc(HWND hDlg, unsigned message, WORD wParam,
							      LONG lParam)
{

    switch (message) {
	case WM_INITDIALOG:
	    bSuccess = FALSE;

	    /* write section title to log file */
	    lstrcpy(szBuffer2, "\n\nA D D   C O N N E C T I O N");
	    WriteLog(szBuffer2, "\n");

	    return (TRUE);

	case WM_COMMAND:

	    switch (wParam) {
		case IDD_ADD: /* Add a net connection */

		    GetDlgItemText(hDlg, IDD_LOCALNAME, (LPSTR) szLocal, 10);
		    GetDlgItemText(hDlg, IDD_NETPATHNAME, (LPSTR) szPath, 79);
		    GetDlgItemText(hDlg, IDD_PASSWORD, (LPSTR) szPassword, 10);

		    switch (WNetAddConnection((LPSTR) szPath, (LPSTR) szPassword, (LPSTR) szLocal)) {
			case WN_SUCCESS:
			    bSuccess = TRUE;
			    wsprintf(szBuffer2," Add Connection Success: Device=%s, Path=%s, \n\tPassword=",
						(LPSTR) szLocal, (LPSTR) szPath);
			    WriteLog(szBuffer2, szPassword);
			    break;
			case WN_NOT_SUPPORTED:
			    lstrcpy(szBuffer, (LPSTR) "Not Supported");
			    break;
			case WN_NET_ERROR:
			    lstrcpy(szBuffer, (LPSTR) "Net Error");
			    break;
			case WN_BAD_POINTER:
			    lstrcpy(szBuffer, (LPSTR) "Bad Pointer");
			    break;
			case WN_BAD_NETNAME:
			    lstrcpy(szBuffer, (LPSTR) "Bad Net Name");
			    break;
			case WN_BAD_LOCALNAME:
			    lstrcpy(szBuffer, (LPSTR) "Bad Local Name");
			    break;
			case WN_BAD_PASSWORD:
			    lstrcpy(szBuffer, (LPSTR) "Bad Password");
			    break;
			case WN_ACCESS_DENIED:
			    lstrcpy(szBuffer, (LPSTR) "Access Denied");
			    break;
			case WN_FUNCTION_BUSY:
			    lstrcpy(szBuffer, (LPSTR) "Already Connected");
			    break;
			default:
			    lstrcpy(szBuffer, (LPSTR) "Add Failed");
			    break;

		    }
		    if (!bSuccess) {
			MessageBox(NULL, szBuffer, (LPSTR) "Add Net Connection", MB_OK | MB_ICONEXCLAMATION);
			wsprintf(szBuffer2, " Add Connection Fail: Device=%s, Path=%s, \n\tPassword=%s, Error= ",
				 (LPSTR) szLocal, (LPSTR) szPath, (LPSTR) szPassword);
			WriteLog(szBuffer2, szBuffer);
		    }

		    EndDialog(hDlg, TRUE);
		    break;

		case IDCANCEL:
		    EndDialog(hDlg, TRUE);
		    break;

		default:
		    return(FALSE);
		    break;

	    }
	    break;

	default:
	    return(FALSE);
	    break;
    }
    return (TRUE);

}

/****************************************************************************

    FUNCTION: RemoveConnectionDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "RemoveConBox" dialog box
	      This procedure calls the WNetCancelConnection() function and
	      lists all the current connections and their status.
    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL RemoveConnectionDlgProc(HWND hDlg, unsigned message, WORD wParam,
							      LONG lParam)
{
    BOOL bResult = FALSE;

    switch (message) {

	case WM_INITDIALOG:
	    bSuccess = FALSE;
	    bForce = FALSE;

	    /* write section title to log file */
	    lstrcpy(szBuffer2, "\n\nC A N C E L   C O N N E C T I O N");
	    WriteLog(szBuffer2, "\n");

	    return (TRUE);

	case WM_COMMAND:

	    switch (wParam) {

		case IDCANCEL:
		    EndDialog(hDlg, TRUE);
		    break;

		case IDD_FORCE:
		    bForce = TRUE; /* NOTE: fall through to Remove, no break */

		case IDD_REMOVE:   /* remove a net connection */

		    GetDlgItemText(hDlg, IDD_DEVICEREM, (LPSTR) szLocal, 8);

		    switch (WNetCancelConnection((LPSTR) szLocal, bForce)) {
			case WN_SUCCESS:
			    bSuccess = TRUE;
			    wsprintf(szBuffer2, " Cancel Connection Success: Force=%d, Device= ", bForce ? 1 : 0);
			    WriteLog(szBuffer2, szLocal);
			    break;
			case WN_NOT_SUPPORTED:
			    lstrcpy(szBuffer, (LPSTR) "Not Supported");
			    break;
			case WN_NET_ERROR:
			    lstrcpy(szBuffer, (LPSTR) "Net Error");
			    break;
			case WN_BAD_POINTER:
			    lstrcpy(szBuffer, (LPSTR) "Bad Pointer");
			    break;
			case WN_BAD_VALUE:
			    lstrcpy(szBuffer, (LPSTR) "Bad Value");
			    break;
			case WN_NOT_CONNECTED:
			    lstrcpy(szBuffer, (LPSTR) "Not Connected");
			    break;
			case WN_OPEN_FILES:
			    lstrcpy(szBuffer, (LPSTR) "Force Failed: Files Open");
			    break;
			default:
			    lstrcpy(szBuffer, (LPSTR) "Remove Failed");
			    break;
		    }
		    if (!bSuccess) {
			MessageBox(NULL, szBuffer, (LPSTR) "Remove Net Connection", MB_OK | MB_ICONEXCLAMATION);
			wsprintf(szBuffer2, " Cancel Connection Fail: Force=%d, Device=%s, Error= ",
					     bForce ? 1 : 0, (LPSTR) szLocal);
			WriteLog(szBuffer2, szBuffer);
		    }

		    EndDialog(hDlg, TRUE);
		    break;

		default:
		    return(FALSE);
		    break;

	    }
	    break;

	default:
	    return(FALSE);
	    break;
    }
    return (TRUE);

}


/****************************************************************************

    FUNCTION: ListConnsDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "ListConnsBox" dialog box
	      This procedure calls the WNetGetConnection() function and
	      lists all the current connections and their status.
    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL ListConnsDlgProc(HWND hDlg, unsigned message, WORD wParam,
							      LONG lParam)
{

    switch (message) {

	case WM_INITDIALOG:

	    /* Write section title to log file */
	    lstrcpy(szBuffer2, "\n\nG E T   N E T   C O N N E C T I O N S");
	    WriteLog(szBuffer2,"\n");
	    bSuccess = FALSE;
	    bConnected = FALSE;
	    break;

	case WM_COMMAND:

	    switch(wParam) {

		case IDCANCEL:
		    EndDialog(hDlg, TRUE);

		    break;

		case IDD_GETNETCONS:   /* Get Network connections */

		    /* Clear List Box in case it is already full */
		    SendDlgItemMessage(hDlg, IDD_NETLIST, LB_RESETCONTENT, NULL, NULL);

		    lstrcpy(szBuffer2, "\n*NETWORK CONNECTION LIST*");
		    WriteLog(szBuffer2, "\n");

		    nBufferSize = 64;
		    lstrcpy(szLocal, "A:");

		    /* list redirected disk drives */
		    for (i = 0; i < 26; i++) {

			szLocal[0] = (char)('A' + i);

			switch (WNetGetConnection((LPSTR) szLocal, (LPSTR) szRemoteName, (LPWORD) &nBufferSize)) {
			    case WN_SUCCESS:
				lstrcpy(szBuffer, "Connected");
				bSuccess = TRUE;
				bConnected = TRUE;
				break;
			    case WN_NOT_SUPPORTED:
				lstrcpy(szBuffer, "Not Supported");
				break;
			    case WN_NET_ERROR:
				lstrcpy(szBuffer, "Network Error");
				break;
			    case WN_BAD_POINTER:
				lstrcpy(szBuffer, "Bad Pointer");
				break;
			    case WN_BAD_VALUE:
				lstrcpy(szBuffer, "Not a Valid Local Name");
				break;
			    case WN_NOT_CONNECTED:
				lstrcpy(szBuffer, "Not Connected");
				break;
			    case WN_MORE_DATA:
				/* try to allocate a larger buffer */

				nBufferSize = 128;

				szBigRemoteName = (LPSTR)(PSTR) LocalAlloc(LPTR, (int) nBufferSize * sizeof(char)); /* Alloc memory for string */
				WriteLog(szBuffer2, "More Data!");
				/* see if alloc and re-call is successful */
				switch (WNetGetConnection((LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPWORD) &nBufferSize)) {
				    case WN_SUCCESS:
					lstrcpy(szBuffer, "Connected");
					bSuccess = TRUE;
					bConnected = TRUE;
					break;
				    case WN_NOT_SUPPORTED:
					lstrcpy(szBuffer, "Not Supported");
					break;
				    case WN_NET_ERROR:
					lstrcpy(szBuffer, "Network Error");
					break;
				    case WN_BAD_POINTER:
					lstrcpy(szBuffer, "Bad Pointer");
					break;
				    case WN_BAD_VALUE:
					lstrcpy(szBuffer, "Not a Valid Local Name");
					break;
				    case WN_NOT_CONNECTED:
					lstrcpy(szBuffer, "Not Connected");
					break;
				    case WN_MORE_DATA:
					lstrcpy(szBuffer, "Warning! More Data!");
					break;
				}
				if (bConnected) {
				    if (lstrlen(szRemoteName) < 16) /* make output real pretty and aligned */
					wsprintf(szBuffer2, "\t%s\t%s\t\t%s", (LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPSTR) szBuffer);
				    else
					wsprintf(szBuffer2, "\t%s\t%s\t%s", (LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPSTR) szBuffer);
				    SendDlgItemMessage(hDlg, IDD_NETLIST, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer2);
				    WriteLog(szBuffer2, "");
				    bConnected = FALSE;
				}

				LocalFree((LOCALHANDLE) szBigRemoteName); /* free previous handle before another alloc */
				break;
			}
			if (bConnected) {
			    if (lstrlen(szRemoteName) < 16) /* make output real pretty and aligned */
				wsprintf(szBuffer2, "\t%s\t%s\t\t%s", (LPSTR) szLocal, (LPSTR) szRemoteName, (LPSTR) szBuffer);
			    else
				wsprintf(szBuffer2, "\t%s\t%s\t%s", (LPSTR) szLocal, (LPSTR) szRemoteName, (LPSTR) szBuffer);
			    SendDlgItemMessage(hDlg, IDD_NETLIST, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer2);
			    WriteLog(szBuffer2, "");
			    bConnected = FALSE;
			}

		    } /* of For i, for drives */

		    if (!bSuccess) {
			lstrcpy(szBuffer, " No Redirected Local Drives");
			MessageBox(NULL, szBuffer, (LPSTR) "Network Connections", MB_OK | MB_ICONEXCLAMATION);
			WriteLog(szBuffer,  "");
			bSuccess = FALSE;
		    }

		    nBufferSize = 64;
		    bConnected = FALSE;
		    lstrcpy(szLocal, "LPT1:");

		    /* list redirected lpt ports */
		    for (i = 0; i < 3; i++) {

			szLocal[3] = (char)('1' + i);

			switch (WNetGetConnection((LPSTR) szLocal, (LPSTR) szRemoteName, (LPWORD) &nBufferSize)) {
			    case WN_SUCCESS:
				lstrcpy(szBuffer, "Connected");
				bSuccess = TRUE;
				bConnected = TRUE;
				break;
			    case WN_NOT_SUPPORTED:
				lstrcpy(szBuffer, "Not Supported");
				break;
			    case WN_NET_ERROR:
				lstrcpy(szBuffer, "Network Error");
				break;
			    case WN_BAD_POINTER:
				lstrcpy(szBuffer, "Bad Pointer");
				break;
			    case WN_BAD_VALUE:
				lstrcpy(szBuffer, "Not a Valid Local Name");
				break;
			    case WN_NOT_CONNECTED:
				lstrcpy(szBuffer, "Not Connected");
				break;
			    case WN_MORE_DATA: {
				/* try to allocate a larger buffer */

				nBufferSize = 128;

				szBigRemoteName = (LPSTR)(PSTR) LocalAlloc(LPTR, (int) nBufferSize * sizeof(char)); /* Alloc memory for string */
				WriteLog(szBuffer2, "More Data!");
				/* see if alloc and re-call is successful */
				switch (WNetGetConnection((LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPWORD) &nBufferSize)) {
				    case WN_SUCCESS:
					lstrcpy(szBuffer, "Connected");
					bSuccess = TRUE;
					bConnected = TRUE;
					break;
				    case WN_NOT_SUPPORTED:
					lstrcpy(szBuffer, "Not Supported");
					break;
				    case WN_NET_ERROR:
					lstrcpy(szBuffer, "Network Error");
					break;
				    case WN_BAD_POINTER:
					lstrcpy(szBuffer, "Bad Pointer");
					break;
				    case WN_BAD_VALUE:
					lstrcpy(szBuffer, "Not a Valid Local Name");
					break;
				    case WN_NOT_CONNECTED:
					lstrcpy(szBuffer, "Device Not Connected");
					break;
				    case WN_MORE_DATA:
					lstrcpy(szBuffer, "Warning! More Data");
					break;
				}
				if (bConnected) {
				    if (lstrlen(szRemoteName) < 16) /* make output real pretty and aligned */
					wsprintf(szBuffer2, "\t%s\t%s\t\t%s", (LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPSTR) szBuffer);
				    else
					wsprintf(szBuffer2, "\t%s\t%s\t%s", (LPSTR) szLocal, (LPSTR) szBigRemoteName, (LPSTR) szBuffer);
				    SendDlgItemMessage(hDlg, IDD_NETLIST, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer2);
				    WriteLog(szBuffer2, "");
				    bConnected = FALSE;
				}

				LocalFree((LOCALHANDLE) szBigRemoteName); /* free previous handle before another alloc */
				} /* end of WN_MORE_DATA */
				break;
			}
			if (bConnected) {
			    if (lstrlen(szRemoteName) < 16) /* make output real pretty and aligned */
				wsprintf(szBuffer2, "\t%s\t%s\t\t%s", (LPSTR) szLocal, (LPSTR) szRemoteName, (LPSTR) szBuffer);
			    else
				wsprintf(szBuffer2, "\t%s\t%s\t%s", (LPSTR) szLocal, (LPSTR) szRemoteName, (LPSTR) szBuffer);
			    SendDlgItemMessage(hDlg, IDD_NETLIST, LB_ADDSTRING, NULL, (LONG)(LPSTR) szBuffer2);
			    WriteLog(szBuffer2, "");
			    bConnected = FALSE;
			}
   
		    } /* of For i, LPT Ports */

		    if (!bSuccess) {
			lstrcpy(szBuffer, " No Redirected LPT Ports");
			MessageBox(NULL, szBuffer, (LPSTR) "Network Connections", MB_OK | MB_ICONEXCLAMATION);
			WriteLog(szBuffer,  "");
		    }

		    break;  /* WN_GETNETCONS */

		default:
		    return(FALSE);

	    }	   /* switch(wParam) */
	    break; /* case WM_COMMAND */

	default:
	    return(FALSE);


    }  /* switch (message) */

    return (TRUE);  /* Didn't process a message    */

}
