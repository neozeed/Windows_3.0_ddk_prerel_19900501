/*---------------------------------------------------------------------------*\
| WINDOWS PRINTER TEST APPLICATION                                            |
|   This is an application to test Windows Printer Drivers.  It is not meant  |
|   to test GDI and it's ability to interact with printer drivers.  It is     |
|   intended to touch on the functionality of the driver itself.              |
|                                                                             |
|   This program is divided into several Code-segments.  The main benefit of  |
|   doing this is to maximize Kernel's Memory Management capabilities.  By    |
|   using several small segments, the possiblity of the system running out of |
|   memory is small.  The _INIT and _GETINFO, and _HEADER segments should be  |
|   loaded only once, since their routines are used only once.  Once the      |
|   Test-segments (CLIPPING, RASTER..) are completed they can be discarded to |
|   free up memory.                                                           |
|                                                                             |
|     _INIT     Window Registration/Creation routines.                        |
|     _TEXT     Main Program, WM_* processing functions.                      |
|     _INFO     Routines form retrieving device information.                  |
|     _HEADER   Routines for outputing header information.                    |
|     _CLIPPING Routines concerning CLIPPING tests.                           |
|     _RASTER   Routines concerning RASTER (bitmap) tests.                    |
|     _CURVES   Routines concerning CURVE tests.                              |
|     _LINE     Routines concerning LINE tests.                               |
|     _POLYGON  Routines concerning POLYGON tests.                            |
|     _TEXTX    Routines concerning TEXT tests.                               |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"
#include "Global.h"

/*---------------------------------------------------------------------------*\
| APPLICATION MAIN ENTRY POINT                                                |
|   This routine Registers and Creates the main application window(s) for     |
|   use during the task's existence.  After which, the application polls the  |
|   the system queue for messages to be dispatched.                           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   RegisterPrntTestClass() - (init.c)                                        |
|   CreatePrntTestWindow()  - (init.c)                                        |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE hInstance     - Indicates the Task instance of this app.           |
|   HANDLE hPrevInstance - Indicates the previous Task instance of app.       |
|   LPSTR  lpszCmdLine   - Parameters passed to application.                  |
|   int    nCmdShow      - How to display application.                        |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst    - Initially set to instance.                              |
|   BOOL   bAutoRun - Set from private profile.                               |
|   BOOL   hPrntDlg - Handle to interface control window.                     |
|                                                                             |
| RETURNS                                                                     |
|   int - Passes back the wParam of the message structure.                    |
\*---------------------------------------------------------------------------*/
int PASCAL WinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
     HANDLE hInstance;
     HANDLE hPrevInstance;
     LPSTR  lpszCmdLine;
     int    nCmdShow;
{
     extern HANDLE hInst;
     extern BOOL   bAutoRun;
     extern HANDLE hPrntDlg;

     HWND     hWnd;                              /* Handle to Main Window    */
     MSG      msg;                               /* Window Message Structure */

     hInst = hInstance;

     /*-----------------------------------------*\
     | Register Classes, then create the Windows.|
     | If there's an error, the quit application.|
     \*-----------------------------------------*/
     if(!hPrevInstance)
          if(!RegisterPrntTestClass(hInst))
               return(NULL);
     if(!(hWnd = CreatePrntTestWindow(hInst)))
          return(NULL);

     /*-----------------------------------------*\
     | Update the Window Client area before      |
     | entering the main loop.                   |
     \*-----------------------------------------*/
     ShowWindow(hWnd,nCmdShow);
     UpdateWindow(hWnd);

     /*-----------------------------------------*\
     | If autorun, then start executing tests.   |
     \*-----------------------------------------*/
     if(bAutoRun)
          PostMessage(hWnd,WM_COMMAND,IDM_TEST_RUN,0l);

     /*-----------------------------------------*\
     | MAIN MESSAGE PROCESSING LOOP.             |
     \*-----------------------------------------*/
     while(GetMessage(&msg,NULL,0,0))
     {
          if(!hPrntDlg || !IsDialogMessage(hPrntDlg,&msg))
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
|   handling/Processing/Filtering for the application.                        |
|                                                                             |
| CALLED ROUTINES                                                             |
|   ProcessPrntTestCommands() - (command.c)                                   |
|   PaintPrntTestWindow()     - (paint.c)                                     |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND     hWnd     - The Window Handle.                                    |
|   unsigned iMessage - Message to be processed.                              |
|   WORD     wParam   - Information associated with message.                  |
|   LONG     lParam   - Information associated with message.                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   LONG - Returns a long integer to Windows.  If this routine can't          |
|          handle the message, then it passes it to Windows Default           |
|          Window Procedure (DefWindowProc).                                  |
\*---------------------------------------------------------------------------*/
LONG FAR PASCAL PrntTestProc(hWnd, iMessage, wParam, lParam)
     HWND     hWnd;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern HANDLE hInst;
     extern WORD   wHeaderSet,wTestsSet;
     extern BOOL   bAutoRun;

     FARPROC lpProc;
     RECT    rRect;

     switch(iMessage)
     {
          /*------------------------------------*\
          | Handle any variable which need to be |
          | initialized upon creation of the app.|
          \*------------------------------------*/
          case WM_CREATE:
               lpProc   = MakeProcInstance(PrntTestDlg,hInst);
               hPrntDlg = CreateDialog(hInst,"INTERFACE",hWnd,lpProc);

               /*-------------------------------*\
               | Retrieve the settings from the  |
               | Initialization File.            |
               \*-------------------------------*/
               wHeaderSet = (WORD)GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"HeaderTest",0,(LPSTR)"prnttest.ini");
               wTestsSet = (WORD)GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"TestsTest",0,(LPSTR)"prnttest.ini");
               bAutoRun = (BOOL)GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"AutoRun",0,(LPSTR)"prnttest.ini");

               /*-------------------------------*\
               | If the initialization file does |
               | not have the settings, then you |
               | can assume ALL possible comb.   |
               \*-------------------------------*/
               if(!wHeaderSet)
                    wHeaderSet = PH_PRINTHEADER  | PH_CONDENSED |
                                 PH_CAPABILITIES | PH_FONTS     |
                                 PH_BRUSHES      | PH_PENS;
               if(!wTestsSet)
                    wTestsSet = PT_TEXT   | PT_BITMAPS | PT_POLYGONS |
                                PT_CURVES | PT_LINES;
               SetFocus(hPrntDlg);
               break;


          /*------------------------------------*\
          | Handle the processing of commands in |
          | which user selects through the menu. |
          \*------------------------------------*/
          case WM_COMMAND:
               ProcessPrntTestCommands(hWnd,wParam);
               break;

          /*------------------------------------*\
          | Handle sizing of window. (static)    |
          \*------------------------------------*/
          case WM_SIZE:
               GetClientRect(hPrntDlg,&rRect);
               AdjustWindowRect(&rRect,WS_OVERLAPPEDWINDOW,TRUE);
               SetWindowPos(hWnd,NULL,0,0,
                    rRect.right-rRect.left,rRect.bottom-rRect.top,SWP_NOMOVE |
                    SWP_NOZORDER);
               SetFocus(hPrntDlg);
               break;

          /*------------------------------------*\
          | Paint the client area window.        |
          \*------------------------------------*/
          case WM_PAINT:
               PaintPrntTestWindow(hWnd);
               SetFocus(hPrntDlg);
               break;

          /*------------------------------------*\
          | Time to die... The is sent from the  |
          | DestroyWindow() function.  Clean up  |
          | any windows/objects used in app.     |
          \*------------------------------------*/
          case WM_DESTROY:
               DestroyWindow(hPrntDlg);
               PostQuitMessage(0);
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