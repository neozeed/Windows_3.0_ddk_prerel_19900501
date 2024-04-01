/*---------------------------------------------------------------------------*\
| DISPLAY TEST SETUP DIALOG                                                   |
|   This module contains the Display Test Setup Dialog procedure for setting  |
|   up the test structure.                                                    |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _SETUP                                                             |
|                                                                             |
| HISTORY: June 03, 1989 - created.                                           |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "DispTest.h"


BOOL FAR PASCAL InterfaceDlg(hDlg,iMessage,wParam,lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern WORD wTestsSet;
     extern TEST tlTest;

     LOCALHANDLE hBuffer;
     LPSTR       lpBuffer;
     short       nIdx;
     static WORD wFlags[] = {DT_RAST,DT_CURV,DT_LINE,DT_POLY,DT_TEXT};
     static int  nTimer;

     switch(iMessage)
     {
          /*------------------------------------*\
          | Initialize Test WORD for all tests.  |
          \*------------------------------------*/
          case WM_INITDIALOG:
               hBuffer = LocalAlloc(LHND,(WORD)128);
               lpBuffer = LocalLock(hBuffer);
               GetPrivateProfileString((LPSTR)"StartUp",(LPSTR)"LogFile",
                    (LPSTR)"\0",lpBuffer,128,(LPSTR)"DispTest.ini");
               SetDlgItemText(hDlg,IDD_INTR_LOGE,lpBuffer);
               GetPrivateProfileString((LPSTR)"StartUp",(LPSTR)"MstFile",
                    (LPSTR)"\0",lpBuffer,128,(LPSTR)"DispTest.ini");
               SetDlgItemText(hDlg,IDD_INTR_MSTE,lpBuffer);
               GetPrivateProfileString((LPSTR)"StartUp",(LPSTR)"CpyFile",
                    (LPSTR)"\0",lpBuffer,128,(LPSTR)"DispTest.ini");
               SetDlgItemText(hDlg,IDD_INTR_CPYE,lpBuffer);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);

               wTestsSet = GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"TestsTest",0,(LPSTR)"DispTest.ini");
               tlTest.wStatus |= (BOOL)GetPrivateProfileInt((LPSTR)"Startup",
                                 (LPSTR)"AutoRun",0,(LPSTR)"DispTest.ini");
               for(nIdx=0; nIdx < 5; nIdx++)
                    if(wTestsSet & wFlags[nIdx])
                         SendDlgItemMessage(hDlg,IDD_INTR_RAST+nIdx,BM_SETCHECK,TRUE,0l);
                    else
                         SendDlgItemMessage(hDlg,IDD_INTR_RAST+nIdx,BM_SETCHECK,FALSE,0l);

               SetScrollRange(GetDlgItem(hDlg,IDD_INTR_TIME),SB_CTL,100,2000,TRUE);
               SetScrollPos(GetDlgItem(hDlg,IDD_INTR_TIME),SB_CTL,1000,TRUE);
               SetDlgItemInt(hDlg,IDD_INTR_SEC1,1000,TRUE);
               nTimer = 1000;
               break;

          /*------------------------------------*\
          | Process dialog commands.  OR the bit |
          | when user selects and XOR to elim.   |
          \*------------------------------------*/
          case WM_COMMAND:
               switch(wParam)
               {
                    case IDD_INTR_RAST:
                    case IDD_INTR_CURV:
                    case IDD_INTR_POLY:
                    case IDD_INTR_LINE:
                    case IDD_INTR_TEXT:
                         if(IsDlgButtonChecked(hDlg,wParam))
                              wTestsSet |= wFlags[wParam-IDD_INTR_RAST];
                         else
                              wTestsSet ^= wFlags[wParam-IDD_INTR_RAST];
                         break;

                    default:
                         return(FALSE);
               }
               break;

          case WM_HSCROLL:
               switch(wParam)
               {
                    case SB_PAGEDOWN:
                         nTimer +=50;
                    case SB_LINEDOWN:
                         nTimer = min(2000,nTimer+1);
                         break;
                    case SB_PAGEUP:
                         nTimer -=50;
                    case SB_LINEUP:
                         nTimer = max(100,nTimer-1);
                         break;
                    case SB_TOP:
                         nTimer = 100;
                         break;
                    case SB_BOTTOM:
                         nTimer = 2000;
                         break;
                    case SB_THUMBPOSITION:
                    case SB_THUMBTRACK:
                         nTimer = LOWORD(lParam);
                         break;
               }
               SetScrollPos(GetDlgItem(hDlg,IDD_INTR_TIME),SB_CTL,nTimer,TRUE);
               SetDlgItemInt(hDlg,IDD_INTR_SEC1,nTimer,TRUE);
               break;
          /*------------------------------------*\
          | No message to process.               |
          \*------------------------------------*/
          default:
               return(FALSE);
     }

     return(TRUE);
}


BOOL FAR PASCAL SetupObjectsDlg(hDlg,iMessage,wParam,lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern TEST       tlTest;
     extern HANDLE     hInst;
     extern HDEVOBJECT hDevPens,hDevBrushes,hDevFonts;

     HDC        hDC;
     TEXTMETRIC tm;
     RECT       rRect;
     int        nIdx;
     int        x,y,dx,dy;
     LOGPEN     lpPen;
     LOGBRUSH   lpBrush;
     FONT       lpFont;
     HPEN       hPen,hOldPen;
     HBRUSH     hBrush,hOldBrush;
     HFONT      hFont;
     char       szBuffer[80];
     LPSTR      lpFlag;

     switch(iMessage)
     {
          case WM_MEASUREITEM:
               hDC = GetDC(hDlg);
               GetTextMetrics(hDC,&tm);
               ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = tm.tmHeight+tm.tmExternalLeading;
               ((LPMEASUREITEMSTRUCT)lParam)->itemWidth = 25;
               ReleaseDC(hDlg,hDC);
               break;

          case WM_DRAWITEM:
               y = ((LPDRAWITEMSTRUCT)lParam)->rcItem.top+((((LPDRAWITEMSTRUCT)lParam)->rcItem.bottom-((LPDRAWITEMSTRUCT)lParam)->rcItem.top)/2);
               hDC = GetDC(hDlg);
               GetTextMetrics(hDC,&tm);
               ReleaseDC(hDlg,hDC);
               hDC  = ((LPDRAWITEMSTRUCT)lParam)->hDC;
               nIdx = LOWORD(((LPDRAWITEMSTRUCT)lParam)->itemData);
               SetBkMode(hDC,TRANSPARENT);

               /*-------------------------------*\
               |                                 |
               \*-------------------------------*/
               if((((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_DRAWENTIRE) ||
                  (((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_SELECT))
               {
                    if(((LPDRAWITEMSTRUCT)lParam)->itemState & ODS_SELECTED)
                    {
                         hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                         FillRect(((LPDRAWITEMSTRUCT)lParam)->hDC,
                              (LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem,
                              hBrush);
                         DeleteObject(hBrush);
                    }
                    else
                         FillRect(((LPDRAWITEMSTRUCT)lParam)->hDC,
                              (LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem,
                              GetClassWord(((LPDRAWITEMSTRUCT)lParam)->hwndItem,
                                   GCW_HBRBACKGROUND));

                    switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
                    {
                         case IDD_OBJT_PENLIST:
                              SetCurrentObject(hDevPens,nIdx);
                              hPen = CreateDeviceObject(hDevPens);
                              hOldPen = SelectObject(hDC,hPen);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              MoveTo(hDC,tm.tmAveCharWidth*2,y);
                              LineTo(hDC,((LPDRAWITEMSTRUCT)lParam)->rcItem.right-(tm.tmAveCharWidth*2),y);
                              DeleteObject(SelectObject(hDC,hOldPen));
                              break;
                         case IDD_OBJT_BRSHLIST:
                              SetCurrentObject(hDevBrushes,nIdx);
                              hBrush = CreateDeviceObject(hDevBrushes);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              hOldBrush = SelectObject(hDC,hBrush);
                              Rectangle(hDC,rRect.left,rRect.top,rRect.right,rRect.bottom);
                              DeleteObject(SelectObject(hDC,hOldBrush));
                              break;
                    }
               }

               /*-------------------------------*\
               |                                 |
               \*-------------------------------*/
               if(((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_FOCUS)
                    DrawFocusRect(hDC,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
               SetBkMode(hDC,OPAQUE);
               break;

          case WM_INITDIALOG:
               for(nIdx=0; nIdx < GetObjectCount(hDevPens); nIdx++)
                    SendDlgItemMessage(hDlg,IDD_OBJT_PENLIST,LB_ADDSTRING,
                         NULL,(LONG)MAKELONG(nIdx,0));
               for(nIdx=0; nIdx < GetObjectCount(hDevBrushes); nIdx++)
                    SendDlgItemMessage(hDlg,IDD_OBJT_BRSHLIST,LB_ADDSTRING,
                         NULL,(LONG)MAKELONG(nIdx,0));
               InitObjectsFontList(hDlg);
               KillTest(&tlTest);
               InitTest("\0",hInst,&tlTest);
               break;

          case WM_COMMAND:
               switch(wParam)
               {
                    case IDOK:
                         for(nIdx=0; nIdx < GetObjectCount(hDevBrushes); nIdx++)
                         {
                              if(SendDlgItemMessage(hDlg,IDD_OBJT_BRSHLIST,LB_GETSEL,nIdx,0l))
                                   AddObject(tlTest.gtTest.hBrushes,(LPSTR)&nIdx);
                         }
                         for(nIdx=0; nIdx < GetObjectCount(hDevPens); nIdx++)
                         {
                              if(SendDlgItemMessage(hDlg,IDD_OBJT_PENLIST,LB_GETSEL,nIdx,0l))
                                   AddObject(tlTest.gtTest.hPens,(LPSTR)&nIdx);
                         }
                         for(nIdx=0; nIdx < GetObjectCount(hDevFonts); nIdx++)
                         {
                              if(SendDlgItemMessage(hDlg,IDD_OBJT_FONTLIST,LB_GETSEL,nIdx,0l))
                                   AddObject(tlTest.gtTest.hFonts,(LPSTR)&nIdx);
                         }
                         EndDialog(hDlg,TRUE);
                         break;

                    case IDCANCEL:
                         KillTest(&tlTest);
                         EndDialog(hDlg,TRUE);
                         break;

                    case IDD_OBJT_PENALL:
                    case IDD_OBJT_BRSHALL:
                    case IDD_OBJT_FONTALL:
                         if(IsDlgButtonChecked(hDlg,wParam))
                         {
                              SendDlgItemMessage(hDlg,wParam,BM_SETCHECK,FALSE,0l);
                              SendDlgItemMessage(hDlg,wParam-3,LB_SETSEL,FALSE,-1l);
                         }
                         else
                         {
                              SendDlgItemMessage(hDlg,wParam,BM_SETCHECK,TRUE,0l);
                              SendDlgItemMessage(hDlg,wParam-3,LB_SETSEL,TRUE,-1l);
                         }
                         break;
                    default:
                         return(FALSE);
               }
               break;

          default:
               return(FALSE);
     }
     return(TRUE);
}


BOOL InitObjectsFontList(hDlg)
     HWND hDlg;
{
     extern HDEVOBJECT hDevFonts;

     int    nIdx;
     char   szBuffer[80];
     char   szTemp[10];
     FONT   fFont;

     for(nIdx=0; nIdx < GetObjectCount(hDevFonts); nIdx++)
     {
          SetCurrentObject(hDevFonts,nIdx);
          CopyDeviceObject((LPSTR)&fFont,hDevFonts);

          lstrcpy(szBuffer,fFont.lf.lfFaceName);
          lstrcat(szBuffer," ");
          lstrcat(szBuffer,litoa(fFont.lf.lfHeight,szTemp,10));

          if(fFont.lf.lfWeight > 550)
               lstrcat(szBuffer," Bold");
          if(fFont.lf.lfItalic)
               lstrcat(szBuffer," Italic");
          if(fFont.lf.lfStrikeOut)
               lstrcat(szBuffer," StrikeOut");
          if(fFont.lf.lfUnderline)
               lstrcat(szBuffer," Underlined");

          SendDlgItemMessage(hDlg,IDD_OBJT_FONTLIST,LB_ADDSTRING,
               NULL,(LONG)(LPSTR)szBuffer);
     }

     return(TRUE);
}
