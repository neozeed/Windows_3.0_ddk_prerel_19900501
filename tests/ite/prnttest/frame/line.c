/*---------------------------------------------------------------------------*\
| LINE TESTS                                                                  |
|   This module contains routnes specific to testing the line capabilities of |
|   a printer driver.                                                         |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _LINE                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT LINE TEST                                                             |
|   This routine peforms the line printing test for the printer driver.       |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintTestDescription() - (misc.c)                                         |
|   PrintFooter()          - (misc.c)                                         |
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
BOOL FAR PASCAL PrintLines(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpTest,bAbort)
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

     /*-----------------------------------------*\
     | Print out exclaimation of test.           |
     \*-----------------------------------------*/
     PrintTestDescription(hDC,IDS_TST_DSCR_LINE,lpDevCaps);
     if(!PrintFooter(hDC,lpDevCaps,"Lines Test"))
          return(FALSE);

     /*-----------------------------------------*\
     | TEST SUITE ---> (append tests to list)    |
     \*-----------------------------------------*/
     PrintPolyLines(hDC,TRANSPARENT,3,lpDevCaps,lpPens,lpTest);
     PrintPolyLines(hDC,OPAQUE,5,lpDevCaps,lpPens,lpTest);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT POLYLINES                                                             |
|   This routine outputs an object created using the polylines function.  it  |
|   varies the pen width and background modes for all combinations of pens    |
|   and background modes.                                                     |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   int               nBkMode   - Background mode.                            |
|   int               nPenWidth - Width of pen.                               |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpPems    - Array of logical pen structures.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL PrintPolyLines(hDC,nBkMode,nPenWidth,lpDevCaps,lpPens,lpTest)
     HDC         hDC;
     int         nBkMode;
     int         nPenWidth;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpPens;
     LPTEST      lpTest;
{
     short      nIdx,nIdy,x,y,nWidth,nHeight,nTxtWidth,nTxtHeight,nPageHeight;
     int        nObj;
     LPLOGPEN   lpPen;
     LOGPEN     lpModPen;
     HPEN       hPen,hOldPen;
     TEXTMETRIC tm;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;

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

     for(nIdy=0; nIdy < GetObjectCount(lpTest->gtTest.hPens); nIdy++)
     {
          if((hBuffer = LocalAlloc(LHND,80)))
          {
               if((lpBuffer = LocalLock(hBuffer)))
               {
                    if(nBkMode == TRANSPARENT)
                         wsprintf(lpBuffer,"Transparent Background: Pen %d",nIdy);
                    else
                         wsprintf(lpBuffer,"Opaque Background: Pen %d",nIdy);
                    TextOut(hDC,x,y,lpBuffer,lstrlen(lpBuffer));

                    LocalUnlock(hBuffer);
               }
               LocalFree(hBuffer);
          }

          SetCurrentObject(lpTest->gtTest.hPens,nIdy);
          CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hPens);
          SetCurrentObject(lpPens,nObj);
          for(nIdx=0; nIdx < 8; nIdx++)
          {
               CopyDeviceObject((LPSTR)&lpModPen,lpPens);

               lpModPen.lopnWidth.x = nIdx;
               lpModPen.lopnWidth.y = nIdx;
               if(!(hPen = CreatePenIndirect(&lpModPen)))
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
                         if(!PrintFooter(hDC,lpDevCaps,"Lines Test"))
                         {
                              DeleteObject(SelectObject(hDC,hOldPen));
                              return(FALSE);
                         }
                    }
               }
               SetBkMode(hDC,nBkMode);
               TstDrawObject(hDC,x,y+nTxtHeight,nWidth,nHeight,OBJ_POLYLINE);
               DeleteObject(SelectObject(hDC,hOldPen));

               x+=nWidth+(nWidth/4);
          }

          x=0;
          y+=nHeight+(nHeight/2);
          if((y+nHeight+nTxtHeight) > nPageHeight)
          {
               y=0;
               if(!PrintFooter(hDC,lpDevCaps,"Lines Test"))
                    return(FALSE);
          }
     }

     if(!PrintFooter(hDC,lpDevCaps,"Lines Tests"))
          return(FALSE);

     return(TRUE);
}
