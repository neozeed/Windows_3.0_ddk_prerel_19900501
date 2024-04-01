/*---------------------------------------------------------------------------*\
| FONTS                                                                       |
|   This module contains routines to handle the gathering of the device fonts.|
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _HEADER                                                            |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Jul 13, 1989 - added condensed header routine.                     |
|          Jul 27, 1989 - ported to DLL.                                      |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntFram.h"

/*---------------------------------------------------------------------------*\
| PRINT DEVICE FONTS TO PRINTER (Expanded Version)                            |
|   This routine prints out the expanded information for every font the       |
|   device supports.                                                          |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to printer device context.           |
|   LPENUMERATE       lpFonts   - Fonts structure.                            |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything was OK.                                         |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintDeviceFonts(hDC,lpFonts,lpDevCaps,bAbort)
     HDC         hDC;
     HDEVOBJECT lpFonts;
     LPDEVINFO   lpDevCaps;
     LPSTR       bAbort;
{
     extern BOOL FAR* bPrintAbort;

     short  nIdx;                                /* Loop Variable.           */
     FONT   fFont;

     bPrintAbort = (BOOL FAR*)bAbort;

     /*-----------------------------------------*\
     | For all fonts, print out the expanded     |
     | information.                              |
     \*-----------------------------------------*/

          for(nIdx=0; nIdx < GetObjectCount(lpFonts); nIdx++)
          {
               SetCurrentObject(lpFonts,nIdx);
               CopyDeviceObject((LPSTR)&fFont,lpFonts);
               PrintFontInfoExp(hDC,(LPFONT)&fFont,nIdx,lpDevCaps);
          }
/*
          for(nIdx=0; nIdx < GetObjectCount(lpFonts); nIdx++)
          {
               SetCurrentObject(lpFonts,nIdx);
               CopyDeviceObject((LPSTR)&fFont,lpFonts);
               PrintFontInfoCon(hDC,(LPFONT)&fFont,nIdx,lpDevCaps);
          }
*/

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT DEVICE FONT INFORMATION (Expanded Version)                            |
|   This routine takes the font given as a parameter, and prints out the      |
|   information regarding it's logical font strucuture.                       |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPFONT            fFont     - Pointer to a logical font.                  |
|   LPDEVCAPS         lpDevCaps - Device capabilities structure.              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything was OK.                                         |
\*---------------------------------------------------------------------------*/
BOOL PrintFontInfoExp(hDC,fFont,iFontNum,lpDevCaps)
     HDC       hDC;
     LPFONT    fFont;
     int       iFontNum;
     LPDEVINFO lpDevCaps;
{
     short      nLC,nHeight,nWidth;
     TEXTMETRIC tm;
     HFONT      hFont;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;

     static PRINTCAPS pcLogQuality[] = {DRAFT_QUALITY  ,"Draft Quality "  ,
                                        PROOF_QUALITY  ,"Proof Quality"};
     static PRINTCAPS pcLogPitch[]   = {FIXED_PITCH   ,"Fixed Pitch "  ,
                                        VARIABLE_PITCH,"Variable Pitch"};
     static PRINTCAPS pcLogFamily[]  = {FF_DECORATIVE,"Decorative ",
                                        FF_MODERN    ,"Modern "    ,
                                        FF_ROMAN     ,"Roman "     ,
                                        FF_SCRIPT    ,"Script "    ,
                                        FF_SWISS     ,"Swiss"};

     /*-----------------------------------------*\
     | Get text sizes.                           |
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
     | Print out the Logical Font Information.   |
     \*-----------------------------------------*/
     nLC=0;
     wsprintf(lpBuffer,(LPSTR)"LOGICAL FONT STRUCTURE INFORMATION - Font %d",iFontNum);
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     nLC++;
     wsprintf(lpBuffer,(LPSTR)"Font Height        - %d",fFont->lf.lfHeight);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Font Width         - %d",fFont->lf.lfWidth);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Font Escapement    - %d",fFont->lf.lfEscapement);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Font Orientation   - %d",fFont->lf.lfOrientation);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Font Weight        - %d",fFont->lf.lfWeight);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     lstrcpy(lpBuffer,(LPSTR)"Italicized         - ");
     lstrcat(lpBuffer,(fFont->lf.lfItalic ? "Yes" : "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,(LPSTR)"Underlined         - ");
     lstrcat(lpBuffer,(fFont->lf.lfUnderline ? "Yes" : "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,(LPSTR)"StrikeOut          - ");
     lstrcat(lpBuffer,(fFont->lf.lfStrikeOut ? "Yes" : "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->lf.lfCharSet)
     {
          case ANSI_CHARSET:
               wsprintf(lpBuffer,"Character Set      - Ansi (%d)",fFont->lf.lfCharSet);
               break;
          case SHIFTJIS_CHARSET:
               wsprintf(lpBuffer,"Character Set      - Kanji (%d)",fFont->lf.lfCharSet);
               break;
          case OEM_CHARSET:
               wsprintf(lpBuffer,"Character Set      - OEM (%d)",fFont->lf.lfCharSet);
               break;
          default:
               wsprintf(lpBuffer,"Character Set      - Unknown (%d)", fFont->lf.lfCharSet);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->lf.lfOutPrecision)
     {
          case OUT_DEFAULT_PRECIS:
               wsprintf(lpBuffer,"Output Precision   - Default (%d)",fFont->lf.lfOutPrecision);
               break;
          case OUT_STRING_PRECIS:
               wsprintf(lpBuffer,"Output Precision   - String (%d)",fFont->lf.lfOutPrecision);
               break;
          case OUT_CHARACTER_PRECIS:
               wsprintf(lpBuffer,"Output Precision   - Character (%d)",fFont->lf.lfOutPrecision);
               break;
          case OUT_STROKE_PRECIS:
               wsprintf(lpBuffer,"Output Precision   - Stroke (%d)",fFont->lf.lfOutPrecision);
               break;
          default:
               wsprintf(lpBuffer,"Output Precision   - Unknown (%d)",fFont->lf.lfOutPrecision);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->lf.lfClipPrecision)
     {
          case CLIP_DEFAULT_PRECIS:
               wsprintf(lpBuffer,"Clipping Precision - Default (%d)",fFont->lf.lfClipPrecision);
               break;
          case CLIP_CHARACTER_PRECIS:
               wsprintf(lpBuffer,"Clipping Precision - Character (%d)",fFont->lf.lfClipPrecision);
               break;
          case CLIP_STROKE_PRECIS:
               wsprintf(lpBuffer,"Clipping Precision - Stroke(%d)",fFont->lf.lfClipPrecision);
               break;
          default:
               wsprintf(lpBuffer,"Clipping Precision - Unknown (%d)",fFont->lf.lfClipPrecision);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch((fFont->lf.lfQuality & 0x0F))
     {
          case DEFAULT_QUALITY:
               wsprintf(lpBuffer,"Output Quality     - Default (%d)",fFont->lf.lfQuality);
               break;
          case DRAFT_QUALITY:
               wsprintf(lpBuffer,"Output Quality     - Draft (%d)",fFont->lf.lfQuality);
               break;
          case PROOF_QUALITY:
               wsprintf(lpBuffer,"Output Quality     - Proof (%d)",fFont->lf.lfQuality);
               break;
          default:
               wsprintf(lpBuffer,"Output Quality     - Unknown (%d)",fFont->lf.lfQuality);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->lf.lfPitchAndFamily & 0x03)
     {
          case DEFAULT_PITCH:
               wsprintf(lpBuffer,"Pitch              - Default (%d)",(fFont->lf.lfPitchAndFamily & 0x03));
               break;
          case FIXED_PITCH:
               wsprintf(lpBuffer,"Pitch              - Fixed Pitch (%d)",(fFont->lf.lfPitchAndFamily & 0x0F));
               break;
          case VARIABLE_PITCH:
               wsprintf(lpBuffer,"Pitch              - Variable Pitch (%d)",(fFont->lf.lfPitchAndFamily & 0x0F));
               break;
          default:
               wsprintf(lpBuffer,"Pitch              - Unknown (%d)",(fFont->lf.lfPitchAndFamily & 0x0F));
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->lf.lfPitchAndFamily & 0xF0)
     {
          case FF_DECORATIVE:
               wsprintf(lpBuffer,"Font Family        - Decorative (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_MODERN:
               wsprintf(lpBuffer,"Font Family        - Modern (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_ROMAN:
               wsprintf(lpBuffer,"Font Family        - Roman (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_SCRIPT:
               wsprintf(lpBuffer,"Font Family        - Script (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_SWISS:
               wsprintf(lpBuffer,"Font Family        - Swiss (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
          default:
               wsprintf(lpBuffer,"Font Family        - Unknown (%d)",(fFont->lf.lfPitchAndFamily >> 4) & 0x0F);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer,(LPSTR)"FaceName           - %s",fFont->lf.lfFaceName);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     nLC++;
     wsprintf(lpBuffer,(LPSTR)"TEXT METRIC STRUCTURE INFORMATION");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     nLC++;
     wsprintf(lpBuffer,(LPSTR)"Character Height           - %d",fFont->tm.tmHeight);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Character Ascent           - %d",fFont->tm.tmAscent);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Character Descent          - %d",fFont->tm.tmDescent);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Character Internal Leading - %d",fFont->tm.tmInternalLeading);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Character External Leading - %d",fFont->tm.tmExternalLeading);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Average Character Width    - %d",fFont->tm.tmAveCharWidth);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Maximum Character Width    - %d",fFont->tm.tmMaxCharWidth);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Character Weight           - %d",fFont->tm.tmWeight);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,(LPSTR)"Italicized                 - ");
     lstrcat(lpBuffer,(fFont->tm.tmItalic ?  "Yes" :  "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,(LPSTR)"Underlined                 - ");
     lstrcat(lpBuffer,(fFont->tm.tmUnderlined ?  "Yes" :  "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     lstrcpy(lpBuffer,(LPSTR)"Struck Out                 - ");
     lstrcat(lpBuffer,(fFont->tm.tmStruckOut ?  "Yes" :  "No"));
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"First Character Value      - %d",fFont->tm.tmFirstChar);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Last Character Value       - %d",fFont->tm.tmLastChar);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Default Character Value    - %d",fFont->tm.tmDefaultChar);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Break Character            - %d",fFont->tm.tmBreakChar);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->tm.tmPitchAndFamily & 0x01)
     {
          case 1:
               wsprintf(lpBuffer,"Pitch                      - Variable (%d)",(fFont->tm.tmPitchAndFamily & 0x0F));
               break;
          case 0:
               wsprintf(lpBuffer,"Pitch                      - Fixed Pitch (%d)",(fFont->tm.tmPitchAndFamily & 0x0F));
               break;
          default:
               wsprintf(lpBuffer,"Pitch                      - Unknown (%d)",(fFont->tm.tmPitchAndFamily & 0x0F));
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->tm.tmPitchAndFamily & 0xF0)
     {
          case FF_DECORATIVE:
               wsprintf(lpBuffer,"Font Family                - Decorative (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_MODERN:
               wsprintf(lpBuffer,"Font Family                - Modern (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_ROMAN:
               wsprintf(lpBuffer,"Font Family                - Roman (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_SCRIPT:
               wsprintf(lpBuffer,"Font Family                - Script (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
          case FF_SWISS:
               wsprintf(lpBuffer,"Font Family                - Swiss (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
          default:
               wsprintf(lpBuffer,"Font Family                - Unknown (%d)",(fFont->tm.tmPitchAndFamily >> 4) & 0x0F);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     switch(fFont->tm.tmCharSet)
     {
          case ANSI_CHARSET:
               wsprintf(lpBuffer,"Character Set              - Ansi (%d)",fFont->tm.tmCharSet);
               break;
          case SHIFTJIS_CHARSET:
               wsprintf(lpBuffer,"Character Set              - Kanji (%d)",fFont->tm.tmCharSet);
               break;
          case OEM_CHARSET:
               wsprintf(lpBuffer,"Character Set              - OEM (%d)",fFont->tm.tmCharSet);
               break;
          default:
               wsprintf(lpBuffer,"Character Set              - Unknown (%d)", fFont->tm.tmCharSet);
               break;
     }
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     wsprintf(lpBuffer,(LPSTR)"Overhang                   - %d",fFont->tm.tmOverhang);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Digitized Aspect X         - %d",fFont->tm.tmDigitizedAspectX);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     wsprintf(lpBuffer,(LPSTR)"Digitized Aspect Y         - %d",fFont->tm.tmDigitizedAspectY);
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     /*-----------------------------------------*\
     | Print out the Font Type information.      |
     \*-----------------------------------------*/
     nLC++;
     wsprintf(lpBuffer,(LPSTR)"FONT TYPE INFORMATION");
     TextOut(hDC,0,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));
     nLC++;
     if(fFont->nFontType & RASTER_FONTTYPE)
          lstrcpy(lpBuffer,(LPSTR)"Raster-");
     else
          lstrcpy(lpBuffer,(LPSTR)"Vector-");

     if(fFont->nFontType & DEVICE_FONTTYPE)
          lstrcat(lpBuffer,(LPSTR)"Device Based Font");
     else
          lstrcat(lpBuffer,(LPSTR)"GDI Based Font");
     TextOut(hDC,nWidth*5,nHeight*nLC++,lpBuffer,lstrlen(lpBuffer));

     /*-----------------------------------------*\
     | Print out a sample of the font.           |
     \*-----------------------------------------*/
     nLC++;
     hFont = SelectObject(hDC,CreateFontIndirect(&fFont->lf));
     TextOut(hDC,0,nHeight*nLC++,(LPSTR)"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",52);
     DeleteObject(SelectObject(hDC,hFont));
     nLC++;

     if((lpDevCaps->wTextCaps & TC_UA_ABLE) && !fFont->lf.lfUnderline)
     {
          fFont->lf.lfUnderline = 1;
          hFont = SelectObject(hDC,CreateFontIndirect(&fFont->lf));
          TextOut(hDC,0,nHeight*nLC++,(LPSTR)"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",52);
          DeleteObject(SelectObject(hDC,hFont));
          fFont->lf.lfUnderline = 0;
     }
     if((lpDevCaps->wTextCaps & TC_IA_ABLE) && !fFont->lf.lfItalic)
     {
          fFont->lf.lfItalic = 1;
          hFont = SelectObject(hDC,CreateFontIndirect(&fFont->lf));
          TextOut(hDC,0,nHeight*nLC++,(LPSTR)"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",52);
          DeleteObject(SelectObject(hDC,hFont));
          fFont->lf.lfItalic = 0;
     }
     if((lpDevCaps->wTextCaps & TC_SO_ABLE) && !fFont->lf.lfStrikeOut)
     {
          fFont->lf.lfStrikeOut = 1;
          hFont = SelectObject(hDC,CreateFontIndirect(&fFont->lf));
          TextOut(hDC,0,nHeight*nLC++,(LPSTR)"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",52);
          DeleteObject(SelectObject(hDC,hFont));
          fFont->lf.lfStrikeOut = 0;
     }

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     if(!PrintFooter(hDC,lpDevCaps,"Header"))
          return(FALSE);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| PRINT DEVICE FONT INFORMATION (Condensed Version)                           |
|   This routine takes the font given as a parameter, and prints out the      |
|   information regarding it's logical font strucuture.                       |
|                                                                             |
| CALLED ROUTINES                                                             |
|   PrintFooter() - (misc.c)                                                  |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC               hDC       - Handle to a printer device context.         |
|   LPFONT            fFont     - Pointer to a logical font.                  |
|   LPDEVCAPS         lpDevCaps - Device information structure.               |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE if everything was OK.                                         |
\*---------------------------------------------------------------------------*/
BOOL PrintFontInfoCon(hDC,fFont,iFontNum,lpDevCaps)
     HDC       hDC;
     LPFONT    fFont;
     int       iFontNum;
     LPDEVINFO lpDevCaps;
{
     extern BOOL FAR PrintFooter(HDC,LPDEVINFO,LPSTR);

     static int nLC = 0;
     static int nFontCount = 0;
     HANDLE     hBuffer;
     LPSTR      lpBuffer;
     TEXTMETRIC tm;
     int        nHeight;
     HFONT      hFont,hOldFont;

     /*-----------------------------------------*\
     | Select the font.                          |
     \*-----------------------------------------*/
     if(!(hFont = CreateFontIndirect(&fFont->lf)))
     {
          GetTextMetrics(hDC,&tm);
          nHeight = tm.tmHeight+tm.tmExternalLeading;
          TextOut(hDC,0,nLC,(LPSTR)"Error: Creating Font",20);
          nLC += nHeight;
          return(FALSE);
     }
     if(!(hOldFont = SelectObject(hDC,hFont)))
     {
          GetTextMetrics(hDC,&tm);
          nHeight = tm.tmHeight+tm.tmExternalLeading;
          DeleteObject(hFont);
          TextOut(hDC,0,nLC,(LPSTR)"Error: Selecting Font",21);
          nLC += nHeight;
          return(FALSE);
     }

     GetTextMetrics(hDC,&tm);
     nHeight = tm.tmHeight+tm.tmExternalLeading;

     /*-----------------------------------------*\
     | Print footer at bottom of page if EOP.    |
     \*-----------------------------------------*/
     if((nHeight+nLC) > lpDevCaps->nVertRes)
     {
          nLC = 0;
          nFontCount = 0;
          if(!PrintFooter(hDC,lpDevCaps,(LPSTR)"Header"))
               return(FALSE);
     }
     nFontCount++;

     /*-----------------------------------------*\
     | Allocate and lock buffer for use.         |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
          return(FALSE);
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Print the condensed information.          |
     \*-----------------------------------------*/
     wsprintf(lpBuffer,(LPSTR)"%s %d",fFont->lf.lfFaceName,fFont->lf.lfHeight);
     lstrcat(lpBuffer,(LPSTR)" - ");

     if(fFont->lf.lfWeight > 550)
          lstrcat(lpBuffer,(LPSTR)"Bold ");
     else
          lstrcat(lpBuffer,(LPSTR)"Normal ");

     if(fFont->lf.lfItalic)
          lstrcat(lpBuffer,(LPSTR)"Italic ");

     if(fFont->lf.lfUnderline)
          lstrcat(lpBuffer,(LPSTR)"Underlined ");

     if(fFont->lf.lfStrikeOut)
          lstrcat(lpBuffer,(LPSTR)"StrikeOut");

     TextOut(hDC,0,nLC,lpBuffer,lstrlen(lpBuffer));
     nLC += nHeight;

     /*-----------------------------------------*\
     | Restore for the next font.                |
     \*-----------------------------------------*/
     DeleteObject(SelectObject(hDC,hOldFont));

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     return(TRUE);
}
