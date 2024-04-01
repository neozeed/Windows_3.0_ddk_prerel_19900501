/*---------------------------------------------------------------------------*\
| PRINTER SPECIFIC MODULE                                                     |
|                                                                             |
| STRUCTURE (----)                                                            |
|                                                                             |
| FUNCTION EXPORTS                                                            |
|   GetPrinterDC()                                                            |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 08, 1990                                                   |
| SEGMENT: _TEXT                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include <drivinit.h>
#include "isg_test.h"

HDC FAR PASCAL GetPrinterDC(hWnd,lpProfile)
     HWND      hWnd;
     LPSTR     lpProfile;
{
     char      szPrinterLine[80];
     char      szModule[80];
     LPSTR     szType,szDriver,szPort,szTemp;
     LPDEVMODE lpDevMode;
     HANDLE    hDevMode,hLibrary;
     short     nSize;
     FARPROC   lpProc;
     HDC       hDC;

     /*-----------------------------------------*\
     | Initialize pointer variables for the      |
     | processing of the printer strings.        |
     \*-----------------------------------------*/
     GetPrivateProfileString("Windows","Device","",szPrinterLine,sizeof(szPrinterLine),lpProfile);
     szTemp = szType = szPrinterLine;
     szDriver = szPort = NULL;

     /*-----------------------------------------*\
     | Get printer device strings.  This will    |
     | parse the device string so that the NAME  |
     | DRIVER and PORT strings can be identified |
     | seperately.                               |
     \*-----------------------------------------*/
     while(*szTemp)
     {
          if(*szTemp == ',')
          {
               *szTemp++ = 0;
               while(*szTemp == ' ')
                    szTemp++;
               if(!szDriver)
                    szDriver = szTemp;
               else
               {
                    szPort = szTemp;
                    break;
               }
          }
          else
               szTemp++;
     }

     /*-----------------------------------------*\
     | Modify the driver string to include the   |
     | .drv extension for the LoadLibrary().     |
     \*-----------------------------------------*/
     lstrcpy((LPSTR)szModule,szDriver);
     lstrcat((LPSTR)szModule,(LPSTR)".DRV");
     if((hLibrary = LoadLibrary(szModule)) < 32)
          return(NULL);

     /*-----------------------------------------*\
     | Attempt to get ProcAddress of ExtDevMode. |
     | If it doesn't exist, then we must use the |
     | call to the DevMode to create the DC.     |
     \*-----------------------------------------*/
     if(!(lpProc = GetProcAddress(hLibrary,(LPSTR)"ExtDeviceMode")))
     {
          if(!(lpProc = GetProcAddress(hLibrary,(LPSTR)"DeviceMode")))
          {
               FreeLibrary(hLibrary);
               return(NULL);
          }
          (*lpProc)((HWND)hWnd,(HANDLE)hLibrary,(LPSTR)szType,(LPSTR)szPort);
          FreeLibrary(hLibrary);

          return(CreateDC(szDriver,szType,szPort,(LPSTR)NULL));
     }

     /*-----------------------------------------*\
     | Get the size of the DevMode Structure.    |
     \*-----------------------------------------*/
     nSize = (short)(*lpProc)((HWND)hWnd,(HANDLE)hLibrary,(LPSTR)NULL,(LPSTR)szType,(LPSTR)szPort,
                  (LPSTR)NULL,(LPSTR)lpProfile,0);

     /*-----------------------------------------*\
     | Allocate space for the devicemode.  Call  |
     | the ExtDeviceMode() for settings.         |
     \*-----------------------------------------*/
     hDevMode = LocalAlloc(LHND,(WORD)nSize);
     lpDevMode = (LPDEVMODE)LocalLock(hDevMode);
     (*lpProc)((HWND)hWnd,(HANDLE)hLibrary,(LPSTR)lpDevMode,(LPSTR)szType,(LPSTR)szPort,
              (LPSTR)NULL,(LPSTR)lpProfile,DM_OUT_BUFFER);
     FreeLibrary(hLibrary);

     if(!(hDC = CreateDC(szDriver,szType,szPort,(LPSTR)lpDevMode)))
     {
          LocalUnlock(hDevMode);
          LocalFree(hDevMode);
          return(NULL);
     }
     LocalUnlock(hDevMode);
     LocalFree(hDevMode);

     return(hDC);
}
