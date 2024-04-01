/**[f******************************************************************
 * physical.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.  
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

//*********************************************************************
//
// 04 dec 89	clarkc	EndStyle() called before nextWidth added to size.
//                      nNotStruckOut subtracted from size when passed
//                      to EndStyle().  Fix for Bug #3770.
//
// 30 nov 89	clarkc	xpos no longer set to zero within the hack
//                      for the Z Cartridge.  Cliprect.left now has
//                      minimum of 0.  Fix for Bug #5822.
//
// 25 aug 89	craigc	Added MATH8 support
//
// 04 aug 89	peterbe	Fixing bug 2229: was randomly printing bold justified
//			text: reset epXerr to 0 at end of str_out().
//
//   1-17-89	jimmat	Added PCL_* entry points to lock/unlock data seg.
//

#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#include "fontman.h"
#include "strings.h"
#include "utils.h"
#define SEG_PHYSICAL
#include "memoman.h"
#include "transtbl.h"


/*  Utilities
 */
#include "message.c"


/*  Debug switches
 */
#define DBGtrace(msg) /*DBMSG(msg)*/
#undef DBGEscapes
#define DBGStrOut(msg) /*DBMSG(msg)*/
#define DBGtrans(msg) /*DBMSG(msg)*/
#undef DBGdumpFontInfo

#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb)>>16))

#define WHITENESS   (DWORD)0x00FF0062  /* dest = WHITE */
#define EXTTEXT_D1 0x0002
#define EXTTEXT_D2 0x0004


/*  Forward definitions.
 */

long  FAR PASCAL StrBlt(LPDEVICE,short,short,LPRECT,LPSTR,short,
	LPFONTINFO,LPDRAWMODE,LPTEXTXFORM);
long far PASCAL ExtTextOut(LPDEVICE, short, short, LPRECT, LPSTR,
	short, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM, short far *, LPRECT, WORD);
static void OverLap(LPDEVICE, short, short, short, short);
static short FakeBoldOverhang(LPFONTINFO, LPTEXTXFORM);
static long str_out(LPDEVICE, short, short, LPFONTINFO, LPDRAWMODE,
        LPTEXTXFORM, short, LPSTR, LPRECT, short far *);
static int char_out(LPDEVICE, char, short, BYTE);
static int RelXMove(LPDEVICE, short);
static int StartStyle(LPDEVICE, LPTEXTXFORM);
static int EndStyle(LPDEVICE, LPTEXTXFORM, short);
static BOOL SetJustification(LPDEVICE, LPDRAWMODE, LPJUSTBREAKTYPE,
        LPJUSTBREAKREC, LPJUSTBREAKREC);
static short GetJustBreak (LPJUSTBREAKREC, JUSTBREAKTYPE);
static LPSTR DoubleSizeCopy(LPHANDLE, LPSTR, short, short);
static BOOL isWhite(long, short);


#define MAXNUM_WIDTHS 100


#ifdef DEBUG
static void debugStr(pstr, count)
    LPSTR pstr;
    short count;
    {
    int i = 0;
    short escflag;
    DBMSG(("string: "));

    for (i = 0; i < count; i++, pstr++)
        if (*pstr == '\033')
            DBMSG(("ESC"));
        else
            DBMSG(("%c", *pstr));

    DBMSG(("\n"));
    }
#endif

/**************************************************************************/
/*************************   Windows Text Stubs   *************************/


/*  PCL_StrBlt()
 *
 *  StrBlt entry point to lock/unlock the data segment.
 */

long far PASCAL
PCL_StrBlt(lpDevice, x, y, lpClipRect, lpString, count,
       lpFont, lpDrawMode, lpXform)
LPDEVICE lpDevice;
short x, y, count;
LPRECT lpClipRect;
LPSTR lpString;
LPFONTINFO lpFont;
LPDRAWMODE lpDrawMode;
LPTEXTXFORM lpXform;
{
    long rc;

    LockSegment(-1);

    rc = StrBlt(lpDevice, x, y, lpClipRect, lpString, count,
		      lpFont, lpDrawMode, lpXform);

    UnlockSegment(-1);

    return rc;
}	// PCL_StrBlt()

/*  StrBlt()
 *
 *  Windows 1.04 and earlier string output routine.
 */
long far PASCAL StrBlt(lpDevice, x, y, lpClipRect, lpString, count,
        lpFont, lpDrawMode, lpXform)
    LPDEVICE lpDevice;
    short x;
    short y;
    LPRECT lpClipRect;
    LPSTR lpString;
    short count;
    LPFONTINFO lpFont;
    LPDRAWMODE lpDrawMode;
    LPTEXTXFORM lpXform;
    {
    if (!lpDevice->epType)
        {
        DBGtrace(("StrBlt(): ***calling dmStrBlt\n"));

        return (dmStrBlt(lpDevice, x, y, lpClipRect, lpString, count,
            lpFont, lpDrawMode, lpXform));
        }

    return (ExtTextOut(lpDevice, x, y, lpClipRect, lpString,
        count, lpFont, lpDrawMode, lpXform, 0L, 0L, 0));

    }	// StrBlt()

/*  PCL_ExtTextOut()
 *
 *  ExtTextOut entry point to lock/unlock data segment.
 */

long FAR PASCAL
PCL_ExtTextOut(lpDevice, x, y, lpClipRect, lpString, count,
	   lpFont, lpDrawMode, lpXform, lpWidths, lpOpaqRect, options)
LPDEVICE lpDevice;
short x, y, count;
LPRECT lpClipRect, lpOpaqRect;
LPSTR lpString;
LPFONTINFO lpFont;
LPDRAWMODE lpDrawMode;
LPTEXTXFORM lpXform;
short far *lpWidths;
WORD options;
{
    long rc;

    LockSegment(-1);

    rc = ExtTextOut(lpDevice, x, y, lpClipRect, lpString, count,
	       lpFont, lpDrawMode, lpXform, lpWidths, lpOpaqRect, options);

    UnlockSegment(-1);

    return rc;

}	// PCL_ExtTextOut()

/*  ExtTextOut()
 *
 *  Extended Text Output:  display a text string and return its width.
 *  If count < 0, then just return the width of the string.
 */
long far PASCAL ExtTextOut(lpDevice, x, y, lpClipRect, lpString, count,
        lpFont, lpDrawMode, lpXform, lpWidths, lpOpaqRect, options)
    LPDEVICE lpDevice;
    short x;
    short y;
    LPRECT lpClipRect;
    LPSTR lpString;
    short count;
    LPFONTINFO lpFont;
    LPDRAWMODE lpDrawMode;
    LPTEXTXFORM lpXform;
    short far *lpWidths;
    LPRECT lpOpaqRect;
    WORD options;
    {
    long status = 0;
    short overhang;
    RECT cliprect, opaqrect;

    #ifdef DEBUG
    /*  Print debug messages on only the interesting cases.
     */
    if (count != 1 && count != -1)
        {
        DBGtrace(("ExtTextOut(%lp,%d,%d,%lp,%lp,%d,%lp\n",
            lpDevice, x, y, lpClipRect, lpString, count, lpFont));
        DBGtrace(("     %lp,%lp,%lp,%lp,%d)\n",
            lpDrawMode, lpXform, lpWidths, lpOpaqRect, options));
        DBGtrace(("     string=%c", '"'));
        {
        short DBGcount = (count < 0) ? -count : count;
        LPSTR DBGs = lpString;
        for (; DBGcount > 0; --DBGcount, ++DBGs)
            { DBGtrace(("%c", (char)*DBGs)); }
        DBGtrace(("%c\n", '"'));
        }
        DBGtrace(("     band=%d,count=%d\n", lpDevice->epNband, count));
        if (lpClipRect) {
            DBGtrace(("     Clip Rect is left=%d,top=%d,right=%d,bottom=%d\n",
                lpClipRect->left, lpClipRect->top, lpClipRect->right,
                lpClipRect->bottom));
        } else
            { DBGtrace(("     Clip Rect is *undefined*\n")); }
        if (lpOpaqRect) {
            DBGtrace(("     Opaq Rect is left=%d,top=%d,right=%d,bottom=%d\n",
                lpOpaqRect->left, lpOpaqRect->top, lpOpaqRect->right,
                lpOpaqRect->bottom));
        } else
            { DBGtrace(("     Opaq Rect is *undefined*\n")); }
        }
    #endif

    if (!lpDevice->epType)
        {
        DBGtrace(("ExtTextOut(): !epType, return failure\n"));
        return (0L);
        }

    /*  Synthesized bold.
     */
    if (lpFont->dfWeight < lpXform->ftWeight)
        {
        overhang = lpXform->ftOverhang;
        DBGtrace(
	 ("ExtTextOut(): overhang for synthesized bold=%d\n", overhang));
        }
    else
        overhang = 0;
    
    /*  If count < 0, the caller wants only the string length.
     */
    if (count < 0)
        {
        lpClipRect = 0L;
        lpOpaqRect = 0L;
        goto getlength;
        }

    /*  Test for white type.  If we're just getting the extent of
     *  the string, always assume black type.
     */
    if (count > 0 && isWhite(lpDrawMode->TextColor, lpDevice->epTxWhite))
        {
        /*  Invert the count so we won't output the string,
         *  but will return its length.
         */
        count = -count;
        }
    else
        lpDrawMode->TextColor = 0L;

    /*  Make local copies of clip and opaque rectangles, and shift them
     *  based upon x and y offsets.
     */
    if (lpClipRect)
        {
        lmemcpy((LPSTR) &cliprect, (LPSTR)lpClipRect, sizeof (RECT));
        lpClipRect = (LPRECT) &cliprect;


        if (OffsetClipRect(lpClipRect,
            lpDevice->epXOffset >> lpDevice->epScaleFac,
            lpDevice->epYOffset >> lpDevice->epScaleFac) <= 0)
            {
            DBGtrace(("OffsetClipRect() failed on lpClipRect, return\n"));
            lpClipRect = 0L;
            return (0L);
            }

        #ifdef DEBUG
        if (lpClipRect)
            {
            DBGtrace(("Clip Rect is left=%d,top=%d,right=%d,bottom=%d\n",
                lpClipRect->left, lpClipRect->top, lpClipRect->right,
                lpClipRect->bottom));
            }
        #endif
        }


    if (lpOpaqRect)
        {
        lmemcpy((LPSTR) &opaqrect, (LPSTR)lpOpaqRect, sizeof(RECT));
        lpOpaqRect = (LPRECT) &opaqrect;

        if (OffsetClipRect(lpOpaqRect,
            lpDevice->epXOffset >> lpDevice->epScaleFac,
            lpDevice->epYOffset >> lpDevice->epScaleFac) <= 0)
            {
            DBGtrace(("OffsetClipRect() failed on lpOpaqRect\n"));
            lpOpaqRect = 0L;
            }

        #ifdef DEBUG
        if (lpOpaqRect)
            {
            DBGtrace(("Opaq Rect is left=%d,top=%d,right=%d,bottom=%d\n",
                lpOpaqRect->left, lpOpaqRect->top, lpOpaqRect->right,
                lpOpaqRect->bottom));
            }
        #endif
        }


    /*  Modify opaque and clip regions per options switches.
     */
    if (lpOpaqRect)
        {
        if (options & EXTTEXT_D2)
            {
            /*  lpOpaqRect should be used as a clipping rectangle.
             */
            if (lpClipRect)
                IntersectRect(lpClipRect, lpClipRect, lpOpaqRect);
            else
                {
                lmemcpy((LPSTR) &cliprect, (LPSTR)lpOpaqRect, sizeof (RECT));
                lpClipRect = (LPRECT) &cliprect;
                }
            }

        if (options & EXTTEXT_D1)
            {
            /*  lpOpaqRect should be used as an opaque rectangle.
             */
            if (lpClipRect)
                IntersectRect(lpOpaqRect, lpClipRect, lpOpaqRect);
            }
        else
            lpOpaqRect = 0L;
        }


    /*  If count == 0, then just output the opaque rectangle if supplied.
     */
    if (count == 0)
        {
        if (lpDevice->epNband == TEXTBAND)
            return (0L);
        else
            goto justopaque;
        }

    if (lpDevice->epNband == TEXTBAND)
        {
        short strikes;

        /*  If the background will be white'd out, we set the epOpaqText
         *  flag.  The BANDINFO escape will request the application to
         *  send down text on graphics bands.
         */
        if (lpOpaqRect || (lpDrawMode->bkMode == OPAQUE))
            {
            lpDevice->epOpaqText = TRUE;
            }

        for (strikes = 1; strikes <= overhang; strikes++)
            {
            str_out(lpDevice, x + strikes, y, lpFont, lpDrawMode, lpXform,
                count, lpString, lpClipRect, lpWidths);
            }

getlength:
        return (str_out(lpDevice, x, y, lpFont, lpDrawMode, lpXform, count,
            lpString, lpClipRect, lpWidths) + overhang);
        }

    DBGtrace(("Not textband\n"));

    /*  Pick up the dimensions of the string
     *  (but don't output it).
     */
    status = str_out(lpDevice, x, y, lpFont, lpDrawMode, lpXform,
        count > 0 ? -count : count, lpString, lpClipRect,
        lpWidths) + overhang;

    /*  In OPAQUE text mode we make sure that the background of the exact
     *  line of text is white'd out.
     */
    if (lpDrawMode->bkMode == OPAQUE)
        {
        RECT backRect;

        DBGtrace(("opaque background\n"));

        /*  Pick up dimensions of opaque rectangle.
         */
        backRect.left = x;
        backRect.top = y;
        backRect.right = x + (LWORD(status) >> lpDevice->epScaleFac);
        backRect.bottom = y + (HWORD (status) >> lpDevice->epScaleFac);

        /*  Clip.
         */
        if (lpClipRect)
            IntersectRect(&backRect, lpClipRect, &backRect);

        /*  Turn right and bottom into width and depth.
         */
        backRect.right -= backRect.left;
        backRect.bottom -= backRect.top;

        /*  Offset top left corner by current band rect.
         */
        backRect.left -= lpDevice->epXOffset >> lpDevice->epScaleFac;
        backRect.top -= lpDevice->epYOffset >> lpDevice->epScaleFac;

        /*  Output via GDI.
         */
        lpDevice->epBmpHdr.bmBits = lpDevice->epBmp;
        OverLap(lpDevice, backRect.left, backRect.top,
			backRect.right, backRect.bottom);
        }

justopaque:
    /*  If we received an OPAQUE rectangle, then we white out the
     *  region specified by the rectangle.
     */
    if (lpOpaqRect)
        {
        DBGtrace(("opaque rectangle\n"));

        lpDevice->epBmpHdr.bmBits = lpDevice->epBmp;
        OverLap(lpDevice, lpOpaqRect->left, lpOpaqRect->top, 
            (lpOpaqRect->right - lpOpaqRect->left),
            (lpOpaqRect->bottom - lpOpaqRect->top));
        }

    return status;

    }	// ExtTextOut()

/*  OverLap()
 *
 *  Output white rectangle.
 */
static void OverLap(lpDevice, x, y, xext, yext)
    LPDEVICE lpDevice;
    short x, y, xext, yext;
    {
    DBGtrace(("OverLap(%lp,%d,%d,%d,%d)\n", lpDevice, x, y, xext, yext));

    if (x < 0)
        {
        xext += x;
        x = 0;
        }

    if (y < 0)
        {
        yext += y;
        y = 0;
        }

    if ((xext <= 0) || (yext <= 0))
        return;

    if (x + xext > lpDevice->epBmpHdr.bmWidth)
        xext = lpDevice->epBmpHdr.bmWidth - x;

    if (y + yext > lpDevice->epBmpHdr.bmHeight)
        yext = lpDevice->epBmpHdr.bmHeight - y;

    if ((xext <= 0) || (yext <= 0))
        return;

    DBGtrace(("...rect: x=%d,y=%d,width=%d,depth=%d\n", x, y, xext, yext));

    if (xext > 0 && yext > 0)
        dmBitblt((LPDEVICE) &lpDevice->epBmpHdr, x, y, 0, 0, 0, xext, yext,
            WHITENESS, (long)0L, (long)0L);

    }	//OverLap()

/**************************************************************************/
/***************************   Text Utilities   ***************************/


/*  str_out()
 *
 *  Low-level routine for getting widths and outputting text.
 */
static long str_out(lpDevice, xpos, ypos, lpFont, lpDrawMode, lpXform,
        count, lpString, lpClipRect, lpWidths)
    LPDEVICE lpDevice;
    short xpos;
    short ypos;
    LPFONTINFO lpFont;
    LPDRAWMODE lpDrawMode;
    LPTEXTXFORM lpXform;
    short count;
    LPSTR lpString;
    LPRECT lpClipRect;
    short far *lpWidths;
    {
    JUSTBREAKREC JustifyWB, JustifyLTR;
    JUSTBREAKTYPE JustType;
    short width, real;
    short prevwidth, nextwidth;
    short size;
    short nNotStruckOut; /* When passing size to EndStyle, don't include this */
    BYTE dfBreakChar;
    BYTE dfFirstChar;
    BYTE dfLastChar;
    BYTE dfDefaultChar;
    BYTE thisChar;
    BYTE overStrike;
    short far *widthptr;
    short far *lpdx;
    short far *lpw;
    short i, j;
    BOOL gotdeltax;
    BOOL updatex;
    BOOL shiftCH;
    LPSTR lpTransTbl;
    HANDLE hTransData;
    long status = OEM_FAILED;
    int err = SUCCESS;
    RECT ZCARTrect;
    extern HANDLE hLibInst;
#if 1
    RECT cliprect;                        /* in case no cliprect exists */

    if (!lpClipRect)
        SetRect(lpClipRect = (LPRECT) &cliprect, 0, 0, lpDevice->epPF.xImage,
                                                       lpDevice->epPF.yImage);
#endif

    real = (count > 0);

    if (count < 0)
        count = -count;

    #if 0
    if (real)
        {
        DBMSG(("In str_out, count=%d, ", count));
        #ifdef DEBUG
        debugStr(lpString, count);
        #endif
        }
    #endif

    /*  Check for vertical clipping if line is going to be output.
     */
    if (real && lpClipRect)
      {
	if (lpDevice->epOptions & OPTIONS_VERTCLIP)
	  {
	    if ((ypos + lpFont->dfInternalLeading < lpClipRect->top) ||
		(ypos + lpFont->dfPixHeight > lpClipRect->bottom))
	      {

		DBGtrace(("str_out(): vertical clipping --> whole line clipped\n"));
		DBGtrace(
	    ("           ypos=%d, top=%d, bottom=%d, pixHeight=%d, intrnLead=%d\n",
		    ypos, lpClipRect->top, lpClipRect->bottom,
		    lpFont->dfPixHeight, lpFont->dfInternalLeading));

		goto backout0;
	      }
	  }
	else
	  {
	    if ((ypos + lpFont->dfInternalLeading > lpClipRect->bottom) ||
		(ypos + lpFont->dfPixHeight < lpClipRect->top))
	      {

		DBGtrace(("str_out(): vertical clipping --> whole line clipped\n"));
		DBGtrace(
	    ("           ypos=%d, top=%d, bottom=%d, pixHeight=%d, intrnLead=%d\n",
		    ypos, lpClipRect->top, lpClipRect->bottom,
		    lpFont->dfPixHeight, lpFont->dfInternalLeading));

		goto backout0;
	      }
	  }
      }

    /*  Make a copy of the array of widths.  This structure is used to
     *  hold the width of each character and the delta-x move passed in
     *  from ExtTextOut().  The structure starts out looking like this:
     *
     *      array-of-desired-widths[count];  (from ExtTextOut())
     *      unused[count];
     *
     *  Then, when we roll through the string picking up the true widths
     *  of the characters, the structure changes to look like this:
     *
     *      array-of-true-widths[count];
     *      array-of-deltax-moves[count];
     */
    gotdeltax = (lpWidths != 0L);
    lpWidths = (short far *)DoubleSizeCopy(&lpDevice->epHWidths,
        (LPSTR)lpWidths, count, sizeof(short));

    if (!lpWidths)
        {
        DBMSG(("str_out(): could *not* alloc array of widths\n"));
        goto backout0;
        }

    dfFirstChar = lpFont->dfFirstChar;
    dfLastChar = lpFont->dfLastChar;
    dfBreakChar = lpFont->dfBreakChar + lpFont->dfFirstChar;
    dfDefaultChar = lpFont->dfDefaultChar + lpFont->dfFirstChar;

    /*  Send the information to the printer to change the font if
     *  we're actually outputting the string.
     */
    if (real)
        {
        register short ctl = ((LPPRDFONTINFO)lpFont)->indFontSummary;

        if (lpDevice->epECtl != ctl)
            {
            char temp[80];
            DBGStrOut(("str_out(): fontind to print is %d\n", ctl));

            if (LoadFontString(lpDevice, (LPSTR)temp, sizeof (temp),
                    fontescape, ctl))
                {
                DBGStrOut(("Str_out(): escape is %ls\n", (LPSTR)temp));

                /*  Send escape sequence to set font.
                 */
                err = myWrite(lpDevice, (LPSTR)temp, lstrlen((LPSTR)temp));
                }
            else if (!DownLoadSoft(lpDevice, ctl))
                {
                if (!(lpDevice->epFontSub))
                    {
                    /*send wrning message
                     */
                    WarningMsg(lpDevice, SOFT_LIMIT);
                    lpDevice->epFontSub = TRUE;
                    }

                /*send escape for default courier
                 */
                err = myWrite(lpDevice, FONT_DEFAULT);
                }

            lpDevice->epECtl = ctl;
            }
        }

    /*  Get width of string -- if it is variable pitch, then load
     *  the width table and build up the widths.  If it is fixed pitch,
     *  or we fail to load the width table, then use dfPixWidth.
     *  This code is repeated in GetCharWidth(), if you change it here,
     *  then change it there.
     *
     *  For ExtTextOut(), expand the array widths into two arrays -- an
     *  array of true character widths followed by an array of delta-x
     *  moves.
     */
    if ((lpFont->dfPitchAndFamily & 0x1) &&
        (widthptr = (short far *)LoadWidthTable(lpDevice,
            ((LPPRDFONTINFO)lpFont)->indFontSummary)))
        {
        /*  NOTE: The width table has been built in Windows ANSI order,
         *  so we get the widths by referencing the original string,
         *  not the translated string.
         */
        for (lpw=lpWidths, lpdx=&lpWidths[count], i=0; i < count;
            ++i, ++lpw, ++lpdx)
            {
            thisChar = (BYTE)lpString[i];

            if (thisChar == (BYTE)0xA0)
                {
                /*  Detect fixed space and return width of normal space.
                 */
                width = widthptr[(BYTE)' ' - dfFirstChar];
                }
            else if ((thisChar >= dfFirstChar) && (thisChar <= dfLastChar))
                width = widthptr[thisChar - dfFirstChar];
            else
                width = widthptr[dfDefaultChar];

            /*  Build an array of delta-x moves if ExtTextOut() passed
             *  in array of widths -- we subtract the true character
             *  width from the desired width (passed in) to get the
             *  amount by which we need to adjust.
             */
            if (gotdeltax)
                {
                *lpdx = *lpw;
                *lpdx -= width;
                }
            else
                *lpdx = 0;

            /*  Build an array of character widths used for clipping
             *  text.  This code MUST come AFTER we adjust the array
             *  of delta-x moves.
             */
            *lpw = width;

            #ifdef DBGdumpwidth
            DBMSG(("str_out(): width [%c%d] = %d\n",
                (char)thisChar, (WORD)thisChar, width));
            #endif
            }

        UnloadWidthTable(lpDevice, ((LPPRDFONTINFO)lpFont)->indFontSummary);
        }
    else
        {
        size = lpFont->dfPixWidth * count;

        /*  Set up the array of true character widths followed by an
         *  array of delta-x moves -- the width-getting code above for
         *  variable width characters describes what is happening.
         */
        for (lpw=lpWidths, lpdx=&lpWidths[count], i=0; i < count;
            ++i, ++lpw, ++lpdx)
            {
            if (gotdeltax)
                {
                *lpdx = *lpw;
                *lpdx -= lpFont->dfPixWidth;
                }
            else
                *lpdx = 0;

            *lpw = lpFont->dfPixWidth;

            #ifdef DBGdumpwidth
            DBMSG(("str_out(): fixed-pitch width = %d\n", (short)*lpw));
            #endif
            }
        }

    if (real)
      {
	/*
	 * aldus_change - The following line added to correct a problem
	 * with random bolding when simulating bolding.
	 */
	lpDevice->epXerr = 0;

        ymoveto(lpDevice, ypos + lpFont->dfAscent);

        /*  HACK for the Z cartridge.  The fonts in the Z cartridge
         *  are offset 0.017 inch to the right.  We detect if the
         *  font is from the Z cartridge and adjust by shifting our
         *  x position 0.017 inch to the left (this applies only to
         *  the variable width fonts).
         */
        if (((LPPRDFONTINFO)lpFont)->ZCART_hack &&
            (lpFont->dfPitchAndFamily & 0x1))
	  {
           /* Always subtract the 0.017 inches.  xmoveto() will fail if
            * xpos is < 0, but ClipRect prevents such call from being
            * made since ClipRect.left is (now) always >= 0.
            */
            if (xpos >= 5)
                xpos -= 5;
            else if (xpos > 0)
                xpos = 0;
#if 0
            if ((xpos -= 5) < 0)
                xpos = 0;
#endif

            if (lpClipRect)
	      {
                lmemcpy((LPSTR) &ZCARTrect, (LPSTR)lpClipRect, sizeof(RECT));
                lpClipRect = (LPRECT) &ZCARTrect;

                if ((lpClipRect->left -= 5) < 0)
                    lpClipRect->left = 0;
                lpClipRect->right -= 5;
	      }
	  }
      }

    SetJustification(lpDevice,lpDrawMode,&JustType,&JustifyWB,&JustifyLTR);

    hTransData = 0;
    lpTransTbl = 0L;
    updatex = TRUE;
    shiftCH = FALSE;
    size = 0;

    /*  For each character...figure out the justification to be
     *  applied to the next character, then output the character
     *  provided it is within the clipping rectangle.
     */
    for (lpw=lpWidths, lpdx=&lpWidths[count], nextwidth=prevwidth=i=0;
        i < count;
        xpos += *lpw + prevwidth, prevwidth=nextwidth, ++i, ++lpw, ++lpdx)
        {
        thisChar = lpString[i];
        overStrike = 0;

        /*  Yet another HACK:  set a flag indicating that a
         *  shift-out and shift-in should embrace output of this
         *  character so as to get good looking quotes.
         *
         *  This happens in the case where the symbol set is
         *  ECMA-94 (Z1a and S2 cartridges).
         */
        if (((LPPRDFONTINFO)lpFont)->QUOTE_hack &&
            (thisChar == 145 || thisChar == 146) &&
            (((LPPRDFONTINFO)lpFont)->symbolSet == epsymECMA94))
            {
            shiftCH = TRUE;
            }
        else
            {
            shiftCH = FALSE;
            }

        if (thisChar == (BYTE)0xA0)
            {
            /*  Fixed space, do no translation.
             */
            }
        else if ((thisChar < dfFirstChar) || (thisChar > dfLastChar))
            {
            /*  Character out of range, use default.
             */
            thisChar = dfDefaultChar;
	    }
	else if (((LPPRDFONTINFO)lpFont)->symbolSet == epsymMath8)
	    {
	    /* translation of Windows Symbol into HP Math 8
	     */
	    if (!lpTransTbl)
		{
		if (!(hTransData = GetTransTable(hLibInst,
                    &lpTransTbl, ((LPPRDFONTINFO)lpFont)->symbolSet)))
                    {
                    goto backout1;
                    }
		}
	    thisChar = lpTransTbl[(WORD)thisChar];
	    }
        else if (thisChar > (BYTE)0x7F)
            {
            /*  We encountered a character which must be translated, so
             *  we'll load in the translation table from the resources.
             */
            if (!lpTransTbl)
                {
		if (!(hTransData = GetTransTable(hLibInst,
                    &lpTransTbl, ((LPPRDFONTINFO)lpFont)->symbolSet)))
                    {
                    goto backout1;
                    }
                }

            /*  Pick up the replacement character and its overstrike.
             */
            j = ((WORD)lpString[i] - TRANS_MIN) * 2;
            thisChar = lpTransTbl[j];
            overStrike = lpTransTbl[j+1];
            }

        if (JustType == justifyletters)
            nextwidth = GetJustBreak(&JustifyLTR, JustType);
        else
            {
            nextwidth = JustifyLTR.extra;

            if ((JustType != fromdrawmode) && JustifyLTR.count &&
                (++JustifyLTR.ccount > JustifyLTR.count))
                {
                /*  Justification should drop off after we've
                 *  output count characters.
                 */
                nextwidth = 0;
                }
            }

        if (thisChar == dfBreakChar)
            nextwidth += GetJustBreak(&JustifyWB, JustType);

        /*  Extended text out, add in the per-character
         *  adjustment.
         */
        nextwidth += *lpdx;

        if (real)
            {
            if (lpClipRect)
                {
                /*  Do not output any text outside of the
                 *  clipping rectangle.
                 *
                 *  added nextwidth to righthand test  5 Oct 1989  clarkc
                 */
                if ((xpos + prevwidth) < lpClipRect->left)
                    continue;
                else if ((xpos + *lpw + nextwidth) > lpClipRect->right)
                    break;
                }

            if (updatex)
                {
                /*  Once we've figured out the correct beginning of
                 *  the string, then move to it.  Also set size to 
                 *  zero -- we'll completely re-compute the length
                 *  of the string taking into account justification
                 *  and clipping.
                 */
                nNotStruckOut = size = 0;
                if (prevwidth)
                    {
                    /*  prevwidth must be zero, adjust size and xpos
                     *  so we can set prevwidth to zero.
                     */
                    nNotStruckOut = size = prevwidth;
                    xpos += prevwidth;
                    prevwidth = 0;
		    }

		/* FORCE the xmove to be output by invalidating the x
		 * position... Fix for Whimper bug...  CraigC
		 */
		lpDevice->epCurx = 0xFFFF;

                xmoveto(lpDevice, xpos);
                err = StartStyle(lpDevice, lpXform);
                updatex = FALSE;
                }

            if (shiftCH)
                myWrite(lpDevice, SHIFT_OUT);

            /*  Output the character.
             */
            err = char_out(lpDevice, thisChar, prevwidth, overStrike);

            if (shiftCH)
                myWrite(lpDevice, SHIFT_IN);
            }

        /*  If we got this far, the character was actually output,
         *  so adjust the length of the string.
         */
        size += *lpw + prevwidth;
        }

    /*  If we loaded up a translation table, unlock it now.  The
     *  table is compiled as discardable so Windows will free it
     *  if it needs the space.
     */
    if (hTransData)
        {
        GlobalUnlock(hTransData);
        lpTransTbl = 0L;
        }

    if (real)
        {
        /*  We really output the line, end any special styles
         *  and update our record of xpos.
         */
        err = EndStyle(lpDevice, lpXform, size - nNotStruckOut);
        lpDevice->epCurx = xpos;

	// Reset running error (04 aug 89 peterbe)
	lpDevice->epXerr = 0;
        }
    else if (!isWhite(lpDrawMode->TextColor, lpDevice->epTxWhite))
        {
        /*  Justification information gets updated if we
         *  are getting the extent of the line, but not if
         *  we are outputting the line.
         *
         *  If the text color is white, then str_out was called
         *  just to return a valid width (count < 0), but this
         *  call originated as a text output, so do not update
         *  the justification parameters.
         */
        lpDrawMode->BreakErr = JustifyWB.err;

        if (JustType != fromdrawmode)
            {
            lpDevice->epJustWB.err = JustifyWB.err;
            lpDevice->epJustWB.ccount = JustifyWB.ccount;
            lpDevice->epJustLTR.err = JustifyLTR.err;
            lpDevice->epJustLTR.ccount = JustifyLTR.ccount;
            }
        }

    /*  The if statement below has been moved to AFTER the call to
     *  EndStyle().  This helps synthesized strikeout work.
     */
    /*  Add in the adjustment to the last character, so the
     *  length of the line will correctly reflect justification
     *  and ExtTextOut() adjustments.
     */
    if (nextwidth)
        {
        size += nextwidth;

        /*  If we're doing strikeout or underline, we have to actually
         *  move the cursor so the lines end up in the right place.
         */
        if (real && (lpXform->ftStrikeOut || lpXform->ftUnderline))
            RelXMove(lpDevice, nextwidth);
        }

    status = MAKELONG(lpFont->dfPixHeight, size);

backout1:
    /*  Unlock our work area for character widths -- we'll
     *  lock it down the next time we need it, and delete it
     *  when Disable() is called.
     */
    if (lpWidths && lpDevice->epHWidths)
        {
        GlobalUnlock(lpDevice->epHWidths);
        lpWidths = 0L;
        }

backout0:
    return (status);
    }	// str_out()

/*  char_out()
 */
static int char_out(lpDevice, c, width, overstrike)
    LPDEVICE lpDevice;
    char c;
    short width;
    BYTE overstrike;
    {
    int err = SUCCESS;
    char buf[2];

    DBGtrace(("In char_out,c=%c\n", c));

    /*  Advance cursor if necessary.
     */
    if (width)
        err = RelXMove(lpDevice, width);

    /*  Trap fixed space (160) and turn into normal space.
     */
    if ((BYTE)c == (BYTE)0xA0)
        c = ' ';

    err = myWrite(lpDevice, (LPSTR) &c, 1);

    if (overstrike)
        {
        myWrite(lpDevice, PUSH_POSITION);
        buf[0] = '\010';
        buf[1] = overstrike;
        err = myWrite(lpDevice, (LPSTR)buf, 2);
        myWrite(lpDevice, POP_POSITION);
        }

    return (err);

    }	// char_out()

/*  RelXMove()
 *
 *  Send out relative cursor positioning.
 */
static int RelXMove(lpDevice, xmove)
    LPDEVICE lpDevice;
    short xmove;
    {
    register short n, m = 0;
    ESCtype escape;
    int err = SUCCESS;

    xmove *= 12;
    xmove += lpDevice->epXerr;
    lpDevice->epXerr = xmove % 5;
    xmove /= 5;

    if (lpDevice->epXerr > 0)
        {
        ++xmove;
        lpDevice->epXerr -= 5;
        }

    if (xmove)
        {
        /* #define HP_HCP   '&', 'a', 'H'
         */
        escape.esc = '\033';
        escape.start1 = '&';
        escape.start2 = 'a';
        if (xmove >= 0)
            escape.num[m++] = '+';

        n = itoa(xmove, &escape.num[m]);
        escape.num[n+m] = 'H';
        err = myWrite(lpDevice, (LPSTR) &escape, n+m+4);
        }

    return (err);

    }	// RelXMove()

/*  StartStyle()
 */
static int StartStyle(lpDevice, lpXform)
    LPDEVICE lpDevice;
    LPTEXTXFORM lpXform;
    {
    int err = SUCCESS;

    if (lpXform->ftUnderline)
        err = myWrite(lpDevice, (LPSTR)HP_UNDERLINE_ON);

    return (err);

    }	// StartStyle()

/*  EndStyle()
 */
static int EndStyle(lpDevice, lpXform, size)
    LPDEVICE lpDevice;
    LPTEXTXFORM lpXform;
    short size;
    {
    ESCtype escape;
    int err;

    /*  Strike-out the line -- only if device is capable of handling
     *  the rule command.
     */
    if (lpXform->ftStrikeOut && !(lpDevice->epCaps & HPJET) && (size > 0))
        {
        short hrule = size;
        short vrule = STRIKEOUT_THICKNESS;
        short vpos = lpXform->ftHeight / 5 + vrule;

        /*  Save current position, set new position (beginning of line),
         *  set horizontal and vertical rule size, output the rule,
         *  restore position.
         */
        err = myWrite(lpDevice, (LPSTR)PUSH_POSITION);

        err = myWrite(lpDevice, (LPSTR) &escape,
            MakeEscape(&escape, DOT_HCP, -size));

        if (vpos > 0)
            err = myWrite(lpDevice, (LPSTR) &escape,
                MakeEscape(&escape, DOT_VCP, -vpos));

        err = myWrite(lpDevice, (LPSTR) &escape,
            MakeEscape(&escape, DOT_HRPS, hrule));
        err = myWrite(lpDevice, (LPSTR) &escape,
            MakeEscape(&escape, DOT_VRPS, vrule));
        err = myWrite(lpDevice, BLACK_PATTERN);
        err = myWrite(lpDevice, (LPSTR)POP_POSITION);
        }

    if (lpXform->ftUnderline)
        err = myWrite(lpDevice, (LPSTR)HP_UNDERLINE_OFF);

    return (err);

    }	// EndStyle()

/*  SetJustification()
 *
 *  Set up the justification records and return TRUE if justification
 *  will be required on the line.  The justification values can come
 *  from two places:
 *
 *      1. Windows GDI SetTextCharacterExtra() and
 *          SetTextJustification(), which handle only
 *          positive justification.
 *      2. The SETALLJUSTVALUES escape, which handles negative
 *          and positive justification.
 *
 *  Windows' justification parameters are stored in the DRAWMODE struct,
 *  while SETALLJUSTVALUES stuff comes from our DEVICE struct.
 */
static BOOL SetJustification(lpDevice, lpDrawMode, lpJustType,
        lpJustifyWB, lpJustifyLTR)
    LPDEVICE lpDevice;
    LPDRAWMODE lpDrawMode;
    LPJUSTBREAKTYPE lpJustType;
    LPJUSTBREAKREC lpJustifyWB;
    LPJUSTBREAKREC lpJustifyLTR;
    {
    if ((*lpJustType = lpDevice->epJust) == fromdrawmode)
        {
        /*  Normal Windows justification.
         */
        if (lpDrawMode->TBreakExtra)
            {
            lpJustifyWB->extra = lpDrawMode->BreakExtra;
            lpJustifyWB->rem = lpDrawMode->BreakRem;
            lpJustifyWB->err = lpDrawMode->BreakErr;
            lpJustifyWB->count = lpDrawMode->BreakCount;
            lpJustifyWB->ccount = 0;
            }
        else
            {
            lpJustifyWB->extra = 0;
            lpJustifyWB->rem = 0;
            lpJustifyWB->err = 1;
            lpJustifyWB->count = 0;
            lpJustifyWB->ccount = 0;
            }

        lpJustifyLTR->extra = lpDrawMode->CharExtra;
        lpJustifyLTR->rem = 0;
        lpJustifyLTR->err = 1;
        lpJustifyLTR->count = 0;
        lpJustifyLTR->ccount = 0;
        }
    else
        {
        /*  SETALLJUSTVALUES -- the records were filled when the
         *  escape was called, now make local copies.
         */
        lmemcpy((LPSTR)lpJustifyWB, (LPSTR) &lpDevice->epJustWB,
            sizeof(JUSTBREAKREC));
        lmemcpy((LPSTR)lpJustifyLTR, (LPSTR) &lpDevice->epJustLTR,
            sizeof(JUSTBREAKREC));
        }

    /*  Advise the caller as to whether or not justification
     *  adjustments will be required.
     */
    if (lpJustifyWB->extra || lpJustifyWB->rem || lpJustifyWB->count ||
            lpJustifyLTR->extra || lpJustifyLTR->rem || lpJustifyLTR->count)
        return TRUE;
    else
        return FALSE;

    }	// SetJustification()

/*  GetJustBreak()
 *
 *  Calculate the additional pixels to add/subtract from the horizontal
 *  position.
 */
static short GetJustBreak (lpJustBreak, justType)
    LPJUSTBREAKREC lpJustBreak;
    JUSTBREAKTYPE justType;
    {
    short adjust = lpJustBreak->extra;

    /*  Update the err value and add in the distributed adjustment.
     */
    if ((lpJustBreak->err -= lpJustBreak->rem) <= 0)
        {
        ++adjust;
        lpJustBreak->err += (short)lpJustBreak->count;
        }

    if ((justType != fromdrawmode) && lpJustBreak->count &&
        (++lpJustBreak->ccount > lpJustBreak->count))
        {
        /*  Not at a valid character position, return zero adjustment.
         */
        adjust = 0;
        }

    return (adjust);
    }	// GetJustBreak()

/*  DoubleSizeCopy()
 *
 *  Allocate a structure twice the size of the passed-in struct, and
 *  copy the passed-in struct into the first half.
 */
static LPSTR DoubleSizeCopy(lpHSrc, lpSrc, len, size)
    LPHANDLE lpHSrc;
    LPSTR lpSrc;
    short len;
    short size;
    {
    LPSTR lpDst = 0L;
    DWORD allocSize = 0;

    /*  Size of array must be atleast MAXNUM_WIDTHS * 2.
     */
    if (len <= MAXNUM_WIDTHS)
        allocSize = MAXNUM_WIDTHS * 2 * size;
    else
        {
        allocSize = len * 2 * size;

        /*  Array needs to be longer than MAXNUM_WIDTHS * 2.
         *  If it already exists, attempt to lengthen it.
         */
        if (*lpHSrc && (GlobalSize(*lpHSrc) < allocSize))
            {
            GlobalFree(*lpHSrc);
            *lpHSrc = 0;
            }
        }

    /*  If the array does not exist, then allocate it.
     */
    if (!(*lpHSrc))
        *lpHSrc = GlobalAlloc(GMEM_MOVEABLE, allocSize);

    /*  Lock down the array if it exists.
     */
    if ((*lpHSrc) && (!(lpDst = GlobalLock(*lpHSrc))))
        {
        GlobalFree(*lpHSrc);
        *lpHSrc = 0;
        }

    /*  Initialize to all zeros.
     */
    if (lpDst)
        lmemset(lpDst, 0, len * size * 2);

    /*  Copy the array of widths.
     */
    if (lpDst && lpSrc)
        lmemcpy(lpDst, lpSrc, len * size);

    return (lpDst);
    }	// DoubleSizeCopy()

/*  isWhite()
 *
 *  Return TRUE if each component of pcolor has an intensity greater
 *  than or equal to the value of white.
 */
static BOOL isWhite(pcolor, white)
    long pcolor;
    short white;
    {
    BYTE r, g, b, cmp;

    if (white > 255)
        {
        DBGtrace(("isWhite(%ld,%d): color is not white\n", pcolor, white));
        return FALSE;
        }

    cmp = (BYTE)white;
    r = GetRValue(pcolor);
    g = GetGValue(pcolor);
    b = GetBValue(pcolor);

    #ifdef DEBUG
    if (r >= cmp && g >= cmp && b >= cmp)
        { DBGtrace(("isWhite(r%d,g%d,b%d,%d): color is white\n",
            (WORD)r, (WORD)g, (WORD)b, white)); }
    else
        { DBGtrace(("isWhite(r%d,g%d,b%d,%d): color is not white\n",
            (WORD)r, (WORD)g, (WORD)b, white)); }
    #endif

    return (r >= cmp && g >= cmp && b >= cmp);

    }	// isWhite()
