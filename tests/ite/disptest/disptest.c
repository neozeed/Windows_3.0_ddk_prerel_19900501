/*--------------------------------------------------------------------------*\
| WINDOWS DISPLAY TEST APPLICATION                                            |
|   This module is the main entry code for the Display Test application.  It  |
|   mostly contains the DispTestProc() for handling the system messages.      |
|   This application makes use of a different type of procedure handling.  In |
|   order to allow processing during the execution of the tests, a timer is   |
|   used to branch to the test procedures.  The test procedures are given     |
|   message control from the main procedure by setting the Window Proc.       |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - created.                                            |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "DispTest.h"                            /* Program Header File      */
#include "Global.h"

/*---------------------------------------------------------------------------*\
| APPLICATION MAIN ENTRY POINT                                                |
|   This routine Registers and Creates the main application window(s) for     |
|   use during the task's existence.  After which, the application polls the  |
|   the system queue for messages to be dispatched.                           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   RegisterDispTestClass() - (init.c)                                        |
|   CreateDispTestWindow()  - (init.c)                                        |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE hInstance     - Indicates the Task instance of this app.           |
|   HANDLE hPrevInstance - Indicates the previous Task instance of app.       |
|   LPSTR  lpszCmdLine   - Parameters passed to application.                  |
|   int    nCmdShow      - How to display application.                        |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst    - Module instance handle initially set.                   |
|   BOOL   bAutoRun - Auto Run flag to execute autmatically.                  |
|   HWND   hIntrDlg - Window handle to interface dialog.                      |
|                                                                             |
| RETURNS                                                                     |
|   int - Passes back the wParam of the message structure.                    |
\*---------------------------------------------------------------------------*/
int PASCAL WinMain(hInstance,hPrevInstance,lpszCmdLine,nCmdShow)
     HANDLE hInstance;
     HANDLE hPrevInstance;
     LPSTR  lpszCmdLine;
     int    nCmdShow;
{
     extern HANDLE  hInst;
     extern HWND    hIntrDlg;
     extern FARPROC lpIntrDlg;
     extern TEST    tlTest;

     HWND     hWnd;
     MSG      msg;

     hInst = hInstance;

     /*-----------------------------------------*\
     | Register Classes, then create the Windows.|
     | If there's an error, the quit application.|
     \*-----------------------------------------*/
     if(!hPrevInstance)
          if(!RegisterDispTestClass(hInst))
               return(NULL);
     if(!(hWnd = CreateDispTestWindow(hInst)))
          return(NULL);

     lpIntrDlg = MakeProcInstance(InterfaceDlg,hInst);
     hIntrDlg  = CreateDialog(hInst,"INTERFACE",hWnd,lpIntrDlg);

     /*-----------------------------------------*\
     | Update the Window Client area before      |
     | entering the main loop.                   |
     \*-----------------------------------------*/
     ShowWindow(hWnd,nCmdShow);
     UpdateWindow(hWnd);
     SetFocus(hIntrDlg);

     /*-----------------------------------------*\
     | If autorun, then start executing tests.   |
     \*-----------------------------------------*/
     if(tlTest.wStatus & STAT_AUTORUN)
          PostMessage(hWnd,WM_COMMAND,IDM_TEST_RUN,0l);

     /*-----------------------------------------*\
     | MAIN MESSAGE PROCESSING LOOP.             |
     \*-----------------------------------------*/
     while(GetMessage(&msg,NULL,0,0))
     {
          if(!hIntrDlg || !IsDialogMessage(hIntrDlg,&msg))
          {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
          }
     }

     return(msg.wParam);
}


/*---------------------------------------------------------------------------*\
| MAIN WINDOW PROCEDURE                                                       |
|   This is the main Window-Function routine.  It handles the message         |
|   handling/Processing/Filtering for the application.  It changes the        |
|   direction of messages by setting the Window Proc.  Each Window Proc is    |
|   a different Test Area.                                                    |
|                                                                             |
| CALLED ROUTINES                                                             |
|   InterfaceDlg() - (Intrface.c)                                             |
|   DispBitmProc() - (DispBitm.c)                                             |
|   DispCurvProc() - (DispCurv.c)                                             |
|   DispLineProc() - (DispLine.c)                                             |
|   DispPolyProc() - (DispPoly.c)                                             |
|   DispTextProc() - (DispText.c)                                             |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND     hWnd     - The Window Handle.                                    |
|   unsigned iMessage - Message to be processed.                              |
|   WORD     wParam   - Information associated with message.                  |
|   LONG     lParam   - Information associated with message.                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst     - Module instance handle.                                |
|   BOOL   bAutoRun  - AutoRun flag.                                          |
|   WORD   wTestsSet - Test settings flag.                                    |
|   HWND   hIntrDlg  - Handle to interface dialog window.                     |
|                                                                             |
| RETURNS                                                                     |
|   LONG - Returns a long integer to Windows.  If this routine can't          |
|          handle the message, then it passes it to Windows Default           |
|          Window Procedure (DefWindowProc).                                  |
\*---------------------------------------------------------------------------*/
LONG FAR PASCAL DispTestProc(hWnd,iMessage,wParam,lParam)
     HWND     hWnd;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern HANDLE      hInst;
     extern WORD        wTestsSet;
     extern HWND        hIntrDlg;
     extern char        szLogFile[];
     extern TEST        tlTest;
     extern HDEVOBJECT  hDevFonts,hDevBrushes,hDevPens;
     extern DEVINFO     dcDevCaps;
     extern TEST        tlTest;

     static TESTFUNCTION tfTestFunct[] = {(FARPROC)DispBitmProc,"Bitmap/Raster Tests",
                                          (FARPROC)DispCurvProc,"Curve Tests",
                                          (FARPROC)DispLineProc,"Line Tests",
                                          (FARPROC)DispPolyProc,"Polygon Tests",
                                          (FARPROC)DispTextProc,"Text/Font Tests"};
     FARPROC     lpProc;
     RECT        rRect;
     HDC         hDC;

     switch(iMessage)
     {
          /*------------------------------------*\
          | Handle any variable which need to be |
          | initialized upon creation of the app.|
          \*------------------------------------*/
          case WM_CREATE:
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_STOP,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd),IDM_TEST_RESET,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
               DrawMenuBar(hWnd);
               SetFocus(hIntrDlg);

               tlTest.wStatus |= (BOOL)GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"AutoRun",0,(LPSTR)"prnttest.ini");
               if(tlTest.wStatus & STAT_AUTORUN)
                    wTestsSet = DT_RAST;

               hDC = GetDC(hWnd);
               GetDeviceInfo(hDC,&dcDevCaps);
               hDevFonts = GetDeviceObjects(hDC,DEV_FONT);
               hDevPens = GetDeviceObjects(hDC,DEV_PEN);
               hDevBrushes = GetDeviceObjects(hDC,DEV_BRUSH);
               ReleaseDC(hWnd,hDC);
               break;

          /*------------------------------------*\
          | Explicitly size the box to surround  |
          | the dialogbox.                       |
          \*------------------------------------*/
          case WM_SIZE:
               GetClientRect(hIntrDlg,&rRect);
               AdjustWindowRect(&rRect,WS_OVERLAPPEDWINDOW,TRUE);
               SetWindowPos(hWnd,NULL,0,0,
                    rRect.right-rRect.left,rRect.bottom-rRect.top,SWP_NOMOVE |
                    SWP_NOZORDER);
               SetFocus(hIntrDlg);
               break;

          /*------------------------------------*\
          | Handle the processing of commands in |
          | which user selects through the menu. |
          \*------------------------------------*/
          case WM_COMMAND:
               ProcessDispTestCommands(hWnd,wParam);
               break;

          /*------------------------------------*\
          | Paint the client area window.        |
          \*------------------------------------*/
          case WM_PAINT:
               PaintDispTestWindow(hWnd);
               break;

          /*------------------------------------*\
          | Time to die... The is sent from the  |
          | DestroyWindow() function.  Clean up  |
          | any windows/objects used in app.     |
          \*------------------------------------*/
          case WM_DESTROY:
               DestroyDispWindow(hWnd);
               PostQuitMessage(0);
               break;

          /*------------------------------------*\
          | Special block.  Used to control the  |
          | execution of the tests.  This is     |
          | entered originally by the WM_COMMAND |
          | block, and exits once the tests are  |
          | completed.  wParam == test number.   |
          \*------------------------------------*/
          case WM_USER:
               switch(wParam)
               {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                         if(!IsDlgButtonChecked(hIntrDlg,wParam+IDD_INTR_RAST))
                         {
                              PostMessage(hWnd,WM_USER,wParam+1,0l);
                              return(0l);
                         }
                         ShowWindow(hIntrDlg,SW_HIDE);
                         lpProc = MakeProcInstance(tfTestFunct[wParam].lpProc,hInst);
                         SetWindowLong(hWnd,GWL_WNDPROC,(LONG)lpProc);
                         SetWindowText(hWnd,tfTestFunct[wParam].szText);
                         SendMessage(hWnd,WM_CREATE,0,0l);
                         InvalidateRect(hWnd,NULL,TRUE);
                         break;

                    default:
                         WriteLogFile(szLogFile,(LPSTR)"DISPLAY TEST COMPLETED");
                         KillTimer(hWnd,1);
                         ShowWindow(hIntrDlg,SW_SHOWNORMAL);
                         EnableMenuItem(GetMenu(hWnd),IDM_TEST_RUN,MF_BYCOMMAND | MF_ENABLED);
                         EnableMenuItem(GetMenu(hWnd),IDM_SETTINGS_OBJECTS,MF_BYCOMMAND | MF_ENABLED);
                         EnableMenuItem(GetMenu(hWnd),IDM_TEST_STOP,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                         EnableMenuItem(GetMenu(hWnd),IDM_TEST_RESET,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                         DrawMenuBar(hWnd);
                         SetFocus(hIntrDlg);
                         InvalidateRect(hWnd,NULL,TRUE);
                         if(tlTest.wStatus & STAT_AUTORUN)
                              ExitWindows((DWORD)NULL,0);
                         break;
               }
               break;

          /*------------------------------------*\
          | Let Windows handle the Message.      |
          \*------------------------------------*/
          default:
               return(DefWindowProc(hWnd,iMessage,wParam,lParam));
     }

     return(0L);
}
