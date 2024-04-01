/***************************************************************************
*
*
*   Program: NetTest.c: a Windows 3.0 Test Application
*
*   Author: Richard Saunders
*
*   Description: NetTest.c is a Windows test application which tests the functional-
*
*	    NetTest also allows the user to run the program in batch mode which
*	    sequencially executes all included function calls with no attention from
*	    the user.
*
*   Functions:
*
*	WinMain() - calls initialization function, processes message loop
*	NetTestInit() - initializes window data and registers window
*	NetTestWndProc() - processes messages
*	About() - processes messages for "About" dialog box
*	Help() - processes messages for "Help" dialog box and displays Help info
*
*   Log History:
*	    5/18/89   Created
*
*
******************************************************************************/

#include "windows.h"
#include "winnet.h"
#include "nettest.h"

HWND hParWnd;
HANDLE hInst;
char szAppName[] = "NetTest";

char szLogFile[] = "Nettest.log"; /* Define vars for Test Log File */

char szBuffer[79];
BOOL bNetwork;

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

	This will initialize the window class if it is the first time this
	application is run.  It then creates the window, and processes the
	message loop until a PostQuitMessage is received.  It exits the
	application by returning the value passed by the PostQuitMessage.

****************************************************************************/

int NEAR PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
    int i;
    char szTest[85];

    MSG msg;				     /* message			     */
    int hLogFile;


    if (!hPrevInstance)			/* Has application been initialized? */
	if (!NetTestInit(hInstance))
	    return (NULL);		/* Exits if unable to initialize     */

    hInst = hInstance;			/* Saves the current instance	     */

    hParWnd = CreateWindow(szAppName,		      /* window class		 */
	"MS Windows 3.0 WinNet Driver Test App",   /* window name	      */
	WS_OVERLAPPEDWINDOW,			  /* window style	     */
	CW_USEDEFAULT,
	CW_USEDEFAULT,				  /* x position 	     */
	CW_USEDEFAULT,				  /* y position 	     */
	CW_USEDEFAULT,				  /* width		     */
	NULL,					 /* height		    */
	NULL,					  /* parent handle	     */
	hInstance,				 /* menu or child ID	    */
	NULL);					  /* instance		     */
						  /* additional info	     */

    if (!hParWnd)				     /* Was the window created? */
	return (NULL);

    /* Create and initialize log file */
    hLogFile = _lcreat( (LPSTR) szLogFile, 0);
    lstrcpy(szBuffer, " Window 3.0 WinNet Driver Test	    ");
    WriteLog(szBuffer, GiveMeTime());
    lstrcpy(szBuffer, "===============================	    ");
    WriteLog(szBuffer, "Log File - Loggin' With Love.\n\n");

    ShowWindow(hParWnd, nCmdShow);		     /* Shows the window	*/
    UpdateWindow(hParWnd);			     /* Sends WM_PAINT message	*/

    /* See if network driver is here and responding */
    if (!(bNetwork = (BOOL)WNetGetCaps((WORD) WNNC_NET_TYPE))) {
	MessageBox(hParWnd, (LPSTR) "Network Driver or Network Software Not Installed", (LPSTR) "Network Detection", MB_OK | MB_ICONEXCLAMATION);
	lstrcpy(szBuffer, "\n!Warning: Network Driver or Network Software Not Installed!");
	WriteLog(szBuffer, "");
    }

    while (GetMessage(&msg, NULL, NULL, NULL)) {

	TranslateMessage(&msg);	   /* Translates virtual key codes	     */
	DispatchMessage(&msg);	   /* Dispatches message to window	     */
    }
    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}

/****************************************************************************

    FUNCTION: NetTestInit(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:



****************************************************************************/

BOOL FAR PASCAL NetTestInit(HANDLE hInstance)
{
    HANDLE hMemory;			       /* handle to allocated memory */
    PWNDCLASS pWndClass;		       /* structure pointer	     */
    BOOL bSuccess;			       /* RegisterClass() result     */

    hMemory = LocalAlloc(LPTR, sizeof(WNDCLASS));
    pWndClass = (PWNDCLASS) LocalLock(hMemory);

    pWndClass->style = NULL;
    pWndClass->lpfnWndProc = NetTestWndProc;
    pWndClass->hInstance = hInstance;
    pWndClass->hIcon = LoadIcon(hInst, "bug");
    pWndClass->hCursor = NULL;
    pWndClass->hbrBackground = GetStockObject(WHITE_BRUSH);
    pWndClass->lpszMenuName = (LPSTR) "NetTest";
    pWndClass->lpszClassName = (LPSTR) "NetTest";

    bSuccess = RegisterClass(pWndClass);

    LocalUnlock(hMemory);			    /* Unlocks the memory    */
    LocalFree(hMemory);				    /* Returns it to Windows */

    return (bSuccess);		 /* Returns result of registering the window */
}


/****************************************************************************

    FUNCTION: Help(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "Help" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

****************************************************************************/

BOOL FAR PASCAL HelpDlgProc(HWND hDlg, unsigned message, WORD wParam,
							 LONG lParam)
{


    switch (message) {

	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    if (wParam == IDOK) {	      /* "OK" box selected?	     */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
		return (TRUE);
	    }
	    break;


    }
    return (FALSE);			      /* Didn't process a message    */
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

	No initialization is needed for this particular dialog box, but TRUE
	must be returned to Windows.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL AboutDlgProc(HWND hDlg, unsigned message, WORD wParam,
							  LONG lParam)
{


    switch (message) {

	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    if (wParam == IDOK) {	      /* "OK" box selected?	     */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
		return (TRUE);
	    }
	    break;


    }
    return (FALSE);			      /* Didn't process a message    */
}


/****************************************************************************

    FUNCTION: BatchFileDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Gets output file for Batch mode execution output

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

    COMMENTS:

****************************************************************************/

BOOL FAR PASCAL BatchFileDlgProc(HWND hDlg, unsigned message, WORD wParam,
							      LONG lParam)
{
    switch (message) {
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    switch (wParam) {
		case IDOK:
		    EndDialog(hDlg, TRUE);	  /* Exits the dialog box	 */
		    break;

		case IDCANCEL:
		    EndDialog(hDlg, FALSE);	   /* Exits the dialog box	  */
		    break;
	     }
    }
    return (FALSE);			      /* Didn't process a message    */
}
