/*---------------------------------------------------------------------------*\
| GET DEVICE INFORMATION                                                      |
|   This module contains the routines necessary to retrieve the device        |
|   capabilities for a particular driver.  It will retrieve the following     |
|   device capabilities:                                                      |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _INFO                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| GET PRINTER INFORMATION                                                     |
|   This routine retrieves the printer information concerning the device.     |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC       hDC       - Handle to a printer device context.                 |
|   LPPRINTER lpPrinter - Long pointer to printer information structure.      |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL FAR GetPrinterInformation(hDC,pPrinter,szProfile,szString)
     HDC       hDC;
     LPPRINTER pPrinter;
     LPSTR     szProfile;
     LPSTR     szString;
{
     LPSTR     lpBuffer;
     HANDLE    hLibrary;
     FARPROC   lpProc;
     char      szModule[80];

     lstrcpy(pPrinter->szProfile,szProfile);

     lpBuffer = szString;
     lstrcpy(pPrinter->szName,lpBuffer);
     while(*lpBuffer++ != 0);
     lstrcpy(pPrinter->szDriver,lpBuffer);
     while(*lpBuffer++ != 0);
     lstrcpy(pPrinter->szPort,lpBuffer);
     wsprintf(pPrinter->szDriverVer,"%#X",GetDeviceCaps(hDC,DRIVERVERSION));
     wsprintf(pPrinter->szSystemVer,"%d:%d",LOBYTE(GetVersion()),HIBYTE(GetVersion));

     /*-----------------------------------------*\
     | Get Device Capabilities part of struct.   |
     \*-----------------------------------------*/
     lstrcpy((LPSTR)szModule,pPrinter->szDriver);
     lstrcat((LPSTR)szModule,(LPSTR)".DRV");

     if((hLibrary = LoadLibrary(szModule)) < 32)
          return(FALSE);

     /*-----------------------------------------*\
     | Call the DeviceCapabilities from driver.  |
     \*-----------------------------------------*/
/*   if(!(lpProc = GetProcAddress(hLibrary,(LPSTR)"DeviceCapabilities")))
     {
          FreeLibrary(hLibrary);
          return(FALSE);
     }

     (*lpProc)((LPSTR)pPrinter->szName,(LPSTR)pPrinter->szPort,
          (LPSTR)pPrinter->szSystemVer,DC_VERSION,NULL);
     (*lpProc)((LPSTR)pPrinter->szName,(LPSTR)pPrinter->szPort,
          (LPSTR)pPrinter->szDriverVer,DC_DRIVER,NULL);
*/
     FreeLibrary(hLibrary);

     return(TRUE);
}
