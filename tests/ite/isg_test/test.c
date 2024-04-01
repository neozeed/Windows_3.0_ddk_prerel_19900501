/*---------------------------------------------------------------------------*\
| TEST MODULE                                                                 |
|   This module is an object oriented library for common routines used by     |
|   testing applications.                                                     |
|                                                                             |
| OBJECT                                                                      |
|   TEST                                                                      |
|                                                                             |
| METHODS                                                                     |
|   InitTest()                                                                |
|   EnumTests()                                                               |
|   ExecuteTest()                                                             |
|   KillTest()                                                                |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 22, 1990                                                   |
| SEGMENT: _TEST                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"

/*---------------------------------------------------------------------------*\
| ENUMERATE TESTS                                                             |
|   This routine returns the test routines (objects) which this module        |
|   supports.  It does so by loading the resouce file (tstlist.txt) and       |
|   parsing the tests into a TEST object format.  The object is then returned |
|   to the call-back function supplied by the caller.                         |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE  hInstance  - Instance of application to load list from.           |
|   WORD    wTestArea  - Area of tests (GDI, KERNEL, USER, ...)               |
|   FARPROC fpTestFunc - Callback routine to send objects.                    |
|   LPSTR   lpData     - Data (array) to pass to the callback.                |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   int - returns (1) while there are still tests which exist.                |
|         returns (0) when end of tests are reached.                          |
|         returns (-1) if error.                                              |
\*---------------------------------------------------------------------------*/
int FAR PASCAL EnumTests(lpTest,wTestArea,fpTestFunc,lpData)
     LPTEST  lpTest;
     WORD    wTestArea;
     FARPROC fpTestFunc;
     LPSTR   lpData;
{
     HANDLE hList;
     LPSTR  lpList,lpS,lpD;
     int    nIdx,nTstCount;
     TEST   tTest;

     /*-----------------------------------------*\
     | Load the testing resouce file and lock.   |
     \*-----------------------------------------*/
     if(hList = LoadResource(lpTest->hInstance,FindResource(lpTest->hInstance,"TSTLIST","TEXT")))
     {
          if(!(lpList = LockResource(hList)))
          {
               FreeResource(hList);
               return(-1);
          }
          lpS = lpList;
          lpD = lpList;
          nTstCount=0;
          while(*lpS != '\x1A')
          {
               if(*lpS == '\x0D')
               {
                    *(lpD+FUNCT_SIZE)              = '\0';
                    *(lpD+FUNCT_SIZE+DESCR_SIZE+1) = '\0';
                    *lpS++ = '\0';
                    *lpS++ = '\0';
                    lpD = lpS;
                    nTstCount++;
               }
               else
                    lpS++;
          }
     }
     else
          return(-1);

     /*-----------------------------------------*\
     | Fill in test information.                 |
     \*-----------------------------------------*/
     lpS=lpList;
     for(nIdx=0; nIdx < nTstCount; nIdx++)
     {
          lpD = lpS+(LINE_SIZE*nIdx);

          tTest.lpszFunction    = lpD;
          tTest.lpszDescription = lpD+FUNCT_SIZE+1;
          tTest.wTestArea       = latoi(lpD+FUNCT_SIZE+DESCR_SIZE+2);

          if((wTestArea == tTest.wTestArea) || (wTestArea == NULL))
          {
               if(!(int)(*fpTestFunc)((LPTEST)&tTest,(LPSTR)lpData))
               {
                    UnlockResource(hList);
                    FreeResource(hList);
                    return(0);
               }
          }
     }

     UnlockResource(hList);
     FreeResource(hList);

     return(0);
}


/*---------------------------------------------------------------------------*\
| EXECUTE TEST                                                                |
|   This routine executes the test specified in the TEST structure.           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND  hWnd  - Handle to the application window.                           |
|   LPSTR lpTst - Command string.                                             |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   int  nIterate - Number of iterations to execute test.  (pass to funct)    |
|   WORD fFlags   - Test flag.  (pass to funct)                               |
|   WORD wClipRgn - Type of clipping region.                                  |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - True indicates the test passed, otherwise failure.                 |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL ExecuteTest(hWnd,hDC,tlTest)
     HWND   hWnd;
     HDC    hDC;
     LPTEST tlTest;
{
     HCURSOR hCursor;
     FARPROC lpProc;
     BOOL    bRet;
     HANDLE  hLib;

     /*-----------------------------------------*\
     | Get the procedure address of the test prc.|
     \*-----------------------------------------*/
     hLib = GetModuleHandle(tlTest->lpszModule);
     if(!(lpProc = (FARPROC)GetProcAddress(hLib,tlTest->lpszFunction)))
          return(0l);

     /*-----------------------------------------*\
     | Hide cursor and disable input.            |
     \*-----------------------------------------*/
     hCursor = SetCursor(NULL);
     ClearScreen(hWnd);

     /*-----------------------------------------*\
     | Execute test.  Returns time to execute.   |
     \*-----------------------------------------*/
     SetTestEnvironment(hWnd,hDC,tlTest);
     OutputDebugTest(tlTest,"START");
     bRet = (int)(*lpProc)((HWND)hWnd,(HDC)hDC,(LPTEST)tlTest);
     OutputDebugTest(tlTest,"END");
     RestoreTestEnvironment(hWnd,hDC,tlTest);

     /*-----------------------------------------*\
     | Free up the DC settings.                  |
     \*-----------------------------------------*/
     ClearScreen(hWnd);
     SetCursor(hCursor);

     return(bRet);
}


/*---------------------------------------------------------------------------*\
| INIT TEST                                                                   |
|   This routine initializes the attributes associated with the test object.  |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   LPSTRT lpszModule - String representing the test library name.            |
|   HANDLE hInstance  - Instance handle of the testing library DS.            |
|   LPTEST lpTest     - Testing object.                                       |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - True indicates the test passed, otherwise failure.                 |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL InitTest(lpszModule,hInstance,tTest)
     LPSTR  lpszModule;
     HANDLE hInstance;
     LPTEST tTest;
{
     /*-----------------------------------------*\
     | Initialize STANDARD test settings.        |
     \*-----------------------------------------*/
     lstrcpy(tTest->lpszModule,lpszModule);
     tTest->hInstance       = hInstance;
     tTest->lpszFunction    = NULL;
     tTest->lpszDescription = NULL;
     tTest->wTestArea       = NULL;
     tTest->wStatus         = 0;
     tTest->wGranularity    = 1;
     tTest->wIterations     = 1;
     SetRect(&tTest->rTestRect,0,0,0,0);

     /*-----------------------------------------*\
     | Initialize GDI Flags/Values.              |
     \*-----------------------------------------*/
     tTest->gtTest.hPens         = GetDeviceObjects(NULL,DEV_INDEX);
     tTest->gtTest.hBrushes      = GetDeviceObjects(NULL,DEV_INDEX);
     tTest->gtTest.hFonts        = GetDeviceObjects(NULL,DEV_INDEX);
     tTest->gtTest.hRegion       = NULL;
     tTest->gtTest.nBkMode       = OPAQUE;
     tTest->gtTest.crBkColor     = RGB(255,255,255);
     tTest->gtTest.nPolyFillMode = ALTERNATE;
     tTest->gtTest.nROP2         = R2_COPYPEN;
     tTest->gtTest.nStretchMode  = BLACKONWHITE;
     tTest->gtTest.crTextColor   = RGB(0,0,0);
     tTest->gtTest.dwROP         = SRCCOPY;

     /*-----------------------------------------*\
     | Initialize USER Flags/Values.             |
     \*-----------------------------------------*/
     tTest->utTest.wDlgFlags   = 0x0000;
     tTest->utTest.dwDlgStyles = 0x00000000;
     tTest->utTest.wWinFlags   = 0x0000;
     tTest->utTest.dwWinStyles = 0x00000000;

     /*-----------------------------------------*\
     | Initialize KERNEL Flags/Values.           |
     \*-----------------------------------------*/
     tTest->ktTest.wKernelFlags = 0x0000;

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| KILL TEST                                                                   |
|   This routine cleans up the instances of the test object.                  |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   LPTEST lpTest - Testing object.                                           |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - True indicates the test passed, otherwise failure.                 |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL KillTest(tTest)
     LPTEST tTest;
{
     if(tTest->gtTest.hPens)
          FreeDeviceObjects(tTest->gtTest.hPens);
     if(tTest->gtTest.hBrushes)
          FreeDeviceObjects(tTest->gtTest.hBrushes);
     if(tTest->gtTest.hFonts)
          FreeDeviceObjects(tTest->gtTest.hFonts);
     if(tTest->gtTest.hRegion)
          DeleteObject(tTest->gtTest.hRegion);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
BOOL SetTestEnvironment(hWnd,hDC,ptTest)
     HWND   hWnd;
     HDC    hDC;
     LPTEST ptTest;
{
     HRGN  hTmpRgn[6],hClipRgn;
     POINT pPolygon[5];

     /*-----------------------------------------*\
     | Set GDI specific environment.             |
     \*-----------------------------------------*/
     SaveDC(hDC);

     SetBkMode(hDC,ptTest->gtTest.nBkMode);
     SetBkColor(hDC,ptTest->gtTest.crBkColor);
     SetPolyFillMode(hDC,ptTest->gtTest.nPolyFillMode);
     SetROP2(hDC,ptTest->gtTest.nROP2);
     SetStretchBltMode(hDC,ptTest->gtTest.nStretchMode);
     SetTextColor(hDC,ptTest->gtTest.crTextColor);

     DeleteObject(SetClassWord(hWnd,GCW_HBRBACKGROUND,
          CreateSolidBrush(ptTest->gtTest.crBkColor)));

     /*-----------------------------------------*\
     | Set Clipping region.  If desired.         |
     \*-----------------------------------------*/
     if(ptTest->gtTest.hRegion)
          SelectClipRgn(hDC,ptTest->gtTest.hRegion);
     ptTest->gtTest.hDC = hDC;

     /*-----------------------------------------*\
     | Set Kernel Specific environment.          |
     \*-----------------------------------------*/

     /*-----------------------------------------*\
     | Set User Specific environment.            |
     \*-----------------------------------------*/
     GetClientRect(hWnd,&ptTest->rTestRect);

     return(TRUE);
}

/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
BOOL RestoreTestEnvironment(hWnd,hDC,ptTest)
     HWND   hWnd;
     HDC    hDC;
     LPTEST ptTest;
{
     RestoreDC(hDC,-1);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
BOOL OutputDebugTest(tlTest,lpszString)
     LPTEST tlTest;
     LPSTR  lpszString;
{
     char szBuffer[80];

     lstrcpy(szBuffer,"[");
     lstrcat(szBuffer,tlTest->lpszModule);
     lstrcat(szBuffer," : ");
     lstrcat(szBuffer,tlTest->lpszFunction);
     lstrcat(szBuffer,"]");
     lstrcat(szBuffer," --> ");
     lstrcat(szBuffer,lpszString);
     lstrcat(szBuffer,"\015\012");
     OutputDebugString(szBuffer);

     return(TRUE);
}

BOOL FAR PASCAL OutputTestLog(szLogFile,tlTest,wDetail,wFailure)
     LPSTR  szLogFile;
     LPTEST tlTest;
     WORD   wDetail;
     WORD   wFailure;
{
     int      hFile,nIdx;
     LONG     lFile;
     DATETIME dt;
     DWORD    dwTestBed;
     char     szBuffer[80];
     static   LONGTEXT fTestBed[] = {WF_CPU086,"  8086",
                                     WF_CPU186,"  80186",
                                     WF_CPU286,"  80286",
                                     WF_CPU386,"  80386",
                                     WF_CPU486,"  80486",
                                     WF_STANDARD,", Windows 3.0/Standard Mode",
                                     WF_ENHANCED,", Windows 3.0/Enhanced Mode",
                                     WF_PMODE," - Protected Mode",
                                     WF_SMALLFRAME,", Expanded Memory <Small Frame>",
                                     WF_LARGEFRAME,", Expanded Memory <Large Frame>",
                                     WF_80x87,", CoProcessor Installed"};

     /*-----------------------------------------*\
     | Open logfile.  If one doesn't exist, then |
     | create it and seek to the end.            |
     \*-----------------------------------------*/
     if((hFile = _lopen(szLogFile,OF_READWRITE)) < 0)
     {
          if((hFile = _lcreat(szLogFile,0)) < 0)
               return(-1);
     }
     lFile = _llseek(hFile,0l,2);

     switch(wDetail)
     {
          /*------------------------------------*\
          | Output Header Information.           |
          \*------------------------------------*/
          case LOGFILE_LEVEL0:
               GetSystemDateTime(&dt);
               wsprintf(szBuffer,"TESTBED CONFIGURATION <%02d/%02d/%04d>--<%02d:%02d:%02d>\015\012",
                    dt.bMonth,dt.bDay,dt.wYear,dt.bHours,dt.bMinutes,dt.bSeconds);
               _lwrite(hFile,szBuffer,lstrlen(szBuffer));

               dwTestBed = GetWinFlags();
               for(nIdx=0; nIdx < 11; nIdx++)
               {
                    if(dwTestBed & fTestBed[nIdx].dwFlag)
                    {
                         lstrcpy(szBuffer,fTestBed[nIdx].lpText);
                         _lwrite(hFile,szBuffer,lstrlen(szBuffer));
                    }
               }
               lstrcpy(szBuffer,"\015\012\015\012");
               _lwrite(hFile,szBuffer,lstrlen(szBuffer));
               break;

          /*------------------------------------*\
          |                                      |
          \*------------------------------------*/
          case LOGFILE_LEVEL1:
               lstrcpy(szBuffer,tlTest->lpszFunction);
               if(wFailure == TRUE)
                    lstrcat(szBuffer," -> Passed\015\012");
               else
                    lstrcat(szBuffer," -> Failed\015\012");
               _lwrite(hFile,szBuffer,lstrlen(szBuffer));
               break;

          /*------------------------------------*\
          |                                      |
          \*------------------------------------*/
          case LOGFILE_LEVEL2:
               GetSystemDateTime(&dt);
               break;
     }

     _lclose(hFile);
     return(TRUE);
}
