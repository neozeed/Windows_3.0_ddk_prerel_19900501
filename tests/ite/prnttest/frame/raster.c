/*---------------------------------------------------------------------------*\
| RASTER                                                                      |
|   This module contains the routines for handling the RASTER tests for       |
|   the printer driver.                                                       |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _RASTER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jun 27, 1989 - added comments.                                     |
|          Jul 18, 1989 - added more tests to the application.                |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT BITMAPS                                                               |
|   This routine peforms the raster tests for the printer driver.             |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintTestDescription() - (misc.c)                                         |
|   PrintFooter()          - (misc.c)                                         |
|   DisplayErrorMessage()  - (misc.c)                                         |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND              hWnd      - Handle to parent window.                    |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpBrushes - Array of logical brush structures.          |
|   LPENUMERATE       lpPens    - Array of logical pen structures.            |
|   LPENUMERATE       lpFonts   - Array of logical font structures.           |
|   LPSTR             lpAbort   - Boolean Abort flag.                         |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   DEVCAPS         dcDevCaps - Device capabilities structure.                |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if passed.                                                    |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintBitmaps(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpTest,bAbort)
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
     | Print out test description.               |
     \*-----------------------------------------*/
     PrintTestDescription(hDC,IDS_TST_DSCR_RAST,lpDevCaps);
     if(!PrintFooter(hDC,lpDevCaps,"Raster Test"))
          return(FALSE);

     /*-----------------------------------------*\
     | Check to see if device supports bitmaps.  |
     | If it doesn't, then report to display.    |
     \*-----------------------------------------*/
     if(lpDevCaps->wRasterCaps)
     {
/*        PrintStretchBlt(hDC,lpDevCaps,lpTest);
*/        PrintBitBlt(hDC,lpDevCaps,lpBrushes,lpTest);
     }

     return(TRUE);
}

/*---------------------------------------------------------------------------*\
| PRINT STRETCHBIT TEST                                                       |
|   This routine performst the stretching of bitmaps on the printer device.   |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst - Module instance of DLL (PrntFram)                          |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL PrintStretchBlt(hDC,lpDevCaps,lpTest)
     HDC               hDC;
     LPDEVINFO         lpDevCaps;
     LPTEST            lpTest;
{
     extern HANDLE hInst;

     HBITMAP hBitmap;
     BITMAP  bm;
     HDC     hMemDC;

     /*-----------------------------------------*\
     | Load mono-bitmap from resource file.      |
     \*-----------------------------------------*/
     if(!(hBitmap = LoadBitmap(hInst,"Bitmap1")))
          return(FALSE);

     /*-----------------------------------------*\
     | Create memory DC for BitBlt transfer.     |
     \*-----------------------------------------*/
     if(!(hMemDC = CreateCompatibleDC(hDC)))
          return(FALSE);

     /*-----------------------------------------*\
     | Select bitmap into DC-ready for transfer. |
     \*-----------------------------------------*/
     if(!SelectObject(hMemDC,hBitmap))
     {
          DeleteDC(hMemDC);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Transfer to Printer DC.                   |
     \*-----------------------------------------*/
     GetObject(hBitmap,sizeof(BITMAP),(LPSTR)&bm);
     StretchBlt(hDC,0,0,576,576,hMemDC,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);

     /*-----------------------------------------*\
     | Clean up.                                 |
     \*-----------------------------------------*/
     DeleteDC(hMemDC);
     DeleteObject(hBitmap);
     if(!PrintFooter(hDC,lpDevCaps,"Raster Test"))
          return(FALSE);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| BITBLT/ROP TEST                                                             |
|   This routine performs the BitBlt of Source/Destination/Pattern bitmap     |
|   combinations.  Currently, it does this for the 15 defined Rop Codes.      |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|   BitBltRop()   - (ITE_BITM)                                                |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPENUMERATE       lpBrushes - Array of logical brush structures.          |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL PrintBitBlt(hDC,lpDevCaps,lpBrushes,lpTest)
     HDC         hDC;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpBrushes;
     LPTEST      lpTest;
{
     extern BOOL FAR PrintFooter(HDC,LPDEVINFO,LPSTR);

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

     HBRUSH     hPatBrsh,hDstBrsh,hSrcBrsh;
     int        nIdx,nIdy,nIdz,x,y,nHeight,nWidth;
     int        nObj;
     short      nBitHeight,nBitWidth,nTstHeight,nTstWidth;
     TEXTMETRIC tm;
     LPSTR      lpFlag;

     /*-----------------------------------------*\
     | Set the height/width of the font text.    |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     nHeight = tm.tmHeight+tm.tmExternalLeading;
     nWidth  = tm.tmAveCharWidth;

     /*-----------------------------------------*\
     | Set the height/width of the bitmaps.      |
     \*-----------------------------------------*/
     nBitHeight = lpDevCaps->nLogPixelsY - lpDevCaps->nLogPixelsY/4;
     nBitWidth  = lpDevCaps->nLogPixelsX - lpDevCaps->nLogPixelsX/4;
     nTstHeight = lpDevCaps->nLogPixelsY/4;
     nTstWidth  = (lpDevCaps->nLogPixelsX/2)+(lpDevCaps->nLogPixelsX/4);

     /*-----------------------------------------*\
     | Create the PAT brush and bitmap.          |
     \*-----------------------------------------*/
     if(!(hPatBrsh = CreateHatchBrush(HS_FDIAGONAL,RGB(0,0,0))))
          return(FALSE);

     /*-----------------------------------------*\
     | Let's do it.                              |
     \*-----------------------------------------*/
     x=0;
     y=0;
     for(nIdx=0; nIdx < GetObjectCount(lpTest->gtTest.hBrushes); nIdx++)
     {
          /*-------------------------------*\
          | Create the Dst Bitmap.          |
          \*-------------------------------*/
          SetCurrentObject(lpTest->gtTest.hBrushes,nIdx);
          CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hBrushes);
          SetCurrentObject(lpBrushes,nObj);
          hDstBrsh = CreateDeviceObject(lpBrushes);

          for(nIdy=0; nIdy < GetObjectCount(lpTest->gtTest.hBrushes); nIdy++)
          {
               /*---------------------*\
               | Create the Src Bitmap.|
               \*---------------------*/
               SetCurrentObject(lpTest->gtTest.hBrushes,nIdy);
               CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hBrushes);
               SetCurrentObject(lpBrushes,nObj);
               hSrcBrsh = CreateDeviceObject(lpBrushes);

               for(nIdz=0; nIdz < 15; nIdz++)
               {
                    if((x+nTstWidth) > lpDevCaps->nHorzRes)
                    {
                         x=0;
                         y+=(lpDevCaps->nLogPixelsY+nHeight);

                         if((y+lpDevCaps->nLogPixelsY+nHeight) > lpDevCaps->nVertRes)
                         {
                              x=0;
                              y=0;

                              if(!PrintFooter(hDC,lpDevCaps,"Raster Test"))
                              {
                                   DeleteObject(hPatBrsh);
                                   DeleteObject(hSrcBrsh);
                                   DeleteObject(hDstBrsh);
                                   return(FALSE);
                              }
                         }
                    }
                    TextOut(hDC,x,y,pRopCodes[nIdz].szTitle,
                         lstrlen(pRopCodes[nIdz].szTitle));

                    TstBitBltRop(hDC,x,y+nHeight,nBitHeight,
                         nBitWidth,hSrcBrsh,hDstBrsh,hPatBrsh,
                         pRopCodes[nIdz].nIndex,FALSE);

                    x+=(lpDevCaps->nLogPixelsX + nBitWidth);
               }
               DeleteObject(hSrcBrsh);
          }
          DeleteObject(hDstBrsh);
     }
     /*-----------------------------------------*\
     | Free all remaining objects.               |
     \*-----------------------------------------*/
     DeleteObject(hPatBrsh);

     if(!PrintFooter(hDC,lpDevCaps,"Raster Test"))
          return(FALSE);

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
