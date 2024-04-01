/*---------------------------------------------------------------------------*\
| PROCESS DISPTEST COMMANDS                                                   |
|   This module contains the routine(s) necessary to process the message      |
|   WM_COMMAND for windows.                                                   |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - createed.                                           |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "DispTest.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PROCESS APPLICATION COMMANDS                                                |
|   This routine handles all WM_COMMAND messages sent by Windows to this      |
|   application.  The message ID is passed in the wParam variable.            |
|                                                                             |
| CALLED ROUTINES                                                             |
|   AboutDlg()      -                                                         |
|   SetupTestsDlg() -                                                         |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hWnd   - Handle to client window.                                    |
|   WORD wParam - Message to be processed.                                    |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL            bAutoRun  - AutoRun flag.                                 |
|   HANDLE          hInst     - Module handle for application.                |
|   ENUMERATE       eBrushes  - Brushes structure array.                      |
|   ENUMERATE       ePens     - Pens structure array.                         |
|   ENUMERATE       eFonts    - Fonts structure array.                        |
|   DEVCAPABILITIES dcDevCaps - Device capabilities array.                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if message was processed.                                     |
\*---------------------------------------------------------------------------*/
BOOL FAR ProcessDispTestCommands(hWnd,wParam)
     HWND hWnd;
     WORD wParam;
{
     extern int             nTimerSpeed;
     extern char            szLogFile[];
     extern HWND            hIntrDlg;
     extern HANDLE          hInst;

     FARPROC lpProc;

     switch(wParam)
     {
          case IDM_TEST_RUN:
               /*-------------------------------*\
               | Disable/Enable menu items.      |
               \*-------------------------------*/
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_STOP,MF_BYCOMMAND | MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_RUN,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_RESET,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd),IDM_SETTINGS_OBJECTS,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               DrawMenuBar(hWnd);

               /*-------------------------------*\
               | Get the device information.     |
               \*-------------------------------*/
               if(!(lpProc = MakeProcInstance((FARPROC)SetupObjectsDlg,hInst)))
                    return(FALSE);
               DialogBox(hInst,"SETOBJECT",hWnd,lpProc);
               FreeProcInstance(lpProc);

               /*-------------------------------*\
               | Go into TEST LOOP.              |
               \*-------------------------------*/
               PostMessage(hWnd,WM_USER,0,0l);
               GetDlgItemText(hIntrDlg,IDD_INTR_LOGE,(LPSTR)szLogFile,LOGFILE_SIZE);
               nTimerSpeed = GetDlgItemInt(hIntrDlg,IDD_INTR_SEC1,NULL,FALSE);
               OutputTestLog(szLogFile,NULL,LOGFILE_LEVEL0,NULL);
               SetTimer(hWnd,1,nTimerSpeed,NULL);
               break;

          case IDM_TEST_STOP:
               /*-------------------------------*\
               | Stop Test.  Reset everything.   |
               \*-------------------------------*/
               KillTimer(hWnd,1);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_RUN,MF_BYCOMMAND | MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd),IDM_SETTINGS_OBJECTS,MF_BYCOMMAND | MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_RESET,MF_BYCOMMAND | MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_STOP,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               DrawMenuBar(hWnd);
               break;

          case IDM_TEST_RESET:
               lpProc = MakeProcInstance((FARPROC)DispTestProc,hInst);
               SetWindowLong(hWnd,GWL_WNDPROC,(LONG)lpProc);
               SetWindowText(hWnd,(LPSTR)DISPTESTTITLE);
               InvalidateRect(hWnd,NULL,TRUE);
               PostMessage(hWnd,WM_SIZE,NULL,0l);
               PostMessage(hWnd,WM_USER,-1,0l);
               break;

          /*------------------------------------*\
          | Display about dialog box.            |
          \*------------------------------------*/
          case IDM_HELP_ABOUT:
               if(!(lpProc = MakeProcInstance((FARPROC)AboutDlg,hInst)))
                    return(FALSE);
               DialogBox(hInst,"ABOUTDLG",hWnd,lpProc);
               FreeProcInstance(lpProc);
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