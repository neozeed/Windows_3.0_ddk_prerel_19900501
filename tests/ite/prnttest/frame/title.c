/*---------------------------------------------------------------------------*\
| PRINT TITLE PAGE INFORMATION (header)                                       |
|   This module contains the routines necessary to print out the header       |
|   information.                                                              |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _HEADER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 28, 1989 - ported to this DLL.                                 |
|          Aug 22, 1989 - added gray-scale test.                              |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT TITLE PAGE                                                            |
|   This routine prints out the title page information as the first page of   |
|   the header.                                                               |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (Misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPPRINTER         pPrinter  - Printer information structure.              |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPSTR             bAbort    - L-Pointer to printer abort flag.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL bPrintAbort - Printer abort flag.                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything went smoothly.                                  |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintTitlePage(hDC,lpPrinter,lpDevCaps,bAbort)
     HDC       hDC;
     LPPRINTER lpPrinter;
     LPDEVINFO lpDevCaps;
     LPSTR     bAbort;
{
     extern BOOL FAR* bPrintAbort;

     HANDLE     hBuffer;
     LPSTR      lpBuffer;
     WORD       wOldTextAlign;
     TEXTMETRIC tm;
     short      nLC,nHeight,nWidth;
     DATETIME   dtDateTime;

     extern HANDLE hInst;

     bPrintAbort = (BOOL FAR*)bAbort;

     /*-----------------------------------------*\
     | Get text dimensions.                      |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     nHeight = tm.tmHeight+tm.tmExternalLeading;
     nWidth  = tm.tmAveCharWidth;

     /*-----------------------------------------*\
     | Must have a local buffer to store strings.|
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,128)))
          return(FALSE);
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Output Title page information.            |
     \*-----------------------------------------*/
     nLC=0;
     LoadString(hInst,IDS_HEAD_TITLEPAGE,lpBuffer,128);
     wOldTextAlign = GetTextAlign(hDC);
     SetTextAlign(hDC,TA_CENTER | TA_NOUPDATECP);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     GetSystemDateTime(&dtDateTime);
     wsprintf(lpBuffer,"%02d/%02d/%d",dtDateTime.bMonth,dtDateTime.bDay,dtDateTime.wYear);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,"%02d:%02d:%02d",dtDateTime.bHours,dtDateTime.bMinutes,dtDateTime.bSeconds);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     nLC+=10;
     SetTextAlign(hDC,wOldTextAlign);
     lstrcpy(lpBuffer,"Printer Name   - ");
     lstrcat(lpBuffer,lpPrinter->szName);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,"Printer Driver - ");
     lstrcat(lpBuffer,lpPrinter->szDriver);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,"Printer Port   - ");
     lstrcat(lpBuffer,lpPrinter->szPort);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,"Test Profile   - ");
     lstrcat(lpBuffer,lpPrinter->szProfile);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,"Window Version - ");
     lstrcat(lpBuffer,lpPrinter->szSystemVer);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,"Driver Version - ");
     lstrcat(lpBuffer,lpPrinter->szDriverVer);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     /*-----------------------------------------*\
     | Print footer at bottom of page.           |
     \*-----------------------------------------*/
     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT PRINTABLE AREA OF DEVICE (header)                                     |
|   This routine outputs a rectangle to the printer indicating the device     |
|   boundries.                                                                |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (Misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPSTR             bAbort    - L-Pointer to printer abort flag.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL bPrintAbort - Printer abort flag.                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything went smoothly.                                  |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintPrintableArea(hDC,lpDevCaps,bAbort)
     HDC       hDC;
     LPDEVINFO lpDevCaps;
     LPSTR     bAbort;
{
     extern BOOL FAR* bPrintAbort;

     WORD   wOldTextAlign;
     HANDLE hBuffer;
     LPSTR  lpBuffer;

     extern HANDLE hInst;
     bPrintAbort = (BOOL FAR*)bAbort;

     /*-----------------------------------------*\
     | Must have a local buffer to store strings.|
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,128)))
          return(FALSE);
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Output the border-rect to the device.     |
     \*-----------------------------------------*/
     Rectangle(hDC,0,0,lpDevCaps->nHorzRes,lpDevCaps->nVertRes);
     MoveTo(hDC,0,0);
     LineTo(hDC,lpDevCaps->nHorzRes,lpDevCaps->nVertRes);
     MoveTo(hDC,lpDevCaps->nHorzRes,0);
     LineTo(hDC,0,lpDevCaps->nVertRes);

     /*-----------------------------------------*\
     | Display Centered text comment.            |
     \*-----------------------------------------*/
     LoadString(hInst,IDS_HEAD_PRINTAREA,lpBuffer,128);
     wOldTextAlign = GetTextAlign(hDC);
     SetTextAlign(hDC,TA_BASELINE | TA_CENTER | TA_NOUPDATECP);
     TextOut(hDC,lpDevCaps->nHorzRes/2,lpDevCaps->nVertRes/2,lpBuffer,lstrlen(lpBuffer));
     SetTextAlign(hDC,wOldTextAlign);

     /*-----------------------------------------*\
     | Print out a 2x2 inch box to exemplify the |
     | logical pixels/inch spacing.              |
     \*-----------------------------------------*/
     Rectangle(hDC,(lpDevCaps->nHorzRes/2)-lpDevCaps->nLogPixelsX,
                   lpDevCaps->nLogPixelsY,
                   (lpDevCaps->nHorzRes/2)+lpDevCaps->nLogPixelsX,
                   lpDevCaps->nLogPixelsY*3);
     LoadString(hInst,IDS_HEAD_PRINTAREA1,lpBuffer,128);
     wOldTextAlign = GetTextAlign(hDC);
     SetTextAlign(hDC,TA_BASELINE | TA_CENTER | TA_NOUPDATECP);
     TextOut(hDC,lpDevCaps->nHorzRes/2,lpDevCaps->nLogPixelsY*2,lpBuffer,lstrlen(lpBuffer));
     SetTextAlign(hDC,wOldTextAlign);

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     /*-----------------------------------------*\
     | Don't print footer, force a page.         |
     \*-----------------------------------------*/
     if(!PrintFooter(hDC,lpDevCaps,NULL))
          return(FALSE);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT GRAY SCALE (header)                                                   |
|   This routine outputs a rectangle with the gray-scale for the printer.     |
|   The gray-scale ranges (1,1,1) -> (255,255,255).                           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (Misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPSTR             bAbort    - L-Pointer to printer abort flag.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL bPrintAbort - Printer abort flag.                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything went smoothly.                                  |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintGrayScale(hDC,lpDevCaps,bAbort)
     HDC       hDC;
     LPDEVINFO lpDevCaps;
     LPSTR     bAbort;
{
     extern BOOL FAR* bPrintAbort;

     short nHeight;

     bPrintAbort = (BOOL FAR*)bAbort;

     nHeight = PrintTestDescription(hDC,IDS_TST_HEAD_GRAY,lpDevCaps);

     TstGrayScale(hDC,0,nHeight+(lpDevCaps->nLogPixelsY/2),
          lpDevCaps->nHorzRes,lpDevCaps->nLogPixelsY*2);

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}

/*---------------------------------------------------------------------------*\
| PRINT SUPPORTED FUNCTIONS                                                   |
|   This routine outputs a rectangle with the gray-scale for the printer.     |
|   The gray-scale ranges (1,1,1) -> (255,255,255).                           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (Misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to device context.                   |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|   LPSTR             bAbort    - L-Pointer to printer abort flag.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL bPrintAbort - Printer abort flag.                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything went smoothly.                                  |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintFunctionSupport(hDC,pPrinter,lpDevCaps,bAbort)
     HDC       hDC;
     LPPRINTER pPrinter;
     LPDEVINFO lpDevCaps;
     LPSTR     bAbort;
{
     extern BOOL FAR* bPrintAbort;

     short      nLine,nHeight,nIdx;
     HANDLE     hLibrary;
     char       lpBuffer[80];
     TEXTMETRIC tm;

     static PSTR pSupport[] = {"Enable"           ,"Disable"      ,
                               "Control"          ,"DeviceMode"   ,
                               "BitBlt"           ,"StrBlt"       ,
                               "ExtTextOut"       ,"StretchBlt"   ,
                               "FastBorder"       ,"Output"       ,
                               "Pixel"            ,"ScanLR"       ,
                               "ColorInfo"        ,"EnumObj"      ,
                               "EnumDFonts"       ,"GetCharWidth" ,
                               "DeviceBitmap"     ,"RealizeObject",
                               "SetAttribute"     ,"ExtDeviceMode",
                               "DeviceCapabilities"};

     bPrintAbort = (BOOL FAR*)bAbort;
     nHeight = PrintTestDescription(hDC,IDS_TST_HEAD_FUNC,lpDevCaps);

     GetTextMetrics(hDC,&tm);
     nLine = tm.tmHeight+tm.tmExternalLeading;
     lstrcpy(lpBuffer,pPrinter->szDriver);
     lstrcat(lpBuffer,".DRV");
     AnsiUpper(lpBuffer);
     if((hLibrary = LoadLibrary(lpBuffer)) < 32)
          return(FALSE);

     for(nIdx=0; nIdx < 21; nIdx++)
     {
/*        lstrcpy(lpBuffer,pSupport[nIdx]);
*/        wsprintf(lpBuffer,"%20s",(LPSTR)pSupport[nIdx]);
          if(GetProcAddress(hLibrary,pSupport[nIdx]))
          {
               lstrcat(lpBuffer," - Supported");
               TextOut(hDC,5,(nLine*nIdx)+nHeight,lpBuffer,lstrlen(lpBuffer));
          }
          else
          {
               lstrcat(lpBuffer," - Not Supported");
               TextOut(hDC,5,(nLine*nIdx)+nHeight,lpBuffer,lstrlen(lpBuffer));
          }

     }
     FreeLibrary(hLibrary);

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}

