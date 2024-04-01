/*---------------------------------------------------------------------------*\
| POLYGON TESTS                                                               |
|   This module contains routnes specific to testing the polygon capabilities |
|   of a printer driver.                                                      |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _POLYGON                                                           |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 24, 1989 - added poly-star test.                               |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT POLYGON TEST                                                          |
|   This routine peforms the polygon printing test for the printer driver.    |
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
|   DEVCAPS         dcDevCaps - Device capabilities structure.                |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if passed.                                                    |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintPolygons(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpTest,bAbort)
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
     | Print the test description.               |
     \*-----------------------------------------*/
     PrintTestDescription(hDC,IDS_TST_DSCR_POLY,lpDevCaps);
     if(!PrintFooter(hDC,lpDevCaps,"Polygons Test"))
          return(FALSE);

     /*-----------------------------------------*\
     | TEST SUITE ---> (append tests to list)    |
     \*-----------------------------------------*/
     PrintPolygonStar(hDC,ALTERNATE,OPAQUE,lpDevCaps,lpBrushes,lpPens,lpTest);
     PrintPolygonStar(hDC,ALTERNATE,TRANSPARENT,lpDevCaps,lpBrushes,lpPens,lpTest);
     PrintPolygonStar(hDC,WINDING,OPAQUE,lpDevCaps,lpBrushes,lpPens,lpTest);
     PrintPolygonStar(hDC,WINDING,TRANSPARENT,lpDevCaps,lpBrushes,lpPens,lpTest);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT POLYGON STAR                                                          |
|   This routine outputs an object created using the polygon function.  it    |
|   varies the pen and brush as well as the background modes for all          |
|   combinations.                                                             |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   int               nBkMode   - Background mode.                            |
|   int               nPenWidth - Width of pen.                               |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpPens    - Array of logical pen structures.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL PrintPolygonStar(hDC,nFlMode,nBkMode,lpDevCaps,lpBrushes,lpPens,lpTest)
     HDC         hDC;
     int         nFlMode;
     int         nBkMode;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpBrushes;
     HDEVOBJECT lpPens;
     LPTEST      lpTest;
{
     extern int FAR PrintFooter(HDC,LPDEVINFO,LPSTR);

     short      nIdx,nIdy,x,y,nWidth,nHeight,nTxtWidth,nTxtHeight,nPageHeight;
     int        nObj;
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
                    if((nFlMode == ALTERNATE) && (nBkMode == OPAQUE))
                         wsprintf(lpBuffer,"Alternate Fill: Opaque - Brush %d",nIdy);
                    else
                    if((nFlMode == WINDING) && (nBkMode == TRANSPARENT))
                         wsprintf(lpBuffer,"Winding Fill: Transparent - Brush %d",nIdy);
                    else
                    if((nFlMode == ALTERNATE) && (nBkMode == TRANSPARENT))
                         wsprintf(lpBuffer,"Alternate Fill: Transparent - Brush %d",nIdy);
                    else
                         wsprintf(lpBuffer,"Winding Fill: Opaque - Brush %d",nIdy);
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
                         if(!PrintFooter(hDC,lpDevCaps,"Polygons Test"))
                         {
                              DeleteObject(SelectObject(hDC,hOldPen));
                              DeleteObject(SelectObject(hDC,hOldBrush));
                              return(FALSE);
                         }
                    }
               }
               SetBkMode(hDC,nBkMode);
               SetPolyFillMode(hDC,nFlMode);
               TstDrawObject(hDC,x,y+nTxtHeight,nWidth,nHeight,OBJ_POLYGON);
               DeleteObject(SelectObject(hDC,hOldBrush));
               DeleteObject(SelectObject(hDC,hOldPen));

               x+=nWidth+(nWidth/4);
          }

          x=0;
          y+=nHeight+(nHeight/2);
          if((y+nHeight+nTxtHeight) > nPageHeight)
          {
               y=0;
               if(!PrintFooter(hDC,lpDevCaps,"Polygons Test"))
                    return(FALSE);
          }
     }

     if(!PrintFooter(hDC,lpDevCaps,"Polygons Tests"))
          return(FALSE);

     return(TRUE);
}
