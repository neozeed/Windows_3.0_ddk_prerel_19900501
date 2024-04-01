/*---------------------------------------------------------------------------*\
| PRNTTEST INITIALIZATION ROUTINES                                            |
|   This module contains routines specific only to boot-up of the application.|
|   After Windows has Registered and created everything, it will discard the  |
|   segment.  It shouldn't have to be called back until another instance is   |
|   loaded.                                                                   |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT _INIT                                                               |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| REGISTER WINDOW CLASS                                                       |
|   This routine registers the main window class for the application.         |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE hInstance - Window Task Instance.                                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - Returns TRUE if the window was registered.                         |
\*---------------------------------------------------------------------------*/
BOOL FAR RegisterPrntTestClass(hInstance)
     HANDLE hInstance;
{
     WNDCLASS wndClass;

     wndClass.style         = CS_HREDRAW | CS_VREDRAW;
     wndClass.lpfnWndProc   = PrntTestProc;
     wndClass.cbClsExtra    = 0;
     wndClass.cbWndExtra    = 0;
     wndClass.hInstance     = hInstance;
     wndClass.hIcon         = LoadIcon(hInstance,PRNTTESTICON);
     wndClass.hCursor       = LoadCursor(NULL,IDC_ARROW);
     wndClass.hbrBackground = GetStockObject(WHITE_BRUSH);
     wndClass.lpszMenuName  = PRNTTESTMENU;
     wndClass.lpszClassName = PRNTTESTCLASS;

     return(RegisterClass(&wndClass));
}


/*---------------------------------------------------------------------------*\
| CREATE APPLICATION MAIN WINDOW                                              |
|   This routine creates the main client window for the application.          |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE hInstance - Window Task Instance.                                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   HWND - Handle to the newly created window.                                |
\*---------------------------------------------------------------------------*/
HWND FAR CreatePrntTestWindow(hInstance)
     HANDLE hInstance;
{
     return(CreateWindow(PRNTTESTCLASS,          /* Window Class             */
                         PRNTTESTTITLE,          /* Window Title Caption     */
                         WS_OVERLAPPEDWINDOW,    /* Style of window          */
                         CW_USEDEFAULT,          /* Start x                  */
                         CW_USEDEFAULT,          /* Start y                  */
                         CW_USEDEFAULT,          /* Change x                 */
                         CW_USEDEFAULT,          /* Change y                 */
                         NULL,                   /* No Parent Window (main)  */
                         NULL,                   /* No Menu Handle           */
                         hInstance,              /* Task Instance            */
                         NULL));                 /* No Extra Parameters      */
}
