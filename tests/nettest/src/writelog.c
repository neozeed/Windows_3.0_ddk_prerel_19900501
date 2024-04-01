/***************************************************************************
*
*
*   Program: NetTest.c: a Windows 3.0 Test Application
*
*   Author: Richard Saunders
*
*   Description: Writelog.c handles all file logging
*
*   Functions:
*
*   Log History:
*	    8/89   Created
*
*
******************************************************************************/

#include "windows.h"
#include "nettest.h"
#include "time.h"

/****************************************************************************

    FUNCTION: WriteLog(char *szString1,char *szString)

    PURPOSE:  Concatenates szString2 to szString1 and then
	      writes szString1 to a log file, szLogFile.

    COMMENTS:

	      szLogFile is a inherited from Nettest.c

****************************************************************************/

BOOL PASCAL WriteLog(char szBuff1[128], char szBuff2[79])
{
    int hLogFile;
    LONG   lFile;


    hLogFile = _lopen( (LPSTR) szLogFile, READ_WRITE);
    lFile = _llseek(hLogFile,0l,2);

    lstrcat(szBuff1, szBuff2);	/*  Make just one string */
    lstrcat(szBuff1, "\n");	/*  Line Feeds added for super file formatting! */

    if (!_lwrite(hLogFile, (LPSTR) szBuff1, lstrlen(szBuff1))) {
	MessageBox(NULL, szBuff1, (LPSTR) "Log File Error", MB_ICONHAND | IDOK);
	return(FALSE);
    }

    _lclose(hLogFile);

    return(TRUE);
}

/****************************************************************************

    FUNCTION: GiveMeTime()

    PURPOSE:  Returns a string containing the time

    COMMENTS:

	      Uses C run-time library functions

****************************************************************************/

PSTR PASCAL GiveMeTime()
{

    struct tm *newtime;
    time_t  aclock;

    time(&aclock);
    newtime = localtime(&aclock);

    return(asctime(newtime));


}
