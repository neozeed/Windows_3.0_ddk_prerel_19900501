/*
 *  Test.c - Handles all TEST menu items
 */

#include "windows.h"
#include "nettest.h"
#include "stdio.h"

extern HWND hParWnd;

char szFileName[12];  /* string containing current file */
char szBuffer[79];    /* display string */
FILE *fhandle[60];    /* array of open file handles */

/****************************************************************************

    FUNCTION: BOOL FAR PASCAL NetBiosTest(HWND hWnd)

    PURPOSE:  Spawns Iris' TestNetw NetBIOS test program

    MESSAGES:

    COMMENTS:


****************************************************************************/

BOOL FAR PASCAL NetBiosTest(HWND hWnd)
{
    if (MessageBox(NULL, (LPSTR) "Output Will Go To Com1 and the NetBIOSW.log File.",
		     (LPSTR) "NetBIOS Test", MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONASTERISK) == IDCANCEL)
	return(FALSE);

    if (!WinExec("netbiosw.exe", SW_SHOWNORMAL)) {
	MessageBox(NULL, (LPSTR) "Error Execing NetBIOSW.EXE",
		   (LPSTR) "NetBIOS Test", MB_OK | MB_ICONEXCLAMATION);
	return(FALSE);
    }

    lstrcpy(szBuffer, "\n\nN E T B I O S   T E S T");
    WriteLog(szBuffer, "\n");

    lstrcpy(szBuffer, " NetBIOS Test Run To Completion.");
    WriteLog(szBuffer, "  See NetBIOSW.log for Results.");

    return(TRUE);
}


/****************************************************************************

    FUNCTION: BOOL PASCAL FileCount(void)

    PURPOSE:  Counts the number of Available file handles in the system

    MESSAGES:

    COMMENTS:


****************************************************************************/

BOOL PASCAL FileCount(void)
{
    int i;
    int nfcount;	      /* number of files opened */
    FILE *hLogFile;	      /* array of file handles */


    nfcount = 0;
    hLogFile = TRUE;

    lstrcpy(szFileName, "testA.dat");

    /* Write section title to log file */
    lstrcpy(szBuffer, "\n\nA V A I L A B L E   F I L E	 H A N D L E   C O U N T");
    WriteLog(szBuffer, "\n");

    while (hLogFile != NULL)  {
	szFileName[4] = (char) (nfcount + 'A'); /* change name of file */
	fhandle[nfcount] = hLogFile = fopen(szFileName, "w+");
	nfcount++;
    }

    wsprintf(szBuffer, " There were %d file handles eaten.", nfcount);
    MessageBox(hParWnd, (LPSTR) szBuffer, (LPSTR) "File Handle Count", MB_OK);
    LoadIcon(hInst, "NEW");
    wsprintf(szBuffer, " Resource loaded successfully with\nno available file handles.");
    MessageBox(hParWnd, (LPSTR) szBuffer, (LPSTR) "Load Resource", MB_OK);

    /* free up the handles and delete files*/
    for (i = 0; i < nfcount; i++) {
	fclose(fhandle[i]);
	szFileName[4] = (char) (i + 'A'); /* change name of file */
	remove(szFileName);
    }

    wsprintf(szBuffer, " There are %d available file handles.", nfcount);
    WriteLog(szBuffer, "");
    MessageBox(hParWnd, (LPSTR) szBuffer, (LPSTR) "File Handle Count", MB_OK);

    return(TRUE);
}
