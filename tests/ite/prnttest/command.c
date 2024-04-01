/*---------------------------------------------------------------------------*\
| PROCESS PRNTTEST COMMANDS                                                   |
|   This module contains the routine(s) necessary to process the message      |
|   WM_COMMAND for windows.                                                   |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jun 27, 1989 - changed printing logic to include header as part of |
|                         test.                                               |
|          Sep 03, 1989 - made minor changes for test for randyGs problems.   |
|          Oct 18, 1989 - more comments.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| PROCESS APPLICATION COMMANDS                                                |
|   This routine handles all WM_COMMAND messages sent by Windows to this      |
|   application.  The message ID is passed in the wParam variable.            |
|                                                                             |
| CALLED ROUTINES                                                             |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hWnd   - Handle to client window.                                    |
|   WORD wParam - Message to be processed.                                    |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if message was processed.                                     |
\*---------------------------------------------------------------------------*/
BOOL ProcessPrntTestCommands(hWnd,wParam)
     HWND hWnd;
     WORD wParam;
{
     extern BOOL            bAutoRun,bAbort;
     extern HDEVOBJECT      hFonts,hPens,hBrushes;
     extern PRINTER         pPrinter;
     extern DEVINFO         dcDevCaps;
     extern FARPROC         lpAbortProc,lpAbortDlg;
     extern HANDLE          hInst;
     extern HWND            hAbortDlg,hPrntDlg;
     extern WORD            wHeaderSet,wTestsSet,wTestsEnded;
     extern TEST            tlTest;

     FARPROC lpProc;
     HDC     hDC;
     char    szBuffer[80],szTestProfile[80],szTestString[80];
     LPSTR   lpPtr;
     int     nCount,nIdx;

     switch(wParam)
     {
          /*------------------------------------*\
          | RUN Test.                            |
          \*------------------------------------*/
          case IDM_TEST_RUN:
               /*-------------------------------*\
               | Get the list of test profiles.  |
               \*-------------------------------*/
               nCount = (int)SendDlgItemMessage(hPrntDlg,IDD_INTRFACE_TEST,
                                  LB_GETCOUNT,NULL,0l);

               for(nIdx=0; nIdx < nCount; nIdx++)
               {
                    /*--------------------------*\
                    | Get device string for this |
                    | particular profile string. |
                    \*--------------------------*/
                    SendDlgItemMessage(hPrntDlg,IDD_INTRFACE_TEST,LB_GETTEXT,
                                       nIdx,(LONG)(LPSTR)szBuffer);

                    /*--------------------------*\
                    | get strings from list.     |
                    \*--------------------------*/
                    lpPtr = szBuffer;
                    while(*lpPtr++ != ':');
                    *(lpPtr-1) = '\0';
                    lstrcpy((LPSTR)szTestProfile,szBuffer);
                    lstrcpy((LPSTR)szTestString,lpPtr);

                    /*--------------------------*\
                    | Get Printer DC to test.    |
                    \*--------------------------*/
                    if(!(hDC = GetPrinterDC(hWnd,szTestProfile)))
                    {
                         MessageBox(hWnd,"GetPrinterDC (misc.c)","Assertion",MB_OK);
                         return(FALSE);
                    }

                    /*--------------------------*\
                    | Get device information.    |
                    \*--------------------------*/
                    GetDeviceInfo(hDC,&dcDevCaps);
                    GetPrinterInformation(hDC,&pPrinter,szTestProfile,szTestString);
                    hFonts = GetDeviceObjects(hDC,DEV_FONT);
                    hBrushes = GetDeviceObjects(hDC,DEV_BRUSH);
                    hPens = GetDeviceObjects(hDC,DEV_PEN);

                    /*--------------------------*\
                    | Call SetupObjects for user |
                    | selection.                 |
                    \*--------------------------*/
                    if(!(lpProc = MakeProcInstance(SetupObjectsDlg,hInst)))
                    {
                         DeleteDC(hDC);
                         return(FALSE);
                    }
                    DialogBox(hInst,"SETOBJECT",hWnd,lpProc);
                    FreeProcInstance(lpProc);

                    /*--------------------------*\
                    | Setup the Abort Dialog,    |
                    | Disable main application   |
                    | window.                    |
                    \*--------------------------*/
                    EnableWindow(hWnd,FALSE);
                    bAbort      = FALSE;
                    lpAbortDlg  = MakeProcInstance(AbortDlg,hInst);
                    hAbortDlg   = CreateDialog(hInst,"ABORTDLG",hWnd,lpAbortDlg);
                    lpAbortProc = MakeProcInstance(PrintAbortProc,hInst);
                    if(Escape(hDC,SETABORTPROC,NULL,(LPSTR)lpAbortProc,NULL) < 0)
                    {
                         MessageBox(hWnd,"Escape SetAbortProc (command.c)","Assertion",MB_OK);
                         EnableWindow(hWnd,TRUE);
                         FreeProcInstance(lpAbortDlg);
                         FreeProcInstance(lpAbortProc);
                         DeleteDC(hDC);
                         return(FALSE);
                    }

                    /*--------------------------*\
                    | START print job.           |
                    \*--------------------------*/
                    LoadString(hInst,IDS_TEST_JOBTITLE,szBuffer,sizeof(szBuffer));
                    if(Escape(hDC,STARTDOC,lstrlen(szBuffer),szBuffer,NULL) < 0)
                    {
                         MessageBox(hWnd,"Escape StartDoc (command.c)","Assertion",MB_OK);
                         EnableWindow(hWnd,TRUE);
                         FreeProcInstance(lpAbortDlg);
                         FreeProcInstance(lpAbortProc);
                         DeleteDC(hDC);
                         return(FALSE);
                    }

                    /*--------------------------*\
                    | Output the header and tests|
                    | if the appropriate flags   |
                    | are set.                   |
                    \*--------------------------*/
                    if((wHeaderSet & PH_PRINTHEADER) && !bAbort)
                         PrintHeader(hWnd,hDC);
                    if((wTestsSet & PT_TEXT) && !bAbort)
                          PrintText(hWnd,hDC,&dcDevCaps,hBrushes,hPens,hFonts,&tlTest,(LPSTR)&bAbort);
                    if((wTestsSet & PT_BITMAPS) && !bAbort)
                          PrintBitmaps(hWnd,hDC,&dcDevCaps,hBrushes,hPens,hFonts,&tlTest,(LPSTR)&bAbort);
                    if((wTestsSet & PT_POLYGONS) && !bAbort)
                          PrintPolygons(hWnd,hDC,&dcDevCaps,hBrushes,hPens,hFonts,&tlTest,(LPSTR)&bAbort);
                    if((wTestsSet & PT_CURVES) && !bAbort)
                          PrintCurves(hWnd,hDC,&dcDevCaps,hBrushes,hPens,hFonts,&tlTest,(LPSTR)&bAbort);
                    if((wTestsSet & PT_LINES) && !bAbort)
                          PrintLines(hWnd,hDC,&dcDevCaps,hBrushes,hPens,hFonts,&tlTest,(LPSTR)&bAbort);

                    /*--------------------------*\
                    | END Print job.  Enable the |
                    | main window, then delete   |
                    | printer DC.                |
                    \*--------------------------*/
                    Escape(hDC,ENDDOC,0,NULL,NULL);
                    if(!bAbort)
                    {
                         EnableWindow(hWnd,TRUE);
                         DestroyWindow(hAbortDlg);
                    }
                    FreeProcInstance(lpAbortProc);
                    FreeProcInstance(lpAbortDlg);
                    DeleteDC(hDC);

                    /*--------------------------*\
                    | Free up memory occupied by |
                    | structues.                 |
                    \*--------------------------*/
                    FreeDeviceObjects(hBrushes);
                    FreeDeviceObjects(hPens);
                    FreeDeviceObjects(hFonts);
               }

               /*-------------------------------*\
               | End of run.                     |
               \*-------------------------------*/
               SetFocus(hPrntDlg);
               if(bAutoRun)
                    ExitWindows((DWORD)NULL,0);
               break;

          /*------------------------------------*\
          | Get header settings.                 |
          \*------------------------------------*/
          case IDM_SETTINGS_HEADER:
               if(!(lpProc = MakeProcInstance(SetupHeaderDlg,hInst)))
                    return(FALSE);
               DialogBox(hInst,"SETHEADER",hWnd,lpProc);
               FreeProcInstance(lpProc);
               SetFocus(hPrntDlg);
               break;

          /*------------------------------------*\
          | Get tests settings.                  |
          \*------------------------------------*/
          case IDM_SETTINGS_TESTS:
               if(!(lpProc = MakeProcInstance(SetupTestsDlg,hInst)))
                    return(FALSE);
               DialogBox(hInst,"SETTESTS",hWnd,lpProc);
               FreeProcInstance(lpProc);
               SetFocus(hPrntDlg);
               break;

          /*------------------------------------*\
          | Display about dialog box.            |
          \*------------------------------------*/
          case IDM_HELP_ABOUT:
               if(!(lpProc = MakeProcInstance((FARPROC)AboutDlg,hInst)))
                    return(FALSE);
               DialogBox(hInst,"ABOUTDLG",hWnd,lpProc);
               FreeProcInstance(lpProc);
               SetFocus(hPrntDlg);
               break;

          /*------------------------------------*\
          | No command found.                    |
          \*------------------------------------*/
          default:
               return(FALSE);
     }

     return(TRUE);
}
