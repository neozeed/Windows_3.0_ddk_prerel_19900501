/*---------------------------------------------------------------------------*\
| CURVE TESTS                                                                 |
|   This module contains routines specific to testing the curve capabilities  |
|   of a printer driver.                                                      |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _CURVE                                                             |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 21, 1989 - fix to place multiple ellipses per page.            |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT CURVE TEST                                                            |
|   This routine peforms the curve printing test for the printer driver.      |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter()          - (misc.c)                                         |
|   PrintTestDescription() - (misc.c)                                         |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND              hWnd      - Handle to parent window.                    |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpBrushes - Array of logical brush structures.          |
|   LPENUMERATE       lpPens    - Array of logical pen structures.            |
|   LPENUMERATE       lpFonts   - Array of logical font structures.           |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if passed.                                                    |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintCurves(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpTest,bAbort)
     HWND        hWnd;
     HDC         hDC;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpBrushes;
     HDEVOBJECT lpPens;
     HDEVOBJECT lpFonts;
     LPTEST      lpTest;
     LPSTR       bAbort;
{
     extern BOOL FAR* bPrintAbort;

     bPrintAbort = (BOOL FAR*)bAbort;

     PrintTestDescription(hDC,IDS_TST_DSCR_CURV,lpDevCaps);
     if(!PrintFooter(hDC,lpDevCaps,"Curves Test"))
          return(FALSE);

     /*-----------------------------------------*\
     | TEST SUITE --> (append tests to list)     |
     \*-----------------------------------------*/
     PrintEllipses(hDC,lpDevCaps,lpBrushes,lpPens,OPAQUE,lpTest);
     PrintEllipses(hDC,lpDevCaps,lpBrushes,lpPens,TRANSPARENT,lpTest);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT ELLIPSES TEST                                                         |
|   This routine prints ellipses to the device using the currently selected   |
|   pen and brush.                                                            |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to printer device context.           |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpBrushes - Array of logical brush structures.          |
|   LPENUMERATE       lpPens    - Array of logical pen structues.             |
|   int               nMode     - Background mode.                            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if passed.                                                    |
\*---------------------------------------------------------------------------*/
BOOL PrintEllipses(hDC,lpDevCaps,lpBrushes,lpPens,nMode,lpTest)
     HDC         hDC;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpBrushes;
     HDEVOBJECT lpPens;
     int         nMode;
     LPTEST      lpTest;
{
     short      nIdx,nIdy,x,y,nWidth,nHeight,nTxtWidth,nTxtHeight,nPageHeight;
     LPLOGPEN   lpPen;
     short      nObj;
     HPEN       hPen,hOldPen;
     HBRUSH     hBrush,hOldBrush;
     TEXTMETRIC tm;
     HANDLE     hBuffer;
     LPSTR      lpBuffer,lpBrsF,lpPenF;

     /*-----------------------------------------*\
     | Set the x and y dimensions of the ellipse |
     | to be 1 inch by 1 inch.  This then looks  |
     | like a circle on the device.              |
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
     | Draw the ellipses varying the brushes and |
     | pens which is stored in the global arrays.|
     | THIS SECTION PRINTS WITH OPAQUE BACK.     |
     \*-----------------------------------------*/
     x=0;
     y=0;

     for(nIdy=0; nIdy < GetObjectCount(lpTest->gtTest.hBrushes); nIdy++)
     {
          if((hBuffer = LocalAlloc(LHND,80)))
          {
               if((lpBuffer = LocalLock(hBuffer)))
               {
                    if(nMode == OPAQUE)
                         wsprintf(lpBuffer,"Opaque Background - Brush %d",nIdy);
                    else
                         wsprintf(lpBuffer,"Transparent Background - Brush %d",nIdy);
                    TextOut(hDC,x,y,lpBuffer,lstrlen(lpBuffer));

                    LocalUnlock(hBuffer);
               }
               LocalFree(hBuffer);
          }

          for(nIdx=0; nIdx < GetObjectCount(lpTest->gtTest.hPens); nIdx++)
          {
               SetCurrentObject(lpTest->gtTest.hBrushes,nIdy);
               CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hBrushes);
               SetCurrentObject(lpBrushes,nObj);
               hBrush = CreateDeviceObject(lpBrushes);

               SetCurrentObject(lpTest->gtTest.hPens,nIdx);
               CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hPens);
               SetCurrentObject(lpPens,nObj);
               hPen = CreateDeviceObject(lpPens);

               if(!(hOldBrush = SelectObject(hDC,hBrush)))
               {
                    DeleteObject(hBrush);
                    DeleteObject(hPen);
                    return(FALSE);
               }
               if(!(hOldPen = SelectObject(hDC,hPen)))
               {
                    DeleteObject(SelectObject(hDC,hOldBrush));
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
                         if(!PrintFooter(hDC,lpDevCaps,"Curves Test"))
                         {
                              DeleteObject(SelectObject(hDC,hOldPen));
                              DeleteObject(SelectObject(hDC,hOldBrush));
                              return(FALSE);
                         }
                    }
               }
               SetBkMode(hDC,nMode);
               TstDrawObject(hDC,x,y+nTxtHeight,nWidth,nHeight,OBJ_ELLIPSE);
               DeleteObject(SelectObject(hDC,hOldBrush));
               DeleteObject(SelectObject(hDC,hOldPen));

               x+=nWidth+(nWidth/4);
          }

          x=0;
          y+=nHeight+(nHeight/2);
          if((y+nHeight+nTxtHeight) > nPageHeight)
          {
               y=0;
               if(!PrintFooter(hDC,lpDevCaps,"Curves Test"))
                    return(FALSE);
          }
     }

     if(!PrintFooter(hDC,lpDevCaps,"Curves Tests"))
          return(FALSE);

     return(TRUE);
}
