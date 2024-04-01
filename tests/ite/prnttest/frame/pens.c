/*---------------------------------------------------------------------------*\
| PENS                                                                        |
|   This module contains the routines necessary to handle the device pens     |
|   for the device.                                                           |
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
| PRINT DEVICE PENS TO PRINTER                                                |
|   This routine prints out the device pen information.                       |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPENUMERATE       lpPens    - Array of logical pen structures.            |
|   LPDEVCAPBAILITIES lpDevCaps - Device information structure.               |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything was OK.                                         |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintDevicePens(hDC,lpPens,lpDevCaps,bAbort)
     HDC         hDC;
     HDEVOBJECT lpPens;
     LPDEVINFO   lpDevCaps;
     LPSTR       bAbort;
{
     extern BOOL FAR* bPrintAbort;

     short      nIdy,x,y,nWidth,nHeight,nTxtWidth,nTxtHeight,nPageHeight;
     HPEN       hPen,hOldPen;
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
     | Set the text height (for pen mode and     |
     | type description).  Also, adjust the page |
     | height to accomodate the footer.          |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     nTxtWidth   = tm.tmAveCharWidth;
     nTxtHeight  = tm.tmHeight+tm.tmExternalLeading;
     nPageHeight = lpDevCaps->nVertRes-tm.tmHeight;

     /*-----------------------------------------*\
     | Draw the pen objects.                     |
     \*-----------------------------------------*/
     x=0;
     y=0;

     for(nIdy=0; nIdy < GetObjectCount(lpPens); nIdy++)
     {
         SetCurrentObject(lpPens,nIdy);
         if(!(hPen = CreateDeviceObject(lpPens)))
              return(FALSE);
         if(!(hOldPen = SelectObject(hDC,hPen)))
         {
              DeleteObject(hPen);
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
                        DeleteObject(SelectObject(hDC,hOldPen));
                        return(FALSE);
                   }
              }
         }

         Rectangle(hDC,x,y+nTxtHeight,x+nWidth,y+nTxtHeight+nHeight);
         MoveTo(hDC,x,y+nTxtHeight);
         LineTo(hDC,x+nWidth,y+nTxtHeight+nHeight);
         MoveTo(hDC,x+nWidth,y+nTxtHeight);
         LineTo(hDC,x,y+nTxtHeight+nHeight);
         DeleteObject(SelectObject(hDC,hOldPen));

         x+=nWidth+(nWidth/4);

     }

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}
