/*---------------------------------------------------------------------------*\
| TEXT TESTS                                                                  |
|   This module contains the routines for testing a printer devices text      |
|   capabilities.                                                             |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Jun 05, 1989                                                       |
| SEGMENT: _TEXTX                                                             |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntFram.h"

/*---------------------------------------------------------------------------*\
| PRINT TEXT TEST                                                             |
|   This routine performs the outputing of text tests to the printer.         |
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
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintText(hWnd,hDC,lpDevCaps,lpBrushes,lpPens,lpFonts,lpTest,bAbort)
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
     PrintTestDescription(hDC,IDS_TST_DSCR_TEXT,lpDevCaps);
     if(!PrintFooter(hDC,lpDevCaps,"Text Test"))
          return(FALSE);

     /*-----------------------------------------*\
     | TEST SUITE ---> (append tests to list)    |
     \*-----------------------------------------*/
     PrintExtTextOut(hDC,0,lpDevCaps,lpFonts,lpTest);
     PrintExtTextOut(hDC,2,lpDevCaps,lpFonts,lpTest);
     PrintSYNExtTextOut(hDC,0,SYNFONT_ITALIC,lpDevCaps,lpFonts,lpTest);
     PrintSYNExtTextOut(hDC,2,SYNFONT_UNDERLINED,lpDevCaps,lpFonts,lpTest);
     PrintSYNExtTextOut(hDC,4,SYNFONT_STRIKEOUT,lpDevCaps,lpFonts,lpTest);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT EXTTEXTOUT TEXT                                                       |
|   This routine test the output of a text string within a clipping rectangle |
|   defined on a page.  It prints the text using both ETO_CLIPPED and         |
|   ETO_OPAQUED rectangles.  The text itself is printed at nine positions on  |
|   the rectangle to verify the proper clipping of the string.  An array of   |
|   character spacing is also used to ensure the clipping.                    |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter()    - misc.c    (PRNTFRAM)                                   |
|   ExtTextOutClip() - library.c (ITE_TEXT)                                   |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC          - Handle to a printer device context.      |
|   int               nCharSpacing - Char spacing between characters.         |
|   LPDEVCAPS         lpDevCaps    - Device capabilities structure.           |
|   LPENUMERATE       lpFonts      - Array of logical font structures.        |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst - Module instance of library (PrntFram)                      |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL PrintExtTextOut(hDC,nCharSpacing,lpDevCaps,lpFonts,lpTest)
     HDC         hDC;
     short       nCharSpacing;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpFonts;
     LPTEST      lpTest;
{
     extern HANDLE hInst;

     short        nIdx,nIdy,nLoc,yPos,nRectH,nRectW,nPageHeight;
     int          nObj;
     TEXTMETRIC   tm;
     RECT         rRect;
     GLOBALHANDLE hdx;
     LPINT        lpdx;
     HANDLE       hBuffer;
     LPSTR        lpBuffer,lpFntF;
     HFONT        hFont,hOldFont;

     static PRINTCAPS szOption[] = {ETO_CLIPPED,"Clipped Rectangle",
                                    ETO_OPAQUE,"Opaque Rectangle"};
     static POINT pPosition[] = {0,0,0,-1,0,1,-1,0,-1,-1,-1,1,1,0,1,-1,1,1};

     /*-----------------------------------------*\
     | Get the test string for the test.         |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
          return(FALSE);

     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }

     LoadString(hInst,IDS_TEST_TEXT_STR1,lpBuffer,128);
     /*-----------------------------------------*\
     | Alloc/Lock buffer for integer array.      |
     \*-----------------------------------------*/
     if(!(hdx = GlobalAlloc(GHND,(DWORD)lstrlen(lpBuffer)*sizeof(int))))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          return(FALSE);
     }
     if(!(lpdx = (LPINT)GlobalLock(hdx)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          GlobalFree(hdx);
          return(FALSE);
     }
     /*------------------------------------*\
     | Initialize test character spacing.   |
     \*------------------------------------*/
     for(nIdx=0; nIdx < lstrlen(lpBuffer); nIdx++)
          *(lpdx+nIdx) = (tm.tmAveCharWidth*nCharSpacing);

     if(!nCharSpacing)
          lpdx = NULL;

     /*-----------------------------------------*\
     | For all fonts; perform the tests.         |
     \*-----------------------------------------*/
     for(nIdy=0; nIdy < GetObjectCount(lpTest->gtTest.hFonts); nIdy++)
     {
          SetCurrentObject(lpTest->gtTest.hFonts,nIdy);
          CopyDeviceObject((LPSTR)&nObj,lpTest->gtTest.hFonts);
          SetCurrentObject(lpFonts,nObj);
          hFont = CreateDeviceObject(lpFonts);

          if(!(hOldFont = SelectObject(hDC,hFont)))
          {
               DeleteObject(hFont);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               GlobalUnlock(hdx);
               GlobalFree(hdx);
               return(FALSE);
          }

          /*------------------------------------*\
          | Get/Set rectangle height and width.  |
          \*------------------------------------*/
          GetTextMetrics(hDC,&tm);
          nRectH      = tm.tmHeight+tm.tmExternalLeading;
          nRectW      = lpDevCaps->nHorzRes / 2;
          nPageHeight = lpDevCaps->nVertRes-nRectH;

          for(nIdx=0; nIdx < 2; nIdx++)
          {
               SelectObject(hDC,hOldFont);
               if(szOption[nIdx].nIndex == ETO_CLIPPED)
                    wsprintf(lpBuffer,(LPSTR)"Clipped Rectangle: Spacing - %d: Font %d ",
                         nCharSpacing,nIdy);
               else
                    wsprintf(lpBuffer,(LPSTR)"Opaque Rectangle: Spacing - %d: Font %d ",
                         nCharSpacing,nIdy);
               TextOut(hDC,0,0,lpBuffer,lstrlen(lpBuffer));
               SelectObject(hDC,hFont);

               LoadString(hInst,IDS_TEST_TEXT_STR1,lpBuffer,128);
               /*------------------------------------*\
               | Output string at all 9 pts on rect.  |
               \*------------------------------------*/
               yPos=nRectH;
               for(nLoc=0; nLoc < 9; nLoc++)
               {
                    if((yPos+(3*nRectH)) > nPageHeight)
                    {
                              yPos=0;
                              if(!PrintFooter(hDC,lpDevCaps,"Text Test"))
                              {
                                   LocalUnlock(hBuffer);
                                   LocalFree(hBuffer);
                                   GlobalUnlock(hdx);
                                   GlobalFree(hdx);
                                   DeleteObject(SelectObject(hDC,hOldFont));
                                   return(FALSE);
                               }
                    }

                    yPos+=(nRectH/2)+nRectH;

                    SetRect(&rRect,(lpDevCaps->nHorzRes-nRectW)/2,yPos,
                         lpDevCaps->nHorzRes-(nRectW/2),yPos+nRectH);

                    TstExtTextOutRect(hDC,szOption[nIdx].nIndex,(LPRECT)&rRect,
                         lpBuffer,nLoc,lpdx);

                    yPos+=(nRectH*3);
               }
               if(!PrintFooter(hDC,lpDevCaps,"Text Test"))
               {
                    LocalUnlock(hBuffer);
                    LocalFree(hBuffer);
                    GlobalUnlock(hdx);
                    GlobalFree(hdx);
                    DeleteObject(SelectObject(hDC,hOldFont));
                    return(FALSE);
               }
          }
          /*------------------------------------*\
          | Restore for next font.               |
          \*------------------------------------*/
          DeleteObject(SelectObject(hDC,hOldFont));
     }
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     GlobalUnlock(hdx);
     GlobalFree(hdx);

     return(TRUE);
}


BOOL PrintSYNExtTextOut(hDC,nCharSpacing,wSynFont,lpDevCaps,lpFonts,lpTest)
     HDC         hDC;
     short       nCharSpacing;
     WORD        wSynFont;
     LPDEVINFO   lpDevCaps;
     HDEVOBJECT lpFonts;
     LPTEST      lpTest;
{
     extern HANDLE hInst;

     short        nIdx,nIdy,nLoc,yPos,nRectH,nRectW,nPageHeight;
     TEXTMETRIC   tm;
     RECT         rRect;
     GLOBALHANDLE hdx;
     LPINT        lpdx;
     HANDLE       hBuffer;
     LPSTR        lpBuffer;
     HFONT        hFont,hOldFont;
     FONT         fFont;
     BOOL         wOldSynItalic,wOldSynUnder,wOldSynStrike;

     static PRINTCAPS szOption[] = {ETO_CLIPPED,"Clipped Rectangle",
                                    ETO_OPAQUE,"Opaque Rectangle"};
     static POINT pPosition[] = {0,0,0,-1,0,1,-1,0,-1,-1,-1,1,1,0,1,-1,1,1};


     /*-----------------------------------------*\
     | Get the test string for the test.         |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
          return(FALSE);

     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }

     LoadString(hInst,IDS_TEST_TEXT_STR1,lpBuffer,128);
     /*-----------------------------------------*\
     | Alloc/Lock buffer for integer array.      |
     \*-----------------------------------------*/
     if(!(hdx = GlobalAlloc(GHND,(DWORD)lstrlen(lpBuffer)*sizeof(int))))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          return(FALSE);
     }
     if(!(lpdx = (LPINT)GlobalLock(hdx)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          GlobalFree(hdx);
          return(FALSE);
     }
     /*------------------------------------*\
     | Initialize test character spacing.   |
     \*------------------------------------*/
     for(nIdx=0; nIdx < lstrlen(lpBuffer); nIdx++)
          *(lpdx+nIdx) = (tm.tmAveCharWidth*nCharSpacing);

     if(!nCharSpacing)
          lpdx = NULL;

     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          GlobalUnlock(hdx);
          GlobalFree(hdx);
          return(FALSE);
     }
     /*-----------------------------------------*\
     | For all fonts; perform the tests.         |
     \*-----------------------------------------*/
     for(nIdy=0; nIdy < GetObjectCount(lpTest->gtTest.hFonts); nIdy++)
     {
          SetCurrentObject(lpTest->gtTest.hFonts,nIdy);
          CopyDeviceObject((LPSTR)&fFont,lpTest->gtTest.hFonts);

          wOldSynItalic = fFont.lf.lfItalic;
          wOldSynUnder  = fFont.lf.lfUnderline;
          wOldSynStrike = fFont.lf.lfStrikeOut;

          if(wSynFont & SYNFONT_ITALIC)
               fFont.lf.lfItalic = TRUE;
          if(wSynFont & SYNFONT_UNDERLINED)
               fFont.lf.lfUnderline = TRUE;
          if(wSynFont & SYNFONT_STRIKEOUT)
               fFont.lf.lfStrikeOut = TRUE;

          hFont = CreateFontIndirect(&fFont.lf);

          if(!(hOldFont = SelectObject(hDC,hFont)))
          {
               DeleteObject(hFont);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               GlobalUnlock(hdx);
               GlobalFree(hdx);
               return(FALSE);
          }

          /*-----------------------------------------*\
          | Get/Set rectangle height and width.       |
          \*-----------------------------------------*/
          GetTextMetrics(hDC,&tm);
          nRectH      = tm.tmHeight+tm.tmExternalLeading;
          nRectW      = lpDevCaps->nHorzRes / 2;
          nPageHeight = lpDevCaps->nVertRes-nRectH;

          for(nIdx=0; nIdx < 2; nIdx++)
          {
               SelectObject(hDC,hOldFont);
               if(szOption[nIdx].nIndex == ETO_CLIPPED)
                    wsprintf(lpBuffer,(LPSTR)"Clipped Rectangle: Spacing - %d: Font %d ",
                         nCharSpacing,nIdy);
               else
                    wsprintf(lpBuffer,(LPSTR)"Opaque Rectangle: Spacing - %d: Font %d ",
                         nCharSpacing,nIdy);

               if(wSynFont & SYNFONT_ITALIC)
                    lstrcat(lpBuffer," | Synth Italic");
               if(wSynFont & SYNFONT_UNDERLINED)
                    lstrcat(lpBuffer," | Synth Underlined");
               if(wSynFont & SYNFONT_STRIKEOUT)
                    lstrcat(lpBuffer," | Synth StrikeOut");

               TextOut(hDC,0,0,lpBuffer,lstrlen(lpBuffer));
               SelectObject(hDC,hFont);

               LoadString(hInst,IDS_TEST_TEXT_STR1,lpBuffer,128);
               /*------------------------------------*\
               | Output string at all 9 pts on rect.  |
               \*------------------------------------*/
               yPos=nRectH;
               for(nLoc=0; nLoc < 9; nLoc++)
               {
                    if((yPos+(3*nRectH)) > nPageHeight)
                    {
                         yPos=0;
                         if(!PrintFooter(hDC,lpDevCaps,"Text Test"))
                         {
                              LocalUnlock(hBuffer);
                              LocalFree(hBuffer);
                              GlobalUnlock(hdx);
                              GlobalFree(hdx);
                              return(FALSE);
                         }
                    }

                    yPos+=(nRectH/2)+nRectH;

                    SetRect(&rRect,(lpDevCaps->nHorzRes-nRectW)/2,yPos,
                         lpDevCaps->nHorzRes-(nRectW/2),yPos+nRectH);

                    TstExtTextOutRect(hDC,szOption[nIdx].nIndex,(LPRECT)&rRect,
                         lpBuffer,nLoc,lpdx);

                    yPos+=(nRectH*3);
               }
               if(!PrintFooter(hDC,lpDevCaps,"Text Test"))
               {
                    LocalUnlock(hBuffer);
                    LocalFree(hBuffer);
                    GlobalUnlock(hdx);
                    GlobalFree(hdx);
                    return(FALSE);
               }
          }
          /*------------------------------------*\
          | Restore for next font.               |
          \*------------------------------------*/
          DeleteObject(SelectObject(hDC,hOldFont));
     }

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     GlobalUnlock(hdx);
     GlobalFree(hdx);

     return(TRUE);
}
