/*---------------------------------------------------------------------------*\
| PAINT CLIENT AREA WINDOW                                                    |
|   This module contains the routines necessary to handle the WM_PAINT        |
|   message for the application.  It paints the client area.                  |
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

/*---------------------------------------------------------------------------*\
| PAINT THE CLIENT AREA FOR APPLICATION                                       |
|   This routine Paints the client area for the application.                  |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hWnd - Handle to the application client window.                      |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL : True indicates the message was processed, otherwise a FALSE        |
|          is returned.                                                       |
\*---------------------------------------------------------------------------*/
BOOL FAR PaintDispTestWindow(hWnd)
     HWND hWnd;
{
     PAINTSTRUCT ps;
     HDC         hDC;

     if(!(hDC = BeginPaint(hWnd,&ps)))
          return(FALSE);

     EndPaint(hWnd,&ps);

     return(TRUE);
}
