/*---------------------------------------------------------------------------*\
| PRINTER TEST SETUP DIALOG                                                   |
|   This module contains the Printer Test Setup Dialog procedure for setting  |
|   up the test structure.                                                    |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _SETUP                                                             |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| SETUP PRINT HEADER CHARACTERISTICS                                          |
|   This is the dialog box procedure which prompts for the Print Header       |
|   information.  The user has a choice between Expanded, Condensed options   |
|   for the information to be displayed.  Expanded being more detailed in its |
|   information.  Information can be excluded from the print by checking the  |
|   box.                                                                      |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND     hDlg     - DialogBox window Handle.                              |
|   unsigned iMessage - Message to be processed.                              |
|   WORD     wParam   - Information associated with message.                  |
|   LONG     lParam   - Information associated with message.                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   WORD wHeaderSet - Each bit represents an option to include or exclude.    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if message was processed, or FALSE if it was not.  When the   |
|          user hits the OK button, then control goes back to the application.|
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL SetupHeaderDlg(hDlg, iMessage, wParam, lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern WORD wHeaderSet;

     int nIdx;
     char szBuffer[10];
     static WORD wFlags[] = {IDD_HEAD_YES ,IDD_HEAD_CAPS,
                             IDD_HEAD_FONT,IDD_HEAD_BRSH,
                             IDD_HEAD_PEN};

     switch(iMessage)
     {
          /*------------------------------------*\
          | Initialize Header WORD for all info. |
          | Then check the boxs - def=expanded.  |
          \*------------------------------------*/
          case WM_INITDIALOG:
               for(nIdx=0; nIdx < 5; nIdx++)
                    if(wHeaderSet & wFlags[nIdx])
                         SendDlgItemMessage(hDlg,wFlags[nIdx],BM_SETCHECK,TRUE,0l);
                    else
                         SendDlgItemMessage(hDlg,wFlags[nIdx],BM_SETCHECK,FALSE,0l);

               if(wHeaderSet & PH_EXPANDED)
                    CheckRadioButton(hDlg,IDD_HEAD_EXP,IDD_HEAD_CON,IDD_HEAD_EXP);
               else
                    CheckRadioButton(hDlg,IDD_HEAD_EXP,IDD_HEAD_CON,IDD_HEAD_CON);
               break;

          /*------------------------------------*\
          | Process dialog commands.  OR the bit |
          | when user selects and XOR to elim.   |
          \*------------------------------------*/
          case WM_COMMAND:
               switch(wParam)
               {
                    case IDD_HEAD_YES:
                    case IDD_HEAD_CAPS:
                    case IDD_HEAD_FONT:
                    case IDD_HEAD_BRSH:
                    case IDD_HEAD_PEN:
                         if(IsDlgButtonChecked(hDlg,wParam))
                              wHeaderSet |= wParam;
                         else
                              wHeaderSet ^= wParam;
                         break;

                    case IDD_HEAD_EXP:
                         wHeaderSet ^= IDD_HEAD_CON;
                         wHeaderSet |= IDD_HEAD_EXP;
                         CheckRadioButton(hDlg,IDD_HEAD_EXP,IDD_HEAD_CON,wParam);
                         break;

                    case IDD_HEAD_CON:
                         wHeaderSet ^= IDD_HEAD_EXP;
                         wHeaderSet |= IDD_HEAD_CON;
                         CheckRadioButton(hDlg,IDD_HEAD_EXP,IDD_HEAD_CON,wParam);
                         break;

                    case IDOK:
                         wsprintf((LPSTR)szBuffer,"%u",wHeaderSet);
                         WritePrivateProfileString((LPSTR)"StartUp",(LPSTR)"HeaderTest",
                              (LPSTR)szBuffer,(LPSTR)"PrntTest.ini");

                         EndDialog(hDlg,TRUE);
                         break;

                    default:
                         return(FALSE);
               }
               break;

          /*------------------------------------*\
          | No message to process.               |
          \*------------------------------------*/
          default:
               return(FALSE);
     }

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| SETUP PRINT TEST CHARACTERISTICS                                            |
|   This is the dialog box procedure which prompts for the Print Test         |
|   information.  The user has a choice of which tests to execute.  The bit   |
|   is set in the wTestsSet WORD for which tests to run.                      |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND     hDlg     - DialogBox window Handle.                              |
|   unsigned iMessage - Message to be processed.                              |
|   WORD     wParam   - Information associated with message.                  |
|   LONG     lParam   - Information associated with message.                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   WORD wTestsSet - Each bit represents an option to include or exclude.     |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if message was processed, or FALSE if it was not.  When the   |
|          user hits the OK button, then control goes back to the application.|
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL SetupTestsDlg(hDlg, iMessage, wParam, lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern WORD wTestsSet;

     int nIdx;
     char szBuffer[10];
     static WORD wFlags[] = {IDD_TEST_TEXT,IDD_TEST_BITMAPS,
                             IDD_TEST_POLYGONS, IDD_TEST_CURVES,
                             IDD_TEST_LINES};

     switch(iMessage)
     {
          /*------------------------------------*\
          | Initialize Test WORD for all tests.  |
          \*------------------------------------*/
          case WM_INITDIALOG:
               for(nIdx=0; nIdx < 5; nIdx++)
                    if(wTestsSet & wFlags[nIdx])
                         SendDlgItemMessage(hDlg,wFlags[nIdx],BM_SETCHECK,TRUE,0l);
                    else
                         SendDlgItemMessage(hDlg,wFlags[nIdx],BM_SETCHECK,FALSE,0l);
               break;

          /*------------------------------------*\
          | Process dialog commands.  OR the bit |
          | when user selects and XOR to elim.   |
          \*------------------------------------*/
          case WM_COMMAND:
               switch(wParam)
               {
                    case IDD_TEST_TEXT:
                    case IDD_TEST_BITMAPS:
                    case IDD_TEST_POLYGONS:
                    case IDD_TEST_CURVES:
                    case IDD_TEST_LINES:
                         if(IsDlgButtonChecked(hDlg,wParam))
                              wTestsSet |= wParam;
                         else
                              wTestsSet ^= wParam;
                         break;

                    case IDOK:
                         wsprintf((LPSTR)szBuffer,"%u",wTestsSet);
                         WritePrivateProfileString((LPSTR)"StartUp",(LPSTR)"TestsTest",
                              (LPSTR)szBuffer,(LPSTR)"PrntTest.ini");

                    case IDCANCEL:
                         EndDialog(hDlg,TRUE);
                         break;

                    default:
                         return(FALSE);
               }
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
     extern TEST tlTest;
     extern HANDLE hInst;
     extern HDEVOBJECT hPens,hBrushes,hFonts;

     HDC        hDC;
     TEXTMETRIC tm;
     RECT       rRect;
     int        nIdx,x,y,dx,dy;
     HPEN       hPen,hOldPen;
     HBRUSH     hBrush,hOldBrush;
     HFONT      hFont;
     char       szBuffer[80];

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

               /*-------------------------------*\
               |                                 |
               \*-------------------------------*/
               if(((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_DRAWENTIRE)
               {
                    if(((LPDRAWITEMSTRUCT)lParam)->itemState & ODS_SELECTED)
                    {
                         InvertRect(hDC,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);

                         SetROP2(hDC,R2_NOT);
                         SetBkMode(hDC,TRANSPARENT);
                    }
                    switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
                    {
                         case IDD_OBJT_PENLIST:
                              SetCurrentObject(hPens,nIdx);
                              hPen = CreateDeviceObject(hPens);
                              hOldPen = SelectObject(hDC,hPen);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              MoveTo(hDC,tm.tmAveCharWidth*2,y);
                              LineTo(hDC,((LPDRAWITEMSTRUCT)lParam)->rcItem.right-(tm.tmAveCharWidth*2),y);
                              DeleteObject(SelectObject(hDC,hOldPen));
                              break;
                         case IDD_OBJT_BRSHLIST:
                              SetCurrentObject(hBrushes,nIdx);
                              hBrush = CreateDeviceObject(hBrushes);
                              hOldBrush = SelectObject(hDC,hBrush);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              Rectangle(hDC,rRect.left,rRect.top,rRect.right,rRect.bottom);
                              DeleteObject(SelectObject(hDC,hOldBrush));
                              break;
                    }
                    SetBkMode(hDC,OPAQUE);
                    SetROP2(hDC,R2_COPYPEN);
               }

               /*-------------------------------*\
               |                                 |
               \*-------------------------------*/
               if(((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_SELECT)
               {
                    InvertRect(hDC,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);

                    SetROP2(hDC,R2_XORPEN);
                    SetBkMode(hDC,TRANSPARENT);
                    switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
                    {
                         case IDD_OBJT_PENLIST:
                              SetCurrentObject(hPens,nIdx);
                              hPen = CreateDeviceObject(hPens);
                              hOldPen = SelectObject(hDC,hPen);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              MoveTo(hDC,tm.tmAveCharWidth*2,y);
                              LineTo(hDC,((LPDRAWITEMSTRUCT)lParam)->rcItem.right-(tm.tmAveCharWidth*2),y);
                              DeleteObject(SelectObject(hDC,hOldPen));
                              break;
                         case IDD_OBJT_BRSHLIST:
                              SetCurrentObject(hBrushes,nIdx);
                              hBrush = CreateDeviceObject(hBrushes);
                              hOldBrush = SelectObject(hDC,hBrush);
                              CopyRect((LPRECT)&rRect,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
                              InflateRect((LPRECT)&rRect,-(2*tm.tmAveCharWidth),-2);
                              Rectangle(hDC,rRect.left,rRect.top,rRect.right,rRect.bottom);
                              DeleteObject(SelectObject(hDC,hOldBrush));
                              break;
                    }

                    SetBkMode(hDC,OPAQUE);
                    SetROP2(hDC,R2_COPYPEN);
               }

               /*-------------------------------*\
               |                                 |
               \*-------------------------------*/
               if(((LPDRAWITEMSTRUCT)lParam)->itemAction & ODA_FOCUS)
                    DrawFocusRect(hDC,(LPRECT)&((LPDRAWITEMSTRUCT)lParam)->rcItem);
               break;

          case WM_INITDIALOG:
               for(nIdx=0; nIdx < GetObjectCount(hPens); nIdx++)
                    SendDlgItemMessage(hDlg,IDD_OBJT_PENLIST,LB_ADDSTRING,
                         NULL,(LONG)MAKELONG(nIdx,0));
               for(nIdx=0; nIdx < GetObjectCount(hBrushes); nIdx++)
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
                         for(nIdx=0; nIdx < GetObjectCount(hPens); nIdx++)
                         {
                              if(SendDlgItemMessage(hDlg,IDD_OBJT_PENLIST,LB_GETSEL,nIdx,0l))
                                   AddObject(tlTest.gtTest.hPens,(LPSTR)&nIdx);
                         }
                         for(nIdx=0; nIdx < GetObjectCount(hBrushes); nIdx++)
                         {
                              if(SendDlgItemMessage(hDlg,IDD_OBJT_BRSHLIST,LB_GETSEL,nIdx,0l))
                                   AddObject(tlTest.gtTest.hBrushes,(LPSTR)&nIdx);
                         }
                         for(nIdx=0; nIdx < GetObjectCount(hFonts); nIdx++)
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
                              SendDlgItemMessage(hDlg,wParam-3,LB_SETSEL,TRUE,-1l);
                         else
                              SendDlgItemMessage(hDlg,wParam-3,LB_SETSEL,FALSE,-1l);
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
     extern HDEVOBJECT hFonts;

     int    nIdx;
     char   szBuffer[80];
     char   szTemp[10];
     FONT   fFont;

     for(nIdx=0; nIdx < GetObjectCount(hFonts); nIdx++)
     {
          SetCurrentObject(hFonts,nIdx);
          CopyDeviceObject((LPSTR)&fFont,hFonts);

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