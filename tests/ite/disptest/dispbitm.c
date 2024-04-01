/*---------------------------------------------------------------------------*\
| BITMAPS TEST MODULE (Display Test)                                          |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - created.                                            |
|                                                                             |
\*---------------------------------------------------------------------------*/
#include <windows.h>
#include "DispTest.h"


LONG FAR PASCAL DispBitmProc(hWnd,iMessage,wParam,lParam)
     HWND     hWnd;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern HANDLE     hInst;
     extern DEVINFO    dcDevCaps;
     extern HDEVOBJECT hDevPens,hDevBrushes,hDevFonts;
     extern char       szLogFile[];

     FARPROC lpProc;
     HDC     hDC;
     RECT    rRect;
     static int nBitmapTest;

     switch(iMessage)
     {
          case WM_CREATE:
               nBitmapTest = 0;
               WriteLogFile(szLogFile,(LPSTR)"BITMAP TEST INITIATED");
               break;

          case WM_TIMER:
               switch(nBitmapTest)
               {
                    case 0:
                         hDC = GetDC(hWnd);
                         DisplayBitmaps(hWnd,hDC,&dcDevCaps,hDevBrushes,hDevPens,
                              hDevFonts,(LPSTR)&nBitmapTest);
                         ReleaseDC(hWnd,hDC);
                         break;
                    case 1:
                         hDC = GetDC(hWnd);
                         GetClientRect(hWnd,&rRect);
                         FillRect(hDC,&rRect,GetClassWord(hWnd,GCW_HBRBACKGROUND));
                         InflateRect(&rRect,-10,-10);
                         TstGrayScale(hDC,rRect.left,rRect.top,
                              rRect.right-rRect.left,rRect.bottom-rRect.top);
                         ReleaseDC(hWnd,hDC);
                         nBitmapTest++;
                         break;
                    case 2:
                         hDC = GetDC(hWnd);
                         DisplayColorMapping(hWnd,hDC,&dcDevCaps,(LPSTR)&nBitmapTest);
                         ReleaseDC(hWnd,hDC);
                         break;

                    default:
                         WriteLogFile(szLogFile,(LPSTR)"BITMAP TEST COMPLETED");
                         lpProc = MakeProcInstance((FARPROC)DispTestProc,hInst);
                         SetWindowLong(hWnd,GWL_WNDPROC,(LONG)lpProc);
                         SetWindowText(hWnd,(LPSTR)DISPTESTTITLE);
                         PostMessage(hWnd,WM_USER,1,0l);
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