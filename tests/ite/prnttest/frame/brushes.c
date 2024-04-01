/*---------------------------------------------------------------------------*\
| BRUSHES                                                                     U
|   This module contains the routines necessary to handle the GDI brush       |
|   objects for the device.                                                   |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _HEADER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT DEVICE BRUSHES TO PRINTER (Expanded version)                          |
|   This routine prints out the expanded information for every brush the      |
|   device supports.                                                          |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintTestDescription() - (misc.c)                                         |
|   PrintFooter()          - (misc.c)                                         |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPENUMERATE       lpPens    - Array of logical pen structures.            |
|   LPDEVCAPS         lpDevCaps - Device information structure.               |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything was OK.                                         |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintDeviceBrushes(hDC,lpBrushes,lpDevCaps,bAbort)
     HDC         hDC;
     HDEVOBJECT lpBrushes;
     LPDEVINFO   lpDevCaps;
     LPSTR             bAbort;
{
     extern BOOL FAR* bPrintAbort;

     short      nIdy,x,y,nWidth,nHeight,nTxtWidth,nTxtHeight,nPageHeight;
     HBRUSH     hBrush,hOldBrush;
     TEXTMETRIC tm;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;

     bPrintAbort = (BOOL FAR*)bAbort;

     /*-----------------------------------------*\
     | Set the x and y dimensions of the rect    |
     | to be 1 inch by 1 inch.  This then looks  |
     | like a square on the device.              |
     \*-----------------------------------------*/
     nWidth  = lpDevCaps->nLogPixelsX;
     nHeight = lpDevCaps->nLogPixelsY;

     /*-----------------------------------------*\
     | Set the text height (for brush mode and   |
     | type description).  Also, adjust the page |
     | height to accomodate the footer.          |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     nTxtWidth   = tm.tmAveCharWidth;
     nTxtHeight  = tm.tmHeight+tm.tmExternalLeading;
     nPageHeight = lpDevCaps->nVertRes-tm.tmHeight;

     /*-----------------------------------------*\
     | Draw the brushes.                         |
     \*-----------------------------------------*/
     x=0;
     y=0;

     for(nIdy=0; nIdy < GetObjectCount(lpBrushes); nIdy++)
     {
         SetCurrentObject(lpBrushes,nIdy);
         if(!(hBrush = CreateDeviceObject(lpBrushes)))
              return(FALSE);
         if(!(hOldBrush = SelectObject(hDC,hBrush)))
         {
              DeleteObject(hBrush);
              return(FALSE);
         }

         if((x+nWidth) > lpDevCaps->nHorzRes)
         {
              x=0;
              y+=(lpDevCaps->nLogPixelsY+nTxtHeight);

              if((y+nHeight+nTxtHeight) > nPageHeight)
              {
                   x=0;
                   y=0;
                   if(!PrintFooter(hDC,lpDevCaps,"Header"))
                   {
                        DeleteObject(SelectObject(hDC,hOldBrush));
                        return(FALSE);
                   }
              }
         }

         Rectangle(hDC,x,y+nTxtHeight,x+nWidth,y+nTxtHeight+nHeight);
         DeleteObject(SelectObject(hDC,hOldBrush));

         x+=nWidth+(nWidth/4);

     }

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}
