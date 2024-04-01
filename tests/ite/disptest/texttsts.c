/*---------------------------------------------------------------------------*\
| TEXT TESTS                                                                  |
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

BOOL DisplayExtTextOut(hWnd,hDC,lpDevCaps,hDevBrushes,hDevPens,hDevFonts,lpNextTest)
     HWND       hWnd;
     HDC        hDC;
     LPDEVINFO  lpDevCaps;
     HDEVOBJECT hDevBrushes;
     HDEVOBJECT hDevPens;
     HDEVOBJECT hDevFonts;
     LPSTR      lpNextTest;
{
     extern int nTimerSpeed;
     extern char szLogFile[];
     extern TEST   tlTest;
     extern HANDLE hInst;

     RECT rRect,rTxtRect;
     short nRectHeight,nRectWidth,x,y,nObj;
     TEXTMETRIC tm;
     FONT   fFont;
     HFONT hFont,hOldFont;
     LOCALHANDLE hBuffer;
     LPSTR       lpBuffer,lpFlag;
     static PRINTCAPS szOption[] = {ETO_CLIPPED,"Clipped Rectangle",
                                    ETO_OPAQUE,"Opaque Rectangle"};

     static int nIdx,nIdy,nIdz;

     if((nIdx ==0) && (nIdy == 0) && (nIdz == 0))
     {
          GetClientRect(hWnd,&rRect);
          FillRect(hDC,&rRect,GetStockObject(GRAY_BRUSH));
          WriteLogFile(szLogFile,(LPSTR)"  ExtTextOut Test Started");
     }

     if(nIdy >= GetObjectCount(tlTest.gtTest.hFonts))
     {
          nIdy = 0;
          (*lpNextTest)++;
          return(TRUE);
     }

     SetCurrentObject(tlTest.gtTest.hFonts,nIdy);
     CopyDeviceObject((LPSTR)&nObj,tlTest.gtTest.hFonts);
     SetCurrentObject(hDevFonts,nObj);
     if(!(hFont = (HFONT)CreateDeviceObject(hDevFonts)))
     {
          MessageBox(hWnd,(LPSTR)"Could not create font",NULL,MB_OK);
          return(FALSE);
     }

     hOldFont = SelectObject(hDC,hFont);

     if(hBuffer = LocalAlloc(LHND,128))
     {
          if(lpBuffer = LocalLock(hBuffer))
          {
               GetClientRect(hWnd,&rRect);
               GetTextMetrics(hDC,&tm);
               nRectWidth  = (rRect.right-rRect.left)/2;
               nRectHeight = tm.tmHeight;
               x = nRectWidth/2;
               y = (rRect.bottom-rRect.top)/2;

               LoadString(hInst,IDS_TEXT_TESTSTRING,lpBuffer,128);
               SetRect(&rTxtRect,x,y,x+nRectWidth,y+nRectHeight);
               SetBkColor(hDC,GetNearestColor(hDC,RGB(0,255,0)));
               TstExtTextOutRect(hDC,szOption[nIdx].nIndex,(LPRECT)&rTxtRect,
                    lpBuffer,nIdz,NULL);

               SetBkColor(hDC,RGB(128,128,128));
               SelectObject(hDC,GetStockObject(SYSTEM_FONT));
               CopyDeviceObject((LPSTR)&fFont,hDevFonts);
               wsprintf(lpBuffer,(LPSTR)"Font - %s %d   Rectangle - %s",
                    (LPSTR)fFont.lf.lfFaceName,fFont.lf.lfHeight,
                    (LPSTR)szOption[nIdx].szType);
               TextOut(hDC,0,0,lpBuffer,lstrlen(lpBuffer));
               SelectObject(hDC,hFont);
               LocalUnlock(hBuffer);
          }
          LocalFree(hBuffer);
     }

     DeleteObject(SelectObject(hDC,hOldFont));

     nIdz++;
     if(nIdz >= 9)
     {
          FillRect(hDC,&rRect,GetStockObject(GRAY_BRUSH));
          nIdz = 0;
          nIdy++;
          if(nIdy >= GetObjectCount(tlTest.gtTest.hFonts))
          {
               nIdy=0;
               nIdx++;
               if(nIdx >= 2)
               {
                    nIdx=0;
                    (*lpNextTest)++;
                    WriteLogFile(szLogFile,(LPSTR)"  ExtTextOut Test Completed");
               }
          }
     }

     return(TRUE);
}

BOOL VerifyExtTextOutClip(hDC,nMode,lpRect,lpString,nStrPos,lpDX)
     HDC    hDC;
     short  nMode;
     LPRECT lpRect;
     LPSTR  lpString;
     short  nStrPos;
     LPINT  lpDX;
{
     short  nRectHeight;
     short  nRectWidth;
     short  nStrX,nStrY;
     short  nStrHeight,nStrWidth;
     DWORD  dwTstColor;

     nRectHeight = lpRect->bottom - lpRect->top;
     nRectWidth  = lpRect->right - lpRect->left;
     nStrHeight  = HIWORD(GetTextExtent(hDC,lpString,lstrlen(lpString)));
     nStrWidth   = LOWORD(GetTextExtent(hDC,lpString,lstrlen(lpString)));

     switch(nStrPos)
     {
          case 0:
               nStrX = lpRect->left - (nStrWidth/2) + 1;
               nStrY = lpRect->top - (nStrHeight/2) + 1;
               break;
          case 1:
               nStrX = lpRect->left + 1;
               nStrY = lpRect->top - (nStrHeight/2) + 1;
               break;
          case 2:
               nStrX = lpRect->right - (nStrWidth/2) + 1;
               nStrY = lpRect->top - (nStrHeight/2) + 1;
               break;
          case 3:
               nStrX = lpRect->left - (nStrWidth/2) + 1;
               nStrY = lpRect->top + 1;
               break;
          case 4:
               nStrX = lpRect->left + 1;
               nStrY = lpRect->top + 1;
               break;
          case 5:
               nStrX = lpRect->right - (nStrWidth/2) + 1;
               nStrY = lpRect->top + 1;
               break;
          case 6:
               nStrX = lpRect->left - (nStrWidth/2) + 1;
               nStrY = lpRect->top + (nRectHeight/2) + 1;
               break;
          case 7:
               nStrX = lpRect->left + 1;
               nStrY = lpRect->top + (nRectHeight/2) + 1;
               break;
          case 8:
               nStrX = lpRect->right + (nStrWidth/2) + 1;
               nStrY = lpRect->top + (nRectHeight/2) + 1;
               break;
          default:
               return(FALSE);
     }

     dwTstColor = GetPixel(hDC,nStrX,nStrY);

     switch(nMode)
     {
          case ETO_OPAQUE:
               if(dwTstColor == GetPixel(hDC,0,0))
                    return(FALSE);
               break;
          case ETO_CLIPPED:
               if(dwTstColor != GetPixel(hDC,0,0))
                    return(FALSE);
               break;
          default:
               return(FALSE);
     }

     return(TRUE);
}
