#include <windows.h>
#include "DispTest.h"

BOOL FAR DisplayBitmaps(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpExtra)
     HWND              hWnd;
     HDC               hDC;
     LPDEVCAPABILITIES lpDevCaps;
     LPENUMERATE       lpBrushes;
     LPENUMERATE       lpPens;
     LPENUMERATE       lpFonts;
     LPSTR             lpExtra;
{
     RECT       rRect;
     short      xStart,yStart,nHeight,nWidth;
     int        nIdx,nIdy,nIdz;
     LPLOGBRUSH lpBrush;
     HBRUSH     hSrc,hDst,hPat;
     static  struct
          {
               DWORD nIndex;
               PSTR  szTitle;
          } pRopCodes[] = {BLACKNESS , "Blackness" , NOTSRCERASE, "NotSrcErase",
                           NOTSRCCOPY, "NotSrcCopy", SRCERASE   , "SrcErase",
                           DSTINVERT , "DstInvert" , PATINVERT  , "PatInvert",
                           SRCINVERT , "SrcInvert" , SRCAND     , "SrcAnd",
                           MERGEPAINT, "MergePaint", MERGECOPY  , "MergeCopy",
                           SRCCOPY   , "SrcCopy"   , SRCPAINT   , "SrcPaint",
                           PATCOPY   , "PatCopy"   , PATPAINT   , "PatPaint",
                           WHITENESS , "Whiteness"};

     GetClientRect(hWnd,&rRect);

     nWidth  = (rRect.left-rRect.right)-lpDevCaps->nLogPixelsX;
     nHeight = (rRect.bottom-rRect.top)-lpDevCaps->nLogPixelsY;
     xStart = rRect.left+(lpDevCaps->nLogPixelsX/2);
     yStart = rRect.top+(lpDevCaps->nLogPixelsY/2);

     hPat = GetStockObject(GRAY_BRUSH);

     lpBrush = (LPLOGBRUSH)GlobalLock(lpBrushes->hGMem);
     for(nIdx=0; nIdx < lpBrushes->nCount; nIdx++)
     {    if((lpBrush+nIdx)->lbStyle == BS_SOLID)
          {
               hDst = CreateSolidBrush((lpBrush+nIdx)->lbColor);
               for(nIdy=0; nIdy < lpBrushes->nCount; nIdy++)
               {
                    if((lpBrush+nIdy)->lbStyle == BS_SOLID)
                    {
                         hSrc = CreateSolidBrush((lpBrush+nIdy)->lbColor);

                         for(nIdz=0; nIdz < 15; nIdz++)
                         {
                              PrintBitBltRop(hDC,xStart,yStart,nHeight,nWidth,
                                   hSrc,hDst,hPat,pRopCodes[nIdz].nIndex);
                              InvalidateRect(hWnd,NULL,TRUE);
                         }
                         DeleteObject(hSrc);
                    }
               }
               DeleteObject(hDst);
          }
     }
     GlobalUnlock(lpBrushes->hGMem);

     return(TRUE);
}
