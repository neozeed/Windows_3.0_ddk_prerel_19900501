/**[f******************************************************************
 * realize.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   realize.c   ******************************/
//
//  Font enumeration module.
//
//  rev:
//
// 20 mar 90	clarkc  set dfFace = 0, avoiding walking off segment.
// 02 feb 90	clarkc  lmemcpy() to lstrncpy(), avoiding walking off segment.
// 25 oct 89	peterbe	sorted init. values by type to elim. annoying compiler
//			warning
// 10-12-89@12:32:59 (Thu) CRAIGC realize a default font for memory dc's 
// 15 aug 89	peterbe	Changed 'int .. EnumDFonts()'
// 07 aug 89	peterbe Changed lstrcmp() to lstrcmpi().
//  11-30-86	msd	cleanup/fix checkString() and Translate()
//  11-16-86	msd	integrated fontSummary structure into RealizObject
//  11-09-86	msd	cleanup, document and add lots of debug stuff
//  11-07-86	msd	compile for Wrning level 2
//  12-29-88	jimmat	Added some quick(er)-out checks to RealizeObject()
//			and reduced the number of locks/unlocks by xxxFace()
//			routines.
//   1-17-89	jimmat	Added PCL_* entry points to lock/unlock data seg.
//   1-19-89	jimmat	Return a fixed size for font structure--this way we
//			don't have to realize every font twice (once just
//                      to get the len of the facename, once to really do it).
//   2-24-89    jimmat  Converted the face table from a resource to a table
//                      resident in the code segment--no need to load/lock,etc.
//

#include "generic.h"
#include "resource.h"
#define FONTMAN_ENABLE
#include "fontman.h"
#include "fonts.h"

/*  lockfont utility
 */
#include "lockfont.c"

/*  dfType through dfBreakChar in FONTINFO, for realizing memory DC fonts
 */
#define CBMEMFONT 0x21

/*  Local debug structure.
 */
#ifdef DEBUG
#define LOCAL_DEBUG
/* #define DBGdumprealize */
#endif

#ifdef LOCAL_DEBUG
/*  Debug switches: entry - message on entry to primary procs
 *                  err   - message when an unexpected err occurs
 *                  proc  - detailed messages specific to the proc
 */
#define DBGentry(msg)        /*DBMSG(msg)*/
#define DBGerr(msg)            DBMSG(msg)
#define DBGrealize(msg)        DBMSG(msg)
#define DBGfontselect(msg)   /*DBMSG(msg)*/
#define DBGinfotostruct(msg) /*DBMSG(msg)*/
#define DBGextractfont(msg)  /*DBMSG(msg)*/
#define DBGenumfonts(msg)    /*DBMSG(msg)*/
#define DBGface(msg)           DBMSG(msg)
#else
#define DBGentry(msg)        /*null*/
#define DBGerr(msg)          /*null*/
#define DBGrealize(msg)      /*null*/
#define DBGfontselect(msg)   /*null*/
#define DBGinfotostruct(msg) /*null*/
#define DBGextractfont(msg)  /*null*/
#define DBGenumfonts(msg)    /*null*/
#define DBGface(msg)         /*null*/
#endif


/*  Macro
 */
#define tmPitchTOlfPitch(pitch) \
    ((pitch) ? (BYTE)VARIABLE_PITCH : (BYTE)FIXED_PITCH)


extern  HANDLE hLibInst;            /* driver's instance handle */


/*  Forward procedures.
 */

short FAR PASCAL RealizeObject(LPDEVICE,short,LPLOGFONT,LPFONTINFO,LPTEXTXFORM);
int   FAR PASCAL EnumDFonts(LPDEVICE,LPSTR,FARPROC,long);

void InfoToStruct(LPFONTINFO, short, LPSTR);
void ExtractFontInfo(LPFONTINFO, LPFONTSUMMARY);

#if defined(OLD_FACE_STUFF) /*********************************************/
 void initFace(HANDLE FAR *);
 BOOL aliasFace(LPDEVICE, HANDLE FAR *, LPLOGFONT, LPSTR);
 BOOL defaultFace(LPDEVICE, HANDLE FAR *, LPSTR);
 void endFace(HANDLE FAR *);
 LPSTR lockFace(LPDEVICE, HANDLE FAR *);
#else
 BOOL defaultFace(LPSTR);
 BOOL aliasFace(LPLOGFONT, LPSTR);
#endif			    /*********************************************/

/***********************************************************************
                  P C L _ R E A L I Z E  O B J E C T
 ***********************************************************************/

/*  RealizeObject entry point to lock/unlock the data seg.
 */

short FAR PASCAL
PCL_RealizeObject(LPDEVICE lpDevice, short Style, LPLOGFONT lpInObj,
		  LPFONTINFO lpOutObj, LPTEXTXFORM lpTextXForm) {

    short rc;

    LockSegment(-1);

    rc = RealizeObject(lpDevice, Style, lpInObj, lpOutObj, lpTextXForm);

    UnlockSegment(-1);

    return rc;
}

/***********************************************************************
                      R E A L I Z E  O B J E C T
 ***********************************************************************/

short far PASCAL RealizeObject(lpDevice, Style, lpInObj, lpOutObj,
	lpTextXForm)
    LPDEVICE lpDevice;
    short Style;
    LPLOGFONT lpInObj;
    LPFONTINFO lpOutObj;
    LPTEXTXFORM lpTextXForm;
    {
    HANDLE hResInfo;
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY lpSummary;
    LPSTR fontNameTable, dfFaceName, lpFace;
    unsigned long bestvalue, value;
    short tmp, tmp2, ind, bestind, len;
#if defined(OLD_FACE_STUFF)
    HANDLE hFaces;
#endif

    if (!lpDevice)
	return(0);

    DBGentry(("RealizeObject(%lp,0x%x,%lp,%lp,%lp)\n",
	lpDevice, Style, lpInObj, lpOutObj, lpTextXForm));

    /*  We've been asked to delete the font, take no action but
     *  return success.
     */
    if (Style == -OBJ_FONT)
	{
	DBGrealize(("RealizeObject(): Style(%d) == -OBJ_FONT(%d)\n",
	    Style, OBJ_FONT));
	return (1);
	}

    /*  Cannot realize anything but fonts.
     */
    if (Style != OBJ_FONT)
	{
	/* DBGrealize(("RealizeObject(): Style(%d) != OBJ_FONT(%d)\n",
        **    Style, OBJ_FONT));
        */
	return dmRealizeObject(lpDevice->epType ? (LPDEVICE) &
	    lpDevice->epBmpHdr : lpDevice, Style, (LPSTR)lpInObj,
	    (LPSTR)lpOutObj, (LPSTR)lpTextXForm);
	}

    if (!lpDevice->epType)
      {
	DBGerr(("RealizeObject(): !lpDevice->epType\n"));

	/* realizing a font for a memory DC.  Used to return 0, but that
	 * causes GDI to try and give us a font.  So return a dumb one.
	 */

	if (!lpOutObj)
	    return CBMEMFONT;

	lpOutObj->dfType = 0x80;		/* device */
	lpOutObj->dfPoints = 12;
	lpOutObj->dfVertRes = lpOutObj->dfHorizRes = 300;

	lpOutObj->dfWeight = 400;
	lpOutObj->dfPixWidth = 30;
	lpOutObj->dfPixHeight = 50;
	lpOutObj->dfPitchAndFamily = FF_MODERN;
	lpOutObj->dfAvgWidth = 30;
	lpOutObj->dfMaxWidth = 30;
	lpOutObj->dfFirstChar = 32;
	lpOutObj->dfLastChar = 255;


	lpOutObj->dfAscent =			// 0 int values
	lpOutObj->dfInternalLeading =
	lpOutObj->dfExternalLeading = 0;

	lpOutObj->dfDefaultChar =		// 0 char values
	lpOutObj->dfBreakChar =
	lpOutObj->dfItalic =

	lpOutObj->dfUnderline =
	lpOutObj->dfStrikeOut =
	lpOutObj->dfCharSet = (BYTE) 0;

	lpOutObj->dfFace = 0L;        /* offset to dfType, hiword will be 0 */

	lpTextXForm->ftHeight = 50;
	lpTextXForm->ftWidth = 30;

	lpTextXForm->ftWeight = 400;

	lpTextXForm->ftOutPrecision = OUT_DEFAULT_PRECIS;
	lpTextXForm->ftClipPrecision = CLIP_DEFAULT_PRECIS;

	lpTextXForm->ftItalic =			// 0 byte values
	lpTextXForm->ftUnderline =
	lpTextXForm->ftStrikeOut = (BYTE) 0;

	lpTextXForm->ftEscapement =		// 0 word values
	lpTextXForm->ftOrientation =

	lpTextXForm->ftAccelerator =
	lpTextXForm->ftOverhang = 0;

	return CBMEMFONT;
      }

    /*  Cannot realize OEM character sets (vector fonts).
     */
    if (lpInObj->lfCharSet == OEM_CHARSET)
	{
	DBGerr(("RealizeObject(): OEM_CHARSET\n"));
	return (0);
	}

    /*  If the fontSummary structure is not there, cannot continue.
     */
    if (!(lpFontSummary = lockFontSummary(lpDevice)))
	{
	DBGerr(("RealizeObject(): could *not* lock fontSummary\n"));
	return (0);
	}

    /*  If there are no fonts in the fontSummary structure, then
     *  fail to realize every font.
     */
    if (!(len = lpFontSummary->len))
	{
	DBGerr(("RealizeObject(): no fonts in fontSummary, abort\n"));
	ind = 0;
	goto exit;
	}

    /*  If lpOutObj == 0 then the caller only wants the size of
     *  the struct.
     */
    if (!lpOutObj)
	{
	unlockFontSummary(lpDevice);
	ind = sizeof(PRDFONTINFO) + LF_FACESIZE + 1;
	DBGrealize(("RealizeObject(): !lpOutObj return size %d\n", ind));
	return(ind);
	}

    lpSummary = &lpFontSummary->f[0];
    fontNameTable = (LPSTR) &lpFontSummary->f[len];
    bestvalue = 0xFFFFFFFFL;

#if defined(OLD_FACE_STUFF)
    initFace(&hFaces);
#endif

    DBGrealize(("RealizeObject(): Searching for font, len=%d\n", len));

    /*  Traverse the list of fonts and locate the font with the
     *  lowest penalty value.
     */
    for (ind = 0; ind < len; ++ind, ++lpSummary)
	{
	value = 0L;

	/*  character set
         */
	if (lpInObj->lfCharSet != lpSummary->dfCharSet)
	    {
	    value += 1 << CHARSET_WEIGHT;
	    DBGfontselect(("   %d: charset %d to %d\n",
		(1 << CHARSET_WEIGHT), lpInObj->lfCharSet,
		lpSummary->dfCharSet));
	    if (value > bestvalue)
		continue;
	    }

	/* The facename check can take "some time" so check the pitch and
           family first, with the hope that these checks will rule out
           some fonts that would otherwise go through the facename process. */

	/*  pitch
         *
         *  no penalty if lfPitch or dfPitch == dontcare
         */
	if ((tmp = (lpInObj->lfPitchAndFamily & 0x03)) &&
	    (tmp2 = tmPitchTOlfPitch(lpSummary->dfPitchAndFamily & 0x03)) &&
	    !(tmp & tmp2))
	    {
	    value += 1 << PITCH_WEIGHT;
	    DBGfontselect(("   %d: pitch %d to %d\n",
		(1 << PITCH_WEIGHT), (unsigned)tmp,
		(unsigned)tmPitchTOlfPitch(
			lpSummary->dfPitchAndFamily & 0x0F)));
	    if (value > bestvalue)
		continue;
	    }

	/*  family
         *
         *  no penalty if lfFamily or dfFamily == dontcare
         */
	if ((tmp = (lpInObj->lfPitchAndFamily & 0xF0)) &&
	    (tmp2 = (lpSummary->dfPitchAndFamily & 0xF0)) &&
	    (tmp != tmp2))
	    {
	    value += 1 << FAMILY_WEIGHT;
	    DBGfontselect(("   %d: family %d to %d\n",
		(1 << FAMILY_WEIGHT), (unsigned)tmp,
		(unsigned)(lpSummary->dfPitchAndFamily & 0xF0)));
	    if (value > bestvalue)
		continue;
	    }

	/*  facename
         */
	dfFaceName = (LPSTR) &fontNameTable[lpSummary->indName];

	DBGfontselect(("FONTSELECT: comparing '%ls' to '%ls'\n",
		      (LPSTR)lpInObj->lfFaceName, dfFaceName));

	if ((lstrlen((LPSTR)lpInObj->lfFaceName) > 0) &&
	    (lstrlen((LPSTR)dfFaceName) > 0))
	    {
	    /* We got valid face names, penalize if they do not match.
             */
	    if (lstrcmpi((LPSTR)lpInObj->lfFaceName, dfFaceName))
		{
#if defined(OLD_FACE_STUFF)
		if (aliasFace(lpDevice, &hFaces, lpInObj, dfFaceName))
#else
		if (aliasFace(lpInObj, dfFaceName))
#endif
		    {
		    value += 1;
		    DBGfontselect(("   1: facename (alternate face)\n"));
		    }
		else
		    {
		    value += 1 << FACENAME_WEIGHT;
		    DBGfontselect(
			("   %d: facename\n", (1 << FACENAME_WEIGHT)));
		    }
		}
	    }
	else
	    {
	    /* Penalize if we got zero string lengths, but do not penalize
             * if we are looking at a Courier font and the caller has
             * requested a fixed pitch font and no font family
             * (this ensures that the stock object will end up being Courier).
             */
	    if (!lstrlen((LPSTR)dfFaceName) ||
		((lpInObj->lfPitchAndFamily & 0xF3) !=
			(FIXED_PITCH | FF_DONTCARE)) ||
#if defined(OLD_FACE_STUFF)
		!defaultFace(lpDevice, &hFaces, dfFaceName))
#else
		!defaultFace(dfFaceName))
#endif
		{
		value += 1 << FACENAME_WEIGHT;
		DBGfontselect(
			("   %d: NULL facename\n", (1 << FACENAME_WEIGHT)));
		}
	    }

	if (value > bestvalue)
	    continue;


	/*  height
         *
         * If zero height requested, default to 12 points (this guarantees
         * that we will select a 12 point font for the stock object).
         */
	if (!(tmp = lpInObj->lfHeight))
	    {
	    tmp = -50;
	    }

	if (tmp < 0)
	    {
	    /*  The height we are asking for is negative, which means
             *  ignore internal leading.
             */
	    tmp = -tmp + lpSummary->dfInternalLeading;
	    }

	/*  Capture the difference in height.
         */
	tmp -= lpSummary->dfPixHeight;

	if (tmp > 0)
	    {
	    /*  The height we are asking for is greater than the
             *  height we have = small penalty.
             */
	    value += tmp << HEIGHT_WEIGHT;
	    DBGfontselect(("   %d: taller height %d to %d\n",
		(tmp << HEIGHT_WEIGHT), (tmp + lpSummary->dfPixHeight),
		lpSummary->dfPixHeight));
	    }
	else
	    {
	    /*  The height we are asking for is less than the height
             *  we have = large penalty.
             */
	    value += (-tmp) << LARGE_HEIGHT_WEIGHT;
	    DBGfontselect(("   %d: shorter height %d to %d\n",
		((-tmp) << LARGE_HEIGHT_WEIGHT),
		(tmp + lpSummary->dfPixHeight), lpSummary->dfPixHeight));
	    }

	/*  width
         *
         *  no penalty if lfWidth or dfAvgWidth == dontcare
         */
	if ((tmp = lpInObj->lfWidth) &&
	    (tmp2 = lpSummary->dfAvgWidth) &&
	    (tmp -= tmp2))
	    {
	    value += abs(tmp) << WIDTH_WEIGHT;
	    DBGfontselect(("   %d: width %d to %d\n",
		(abs(tmp) << WIDTH_WEIGHT), (tmp + lpSummary->dfAvgWidth),
		lpSummary->dfAvgWidth));
	    }

	/*  italic
         */
	if (lpInObj->lfItalic != lpSummary->dfItalic)
	    {
	    value += 1 << ITALIC_WEIGHT;
	    DBGfontselect(("   %d: width %d to %d\n",
		(1 << ITALIC_WEIGHT), lpInObj->lfItalic,
		lpSummary->dfItalic));
	    }

	/*  weight
         *
         *  no penalty if lfWeight or dfWeight == dontcare
         */
	if ((tmp = lpInObj->lfWeight) &&
	    (tmp2 = lpSummary->dfWeight) &&
	    (tmp -= tmp2))
	    {
	    value += abs(tmp) >> WEIGHT_WEIGHT;
	    DBGfontselect(("   %d: weight %d to %d\n",
		(abs(tmp) >> WEIGHT_WEIGHT), (tmp + lpSummary->dfWeight),
		lpSummary->dfWeight));
	    }

	DBGfontselect(("   value=%ld, bestvalue=%ld\n",
	    (unsigned long)value, (unsigned long)bestvalue));

	if (value <= bestvalue) {
	    bestvalue = value;
	    bestind = ind;
	}
	}
    lpSummary = &lpFontSummary->f[bestind];

#if defined(OLD_FACE_STUFF)
    endFace(&hFaces);
#endif

    DBGrealize(("RealizeObject(): font found at ind=%d, bestvalue=%ld\n",
	bestind, (unsigned long)bestvalue));

    /*  Debug stuff...
     */
#ifdef DBGdumprealize
    DBMSG(("\n"));
    DBMSG(("RealizeObject(): Font asked for:\n"));
    dfFaceName = (LPSTR) lpInObj->lfFaceName;
    DBMSG(("%ls %lp\n", dfFaceName, lpInObj));
    DBMSG(("   lfPitchAndFamily = %d\n", lpInObj->lfPitchAndFamily));
    DBMSG(("   lfHeight = %d\n", lpInObj->lfHeight));
    DBMSG(("   lfWidth = %d\n", lpInObj->lfWidth));
    DBMSG(("   lfWeight = %d\n", lpInObj->lfWeight));
    DBMSG(("   lfItalic = %d\n", lpInObj->lfItalic));
    DBMSG(("RealizeObject(): Font Selected:\n"));
    DBGdumpFontSummary(lpFontSummary, bestind);
#endif

    /*  Get face name of selected font.
     */
    dfFaceName = (LPSTR) &fontNameTable[lpSummary->indName];

    /*  Init caller's struct.
     */
    ind = sizeof(PRDFONTINFO) + LF_FACESIZE + 1;
    lmemset((LPSTR)lpOutObj, 0, ind);

    /*  Get basic data from fontSummary.
     */
    ExtractFontInfo(lpOutObj, lpSummary);
    ((LPPRDFONTINFO)lpOutObj)->indFontSummary = bestind;

    /*  Load face name.
     */
    len = lstrlen(dfFaceName);
    if (len > LF_FACESIZE)
	len = LF_FACESIZE;
    lpFace = (LPSTR)lpOutObj + (lpOutObj->dfFace = sizeof(PRDFONTINFO));
    lmemcpy(lpFace,dfFaceName,len);	    /* ending 0 set by lmemset() */

    /*  Chose symbol translation table.
     */
    ((LPPRDFONTINFO)lpOutObj)->symbolSet = lpSummary->symbolSet;

    /*  HACK for the Z cartridge -- the fonts on the Z cartridge
     *  are offset 0.017 inch to the right.  We have to detect that
     *  a font came from the Z cartridge and then adjust.
     */
    ((LPPRDFONTINFO)lpOutObj)->ZCART_hack = lpSummary->ZCART_hack;

    /*  HACK for typgraphic quotes -- for cartridges that use ECMA-94,
     *  we want to be able to switch out to USASCII to get better quotes.
     */
    ((LPPRDFONTINFO)lpOutObj)->QUOTE_hack = lpSummary->QUOTE_hack;

    /*  Set flag to indicate where the pfm file is, whether in
     *  resources or external PFM file.
     */
    ((LPPRDFONTINFO)lpOutObj)->isextpfm = (BOOL)(lpSummary->indPFMName > 0);

    /*  Copy to textXform struct.
     */
    InfoToStruct(lpOutObj, HP_TEXTXFORM, (LPSTR)lpTextXForm);

    /*  Adjust textXform data.
     */
#ifdef SYM_BOLD
    if ((lpInObj->lfWeight >= FW_BOLD) && (lpOutObj->dfWeight <= FW_NORMAL))
	{
	/*  Simulated bold (always simulate FW_BOLD regardless of
         *  how much bolding was requested).
         */
	lpTextXForm->ftWeight = FW_BOLD;

	/*  Calculate overstrike amount based on point size.
         */
	lpTextXForm->ftOverhang = 1 + (lpOutObj->dfPixHeight -
	    lpOutObj->dfInternalLeading) / TENPT_PIXHEIGHT;

	DBGrealize(
		("RealizeObject(): simulated bold lfWeight=%d, ftOverhang=%d\n",
	    lpInObj->lfWeight, lpTextXForm->ftOverhang));
	}
    else
	{
	/*  If not specifically bolding a font, then shut
         *  off the overhang stuff.
         */
	lpTextXForm->ftOverhang = 0;
	}
#endif

    lpTextXForm->ftUnderline = lpInObj->lfUnderline;

    if ((lpTextXForm->ftStrikeOut = lpInObj->lfStrikeOut) &&
	(lpDevice->epCaps & HPJET))
	{
	/*  We detected strikeout on the normal laserjet, which does
         *  not support the rule/pattern stuff, so we have to let GDI
         *  simulate the strikeout, which means we'll have to print
         *  all bands on the page (like vector fonts, the strikeout will
         *  come across as bitmaps).  The flag epGDItext is set in the
         *  GDI stub (stubs.c) procedure Pixel().
         */
	lpTextXForm->ftAccelerator |= TC_SO_ABLE;
	}

exit:
    unlockFontSummary(lpDevice);
    DBGrealize(("...end of RealizeObject, return %d\n", ind));
    return (ind);
    }

/***********************************************************************
                   P C L _ E N U M  D  F O N T S
 ***********************************************************************/

/*  EnumDFonts entry point to lock/unlock data segment.
 */

int FAR PASCAL
PCL_EnumDFonts(lpDevice, lpFaceName, lpCallbackFunc, lpClientData)
LPDEVICE lpDevice;
LPSTR lpFaceName;
FARPROC lpCallbackFunc;
long lpClientData;
{
    int rc;

    LockSegment(-1);

    rc = EnumDFonts(lpDevice, lpFaceName, lpCallbackFunc, lpClientData);

    UnlockSegment(-1);

    return rc;
}

/***********************************************************************
                       E N U M  D  F O N T S
 ***********************************************************************/

/*  Enumerate the list of fonts available to the application.  This proc
 *  keys itself on the value of lpFaceName:
 *
 *      1. if lpFaceName == NULL, then EnumDFonts lists the available
 *         family names (Courier, Tms Rmn, Helv) and calls the callback
 *         procedure with each family name.
 *
 *      2. if lpFacename == font family name, then EnumDFonts lists the
 *         available faces within the family (Tms Rmn 10pt, Tms Rmn 12pt)
 *         and calls the callback procedure with each font's logical and
 *         textmetric information.
 *
 *  The application is expected to enumerate fonts in the following manner:
 *
 *      A. EnumDFonts is called with lpFaceName == NULL and a pointer to
 *         callback function #1.  EnumDFonts calls callback function #1
 *         with each font family name.
 *
 *      B. Callback function #1 calls EnumDFonts with lpFaceName == font
 *         family name and a pointer to callback function #2.  EnumDFonts
 *         calls callback function #2 with each fonts' logical and
 *         textmetric information.
 *
 *      C. Callback function #2 stashes the font information into a data
 *         structure the application uses during execution.
 */

//near PASCAL NullFaceName()
//{
//}

int far PASCAL EnumDFonts(lpDevice, lpFaceName, lpCallbackFunc, lpClientData)
    LPDEVICE lpDevice;
    LPSTR lpFaceName;
    FARPROC lpCallbackFunc;
    long lpClientData;
    {
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY lpSummary;
    TEXTMETRIC TextMetric;
    FONTINFO fontInfo;
    LOGFONT LogFont;
    LPSTR fontNameTable, lpRefName;
    short status, ind, len, namelen, indPrevName = -1;

    DBGenumfonts(("EnumDFonts(%lp,%lp,%lp,%lp)\n",
	lpDevice, lpFaceName, lpCallbackFunc, lpClientData));

    /*  Cannot enumerate for anything but a LaserJet DC.
     */
    if (!lpDevice->epType)
	{
	DBGerr(("EnumDFonts(): !lpDevice->epType\n"));
	return (1);
	}

    /*  If the fontSummary structure is not there, cannot continue.
     */
    if (!(lpFontSummary = lockFontSummary(lpDevice)))
	{
	DBGerr(("EnumDFonts(): could *not* lock fontSummary\n"));
	return (1);
	}

    /*  If a face name was passed in, get its length (used to indicate
     *  that facename exists).
     */
    if (lpFaceName && *lpFaceName)
	{
	DBGenumfonts(("EnumDFonts(): FaceName=%ls\n", lpFaceName));
	namelen = lstrlen (lpFaceName);
	}
    else
	{
	// lpFaceName was NULL or pointed to NULL string
	//NullFaceName();		// for debug
	namelen = 0;
	}

    lpSummary = &lpFontSummary->f[0];
    len = lpFontSummary->len;
    fontNameTable = (LPSTR) &lpFontSummary->f[len];

    for (status = 1, ind = 0; ind < len; ind++, lpSummary++)
	{
	lpRefName = (LPSTR) &fontNameTable[lpSummary->indName];

	/* a facename was specified
         */
	if ((namelen && !lstrcmpi(lpFaceName,lpRefName)) ||

	    /* no face name specified
             */
	    (!namelen && (lpSummary->indName != indPrevName)))
	    {
	    indPrevName = lpSummary->indName;
	    lmemset((LPSTR)&fontInfo, 0, sizeof(FONTINFO));
	    ExtractFontInfo((LPFONTINFO)&fontInfo, lpSummary);
	    InfoToStruct((LPFONTINFO)&fontInfo, HP_LOGFONT, (LPSTR) &LogFont);
	    InfoToStruct(
		(LPFONTINFO)&fontInfo, HP_TEXTMETRIC, (LPSTR) &TextMetric);
	    lstrncpy(LogFont.lfFaceName, lpRefName,
		sizeof(LogFont.lfFaceName)-1);
//	    lmemcpy(LogFont.lfFaceName, lpRefName,
//		sizeof(LogFont.lfFaceName)-1);

	    #ifdef LOCAL_DEBUG
	    if (!namelen) {
		short dbg;
		DBGenumfonts(
			("EnumDFonts(): no face specified, returning family "));
		for (dbg=0; (LogFont.lfFaceName[dbg] != '\0'); ++dbg) {
		    DBGenumfonts(("%c", LogFont.lfFaceName[dbg])); }
		DBGenumfonts(("\n"));
		}
	    else {
		short dbg;
		DBGenumfonts(
			("EnumDFonts(): face requested=%ls\n", lpFaceName));
		DBGenumfonts(("              returning font within face "));
		for (dbg=0; (LogFont.lfFaceName[dbg] != '\0'); ++dbg) {
		    DBGenumfonts(("%c", LogFont.lfFaceName[dbg])); }
		DBGenumfonts(("\n"));
		}
	    #endif

	    DBGenumfonts(("EnumDFonts(): Calling callback function...\n"));
	    if ((status = (*lpCallbackFunc)((LPSTR) &LogFont,
		    (LPSTR) &TextMetric, DEVICE_FONTTYPE | RASTER_FONTTYPE,
		    lpClientData)) == 0)
		{
		DBGenumfonts((
		 "EnumDFonts(): Callback function returned 0, breaking FOR\n"));
		break;
		}
	    DBGenumfonts((
	     "EnumDFonts(): Callback function returned %d\n", status));
	    }
	}

    #ifdef LOCAL_DEBUG
    if (indPrevName == -1) {
	DBGenumfonts(("EnumDFonts(): Could *not* find facename, return %d\n",
	    status));
	}
    #endif

    unlockFontSummary(lpDevice);

    DBGenumfonts(("...end of EnumDFonts, return %d\n", status));
    return status;
    }

/***********************************************************************
                       I N F O  T O  S T R U C T
 ***********************************************************************/

 void InfoToStruct(lpFont, style, info)
    LPFONTINFO lpFont;
    short style;
    LPSTR info;
    {
    DBGinfotostruct(("InfoToStruct(%lp,%d,%lp)\n", lpFont, style, info));

    switch (style)
	{
	case HP_LOGFONT:
	    ((LPLOGFONT)info)->lfHeight = lpFont->dfPixHeight;
	    ((LPLOGFONT)info)->lfWidth = lpFont->dfAvgWidth;
	    ((LPLOGFONT)info)->lfEscapement = 0;
	    ((LPLOGFONT)info)->lfOrientation = 0;
	    ((LPLOGFONT)info)->lfUnderline = lpFont->dfUnderline;
	    ((LPLOGFONT)info)->lfStrikeOut = lpFont->dfStrikeOut;
	    ((LPLOGFONT)info)->lfCharSet = lpFont->dfCharSet;
	    ((LPLOGFONT)info)->lfOutPrecision = OUT_CHARACTER_PRECIS;
	    ((LPLOGFONT)info)->lfClipPrecision = CLIP_CHARACTER_PRECIS;
	    ((LPLOGFONT)info)->lfQuality = DEFAULT_QUALITY;
	    ((LPLOGFONT)info)->lfPitchAndFamily =
		(lpFont->dfPitchAndFamily & 0xF0) +
		tmPitchTOlfPitch(lpFont->dfPitchAndFamily & 0x0F);
	    ((LPLOGFONT)info)->lfItalic = lpFont->dfItalic;
	    ((LPLOGFONT)info)->lfWeight = lpFont->dfWeight;
	    break;
	case HP_TEXTXFORM:
	    ((LPTEXTXFORM)info)->ftHeight = lpFont->dfPixHeight;
	    ((LPTEXTXFORM)info)->ftWidth = lpFont->dfAvgWidth;
	    ((LPTEXTXFORM)info)->ftEscapement = 0;
	    ((LPTEXTXFORM)info)->ftOrientation = 0;
	    ((LPTEXTXFORM)info)->ftWeight = lpFont->dfWeight;
	    ((LPTEXTXFORM)info)->ftItalic = lpFont->dfItalic;
	    ((LPTEXTXFORM)info)->ftOutPrecision = OUT_CHARACTER_PRECIS;
	    ((LPTEXTXFORM)info)->ftClipPrecision = CLIP_CHARACTER_PRECIS;
	    ((LPTEXTXFORM)info)->ftAccelerator = TC_OP_CHARACTER;
	    ((LPTEXTXFORM)info)->ftOverhang = OVERHANG;
	    break;
	case HP_TEXTMETRIC:
	    ((LPTEXTMETRIC)info)->tmHeight = lpFont->dfPixHeight;
	    ((LPTEXTMETRIC)info)->tmAscent = lpFont->dfAscent;
	    ((LPTEXTMETRIC)info)->tmDescent = lpFont->dfPixHeight -
		lpFont->dfAscent;
	    ((LPTEXTMETRIC)info)->tmInternalLeading =
		lpFont->dfInternalLeading;
	    ((LPTEXTMETRIC)info)->tmExternalLeading =
		lpFont->dfExternalLeading;
	    ((LPTEXTMETRIC)info)->tmAveCharWidth = lpFont->dfAvgWidth;
	    ((LPTEXTMETRIC)info)->tmMaxCharWidth = lpFont->dfMaxWidth;
	    ((LPTEXTMETRIC)info)->tmItalic = lpFont->dfItalic;
	    ((LPTEXTMETRIC)info)->tmWeight = lpFont->dfWeight;
	    ((LPTEXTMETRIC)info)->tmUnderlined = lpFont->dfUnderline;
	    ((LPTEXTMETRIC)info)->tmStruckOut = lpFont->dfStrikeOut;
	    ((LPTEXTMETRIC)info)->tmFirstChar = lpFont->dfFirstChar;
	    ((LPTEXTMETRIC)info)->tmLastChar = lpFont->dfLastChar;
	    ((LPTEXTMETRIC)info)->tmDefaultChar = lpFont->dfDefaultChar +
		lpFont->dfFirstChar;
	    ((LPTEXTMETRIC)info)->tmBreakChar = lpFont->dfBreakChar +
		lpFont->dfFirstChar;
	    ((LPTEXTMETRIC)info)->tmPitchAndFamily = lpFont->dfPitchAndFamily;
	    ((LPTEXTMETRIC)info)->tmCharSet = lpFont->dfCharSet;
	    ((LPTEXTMETRIC)info)->tmOverhang = OVERHANG;
	    ((LPTEXTMETRIC)info)->tmDigitizedAspectX = VDPI;
	    ((LPTEXTMETRIC)info)->tmDigitizedAspectY = HDPI;
	}
    }

/***********************************************************************
                  E X T R A C T  F O N T  I N F O
 ***********************************************************************/

/*  Copy the metric information stored in the font summary structure
 *  into a FONTINFO structure.
 */

 void ExtractFontInfo(lpFont, lpSummary)
    LPFONTINFO lpFont;
    LPFONTSUMMARY lpSummary;
    {
    DBGextractfont(("ExtractFontInfo(%lp,%lp)\n", lpFont, lpSummary));

    lpFont->dfType		= lpSummary->dfType;
    lpFont->dfPoints		= lpSummary->dfPoints;
    lpFont->dfVertRes		= lpSummary->dfVertRes;
    lpFont->dfHorizRes		= lpSummary->dfHorizRes;
    lpFont->dfAscent		= lpSummary->dfAscent;
    lpFont->dfInternalLeading	= lpSummary->dfInternalLeading;
    lpFont->dfExternalLeading	= lpSummary->dfExternalLeading;
    lpFont->dfItalic		= lpSummary->dfItalic;
    lpFont->dfUnderline		= lpSummary->dfUnderline;
    lpFont->dfStrikeOut		= lpSummary->dfStrikeOut;
    lpFont->dfWeight		= lpSummary->dfWeight;
    lpFont->dfCharSet		= lpSummary->dfCharSet;
    lpFont->dfPixWidth		= lpSummary->dfPixWidth;
    lpFont->dfPixHeight		= lpSummary->dfPixHeight;
    lpFont->dfPitchAndFamily	= lpSummary->dfPitchAndFamily;
    lpFont->dfAvgWidth		= lpSummary->dfAvgWidth;
    lpFont->dfMaxWidth		= lpSummary->dfMaxWidth;
    lpFont->dfFirstChar		= lpSummary->dfFirstChar;
    lpFont->dfLastChar		= lpSummary->dfLastChar;
    lpFont->dfDefaultChar	= lpSummary->dfDefaultChar;
    lpFont->dfBreakChar		= lpSummary->dfBreakChar;
    }

#if defined(OLD_FACE_STUFF) /*******************************************/

/*  initFace
 */
 void initFace(lpHfaces)
    HANDLE FAR *lpHfaces;
    {
    DBGface(("initFace(%lp)\n", lpHfaces));

    *lpHfaces = 0;
    }


/*  aliasFace
 */
 BOOL aliasFace(lpDevice, lpHfaces, lpLogFont, lpFace)
    LPDEVICE lpDevice;
    HANDLE FAR *lpHfaces;
    LPLOGFONT lpLogFont;
    LPSTR lpFace;
    {
    LPSTR lpLogFace = lpLogFont->lfFaceName;
    LPSTR f;
    WORD numfaces;
    WORD family = (lpLogFont->lfPitchAndFamily & 0xF0);
    WORD set, fam;
    int one_in_set = -1;
    int two_in_set = -1;

    DBGface(("aliasFace(%lp,%lp,%lp,%lp): %ls, %ls\n",
	lpDevice, lpHfaces, lpLogFont, lpFace,
	(LPSTR)lpLogFont->lfFaceName, lpFace));

    if (f = lockFace(lpDevice, lpHfaces))
	{
	DBGface(
	    ("aliasFace(): FACES locked, family=%d, first=%ls, second=%ls\n",
	    family, lpLogFace, lpFace));

	numfaces = *f++;

	/*  Advance past default face.
         */
	while (*f)
	    ++f;

	while (numfaces--)
	    {
	    ++f;
	    fam = *f++;
	    set = *f++;

	    DBGface(("aliasFace(): num=%d, set=%d, fam=%d, 1:%d, 2:%d, %ls\n",
		numfaces, set, fam, one_in_set, two_in_set, f));

	    if (fam == family)
		{
		DBGface(("aliasFace(): families match\n"));

		if (lstrcmpi(f, lpLogFace) == 0)
		    {
		    DBGface(("                 ...first face matches\n"));

		    if (two_in_set == (one_in_set = set))
			{
			DBGface(("                 ...both in the set\n"));
			break;
			}
		    }

		if (lstrcmpi(f, lpFace) == 0)
		    {
		    DBGface(("                 ...second face matches\n"));

		    if (one_in_set == (two_in_set = set))
			{
			DBGface(("                 ...both in the set\n"));
			break;
			}
		    }
		}

	    /*  Advance to next face.
             */
	    while (*f)
		++f;
	    }

	}

    return (one_in_set > -1 && two_in_set == one_in_set);
    }

/*  defaultFace
 */
 BOOL defaultFace(lpDevice, lpHfaces, lpFace)
    LPDEVICE lpDevice;
    HANDLE FAR *lpHfaces;
    LPSTR lpFace;
    {
    LPSTR f;
    BOOL match = FALSE;

    DBGface(("defaultFace(%lp,%lp,%lp): %ls\n",
	lpDevice, lpHfaces, lpFace, lpFace));

    if (f = lockFace(lpDevice, lpHfaces))
	{
	++f;

	if (lstrcmpi(f, lpFace) == 0)
	    {
	    DBGface(("defaultFace(): *match*\n"));
	    match = TRUE;
	    }

	}

    return (match);
    }



/*  endFace
 */
 void endFace(lpHfaces)
    HANDLE FAR *lpHfaces;
    {
    DBGface(("endFace(%lp)\n", lpHfaces));

    if (*lpHfaces) {
	GlobalUnlock(*lpHfaces);
	FreeResource(*lpHfaces);
	*lpHfaces = 0;
    }
}

/*  lockFace
 */
 LPSTR lockFace(lpDevice, lpHfaces)
    LPDEVICE lpDevice;
    HANDLE FAR *lpHfaces;
    {
    static LPSTR lpFaces;   /* only valid during a single RealizeObj() call */

    /* Find/load/lock the Face table only if the face Handle is 0--in other
       words, if we need the face table once, we usually need it several times,
       so lock it once, and keep it locked until released by endFace() */

    DBGface(("lockFace(%lp,%lp)\n", lpDevice, lpHfaces));

    if (!*lpHfaces) {	    /* need to load/lock the face table */

	HANDLE hResData;

	DBGface(("lockFace(): getting new faces struct..."));

	if (!(hResData =
		FindResource(hLibInst, (LPSTR)FACES1, (LPSTR)XFACES))) {

	    DBGface(("*failed* to FindResource\n"));
	    return (0L);
	}

	if (!(*lpHfaces = LoadResource(hLibInst, hResData))) {

	    DBGface(("*failed* to LoadResource\n"));
	    return (0L);
	}

	DBGface(("loaded\n"));

	if (!(lpFaces = LockResource(*lpHfaces))) {

	    DBGface(("lockFace(): *failed* to LockResource\n"));
	    FreeResource(*lpHfaces);
	    *lpHfaces = 0;
	    return (0L);
	}

	DBGface(("lockFace(): faces locked\n"));
    }

    return (lpFaces);
}

#else	/* defined(OLD_FACE_STUFF) *************************************/


/***********************************************************************
                          A L I A S  F A C E
 ***********************************************************************/

 BOOL
aliasFace(LPLOGFONT lpLogFont, LPSTR lpFace) {

    extern void FT_NumFaces(void);	/* slimy hack to reference data */
    extern void FT_FaceTable(void);	/*  via far ptr in code segment */

    int one_in_set = -1, two_in_set = -1;
    LPSTR f, lpLogFace = lpLogFont->lfFaceName;
    WORD set, fam, numfaces, family = (lpLogFont->lfPitchAndFamily & 0xF0);


    DBGface(("aliasFace(%lp,%lp): %ls, %ls\n",
	     lpLogFont, lpFace, (LPSTR)lpLogFont->lfFaceName, lpFace));

    numfaces = *(f = (LPSTR)FT_NumFaces);
    f = (LPSTR)FT_FaceTable;

    while (numfaces--) {

	fam = *f++;
	set = *f++;

	DBGface(("aliasFace(): num=%d, set=%d, fam=%d, 1:%d, 2:%d, %ls\n",
	    numfaces, set, fam, one_in_set, two_in_set, f));

	if (fam == family) {

	    DBGface(("aliasFace(): families match\n"));

	    if (lstrcmpi(f, lpLogFace) == 0) {

		DBGface(("                 ...first face matches\n"));

		if (two_in_set == (one_in_set = set)) {

		    DBGface(("                 ...both in the set\n"));
		    break;
		}
	    }

	    if (lstrcmpi(f, lpFace) == 0) {

		DBGface(("                 ...second face matches\n"));

		if (one_in_set == (two_in_set = set)) {

		    DBGface(("                 ...both in the set\n"));
		    break;
		}
	    }
	}

	/*  Advance to next face.
         */
	while (*f++)
	    ;
    }

    return (one_in_set > -1 && two_in_set == one_in_set);
}

/***********************************************************************
                       D E F A U L T  F A C E
 ***********************************************************************/

 BOOL
defaultFace(LPSTR lpFace) {

    BOOL match;
    extern void FT_DefaultFace(void);	    /* hack for data in code seg */

    DBGface(("defaultFace(%lp): %ls", lpFace, lpFace));

    match = (lstrcmpi((LPSTR)FT_DefaultFace, lpFace) == 0);

    DBGface(("  %ls\n",match ? (LPSTR)"*match*" : (LPSTR)"*falied*"));

    return (match);
}

#endif	/* defined(OLD_FACE_STUFF) *************************************/
