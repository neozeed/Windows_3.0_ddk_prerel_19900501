/*---------------------------------------------------------------------------*\
| DESTROY WINDOW                                                              |
|   This module contains the routines necessary to handle the WM_DESTROY      |
|   message for the application.  It paints the client area.                  |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Aug 03, 1989 - created.                                            |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "DispTest.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| DESTROY ALL WINDOW OBJECTS                                                  |
|   This routine Destroys the client application.                             |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hWnd - Handle to the application client window.                      |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL : True indicates the message was processed, otherwise a FALSE        |
|          is returned.                                                       |
\*---------------------------------------------------------------------------*/
BOOL FAR DestroyDispWindow(hWnd)
     HWND hWnd;
{
     extern HWND       hIntrDlg;
     extern HDEVOBJECT hDevPens,hDevBrushes,hDevFonts;
     extern WORD       wTestsSet;

     LOCALHANDLE hBuffer;
     LPSTR       lpBuffer;

     if(hBuffer = LocalAlloc(LHND,128))
     {
          if(lpBuffer = LocalLock(hBuffer))
          {
               GetDlgItemText(hIntrDlg,IDD_INTR_LOGE,lpBuffer,128);
               WritePrivateProfileString((LPSTR)"StartUp",
                    (LPSTR)"LogFile",lpBuffer,(LPSTR)"DispTest.ini");
               GetDlgItemText(hIntrDlg,IDD_INTR_MSTE,lpBuffer,128);
               WritePrivateProfileString((LPSTR)"StartUp",
                    (LPSTR)"MstFile",lpBuffer,(LPSTR)"DispTest.ini");
               GetDlgItemText(hIntrDlg,IDD_INTR_CPYE,lpBuffer,128);
               WritePrivateProfileString((LPSTR)"StartUp",
                    (LPSTR)"CpyFile",lpBuffer,(LPSTR)"DispTest.ini");
               wsprintf(lpBuffer,"%u",wTestsSet);
               WritePrivateProfileString((LPSTR)"StartUp",
                    (LPSTR)"TestsTest",lpBuffer,(LPSTR)"DispTest.ini");
               LocalUnlock(hBuffer);
          }
          LocalFree(hBuffer);
     }

     FreeDeviceObjects(hDevPens);
     FreeDeviceObjects(hDevBrushes);
     FreeDeviceObjects(hDevFonts);
     DestroyWindow(hIntrDlg);

     return(TRUE);
}
