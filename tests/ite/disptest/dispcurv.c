#include <windows.h>
#include "DispTest.h"


LONG FAR PASCAL DispCurvProc(hWnd,iMessage,wParam,lParam)
     HWND     hWnd;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern HWND            hIntrDlg;
     extern HANDLE          hInst;
     extern DEVINFO         dcDevCaps;
     extern HDEVOBJECT      hDevPens,hDevBrushes,hDevFonts;
     extern char            szLogFile[];

     FARPROC lpProc;
     HDC     hDC;
     static int nCurveTest;

     switch(iMessage)
     {
          case WM_CREATE:
               nCurveTest = 0;
               WriteLogFile(szLogFile,(LPSTR)"CURVE TEST INITIATED");
               break;

          case WM_TIMER:
               switch(nCurveTest)
               {
                    case 0:
                         hDC = GetDC(hWnd);
                         DisplayEllipses(hWnd,hDC,&dcDevCaps,hDevBrushes,hDevPens,
                              hDevFonts,(LPSTR)&nCurveTest);
                         ReleaseDC(hWnd,hDC);
                         break;
                    default:
                         WriteLogFile(szLogFile,(LPSTR)"CURVE TEST COMPLETED");
                         lpProc = MakeProcInstance((FARPROC)DispTestProc,hInst);
                         SetWindowLong(hWnd,GWL_WNDPROC,(LONG)lpProc);
                         SetWindowText(hWnd,(LPSTR)DISPTESTTITLE);
                         PostMessage(hWnd,WM_USER,2,0l);
                         break;
               }
               break;

          case WM_COMMAND:
               ProcessDispTestCommands(hWnd,wParam);
               break;

          case WM_PAINT:
               PaintDispTestWindow(hWnd);
               break;

          case WM_DESTROY:
               DestroyDispWindow(hWnd);
               PostQuitMessage(0);
               break;

          default:
               return(DefWindowProc(hWnd,iMessage,wParam,lParam));
     }
     return(0L);
}
