/*---------------------------------------------------------------------------*\
| PAINT CLIENT AREA WINDOW                                                    |
|   This module contains the routines necessary to handle the WM_PAINT        |
|   message for the application.  It paints the client area.                  |
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
|   BOOL - True indicates the message was processed, otherwise a FALSE        |
|          is returned.                                                       |
\*---------------------------------------------------------------------------*/
BOOL PaintPrntTestWindow(hWnd)
     HWND hWnd;
{
     PAINTSTRUCT ps;
     HDC         hDC;

     /*-----------------------------------------*\
     | Start painting.                           |
     \*-----------------------------------------*/

     if(!(hDC = BeginPaint(hWnd,&ps)))
          return(FALSE);

     /*-----------------------------------------*\
     | End painting.                             |
     \*-----------------------------------------*/
     EndPaint(hWnd,&ps);

     return(TRUE);
}
