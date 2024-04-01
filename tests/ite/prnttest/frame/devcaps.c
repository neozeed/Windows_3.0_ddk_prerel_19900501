/*---------------------------------------------------------------------------*\
| DEVICE CAPABILITIES                                                         |
|   This module contains routines for outputing the device capabilities of    |
|   of the device.                                                            |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _HEADER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 27, 1989 - ported to this DLL.                                 |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "PrntFram.h"                            /* Program Header File      */

/*---------------------------------------------------------------------------*\
| PRINT THE DEVICE CAPABILITIES                                               |
|   This routine prints the device capabilities to the printer.  It shows the |
|   values in which other applications can use in deriving the printer        |
|   characteristics.                                                          |
|                                                                             |
| CALLED ROUTINES                                                             |
|   EndOfPage()   - (misc.c)                                                  |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to printer device context.           |
|   LPDEVCAPS         dcDevCaps - Device capabilities structure.              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if successful.                                                |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintDeviceCapabilities(hDC,lpDevCaps,bAbort)
     HDC       hDC;
     LPDEVINFO lpDevCaps;
     LPSTR             bAbort;
{
     extern BOOL FAR PrintFooter(HDC,LPDEVINFO,LPSTR);
     extern BOOL FAR EndOfPage(short,short,short);

     extern BOOL FAR* bPrintAbort;

     static PSTR szTechnology[] = {"VECTOR PLOTTER"  ,"RASTER DISPLAY",
                                   "RASTER PRINTER"  ,"RASTER CAMERA",
                                   "CHARACTER STREAM","METAFILE",
                                   "DISPLAY FILE"};

     static PRINTCAPS pcClipCaps[] = {CP_RECTANGLE,"Can Clip to Rectangle - "};


     static PRINTCAPS pcRasterCaps[] =  {RC_BITBLT      , "Capable of transfering Bitmaps  - ",
                                         RC_BANDING     , "Requires banding support        - ",
                                         RC_SCALING     , "Capable of scaling              - ",
                                         RC_BITMAP64    , "Can support bitmaps > 64K       - ",
                                         RC_GDI20_OUTPUT, "Has 2.0 output calls            - ",
                                         RC_DI_BITMAP   , "Supports DIB to Memory          - ",
                                         RC_PALETTE     , "Supports a Palette              - ",
                                         RC_DIBTODEV    , "Supports DIBitsToDevice         - ",
                                         RC_BIGFONT     , "Supports Fonts > 64K            - ",
                                         RC_STRETCHBLT  , "Supports StrecthBlt             - ",
                                         RC_FLOODFILL   , "Supports Flood Filling          - "};

     static PRINTCAPS pcCurveCaps[] =   {CC_NONE      , "Curves Not Supported            - ",
                                         CC_CIRCLES   , "Device can do Circles           - ",
                                         CC_PIE       , "Device can do Pie-Wedges        - ",
                                         CC_CHORD     , "Device can do Chord-Arcs        - ",
                                         CC_ELLIPSES  , "Device can do Ellipses          - ",
                                         CC_WIDE      , "Device can do Wide Lines        - ",
                                         CC_STYLED    , "Device can do Styled Borders    - ",
                                         CC_WIDESTYLED, "Device can do Wide Borders      - ",
                                         CC_INTERIORS , "Device can do Interiors         - "};

     static PRINTCAPS pcLineCaps[] =    {LC_NONE      , "Lines Not Supported             - ",
                                         LC_POLYLINE  , "Device can do Poly Lines        - ",
                                         LC_MARKER    , "Device can do Markers           - ",
                                         LC_POLYMARKER, "Device can do Poly Markers      - ",
                                         LC_WIDE      , "Device can do Wide Lines        - ",
                                         LC_STYLED    , "Device can do Styled Lines      - ",
                                         LC_WIDESTYLED, "Device can do Wide-Styled Lines - ",
                                         LC_INTERIORS , "Device can do Interiors         - "};

     static PRINTCAPS pcPolygonCaps[] = {PC_NONE      , "Polygonals Not Supported        - ",
                                         PC_POLYGON   , "Device can do Polygons          - ",
                                         PC_RECTANGLE , "Device can do Rectangles        - ",
                                         PC_TRAPEZOID , "Device can do Poly Trapezoids   - ",
                                         PC_SCANLINE  , "Device can do Wide ScanLines    - ",
                                         PC_WIDE      , "Device can do Wide Lines        - ",
                                         PC_STYLED    , "Device can do Styled Lines      - ",
                                         PC_WIDESTYLED, "Device can do Wide-Styled Lines - ",
                                         PC_INTERIORS , "Device can do Interiors         - "};

     static PRINTCAPS pcTextCaps[] =    {TC_OP_CHARACTER, "Device can do Character Output Precision      - ",
                                         TC_OP_STROKE   , "Device can do Stroke Output Precision         - ",
                                         TC_CP_STROKE   , "Device can do Stroke Clip Precision           - ",
                                         TC_CR_90       , "Device can do 90-degree Character Rotation    - ",
                                         TC_CR_ANY      , "Device can do any Character Rotation          - ",
                                         TC_SF_X_YINDEP , "Device can do Scaling Independent of X and Y  - ",
                                         TC_SA_DOUBLE   , "Device can do Doubled Character  or scaling   - ",
                                         TC_SA_INTEGER  , "Device can do Integer Multiples for scaling   - ",
                                         TC_SA_CONTIN   , "Device can do any Multiples for exact scaling - ",
                                         TC_EA_DOUBLE   , "Device can do Double-Weight Characters        - ",
                                         TC_IA_ABLE     , "Device can do Italicizing                     - ",
                                         TC_UA_ABLE     , "Device can do Underlining                     - ",
                                         TC_SO_ABLE     , "Device can do Strike-Outs                     - ",
                                         TC_RA_ABLE     , "Device can do Raster Fonts                    - ",
                                         TC_VA_ABLE     , "Device can do Vector Fonts                    - "};

     TEXTMETRIC tm;
     short      nHeight,nWidth,nLC,nIdx;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;

     bPrintAbort = (BOOL FAR*)bAbort;

     /*-----------------------------------------*\
     | Get text dimensions.                      |
     \*-----------------------------------------*/
     GetTextMetrics(hDC,&tm);
     nHeight = tm.tmHeight + tm.tmExternalLeading;
     nWidth  = tm.tmAveCharWidth;

     /*-----------------------------------------*\
     | Must have a local buffer to store strings.|
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
          return(FALSE);
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Output the device information.            |
     \*-----------------------------------------*/
     nLC = 0;
     wsprintf(lpBuffer, "Driver Version          - %X",lpDevCaps->nDriverVersion);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Driver Technology       - ");
     lstrcat(lpBuffer,szTechnology[lpDevCaps->nTechnology]);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Horizontal Size (mm)    - %d",lpDevCaps->nHorzSizeMM);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Number of Brushes       - %d",lpDevCaps->nBrushes);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Vertical Size (mm)      - %d",lpDevCaps->nVertSizeMM);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Number of Pens          - %d",lpDevCaps->nPens);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Horizontal Resolution   - %d",lpDevCaps->nHorzRes);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Number of Fonts         - %d",lpDevCaps->nFonts);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Vertical Resolution     - %d",lpDevCaps->nVertRes);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Number of Colors        - %d",lpDevCaps->nColors);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Logical Pixels (x)      - %d",lpDevCaps->nLogPixelsX);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Bits per Pixel          - %d",lpDevCaps->nBitsPixel);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Logical Pixels (y)      - %d",lpDevCaps->nLogPixelsY);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Number of Color Planes  - %d",lpDevCaps->nPlanes);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Aspect ratio (width)    - %d",lpDevCaps->nAspectX);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Size of Physical Device - %d",lpDevCaps->nPDeviceSize);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer, "Aspect ratio (height)   - %d",lpDevCaps->nAspectY);
     TextOut(hDC,0,nHeight*nLC,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, "Diagonal ratio (width)  - %d",lpDevCaps->nAspectXY);
     TextOut(hDC,lpDevCaps->nHorzRes/2,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     if(EndOfPage(nLC*nHeight,2*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "CLIPING CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer, pcClipCaps[0].szType);
     lstrcat(lpBuffer,(lpDevCaps->wClipCaps & pcClipCaps[0].nIndex ? "Yes" : "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     if(EndOfPage(nLC*nHeight,sizeof(pcRasterCaps)/sizeof(pcRasterCaps[0])*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "RASTER CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     for(nIdx=0; nIdx < sizeof(pcRasterCaps)/sizeof(pcRasterCaps[0]); nIdx++)
     {
          wsprintf(lpBuffer, pcRasterCaps[nIdx].szType);
          lstrcat(lpBuffer,(lpDevCaps->wRasterCaps & pcRasterCaps[nIdx].nIndex ? "Yes" : "No"));

          TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     }

     if(EndOfPage(nLC*nHeight,sizeof(pcCurveCaps)/sizeof(pcCurveCaps[0])*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "CURVE CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     for(nIdx=0; nIdx < sizeof(pcCurveCaps)/sizeof(pcCurveCaps[0]); nIdx++)
     {
          wsprintf(lpBuffer, pcCurveCaps[nIdx].szType);
          lstrcat(lpBuffer,(lpDevCaps->wRasterCaps & pcCurveCaps[nIdx].nIndex ? "Yes" : "No"));
          TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     }

     if(EndOfPage(nLC*nHeight,sizeof(pcLineCaps)/sizeof(pcLineCaps[0])*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "LINE CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     for(nIdx=0; nIdx < sizeof(pcLineCaps)/sizeof(pcLineCaps[0]); nIdx++)
     {
          wsprintf(lpBuffer, pcLineCaps[nIdx].szType);
          lstrcat(lpBuffer,(lpDevCaps->wLineCaps & pcLineCaps[nIdx].nIndex ? "Yes" : "No"));
          TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     }

     if(EndOfPage(nLC*nHeight,sizeof(pcPolygonCaps)/sizeof(pcPolygonCaps[0])*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "POLYGON CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     for(nIdx=0; nIdx < sizeof(pcPolygonCaps)/sizeof(pcPolygonCaps[0]); nIdx++)
     {
          wsprintf(lpBuffer, pcPolygonCaps[nIdx].szType);
          lstrcat(lpBuffer,(lpDevCaps->wPolygonCaps & pcPolygonCaps[nIdx].nIndex ? "Yes" : "No"));
          TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     }

     if(EndOfPage(nLC*nHeight,sizeof(pcTextCaps)/sizeof(pcTextCaps[0])*nHeight,lpDevCaps->nVertRes))
     {
          if(!PrintFooter(hDC,lpDevCaps,"Header"))
               return(FALSE);
          nLC=0;
     }

     nLC++;
     wsprintf(lpBuffer, "TEXT CAPABILITIES");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     for(nIdx=0; nIdx < sizeof(pcTextCaps)/sizeof(pcTextCaps[0]); nIdx++)
     {
          wsprintf(lpBuffer, pcTextCaps[nIdx].szType);
          lstrcat(lpBuffer,(lpDevCaps->wTextCaps & pcTextCaps[nIdx].nIndex ? "Yes" : "No"));
          TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     }

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}
