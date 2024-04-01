/*---------------------------------------------------------------------------*\
| LINES TESTS                                                                 |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - createed.                                           |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "DispTest.h"                            /* Program Header File      */

BOOL DisplayPolylines(hWnd,hDC,lpDevCaps,hDevBrushes,hDevPens,hDevFonts,lpNextTest)
     HWND       hWnd;
     HDC        hDC;
     LPDEVINFO  lpDevCaps;
     HDEVOBJECT hDevBrushes;
     HDEVOBJECT hDevPens;
     HDEVOBJECT hDevFonts;
     LPSTR      lpNextTest;
{
     extern TEST tlTest;
     extern int nTimerSpeed;
     extern char szLogFile[];

     LOGPEN      lp;
     POINT       pt;
     HPEN        hOldPen,hPen;
     static int  nIdw,nIdx,nIdz;
     int         x,y,dx,dy,nOldROP,nOldBack,nObj;
     RECT        rRect;
     LOCALHANDLE hBuffer;
     LPSTR       lpBuffer,lpFlag;
     TEXTMETRIC  tm;
     short       nTxtH,nObjH,nObjW,nHorzS,nVertS,nRopIdx,nRow,nCol;

     static int nPenWidths[]   = {1,2,4,8,16};
     static PRINTCAPS szBack[] = {OPAQUE        , "Opaque Background",
                                  TRANSPARENT   , "Transparent Background"};
     static PRINTCAPS szROP[]  = {R2_BLACK      , "Black",
                                  R2_NOTMERGEPEN, "NotMergePen",
                                  R2_MASKNOTPEN , "MaskNotPen",
                                  R2_NOTCOPYPEN , "NotCopyPen",
                                  R2_MASKPENNOT , "MaskPenNot",
                                  R2_NOT        , "Not",
                                  R2_XORPEN     , "XorPen",
                                  R2_NOTMASKPEN , "NotMaskPen",
                                  R2_MASKPEN    , "MaskPen",
                                  R2_NOTXORPEN  , "NotXorPen",
                                  R2_NOP        , "Nop",
                                  R2_MERGENOTPEN, "MergeNotPen",
                                  R2_COPYPEN    , "CopyPen",
                                  R2_MERGEPENNOT, "MergePenNot",
                                  R2_MERGEPEN   , "MergePen",
                                  R2_WHITE      , "White"};

     /*-----------------------------------------*\
     | Get Dimensions for the output sizes.      |
     \*-----------------------------------------*/
     GetClientRect(hWnd,&rRect);
     GetTextMetrics(hDC,&tm);
     nTxtH  = (tm.tmHeight + tm.tmExternalLeading);
     nObjH  = (rRect.bottom-rRect.top) / 5;
     nObjW  = (rRect.right-rRect.left) / 5;
     nHorzS = (rRect.right-rRect.left) / 25;
     nVertS = ((rRect.bottom-rRect.top) / 25) - ((2*nTxtH) / 5);
     rRect.top+=(nTxtH*2);

     /*-----------------------------------------*\
     | Set the new brush/pen/fill/back combo's.  |
     \*-----------------------------------------*/
     if(nIdx >= GetObjectCount(tlTest.gtTest.hPens))
     {
          nIdx = 0;
          nIdw = 0;
          nIdz = 0;
          (*lpNextTest)++;
          return(TRUE);
     }

     SetCurrentObject(tlTest.gtTest.hPens,nIdx);
     CopyDeviceObject((LPSTR)&nObj,tlTest.gtTest.hPens);
     SetCurrentObject(hDevPens,nObj);
     CopyDeviceObject((LPSTR)&lp,hDevPens);
     lp.lopnWidth.x = nPenWidths[nIdz];
     lp.lopnWidth.y = nPenWidths[nIdz];
     hPen = CreatePenIndirect(&lp);
     hOldPen = SelectObject(hDC,hPen);

     /*-----------------------------------------*\
     | Output a 4x4 matrix of ROP objects.       |
     | Clear screen before each output.          |
     \*-----------------------------------------*/
     FillRect(hDC,&rRect,GetStockObject(WHITE_BRUSH));
     nRopIdx=0;
     for(nRow=0; nRow < 4; nRow++)
     {
          for(nCol=0; nCol < 4; nCol++)
          {
              /*--------------------------------*\
              | Out the ROP code explaination.   |
              \*--------------------------------*/
              if(hBuffer = LocalAlloc(LHND,128))
              {
                   if(lpBuffer = LocalLock(hBuffer))
                   {
                        lstrcpy(lpBuffer,szROP[nRopIdx].szType);
                        SetTextAlign(hDC,TA_CENTER);
                        TextOut(hDC,(nCol*(nObjW+nHorzS))+nHorzS+(nObjW/2),
                             (nRow*(nObjH+nVertS))+nVertS+(2*nTxtH),
                             lpBuffer,lstrlen(lpBuffer));
                        SetTextAlign(hDC,TA_LEFT);
                        LocalUnlock(hBuffer);
                   }
                   LocalFree(hBuffer);
              }

              /*--------------------------------*\
              | Output the line object.          |
              \*--------------------------------*/
              nOldROP  = SetROP2(hDC,szROP[nRopIdx++].nIndex);
              nOldBack = SetBkMode(hDC,szBack[nIdw].nIndex);
              TstDrawObject(hDC,(nCol*(nObjW+nHorzS))+nHorzS,
                   (nRow*(nObjH+nVertS))+nVertS+(3*nTxtH),
                   nObjW,nObjH-nTxtH,OBJ_POLYLINE);

              SetROP2(hDC,nOldROP);
              SetBkMode(hDC,nOldBack);
          }
     }

     DeleteObject(SelectObject(hDC,hOldPen));

     if(hBuffer = LocalAlloc(LHND,128))
     {
          if(lpBuffer = LocalLock(hBuffer))
          {
               CopyDeviceObject((LPSTR)&lp,hDevPens);
               wsprintf(lpBuffer,"Pen -> %08lX [%d:%d]     ",
                     lp.lopnColor,nIdx,GetObjectCount(hDevPens));
               TextOut(hDC,0,0,lpBuffer,lstrlen(lpBuffer));
               wsprintf(lpBuffer,"Pen Width - %d   Back Mode - %s          ",
                    nPenWidths[nIdz],(LPSTR)szBack[nIdw].szType);
               TextOut(hDC,0,tm.tmHeight+tm.tmExternalLeading,
                    lpBuffer,lstrlen(lpBuffer));
               LocalUnlock(hBuffer);
          }
          LocalFree(hBuffer);
     }

     nIdw++;
     if(nIdw > 1)
     {
          nIdw = 0;
          nIdz++;
          if(nIdz > 4)
          {
               nIdz = 0;
               nIdx++;
               if(nIdx >= GetObjectCount(tlTest.gtTest.hPens))
               {
                    nIdx=0;
                    (*lpNextTest)++;

                    WriteLogFile(szLogFile,(LPSTR)"  Line Test Completed");
               }
          }
     }

     return(TRUE);
}
