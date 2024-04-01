#include <windows.h>
#incude  "isg_test.h"
#include "setpen.h"


HPEN FAR PASCAL CreateTestPen(hWnd)
     HWND     hWnd;
{
     extern HANDLE hLibInst;
     HPEN hPen;

     hPen = DialogBox(hLibInst,"SETPEN",hWnd,(FARPROC)CreatePenDlg);
     return(hPen);
}


#define LPMIS (LPMEASUREITEMSTRUCT)lParam
#define LPDIS (LPDRAWITEMSTRUCT)lParam

HPEN FAR PASCAL CreatePenDlg(hDlg,iMessage,wParam,lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     int nIdx;
     HPEN hPen;
     TEXTMETRIC tm;

     switch(iMessage)
     {
          case WM_INITDIALOG:
               for(nIdx=PS_SOLID; nIdx <= PS_INSIDEFRAME; nIdx++)
                    SendDlgItemMessage(hDlg,IDD_PEN_STYL,CB_INSERTSTRING,nIdx,(LONG)nIdx);
               break;
          case WM_MEASUREITEM:
               {
                    hDC        hDC;
                    TEXTMETRIC tm;
                    RECT       rect;

                    hDC = GetDC(hDlg);
                    GetTextMetrics(hDC,&tm);
                    GetClientRect(GetDlgItem(hDlg,(MIS)->CtlID),&rect);
                    ReleaseDC(hDlg,hDC);

                    (MIS)->itemWidth  = rect.right-rect.left;
                    (MIS)->itemHeight = tm.tmHeight+tm.tmExternalLeading;
               }
               break;
          case WM_DRAWITEM:
               switch((DIS)->itemAction)
               {
                    case ODA_DRAWENTIRE:
                         GetTextMetrics((DIS)->hDC,&tm);

                         hPen = SelectObject((DIS)->hDC,CreatePen((DIS)->itemData,0,RGB(0,0,0)));
                         MoveTo((DIS)->hDC,0,((DIS)->rcItem.top+(DIS)->rcItem.bottom)/2);
                         LineTo((DIS)->hDC,(5*tm.tmAveCharWidth));
                         DeleteObject(SelectObject((DIS)->hDC,hPen);
                         break;
                    case ODA_FOCUS:
                         break;
                    case ODA_SELECT:
                         break;
               }
               break;
          case WM_COMMAND:
               ProcessSetPenCommands(hDlg,wParam,lParam);
               break;
          default:
               return(FALSE);
     }
     return(TRUE);
}

BOOL ProcessSetPenCommands(hDlg,wParam,lParam)
     HWND hDlg;
     WORD wParam;
     LONG lParam;
{
     switch(wParam)
     {
          case IDOK:
               EndDialog(hDlg,TRUE);
               break;
          case IDCANCEL:
               EndDialog(hDlg,FALSE);
               break;
          default:
               return(FALSE);
     }
     return(TRUE);
}
