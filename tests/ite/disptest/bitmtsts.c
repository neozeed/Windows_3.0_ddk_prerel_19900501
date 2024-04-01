/*---------------------------------------------------------------------------*\
| BITMAP TESTS                                                                |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - createed.                                           |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "DispTest.h"

/*---------------------------------------------------------------------------*\
| DISPLAY BITMAPS                                                             |
|   This routine is called from a WM_TIMER message for each combination of    |
|   SRC/DST/PAT bitmap and 15 different ROP codes.                            |
|                                                                             |
|                                                                             |
\*---------------------------------------------------------------------------*/
BOOL DisplayBitmaps(hWnd,hDC,lpDevCaps,hDevBrushes,hDevPens,hDevFonts,lpNextTest)
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

     HBRUSH      hPatBrsh,hDstBrsh,hSrcBrsh;
     int         x,y,nObj;
     static int  nSdx,nDdx,nRdx,nSrcCount,nDstCount,nErrorCount;
     short       nBitHeight,nBitWidth,nTxtHeight;
     TEXTMETRIC  tm;
     LOGBRUSH    lbSrc,lbDst;
     RECT        rRect;
     POINT       pPoint;
     DWORD       dwResult;
     char        szBuffer[128];

     /*-----------------------------------------*\
     | If the indicies are zero, then it's the   |
     | start of the test.  Log as such.          |
     \*-----------------------------------------*/
     if((nSdx == 0) && (nDdx == 0) && (nRdx == 0))
     {
          GetClientRect(hWnd,&rRect);
          FillRect(hDC,&rRect,GetClassWord(hWnd,GCW_HBRBACKGROUND));
          nSrcCount=1; nDstCount=1;
          nErrorCount=0;
     }

     /*-----------------------------------------*\
     | Calculate dimensions of test object based |
     | on the size of the client window.         |
     \*-----------------------------------------*/
     GetClientRect(hWnd,&rRect);
     GetTextMetrics(hDC,&tm);
     nBitHeight = (rRect.bottom-rRect.top) >> 1;
     nBitWidth  = (rRect.right-rRect.left) >> 1;
     nTxtHeight = tm.tmHeight;
     x = nBitWidth >> 1;
     y = nBitHeight >> 1;

     /*-----------------------------------------*\
     | Get the brushes from the index array.  It |
     | should only get BS_SOLID type brushes.    |
     \*-----------------------------------------*/
     if(!(hSrcBrsh = GetNextTestBrush(hDevBrushes,tlTest.gtTest.hBrushes,&nSdx)))
     {
         nSdx = 0; nDdx++;
         hSrcBrsh = GetNextTestBrush(hDevBrushes,tlTest.gtTest.hBrushes,&nSdx);
         nSrcCount=nSdx+1; nDstCount++;
     }
     if(!(hDstBrsh = GetNextTestBrush(hDevBrushes,tlTest.gtTest.hBrushes,&nDdx)))
     {
         if(hSrcBrsh)
              DeleteObject(hSrcBrsh);
         nSdx = 0; nDdx = 0; nRdx = 0;
         nSrcCount=1; nDstCount=1;
         (*lpNextTest)++;
         return(TRUE);
     }
     hPatBrsh = CreateSolidBrush(GetNearestColor(hDC,RGB(128,128,128)));

     /*-----------------------------------------*\
     | Output the test object.                   |
     \*-----------------------------------------*/
     TstBitBltRop(hDC,x,y,nBitHeight,nBitWidth,hSrcBrsh,hDstBrsh,
          hPatBrsh,pRopCodes[nRdx].nIndex,TRUE);

     /*-----------------------------------------*\
     | Get RESULT point from display.            |
     \*-----------------------------------------*/
     pPoint.x = x+(nBitWidth/2);
     pPoint.y = (y+nBitHeight)-8;
     dwResult = GetPixel(hDC,pPoint.x,pPoint.y);

     /*-----------------------------------------*\
     | Output the test header titles.            |
     \*-----------------------------------------*/
     SetTextAlign(hDC,TA_CENTER);
     lstrcpy(szBuffer,(LPSTR)"Src");
     TextOut(hDC,x+(nBitWidth/6),y-nTxtHeight,szBuffer,lstrlen(szBuffer));
     lstrcpy(szBuffer,(LPSTR)"Dst");
     TextOut(hDC,x+((nBitWidth/6)*3),y-nTxtHeight,szBuffer,lstrlen(szBuffer));
     lstrcpy(szBuffer,(LPSTR)"Pat");
     TextOut(hDC,x+((nBitWidth/6)*5),y-nTxtHeight,szBuffer,lstrlen(szBuffer));
     lstrcpy(szBuffer,(LPSTR)"Result");
     TextOut(hDC,x+(nBitWidth/2),y+nBitHeight,szBuffer,lstrlen(szBuffer));
     SetTextAlign(hDC,TA_LEFT);

     /*-----------------------------------------*\
     | Verify the ROP code point.                |
     \*-----------------------------------------*/
     if(!VerifyBitBltROP(hDC,pPoint.x,pPoint.y,hSrcBrsh,hDstBrsh,hPatBrsh,
          pRopCodes[nRdx].nIndex))
     {
          SetCurrentObject(hDevBrushes,nDdx);
          CopyDeviceObject((LPSTR)&lbDst,hDevBrushes);
          SetCurrentObject(hDevBrushes,nSdx);
          CopyDeviceObject((LPSTR)&lbSrc,hDevBrushes);
          wsprintf(szBuffer,"    BitBlt ROP Failed: Src-%08lX  Dst-%08lX  Result-%08lX  ROP->%08lX",
               lbSrc.lbColor,lbDst.lbColor,dwResult,pRopCodes[nRdx].nIndex);
          lstrcat(szBuffer," <");
          lstrcat(szBuffer,pRopCodes[nRdx].szTitle);
          lstrcat(szBuffer,">");
          WriteLogFile(szLogFile,szBuffer);
          nErrorCount++;
     }

     /*-----------------------------------------*\
     | Clean up (free) the device brushes.       |
     \*-----------------------------------------*/
     DeleteObject(hSrcBrsh);
     DeleteObject(hDstBrsh);
     DeleteObject(hPatBrsh);

     /*-----------------------------------------*\
     | Output the test information.              |
     \*-----------------------------------------*/
     wsprintf(szBuffer,"Dest - %d:%d  Source - %d:%d  ROP Code - %s          ",
          nDstCount,lpDevCaps->nColors,nSrcCount,lpDevCaps->nColors,
          (LPSTR)pRopCodes[nRdx].szTitle);
     TextOut(hDC,0,0,szBuffer,lstrlen(szBuffer));
     wsprintf(szBuffer,"Error Count - %d  Estimated Timer - %d Minutes     ",
          nErrorCount,(lpDevCaps->nColors * lpDevCaps->nColors * 15) / ((60 * 1000) / nTimerSpeed));
     TextOut(hDC,0,nTxtHeight,szBuffer,lstrlen(szBuffer));

     /*-----------------------------------------*\
     | Calculate next test.                      |
     \*-----------------------------------------*/
     nRdx++;
     if(nRdx >= 15)
     {
          nRdx = 0;
          nSdx++;
          nSrcCount++;
          if(nSdx >= GetObjectCount(tlTest.gtTest.hBrushes))
          {
               nSdx=0;
               nSrcCount=0;
               nDdx++;
               nDstCount++;
               if(nDdx >= GetObjectCount(tlTest.gtTest.hBrushes))
               {
                    nDdx=0;
                    nDstCount=0;
                    (*lpNextTest)++;
                    WriteLogFile(szLogFile,(LPSTR)"  BitBlt ROP Test Completed");
               }
          }
     }
     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| VERIFY BITBLT ROP                                                           |
|   This routine verifies the Source, Destination and Pattern combinations    |
|   for a BitBlt() call.  Using the brushes, it creates bitmaps from these    |
|   so that it may look at each cooresponding bit to see if it is correct for |
|   the ROP.  The coordinates for obtaining the color from the display is     |
|   passed to this function so that it may generate a bitmap representation.  |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - Pass or Fail.                                                      |
\*---------------------------------------------------------------------------*/
BOOL VerifyBitBltROP(hDC,x,y,hSrcBrsh,hDstBrsh,hPatBrsh,dwROP)
     HDC     hDC;
     int     x;
     int     y;
     HBRUSH  hSrcBrsh;
     HBRUSH  hDstBrsh;
     HBRUSH  hPatBrsh;
     DWORD   dwROP;
{
     register int iPlanes,iBitPix;
     int          iRes,index;
     BOOL         bSrcBit,bDstBit,bPixBit,bPatBit;
     BITMAP       bmSrc,bmDst,bmPat,bmPix;
     HBITMAP      hSrcBmp,hDstBmp,hPatBmp,hPixBmp;
     GLOBALHANDLE hSrcMem,hDstMem,hPatMem,hPixMem;
     LPSTR        pSrcMem,pDstMem,pPatMem,pPixMem;
     LPSTR        pSrcPtr,pDstPtr,pPatPtr,pPixPtr;

     /*-----------------------------------------*\
     | Create the ROP bitmap equivalents.        |
     \*-----------------------------------------*/
     hPixBmp = CreatePixelBitmap(hDC,x,y);
     hSrcBmp = CreateBrushBitmap(hDC,1,1,hSrcBrsh);
     hDstBmp = CreateBrushBitmap(hDC,1,1,hDstBrsh);
     hPatBmp = CreateBrushBitmap(hDC,1,1,hPatBrsh);

     /*-----------------------------------------*\
     | Get bits for each bitmap.                 |
     \*-----------------------------------------*/
     hSrcMem = GetMemoryBitmap(hSrcBmp);
     hDstMem = GetMemoryBitmap(hDstBmp);
     hPatMem = GetMemoryBitmap(hPatBmp);
     hPixMem = GetMemoryBitmap(hPixBmp);

     /*-----------------------------------------*\
     | Get bitmap info, then kill bitmaps.       |
     \*-----------------------------------------*/
     GetObject(hSrcBmp,sizeof(BITMAP),(LPSTR)&bmSrc);
     GetObject(hDstBmp,sizeof(BITMAP),(LPSTR)&bmDst);
     GetObject(hPatBmp,sizeof(BITMAP),(LPSTR)&bmPat);
     GetObject(hPixBmp,sizeof(BITMAP),(LPSTR)&bmPix);
     DeleteObject(hSrcBmp);
     DeleteObject(hDstBmp);
     DeleteObject(hPatBmp);
     DeleteObject(hPixBmp);

     /*-----------------------------------------*\
     | Lock the arrays for checking ROP code.    |
     \*-----------------------------------------*/
     pSrcMem = GlobalLock(hSrcMem);
     pDstMem = GlobalLock(hDstMem);
     pPatMem = GlobalLock(hPatMem);
     pPixMem = GlobalLock(hPixMem);

     /*-----------------------------------------*\
     | Do a plane/plane verification of ROP.     |
     \*-----------------------------------------*/
     for(iPlanes=0; iPlanes < bmSrc.bmPlanes; iPlanes++)
          for(iBitPix=1; iBitPix <= bmSrc.bmBitsPixel; iBitPix++)
          {
               pSrcPtr = pSrcMem + (bmSrc.bmWidthBytes*iPlanes);
               bSrcBit = FindBitmapBit(pSrcPtr,iBitPix);

               pDstPtr = pDstMem + (bmDst.bmWidthBytes*iPlanes);
               bDstBit = FindBitmapBit(pDstPtr,iBitPix);

               pPatPtr = pPatMem + (bmPat.bmWidthBytes*iPlanes);
               bPatBit = FindBitmapBit(pPatPtr,iBitPix);

               pPixPtr = pPixMem + (bmPix.bmWidthBytes*iPlanes);
               bPixBit = FindBitmapBit(pPixPtr,iBitPix);

               if(!CheckROPBit(bSrcBit,bDstBit,bPatBit,bPixBit,HIWORD(dwROP)))
               {
                    GlobalUnlock(hSrcMem);
                    GlobalUnlock(hDstMem);
                    GlobalUnlock(hPatMem);
                    GlobalUnlock(hPixMem);
                    GlobalFree(hSrcMem);
                    GlobalFree(hDstMem);
                    GlobalFree(hPatMem);
                    GlobalFree(hPixMem);
                    return(FALSE);
               }
          }

     /*-----------------------------------------*\
     | Free memory.                              |
     \*-----------------------------------------*/
     GlobalUnlock(hSrcMem);
     GlobalUnlock(hDstMem);
     GlobalUnlock(hPatMem);
     GlobalUnlock(hPixMem);
     GlobalFree(hSrcMem);
     GlobalFree(hDstMem);
     GlobalFree(hPatMem);
     GlobalFree(hPixMem);

     return(TRUE);
}

HBRUSH GetNextTestBrush(hAllBrushes,hTstIndexes,nIdx)
     HDEVOBJECT     hAllBrushes;
     HDEVOBJECT     hTstIndexes;
     register LPINT nIdx;
{
     LOGBRUSH lb;
     int      nObj;

     do
     {
          SetCurrentObject(hTstIndexes,*nIdx);
          CopyDeviceObject((LPSTR)&nObj,hTstIndexes);
          SetCurrentObject(hAllBrushes,nObj);
          CopyDeviceObject((LPSTR)&lb,hAllBrushes);
     }
     while((lb.lbStyle != BS_SOLID) && (++(*nIdx) < GetObjectCount(hTstIndexes)));

     if(lb.lbStyle != BS_SOLID)
          return(NULL);

     return(CreateDeviceObject(hAllBrushes));
}

BOOL DisplayColorMapping(hWnd,hDC,lpDevCaps,lpNextTest)
     HWND       hWnd;
     HDC        hDC;
     LPDEVINFO  lpDevCaps;
     LPSTR      lpNextTest;
{
     extern TEST tlTest;
     extern int nTimerSpeed;
     extern char szLogFile[];

     static int nIdx,nIdy,nIdz,nError;
     char         szBuffer[80];
     int          nHeight,nWidth;
     COLORREF     dwBase,dwMap;
     HBRUSH       hBrush;
     RECT         rect1,rect2,rect;
     TEXTMETRIC   tm;

     GetClientRect(hWnd,&rect1);
     GetTextMetrics(hDC,&tm);
     nHeight = tm.tmHeight*2;

     nWidth  = (rect1.right >> 1) - (tm.tmAveCharWidth*10);

     rect1.top    += nHeight;
     rect1.bottom -= nHeight;
     rect1.left   += (tm.tmAveCharWidth*5);
     rect1.right  =  rect1.left + nWidth;

     CopyRect(&rect2,&rect1);
     OffsetRect(&rect2,nWidth+(tm.tmAveCharWidth*10),0);

     if(!nIdx && !nIdy && !nIdz)
     {
          GetClientRect(hWnd,&rect);
          FillRect(hDC,&rect,GetClassWord(hWnd,GCW_HBRBACKGROUND));
          WriteLogFile(szLogFile,(LPSTR)"  Color Mapping Test Initiated");
          nError=0;
     }

     dwBase = RGB(nIdz,nIdy,nIdx);
     dwMap = GetNearestColor(hDC,dwBase);
     if(!TstColorMapping(hDC,dwBase))
     {
          nError++;
          wsprintf(szBuffer,"Base - %08lX    Mapped - %08lX",dwBase,dwMap);
          WriteLogFile(szLogFile,szBuffer);
     }
     wsprintf(szBuffer,"Base - %08lX    Mapped - %08lX  Error - %03d     ",dwBase,dwMap,nError);
     TextOut(hDC,0,0,szBuffer,lstrlen(szBuffer));

     hBrush = CreateSolidBrush(dwBase);
     FillRect(hDC,&rect1,hBrush);
     DeleteObject(hBrush);

     hBrush = CreateSolidBrush(dwMap);
     FillRect(hDC,&rect2,hBrush);
     DeleteObject(hBrush);

     nIdz+=16;
     if(nIdz > 255)
     {
          nIdz = 0;
          nIdy+=16;
          if(nIdy > 255)
          {
               nIdy=0;
               nIdx+=16;
               if(nIdx > 255)
               {
                    nIdx=0;
                    (*lpNextTest)++;
                    WriteLogFile(szLogFile,(LPSTR)"  Color Mapping Test Completed");
               }
          }
     }

     return(TRUE);
}
