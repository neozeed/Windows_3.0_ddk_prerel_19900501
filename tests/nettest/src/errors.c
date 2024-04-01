/*
 *  Errors.c
 */

#include "windows.h"
#include "winnet.h"
#include "nettest.h"
#include "netdlg.h"

void PASCAL RetrieveErrorText(WORD nError, char *szBuff);

BOOL bSuccess;
char szBuffer[79];
char szBuffer2[128];
char szErrorBuffer[79];
int nIndex, nLength;
WORD nBufferSize, nError;
LPSTR szBigBuffer;
int i;

/* stupid quick way to get at error codes */
typedef struct {
    char *szText;
    WORD nError;
} ErrorStruct;

ErrorStruct ErrorList[25] =
    {
	{"WN_SUCCESS",			    0x00},
	{"WN_NOT_SUPPORTED",		    0x01},
	{"WN_NET_ERROR",		    0x02},
	{"WN_MORE_DATA",		    0x03},
	{"WN_BAD_POINTER",		    0x04},
	{"WN_BAD_VALUE",		    0x05},
	{"WN_BAD_PASSWORD",		    0x06},
	{"WN_ACCESS_DENIED",		    0x07},
	{"WN_FUNCTION_BUSY",		    0x08},
	{"WN_WINDOWS_ERROR",		    0x09},
	{"WN_BAD_USER", 		    0x0a},
	{"WN_OUT_OF_MEMORY",		    0x0b},
	{"WN_CANCEL",			    0xd },
	{"WN_NOT_CONNECTED",		    0x30},
	{"WN_OPEN_FILES",		    0x31},
	{"WN_BAD_NETNAME",		    0x32},
	{"WN_BAD_LOCALNAME",		    0x33},
	{"WN_BAD_JOBID",		    0x40},
	{"WN_JOB_NOT_FOUND",		    0x41},
	{"WN_JOB_NOT_HELD",		    0x42},
	{"WN_BAD_QUEUE",		    0x43},
	{"WN_BAD_FILE_HANDLE",		    0x44},
	{"WN_CANT_SET_COPIES",		    0x45},
	{"WN_ALREADY_LOCKED",		    0x46}
    };

/****************************************************************************

    FUNCTION: GetErrorCode(void)

    PURPOSE:  Gets network status code from the last network operation

    MESSAGES:

    COMMENTS:

****************************************************************************/

BOOL PASCAL GetErrorCode(void)
{
    WORD nError;

    /* write title of section to log file */
    lstrcpy(szBuffer, "\n\nG E T   C U R R E N T   N E T   E R R O R");
    WriteLog(szBuffer, "\n");

    switch (WNetGetError(&nError)) {

	case WN_NOT_SUPPORTED:
	    lstrcpy(szBuffer," GetErrorCode: Function Not Supported");
	    break;

#ifdef WN_NO_ERROR

	case WN_NO_ERROR:
	    lstrcpy(szBuffer," GetErrorCode: No Error Status Available");
	    break;

#endif
	case WN_SUCCESS: /* Success!  See what the error is */

	    /* At this point we have to special case each
	       driver because the errors returned are specific
	       to each network */

	    switch (nError) {

		case WN_SUCCESS:
		    lstrcpy(szBuffer," Success, No Error");
		    break;

		case WN_NOT_SUPPORTED:
		    lstrcpy(szBuffer," Function Not Supported");
		    break;

		case WN_NET_ERROR:
		    lstrcpy(szBuffer," Misc Network Error");
		    break;

		case WN_MORE_DATA:
		    lstrcpy(szBuffer," Warning: Buffer Too Small");
		    break;

		case WN_BAD_POINTER:
		    lstrcpy(szBuffer," Invalid Pointer Specified");
		    break;

		case WN_BAD_VALUE:
		    lstrcpy(szBuffer," Invalid Number Value Specified");
		    break;

		case WN_BAD_PASSWORD:
		    lstrcpy(szBuffer," Incorrect Password Specified");
		    break;

		case WN_ACCESS_DENIED:
		    lstrcpy(szBuffer," Security Violation: Access Denied");
		    break;

		case WN_FUNCTION_BUSY:
		    lstrcpy(szBuffer," Function Can't Be Renentered, Currently Being Used");
		    break;

		case WN_WINDOWS_ERROR:
		    lstrcpy(szBuffer," Windows Error");
		    break;

		case WN_BAD_USER:
		    lstrcpy(szBuffer," Invalid User Name Specified");
		    break;

		case WN_OUT_OF_MEMORY:
		    lstrcpy(szBuffer," Out Of Memory");
		    break;

		case WN_CANCEL:
		    lstrcpy(szBuffer," Bad Cancel");
		    break;

		case WN_NOT_CONNECTED:
		    lstrcpy(szBuffer," Device is Not Redirected");
		    break;

		case WN_OPEN_FILES:
		    lstrcpy(szBuffer," Connection Not Cancelled, Files/Jobs Still Open");
		    break;

		case WN_BAD_NETNAME:
		    lstrcpy(szBuffer," Network Name is Invalid");
		    break;

		case WN_BAD_LOCALNAME:
		    lstrcpy(szBuffer," Invalid Local Name");
		    break;

		case WN_BAD_JOBID:
		    lstrcpy(szBuffer," Invalid Job ID");
		    break;

		case WN_JOB_NOT_FOUND:
		    lstrcpy(szBuffer," No Job Found With This ID");
		    break;

		case WN_JOB_NOT_HELD:
		    lstrcpy(szBuffer," Job Can't Be Released, Not Currently Held");
		    break;

		case WN_BAD_QUEUE:
		    lstrcpy(szBuffer," No Queue For Network Name or Redirected Device");
		    break;

		case WN_BAD_FILE_HANDLE:
		    lstrcpy(szBuffer," Not a Valid File Handle or Print File Open By DRV");
		    break;

		case WN_CANT_SET_COPIES:
		    lstrcpy(szBuffer," Warning: Can't Set Number of Copies; Printing One");
		    break;

		case WN_ALREADY_LOCKED:
		    lstrcpy(szBuffer," Queue Specified is Already Locked by LockQueue");
		    break;

		default:
		    lstrcpy(szBuffer," GetError success but\n undoc'ed code returned by driver!");
		    break;

	    } /* switch (nError) */

	    MessageBox(NULL, (LPSTR) szBuffer, (LPSTR) "Current NetWork Error", MB_ICONEXCLAMATION | IDOK);
	    wsprintf(szBuffer2, " ErrorCode= %x,", nError);
	    WriteLog(szBuffer2, szBuffer);
	    return(TRUE);

	    break; /* WN_SUCCESS */

    }	/* switch WNetGetError */

    MessageBox(NULL, (LPSTR) szBuffer, (LPSTR) "Get Net Error Code Function", MB_ICONEXCLAMATION | IDOK);
    WriteLog(szBuffer, ", No Net Error Code Returned.");
    return(TRUE);

}

/****************************************************************************

    FUNCTION: GetErrorCodeTextDlgProc(void)

    PURPOSE:  Gets text description associated with network error code.

    MESSAGES:

    COMMENTS:

****************************************************************************/

BOOL FAR PASCAL GetErrorTextDlgProc(HWND hDlg, unsigned message, WORD wParam,
							      LONG lParam)
{

    switch (message) {

	case WM_INITDIALOG:
	    /* write title of section to log file */
	    lstrcpy(szBuffer, "\n\nG E T   E R R O R   C O D E	 T E X T");
	    WriteLog(szBuffer, "\n");

	    nBufferSize = 79;
	    bSuccess = FALSE;

	    SendDlgItemMessage(hDlg, IDD_ERRORCODELIST, WM_SETREDRAW, FALSE, 0L);  /* temporarirly turn off redraw while init. */

	    for (i = 0; i < 25; i++) {
		SendDlgItemMessage(hDlg,	/* Send message to hDlg: Execute */
			     IDD_ERRORCODELIST, /* Send the message to the listbox */
			     LB_ADDSTRING,	/* the message is to add a string */
			     NULL,		/* wParam is null */
			     (LONG)(LPSTR) ErrorList[i].szText);     /* add this string */
	    }

	    SendDlgItemMessage(hDlg, IDD_ERRORCODELIST, WM_SETREDRAW, TRUE, 0L);  /* re-enable redraw of listbox */

	    return (TRUE);
	    break;

	case WM_COMMAND:

	    switch (wParam) {

		case IDCANCEL:
		    EndDialog(hDlg, TRUE);
		    break;

		case IDOK:

		    /* Get the nIndex of the current selection */
		    nIndex = (WORD) SendDlgItemMessage(hDlg,
							IDD_ERRORCODELIST,
							LB_GETCURSEL,
							0,
							0L);

		    if (nIndex == LB_ERR) {   /* check for no selection */
			/* no listbox selection made */
			MessageBox(hDlg, (LPSTR) "No Selection Made", "Get Error Text", MB_OK | MB_ICONEXCLAMATION); /* no listbox selection message */
			break;
		    }

		    /* Get the text of the current listbox selection, nIndex */
		    nLength = (WORD) SendDlgItemMessage(hDlg,
						    IDD_ERRORCODELIST,
						    LB_GETTEXT,
						    nIndex,
						    (LONG)(LPSTR) szBuffer);

		    /* write error code name out to log file */
		    lstrcpy(szBuffer2, "\n Error Code = ");
		    WriteLog(szBuffer2, szBuffer);

		    /* get the error value associated with the text... */
		    i = 0;
		    while (lstrcmp(ErrorList[i].szText, szBuffer) != 0)
			i++;

		    nError = ErrorList[i].nError;

		    switch (WNetGetErrorText(nError, (LPSTR) szErrorBuffer, (LPWORD) &nBufferSize)) {

			case WN_SUCCESS:
			    RetrieveErrorText(nError, szBuffer);
			    bSuccess = TRUE;
			    break;

			case WN_NOT_SUPPORTED:
			    lstrcpy(szBuffer," GetErrorText: Function Not Supported");
			    break;

			case WN_NET_ERROR:
			    lstrcpy(szBuffer," GetErrorText: Net Error Occurred");
			    break;

#ifdef WN_NO_ERROR

			case WN_NO_ERROR:
			    lstrcpy(szBuffer," GetErrorText: No Error Status Available");
			    break;

#endif
			case WN_MORE_DATA:
			   /* try to allocate a larger buffer */

			    szBigBuffer = (LPSTR)(PSTR) LocalAlloc(LPTR, 128 * sizeof(char)); /* Alloc memory for string */

			    /* see if alloc and re-call is successful */
			    switch (WNetGetErrorText(nError, (LPSTR) szBigBuffer, (LPWORD) &nBufferSize)) {
				case WN_SUCCESS:
				    RetrieveErrorText(nError, szBigBuffer);
				    bSuccess = TRUE;
				    break;
				case WN_NOT_SUPPORTED:
				    lstrcpy(szBigBuffer, " GetErrorText: Function Not Supported");
				    break;
				case WN_NET_ERROR:
				    lstrcpy(szBigBuffer, " GetErrorText: Net Error Occurred");
				    break;
#ifdef WN_NO_ERROR

				case WN_NO_ERROR:
				    lstrcpy(szBigBuffer," GetErrorText: No Error Status Available");
				    break;

#endif
				case WN_MORE_DATA:
				    lstrcpy(szBigBuffer, " GetErrorText: More Data - Realloc Unsuccessful");
				    break;
			    }
			    if (!bSuccess) {
				MessageBox(NULL, (LPSTR) szBigBuffer, (LPSTR) "Get Error Text", MB_ICONEXCLAMATION | IDOK);
				WriteLog(szBigBuffer,"");
				lstrcpy(szErrorBuffer, "No String Returned");
				lstrcpy(szBigBuffer, "No String Returned");
			    }
			    SendDlgItemMessage(hDlg,
					 IDD_CORRECTTEXT,
					 WM_SETTEXT,
					 NULL,
					 (LONG)(LPSTR) szBigBuffer);
			    SendDlgItemMessage(hDlg,
					 IDD_DRIVERTEXT,
					 WM_SETTEXT,
					 NULL,
					 (LONG)(LPSTR) szErrorBuffer);

			    /* write log */
			    lstrcpy(szBuffer2, " Correct Text =");
			    WriteLog(szBuffer2, szBigBuffer);
			    lstrcpy(szBuffer2, " Driver Text =");
			    WriteLog(szBuffer2, szErrorBuffer);

			    LocalFree((LOCALHANDLE) szBigBuffer); /* free previous handle before another alloc */
			    return(TRUE);
			    break;
		    }

		    if (!bSuccess) {
			MessageBox(NULL, (LPSTR) szBuffer, (LPSTR) "Get Error Text", MB_ICONEXCLAMATION | IDOK);
			WriteLog(szBuffer,"");
			lstrcpy(szErrorBuffer, "No String Returned");
			lstrcpy(szBuffer, "No String Returned");
		    }
		    SendDlgItemMessage(hDlg,
			     IDD_CORRECTTEXT,
			     WM_SETTEXT,
			     NULL,
			     (LONG)(LPSTR) szBuffer);
		    SendDlgItemMessage(hDlg,
			     IDD_DRIVERTEXT,
			     WM_SETTEXT,
			     NULL,
			     (LONG)(LPSTR) szErrorBuffer);

		    /* write log */
		    lstrcpy(szBuffer2, " Correct Text =");
		    WriteLog(szBuffer2, szBuffer);
		    lstrcpy(szBuffer2, " Driver Text =");
		    WriteLog(szBuffer2, szErrorBuffer);

		    return(TRUE);

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

    FUNCTION: RetrieveErrorText(WORD nError)

    PURPOSE:  Retrieves the error text associated with nError.

    MESSAGES:

    COMMENTS:

****************************************************************************/

void PASCAL RetrieveErrorText(WORD nError, char *szBuff)
{

	    switch (nError) {

		case WN_SUCCESS:
		    lstrcpy(szBuff," Success, No Error");
		    break;

		case WN_NOT_SUPPORTED:
		    lstrcpy(szBuff," Function Not Supported");
		    break;

		case WN_NET_ERROR:
		    lstrcpy(szBuff," Misc Network Error");
		    break;

		case WN_MORE_DATA:
		    lstrcpy(szBuff," Warning: Buffer Too Small");
		    break;

		case WN_BAD_POINTER:
		    lstrcpy(szBuff," Invalid Pointer Specified");
		    break;

		case WN_BAD_VALUE:
		    lstrcpy(szBuff," Invalid Number Value Specified");
		    break;

		case WN_BAD_PASSWORD:
		    lstrcpy(szBuff," Incorrect Password Specified");
		    break;

		case WN_ACCESS_DENIED:
		    lstrcpy(szBuff," Security Violation: Access Denied");
		    break;

		case WN_FUNCTION_BUSY:
		    lstrcpy(szBuff," Function Can't Be Renentered, Currently Being Used");
		    break;

		case WN_WINDOWS_ERROR:
		    lstrcpy(szBuff," Windows Error");
		    break;

		case WN_BAD_USER:
		    lstrcpy(szBuff," Invalid User Name Specified");
		    break;

		case WN_OUT_OF_MEMORY:
		    lstrcpy(szBuff," Out Of Memory");
		    break;

		case WN_CANCEL:
		    lstrcpy(szBuff," Bad Cancel");
		    break;

		case WN_NOT_CONNECTED:
		    lstrcpy(szBuff," Device is Not Redirected");
		    break;

		case WN_OPEN_FILES:
		    lstrcpy(szBuff," Connection Not Cancelled, Files/Jobs Still Open");
		    break;

		case WN_BAD_NETNAME:
		    lstrcpy(szBuff," Network Name is Invalid");
		    break;

		case WN_BAD_LOCALNAME:
		    lstrcpy(szBuff," Invalid Local Name");
		    break;

		case WN_BAD_JOBID:
		    lstrcpy(szBuff," Invalid Job ID");
		    break;

		case WN_JOB_NOT_FOUND:
		    lstrcpy(szBuff," No Job Found With This ID");
		    break;

		case WN_JOB_NOT_HELD:
		    lstrcpy(szBuff," Job Can't Be Released, Not Currently Held");
		    break;

		case WN_BAD_QUEUE:
		    lstrcpy(szBuff," No Queue For Network Name or Redirected Device");
		    break;

		case WN_BAD_FILE_HANDLE:
		    lstrcpy(szBuff," Not a Valid File Handle or Print File Open By DRV");
		    break;

		case WN_CANT_SET_COPIES:
		    lstrcpy(szBuff," Warning: Can't Set Number of Copies; Printing One");
		    break;

		case WN_ALREADY_LOCKED:
		    lstrcpy(szBuff," Queue Specified is Already Locked by LockQueue");
		    break;

	    }


}
