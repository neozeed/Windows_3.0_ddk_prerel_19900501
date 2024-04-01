/*---------------------------------------------------------------------------*\
| MISC ROUTINES                                                               |
|   This module contains miscellaneous routines which can be used throughout  |
|   the entire application.  These functions are more than not, common to     |
|   more than one segment.  It is desirable to place these routines in a      |
|   segment which is always loaded.                                           |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 22, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT FOOTER LINE                                                           |
|   This routine is used to print a footer-message at the bottom of each page |
|   of the test output.  The test will pass a string to the routine which     |
|   identifys the test being output.  If szCaption is NULL, then no footer    |
|   is to be printed.                                                         |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to printer device context.           |
|   LPDEVCAPS         lpDevCaps - Device Capabilities structure.              |
|   LPSTR             szCaption - String to append to footer.                 |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   BOOL bPrintAbort - Printer abort flag.                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything is OK, FALSE if error occured.                  |
\*---------------------------------------------------------------------------*/
BOOL FAR PrintFooter(hDC,dcDevCaps,szCaption)
     HDC       hDC;
     LPDEVINFO dcDevCaps;
     LPSTR     szCaption;
{
     extern BOOL FAR* bPrintAbort;

     WORD       wOldTextAlign;                   /* Old text alignment.      */
     TEXTMETRIC tm;                              /* Text metric structure.   */
     HFONT      hFont,hOldFont;                  /* Handles to fonts.        */
     HBRUSH     hOldBrush;
     LOGFONT    lfFont;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;

     /*-----------------------------------------*\
     | If no Caption, then do not print footer.  |
     \*-----------------------------------------*/
     if(!szCaption)
     {
          if((Escape(hDC,NEWFRAME,0,NULL,NULL) <= 0) || *bPrintAbort)
               return(FALSE);
          return(TRUE);
     }

     /*-----------------------------------------*\
     | Create a font based on the stock font.    |
     \*-----------------------------------------*/
     hFont = GetStockObject(ANSI_VAR_FONT);
     GetObject(hFont,sizeof(lfFont),(LPSTR)&lfFont);
     lfFont.lfWeight = 700;
     lfFont.lfItalic = 1;
     hOldFont = SelectObject(hDC,CreateFontIndirect(&lfFont));

     hOldBrush = SelectObject(hDC,GetStockObject(NULL_BRUSH));

     /*-----------------------------------------*\
     | Get the test string for the test.         |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,128)))
          return(FALSE);
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }
     /*-----------------------------------------*\
     | Create the footer string.  Then output to |
     | the printer at the bottom of the page.    |
     | Make sure to restore the old text align.  |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     lstrcpy(lpBuffer,"Printer ->");
     lstrcat(lpBuffer,szCaption);
     wOldTextAlign = GetTextAlign(hDC);
     SetTextAlign(hDC,TA_CENTER | TA_NOUPDATECP | TA_BOTTOM);
     TextOut(hDC,dcDevCaps->nHorzRes/2,dcDevCaps->nVertRes-1,
             lpBuffer,lstrlen(lpBuffer));
     SetTextAlign(hDC,wOldTextAlign);

     /*-----------------------------------------*\
     | Release the font array, and reselect the  |
     | previous font into the DC.  Do a NEWFRAME |
     | to print the page.                        |
     \*-----------------------------------------*/
     DeleteObject(SelectObject(hDC,hOldFont));

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     if((Escape(hDC,NEWFRAME,0,NULL,NULL) <= 0) || *bPrintAbort)
          return(FALSE);

     /*-----------------------------------------*\
     | Reselect the brush and pen.  This is due  |
     | to the fact that the ESCAPE(NEWFRAME) does|
     | a save and restore DC.  (all is lost)     |
     \*-----------------------------------------*/
     SelectObject(hDC,hOldBrush);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT TEST DESCRIPTION                                                      |
|   This routine uses DrawText to output the test description to the printer  |
|   device.                                                                   |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC  hDC         - Handle to printer device context.                      |
|   WORD wTestString - Text ID string in resource file.                       |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hInst - Handle to application instance.                            |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
/*---------------------------------------------------------------------------*/
int FAR PrintTestDescription(hDC,wTestString,lpDevCaps)
     HDC       hDC;
     WORD      wTestString;
     LPDEVINFO lpDevCaps;
{
     extern HANDLE hInst;

     char    szTextRes[20];                      /* Temporary buffer for res */
     HANDLE  hResource;                          /* Handle of text resource  */
     LPSTR   lpText,lpEnd;                       /* Pointers to text         */
     RECT    rRect;                              /* Outputting rectangle     */
     HFONT   hFont,hOldFont;                     /* Handles to font structs  */
     LOGFONT lfFont;                            /* Pointer to logical font  */
     int     iHeight;                            /* Height of text outputted */

     /*-----------------------------------------*\
     | Get text resource for the description.    |
     \*-----------------------------------------*/
     LoadString(hInst,wTestString,(LPSTR)szTextRes,sizeof(szTextRes));
     if(!(hResource = LoadResource(hInst,FindResource(hInst,szTextRes,"TEXT"))))
          return(FALSE);
     if(!(lpText = LockResource(hResource)))
     {
          FreeResource(hResource);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | NULL terminate the text resource.         |
     \*-----------------------------------------*/
     lpEnd = lpText;
     while((*lpEnd != '\0') && (*lpEnd != '\x1A'))
          lpEnd++;
     *lpEnd = '\0';

     /*-----------------------------------------*\
     | Create a font based on the stock font.    |
     \*-----------------------------------------*/
     hFont = GetStockObject(ANSI_VAR_FONT);
     GetObject(hFont,sizeof(lfFont),(LPSTR)&lfFont);
     lfFont.lfWeight = 700;
     hOldFont = SelectObject(hDC,CreateFontIndirect(&lfFont));

     /*-----------------------------------------*\
     | Output the text to the printer DC.        |
     \*-----------------------------------------*/
     SetRect(&rRect,0,0,lpDevCaps->nHorzRes,lpDevCaps->nVertRes);
     iHeight = DrawText(hDC,lpText,-1,&rRect,DT_LEFT | DT_WORDBREAK | DT_EXTERNALLEADING);

     /*-----------------------------------------*\
     | Free up structures and resources.         |
     \*-----------------------------------------*/
     DeleteObject(SelectObject(hDC,hOldFont));
     UnlockResource(hResource);
     FreeResource(hResource);

     return(iHeight);
}


/*---------------------------------------------------------------------------*\
| TEST FOR END-OF-PAGE                                                        |
|   This routine tests for the end of a page depending on the number of lines |
|   already printed.  It determines whether a NEWFRAME should be sent to the  |
|   printer by checking if the start can handle the number of lines to print. |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   short nStartLine - The starting index from which to print.                |
|   short nNumLines  - The number of lines to print beginning at start.       |
|   short nMaxLines  - The number of lines/page.                              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE indicates that EndOfPage has been exceeded.  FALSE says it    |
|          didn't exceed End of Page.                                         |
\*---------------------------------------------------------------------------*/
BOOL FAR EndOfPage(nStartLine,nNumLines,nMaxLines)
     short nStartLine;
     short nNumLines;
     short nMaxLines;
{
     if((nStartLine+nNumLines) > nMaxLines)
          return(TRUE);
     return(FALSE);
}
