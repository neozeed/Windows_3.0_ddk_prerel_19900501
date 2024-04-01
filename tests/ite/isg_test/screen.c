/*---------------------------------------------------------------------------*\
| SCREEN MODULE                                                               |
|   This module contains routines associated with screen captures and screen  |
|   grabs.                                                                    |
|                                                                             |
| STRUCTURE (SCREENS)                                                         |
|                                                                             |
| FUNCTION EXPORTS METHODS                                                    |
|   ClearScreen()                                                             |
|   LoadScreen()                                                              |
|   SaveScreen()                                                              |
|   CaptureScreen()                                                           |
|   CompareScreens()                                                          |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 11, 1990                                                   |
| SEGMENT: _TEXT                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"

/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
void FAR PASCAL ClearScreen(hWnd)
     HWND hWnd;
{
     RECT rRect;
     HDC  hDC;

     GetClientRect(hWnd,&rRect);
     hDC = GetDC(hWnd);
     FillRect(hDC,&rRect,GetClassWord(hWnd,GCW_HBRBACKGROUND));
     ReleaseDC(hWnd,hDC);

     return;
}

/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
HANDLE FAR PASCAL LoadScreen(hFile,nScreen)
     int hFile;
     int nScreen;
{
     BITMAPINFOHEADER   bmih;
     SCREENFILEHEADER   sfh;
     HANDLE             hdib;
     LPBITMAPINFOHEADER lpdib;
     int                nIdx,nColorSize;

     /*-----------------------------------------*\
     | Read in the screen header and determine   |
     | if screen is in the file.                 |
     \*-----------------------------------------*/
     ReadFile(hFile,(LPSTR)&sfh,(DWORD)sizeof(SCREENFILEHEADER));
     if(nScreen > sfh.wScreens)
          return(NULL);

     /*-----------------------------------------*\
     | Get to the screen.  File pointer should   |
     | be pointing to first DIB.                 |
     \*-----------------------------------------*/
     for(nIdx=0; nIdx < (nScreen-1); nIdx++)
     {
          ReadFile(hFile,(LPSTR)&bmih,(DWORD)sizeof(BITMAPINFOHEADER));
          nColorSize = GetColorTableSize(&bmih);
          _llseek(hFile,(DWORD)nColorSize+bmih.biSizeImage,1);
     }

     /*-----------------------------------------*\
     | Read in dib.                              |
     \*-----------------------------------------*/
     ReadFile(hFile,(LPSTR)&bmih,(DWORD)sizeof(BITMAPINFOHEADER));
     nColorSize = GetColorTableSize(&bmih);
     hdib = GlobalAlloc(GHND,(DWORD)sizeof(BITMAPINFOHEADER)+nColorSize+bmih.biSizeImage);
     lpdib = (LPVOID)GlobalLock(hdib);
     *lpdib = bmih;
     ReadFile(hFile,(LPSTR)lpdib+sizeof(BITMAPINFOHEADER),(DWORD)nColorSize+bmih.biSizeImage);
     GlobalUnlock(hdib);

     return(hdib);
}

/*---------------------------------------------------------------------------*\
| SAVE SCREEN                                                                 |
|   This routine saves a DIB to file.  It saves the screen at the end of the  |
|   file.                                                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   - none -                                                                  |
|                                                                             |
| RETURNS                                                                     |
|   int - Screen Number in File.                                              |
\*---------------------------------------------------------------------------*/
int FAR PASCAL SaveScreen(hdib,szFile)
     HANDLE hdib;
     LPSTR  szFile;
{
     int                hFile,nColorTableSize;
     SCREENFILEHEADER   sfh;
     LPBITMAPINFOHEADER lpdib;

     /*-----------------------------------------*\
     | Open the file.  If it doesn't exist, then |
     | create it and initialize the Screen       |
     | header.                                   |
     \*-----------------------------------------*/
     if((hFile = _lopen(szFile,OF_READWRITE)) < 0)
     {
          if((hFile = _lcreat(szFile,0)) < 0)
               return(-1);

          sfh.wScreens   = 0;
          sfh.wDIBFormat = DIB_WIN;
          _llseek(hFile,0l,0);
          _lwrite(hFile,(LPSTR)&sfh,sizeof(SCREENFILEHEADER));
     }
     _llseek(hFile,0l,0);
     ReadFile(hFile,(LPSTR)&sfh,(DWORD)sizeof(SCREENFILEHEADER));

     /*-----------------------------------------*\
     | Write (append) the dib info.              |
     \*-----------------------------------------*/
     if(!(lpdib = (LPVOID)GlobalLock(hdib)))
     {
          _lclose(hFile);
          return(-1);
     }
     nColorTableSize = GetColorTableSize((LPBITMAPINFOHEADER)lpdib);
     _llseek(hFile,0l,2);
     WriteFile(hFile,(LPSTR)lpdib,(DWORD)sizeof(BITMAPINFOHEADER)+nColorTableSize+lpdib->biSizeImage);

     /*-----------------------------------------*\
     | Output the header info.  Save file        |
     \*-----------------------------------------*/
     sfh.wScreens++;
     _llseek(hFile,0l,0);
     WriteFile(hFile,(LPSTR)&sfh,(DWORD)sizeof(SCREENFILEHEADER));
     _llseek(hFile,0l,2);
     _lclose(hFile);

     return(sfh.wScreens);
}


/*---------------------------------------------------------------------------*\
| CAPTURE SCREEN                                                              |
|   This routine takes a picture of the Client Area or entire display         |
|   depending upon the value of hDC.  if hDC is NULL, then the entire display |
|   will be captured.  The screen is captured as a Device Independant Bitmap. |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   - none -                                                                  |
|                                                                             |
| RETURNS                                                                     |
|   HANDLE - Handle to a DIB.                                                 |
\*---------------------------------------------------------------------------*/
HANDLE FAR PASCAL CaptureScreen(hWnd,hDC)
     HWND hWnd;
     HDC  hDC;
{
     HDC     hMemDC;
     HBITMAP hbm;
     HANDLE  hdib;
     RECT    rRect;
     BITMAP  bm;
     DWORD   dwCompress;

     /*-----------------------------------------*\
     |                                           |
     \*-----------------------------------------*/
     if(hDC)
          GetClientRect(hWnd,&rRect);
     else
          SetRect(&rRect,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));

     if(!(hMemDC  = CreateCompatibleDC(hDC)))
          return(NULL);

     if(!(hbm = CreateCompatibleBitmap(hDC,rRect.right,rRect.bottom)))
     {
          DeleteDC(hMemDC);
          return(NULL);
     }
     SelectObject(hMemDC,hbm);

     if(!BitBlt(hMemDC,0,0,rRect.right,rRect.bottom,hDC,0,0,SRCCOPY))
     {
          DeleteDC(hMemDC);
          return(NULL);
     }
     DeleteDC(hMemDC);

     /*-----------------------------------------*\
     |                                           |
     \*-----------------------------------------*/
     GetObject(hbm,sizeof(BITMAP),(LPSTR)&bm);
     switch(bm.bmBitsPixel)
     {
          case 4:
              dwCompress = BI_RLE4;
              break;
          case 8:
              dwCompress = BI_RLE8;
              break;
          default:
              dwCompress = BI_RGB;
              break;
     }
     hdib = ConvertDDBToDIB(hDC,hbm,DIB_WIN,dwCompress);

     return(hdib);
}

/*---------------------------------------------------------------------------*\
|                                                                             |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL CompareScreens(hMast,hCopy)
     HANDLE hMast;
     HANDLE hCopy;
{
     return(TRUE);
}
