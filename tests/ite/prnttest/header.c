/*---------------------------------------------------------------------------*\
| PRINT HEADER                                                                |
|   This module contains the routines necessary to print out the header       |
|   information.                                                              |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _HEADER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| PRINT HEADER INFORMATION                                                    |
|   This routine retreives the Device capabilities of a printer and then      |
|   outputs the information to the printer.                                   |
|                                                                             |
| CALLED ROUTINES                                                             |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hWnd - Handle to application client window.                          |
|   HDC  hDC  - Handle to printer device context.                             |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - True if successful                                                 |
\*---------------------------------------------------------------------------*/
BOOL FAR PrintHeader(hWnd,hDC)
     HWND hWnd;
     HDC  hDC;
{
     extern WORD       wHeaderSet;
     extern DEVINFO    dcDevCaps;
     extern HDEVOBJECT hFonts,hBrushes,hPens;
     extern PRINTER    pPrinter;
     extern BOOL       bAbort;

     int    iErrorCount;

     iErrorCount=0;
     /*-----------------------------------------*\
     | Print the Title page of header.           |
     \*-----------------------------------------*/
     if(!PrintTitlePage(hDC,&pPrinter,&dcDevCaps,(LPSTR)&bAbort))
          iErrorCount++;

     /*-----------------------------------------*\
     | Print the Title page of header.           |
     \*-----------------------------------------*/
     if(!PrintFunctionSupport(hDC,&pPrinter,&dcDevCaps,(LPSTR)&bAbort))
          iErrorCount++;

     /*-----------------------------------------*\
     | Print the gray scale for the device.      |
     \*-----------------------------------------*/
     if(!PrintGrayScale(hDC,&dcDevCaps,(LPSTR)&bAbort))
          iErrorCount++;

     /*-----------------------------------------*\
     | Print a border around the devices physical|
     | page.                                     |
     \*-----------------------------------------*/
     if(!PrintPrintableArea(hDC,&dcDevCaps,(LPSTR)&bAbort))
          iErrorCount++;

     /*-----------------------------------------*\
     | These items depend on the wHeaderSet flag.|
     | Check the bit set on the WORD.            |
     \*-----------------------------------------*/
     if((wHeaderSet & PH_CAPABILITIES) && !bAbort)
          if(!PrintDeviceCapabilities(hDC,&dcDevCaps,(LPSTR)&bAbort))
               iErrorCount++;
     if((wHeaderSet & PH_FONTS) && !bAbort)
          if(!PrintDeviceFonts(hDC,hFonts,&dcDevCaps,(LPSTR)&bAbort))
               iErrorCount++;
     if((wHeaderSet & PH_BRUSHES) && !bAbort)
          if(!PrintDeviceBrushes(hDC,hBrushes,&dcDevCaps,(LPSTR)&bAbort))
               iErrorCount++;
     if((wHeaderSet & PH_PENS) && !bAbort)
          if(!PrintDevicePens(hDC,hPens,&dcDevCaps,(LPSTR)&bAbort))
               iErrorCount++;

     if(iErrorCount)
          return(FALSE);
     return(TRUE);
}
