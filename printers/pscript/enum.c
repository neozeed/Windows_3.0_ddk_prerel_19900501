/**[f******************************************************************
 * enum.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * ENUM.C
 *
 * 20Aug87 sjp	Creation: from TEXT.C.
 * 11Sep87 sjp	MapFont():  changed "iPrecCand>=iPrecFont" from ">"
 *
 *********************************************************************/

#define INTERNAL_LEADING

#include "pscript.h"
#include <winexp.h>
#include "etm.h"
#include "fonts.h"
#include "debug.h"
#include "enum.h"
#include "utils.h"
#include "resource.h"
#include "psdata.h"
#include "driver.h"

#define UnlockResource(h)   GlobalUnlock(h)

/*------------------------ local functions --------------------------*/

void	PASCAL	ScaleFont(LPDV, LPFONTINFO, int, int);
void	PASCAL	SetLeading(LPFONTINFO, int, int);
void	PASCAL	InfoToStruct(LPFONTINFO, short, LPSTR);
int	PASCAL	MapFont(LPDV, LPLOGFONT, LPSTR, int);
BOOL	PASCAL	AliasFace(LPDV, LPLOGFONT, LPSTR);


/* Conversion codes for the InfoToStruct function */
#define TO_TEXTXFORM	0
#define TO_TEXTMETRIC	1
#define TO_LOGFONT	2

/* The font precidence levels.	These are powers of two */
#define FP_ITALIC	     1
#define FP_WEIGHT	     2
#define FP_FAMILY	     4
#define FP_VARIABLEPITCH     8
#define FP_FIXEDPITCH	    16
#define FP_FACE 	    32



/****************************************************************
* Name: LoadFont()
*
* Action: This routine loads a device font from either from
*	  a resource or from an external PFM file.  If the
*	  font's name starts with a $ sign, then the font
*	  metrics are located in an external PFM file.
*
*	  This routine is called twice for each font.  The
*	  first time a NULL destination ptr is passed in
*	  and this routine just returns the size of the font.
*
* Returns: The size of the device font.
*
**************************************************************/

int FAR PASCAL LoadFont(lpdv, lszFont, lpdf)
LPDV	lpdv;		/* Ptr to the device descriptor */
LPSTR	lszFont;	/* Ptr to the font's name string */
LPDF	lpdf;		/* Ptr to the place to put the device font */
{
	HANDLE	hFont;		/* The font's memory handle */
	HANDLE	hres;		/* The fonts resouce handle */
	LPSTR	lpbSrc;		/* Ptr to a place to put the font */
	LPFX	lpfx;		/* Ptr to the extended font info */
	LPPFM	lppfm;		/* Ptr to the pfm file in memory */
	int	cbLeader;
	int	cb;
	int	fh;
	int	soft = 0;	/* flag to recognize soft load djm 12/20/87 */

	DBMSG1((">LoadFont(): name:%ls lpdf:%lx\n", (LPSTR)lszFont, lpdf));

	lppfm = NULL;
	cbLeader = ((LPSTR) & lppfm->df) - (LPSTR) lppfm;

	if (*lszFont == '$') {

		DBMSG1((" LoadFont(): softfont\n"));

		/* The font info is located in an external PFM file */
		if ((fh = _lopen(lszFont + 1, READ)) < 0) {
			DBMSG1((" LoadFont(): can't open %ls\n",
			    (LPSTR)(lszFont + 1)));
			return 0;
		}
		cb = (int)( _llseek(fh, 0L, 2) - (long)cbLeader );

		if (lpdf) {
			_llseek(fh, (long) cbLeader, 0);
			_lread(fh, (LPSTR) lpdf, cb);
		}
		_lclose(fh);
		soft = 1;

	} else {

		DBMSG1((" LoadFont(): resident font\n"));

		/* The font info is located in a resource */

		hres = FindResource(ghInst, lszFont, MAKEINTRESOURCE(MYFONT));

		if (!hres) {
			DBMSG1((" LoadFont(): bad resource handle\n"));
			return 0;
		}

		cb = SizeofResource(ghInst, hres) - cbLeader;

		if (lpdf) {
			if (!(hFont = LoadResource(ghInst, hres))) {
				DBMSG1((" LoadFont(): can't load resource\n"));
				return 0;
			}

			/* Copy the fontinfo structure into the memory provided by GDI */
			if (lpbSrc = LockResource(hFont)) {
				lmemcpy((LPSTR) lpdf, lpbSrc + cbLeader, cb);
				GlobalUnlock(hFont);
				FreeResource(hFont);
			} else {
				DBMSG1(("LockResource() failed in LoadFont()!\n"));
				return 0;
			}
		}
	}


	if (lpdf) {
		/* Append the font-extra structure to the font */
		lpfx = (LPFX) (((LPSTR) lpdf) + cb);
		lpfx->dfFont = lpdf->dfDriverInfo - cbLeader;
		lpdf->dfDriverInfo = cb;

		/* #ifdef NOV18 */

		/* Save the unscaled average width */
		lpfx->fxUnscaledAvgWidth = lpdf->dfAvgWidth;

		/* Adjust the offsets since the copyright notice has been removed */
		lpdf->dfFace -= cbLeader;
		lpdf->dfDevice -= cbLeader;
		if (lpdf->dfTrackKernTable)
			lpdf->dfTrackKernTable -= cbLeader;
		if (lpdf->dfPairKernTable)
			lpdf->dfPairKernTable -= cbLeader;
		if (lpdf->dfExtentTable)
			lpdf->dfExtentTable -= cbLeader;
		if (lpdf->dfExtMetricsOffset)
			lpdf->dfExtMetricsOffset -= cbLeader;

		/* djm 12/20/87 begin */
		lpfx->noTranslate = FALSE;
		if (soft) {
			if (lpdf->dfCharSet == NO_TRANSLATE_CHARSET) {
				lpdf->dfCharSet = ANSI_CHARSET;
				lpfx->noTranslate = TRUE;
			} else {
				if ((lpdf->dfPitchAndFamily & 0x0f0) == FF_DECORATIVE)
					lpfx->noTranslate = TRUE;
			}
		} else { /* check resident fonts for Symbol & Zapf Dingbats */
			if ((lpdf->dfPitchAndFamily & 0x0f0) == FF_DECORATIVE)
				lpfx->noTranslate = TRUE;
		}
		/* djm 12/20/87 end */
	}

	DBMSG1(("<LoadFont(): size=%d\n", cb + sizeof(FX)));

	return cb + sizeof(FX);
}


/*******************************************************
* Name: RealizeFont()
*
* Action:  Realize a font specified by the logical font
*	   descriptor (LOGFONT) and the font transformation
*	   descriptor (FONTXFORM).  The font is realized
*	   in the form of a FONTINFO record.
*
*	   The realization process is broken down into
*	   the following subtasks:
*	    1. match the logical font to a device font
*
*	    2. Scale the character width table for the requested
*	       font size.
*
* The precidence order of attributes for selecting a font
* are as given below (1 = highest, N = lowest).
*
*	1. An exact face-name match.
*	2. Pitch and Family
*	3. Bold
*	4. Italic
*
*********************************************************/

int FAR PASCAL RealizeFont(lpdv, lplf, lpdf, lpft)
LPDV		lpdv;	/* device descriptor */
LPLOGFONT	lplf;	/* input logical font */
LPFONTINFO	lpdf;	/* memory alloc by GDI for the font */
LPTEXTXFORM	lpft;	/* font transformation structure*/
{
	LPSTR	sfLoadPath;	/* ABF (font file) path */
	char	sfPaths[90];	/* name of resource OR PFM and download file */
	int	cb = 0;		/* The size of font record...0 means illegal */
	int	iFont;
	LPFX	lpfx;
	LPSTR	ptr;

	DBMSG1((">RealizeFont(): lplf: %lx lpdf:%lx\n", lplf, lpdf));

	/* check to see if MapFont encountered an illegal font e.g. vector
	 * font and return 0 size to the calling routine...RealizeObject().
	 */
	DBMSG((" RealizeFont(): LF: face %ls char set %d pitch %d weight %d italic %d\n",
	    (LPSTR)lplf->lfFaceName, lplf->lfCharSet, lplf->lfPitchAndFamily,
	    lplf->lfWeight, lplf->lfItalic));

	if ((iFont = MapFont(lpdv, lplf, sfPaths, sizeof(sfPaths))) < 0) {

		DBMSG((" RealizeFont(): illegal font\n"));
		return 0;
	}

	DBMSG1(("sfPaths:%ls\n", (LPSTR)sfPaths));

	/* sfPaths not either contains a resource name or 
	 * the softfont paths (pfm file and font download file).  
	 * we hash through the string and seperate the two (if necessary).
	 * if there is a download file this is a downloadable soft font */

	ptr = sfPaths;

	/* note: there can be no space after the comma! */

	while (*ptr && *ptr != ',')
		ptr++;

	if (*ptr) {			/* did we hit the ','? */
		*ptr++ = 0;		/* seperate with a null */
		sfLoadPath = ptr;	/* point to the download file */
	} else {
		sfLoadPath = NULL;	/* this means not downloadable font */
	}

	DBMSG1((" RealizeFont(): sfPaths=%ls sfLoadPath=%ls\n",
		(LPSTR)sfPaths,
		(LPSTR)sfLoadPath));

	if (!(cb = LoadFont(lpdv, sfPaths, lpdf))) {
		DBMSG((" RealizeFont(): load failed\n"));
		return 0;
	}

	if (lpdf) {

		lpfx = LockFont(lpdf);

		lpfx->iFont = iFont;
		DBMSG1((" RealizeFont(): FX: i=%d,font=%ls,facd=%ls,dv=%ls\n",
		    lpfx->iFont, (LPSTR)lpfx->lszFont, (LPSTR)lpfx->lszFace,
		    (LPSTR )lpfx->lszDevType));
		DBMSG1((" RealizeFont(): DF: t=%d,i=%d,w=%d,cs=%d,p=%d\n",
		    lpdf->dfType, lpdf->dfItalic, lpdf->dfWeight, lpdf->dfCharSet,
		    lpdf->dfPitchAndFamily));

		/*** rb BitStream ****/

		/* save sfLoadPath for later when we may have to down load
		 * the font data file */

		if (sfLoadPath) {  /* put load path in extended info */
			DBMSG(("downloadable softfont %ls\n", (LPSTR)sfLoadPath));

			lstrcpy(lpfx->sfLoadPath, sfLoadPath);
		} else
			lpfx->sfLoadPath[0] = 0;

		/* Set the font metrics as specified in the logical font */
		ScaleFont(lpdv, lpdf, lplf->lfWidth, lplf->lfHeight);

		lpfx->orientation = lplf->lfOrientation;
		lpfx->escapement = lplf->lfEscapement;
		lpfx->lid = ++lpdv->lid;

		lpdf->dfUnderline = lplf->lfUnderline;
		lpdf->dfStrikeOut = lplf->lfStrikeOut;

		InfoToStruct(lpdf, TO_TEXTXFORM, (LPSTR)lpft);
	}

	DBMSG1(("<RealizeFont(): font size=%d\n", cb));
	return cb;
}


/**************************************************************
* Name: InfoToStruct()
*
* Action: Convert the physical font info (LPDF) structure
*	  to one of the three GDI structures: a) LOGFONT,
*	  b) TEXTXFORM, c) TEXTMETRIC.
*
*
* Note: Isn't it amazing that GDI need three different structures
*	to contain basically the same information!
*
******************************************************************/

void PASCAL InfoToStruct(lpdf, iStyle, lpb)
LPFONTINFO lpdf;	/* Far ptr to the device font metrics */
short	iStyle;		/* The conversion style */
LPSTR	lpb;		/* Far ptr to the output structure */
{
	FONTINFO df;

	df = *lpdf;	/* allows use of ES for lpb */

	switch (iStyle) {
	case TO_TEXTXFORM:

		((LPTEXTXFORM)lpb)->ftHeight = df.dfPixHeight;
		((LPTEXTXFORM)lpb)->ftWidth = df.dfAvgWidth;
		((LPTEXTXFORM)lpb)->ftWeight = df.dfWeight;

		((LPTEXTXFORM)lpb)->ftItalic = df.dfItalic;
		((LPTEXTXFORM)lpb)->ftUnderline = df.dfUnderline;
		((LPTEXTXFORM)lpb)->ftStrikeOut = df.dfStrikeOut;

		((LPTEXTXFORM)lpb)->ftEscapement = 0;
		((LPTEXTXFORM)lpb)->ftOrientation = 0;
		((LPTEXTXFORM)lpb)->ftAccelerator = 0;

		((LPTEXTXFORM)lpb)->ftOverhang = OVERHANG;	/* 0 */
		((LPTEXTXFORM)lpb)->ftClipPrecision = CLIP_CHARACTER_PRECIS; /* 1 */
		((LPTEXTXFORM)lpb)->ftOutPrecision = OUT_CHARACTER_PRECIS; /* 2 */
		break;

	case TO_LOGFONT:
		((LPLOGFONT)lpb)->lfHeight = df.dfPixHeight;
		((LPLOGFONT)lpb)->lfWidth =  df.dfAvgWidth;
		((LPLOGFONT)lpb)->lfEscapement = 0;
		((LPLOGFONT)lpb)->lfOrientation = 0;
		((LPLOGFONT)lpb)->lfItalic = df.dfItalic;
		((LPLOGFONT)lpb)->lfWeight = df.dfWeight;
		((LPLOGFONT)lpb)->lfUnderline = df.dfUnderline;
		((LPLOGFONT)lpb)->lfStrikeOut = df.dfStrikeOut;
		((LPLOGFONT)lpb)->lfOutPrecision = OUT_CHARACTER_PRECIS;
		((LPLOGFONT)lpb)->lfClipPrecision = CLIP_CHARACTER_PRECIS;
		((LPLOGFONT)lpb)->lfQuality = PROOF_QUALITY;
		((LPLOGFONT)lpb)->lfPitchAndFamily = df.dfPitchAndFamily + 1;
		((LPLOGFONT)lpb)->lfFaceName[0] = 0;

		((LPLOGFONT)lpb)->lfCharSet = (df.dfCharSet == NO_TRANSLATE_CHARSET) ?
			(BYTE)ANSI_CHARSET : (BYTE)df.dfCharSet;

		break;

	case TO_TEXTMETRIC:
		((LPTEXTMETRIC)lpb)->tmHeight = df.dfPixHeight;
		((LPTEXTMETRIC)lpb)->tmAscent = df.dfAscent;
		((LPTEXTMETRIC)lpb)->tmDescent = df.dfPixHeight - df.dfAscent;
		((LPTEXTMETRIC)lpb)->tmInternalLeading = df.dfInternalLeading;
		((LPTEXTMETRIC)lpb)->tmExternalLeading = df.dfExternalLeading;
		((LPTEXTMETRIC)lpb)->tmAveCharWidth = df.dfAvgWidth;
		((LPTEXTMETRIC)lpb)->tmMaxCharWidth = df.dfMaxWidth;
		((LPTEXTMETRIC)lpb)->tmItalic = df.dfItalic;
		((LPTEXTMETRIC)lpb)->tmWeight = df.dfWeight;
		((LPTEXTMETRIC)lpb)->tmUnderlined = 0;
		((LPTEXTMETRIC)lpb)->tmStruckOut = 0;
		((LPTEXTMETRIC)lpb)->tmFirstChar = df.dfFirstChar;
		((LPTEXTMETRIC)lpb)->tmLastChar = df.dfLastChar;
		((LPTEXTMETRIC)lpb)->tmDefaultChar = df.dfDefaultChar + df.dfFirstChar;
		((LPTEXTMETRIC)lpb)->tmBreakChar = df.dfBreakChar + df.dfFirstChar;
		((LPTEXTMETRIC)lpb)->tmPitchAndFamily = df.dfPitchAndFamily;
		((LPTEXTMETRIC)lpb)->tmOverhang = OVERHANG;
		((LPTEXTMETRIC)lpb)->tmDigitizedAspectX = HRES;
		((LPTEXTMETRIC)lpb)->tmDigitizedAspectY = VRES;

		((LPTEXTMETRIC)lpb)->tmCharSet = (df.dfCharSet == NO_TRANSLATE_CHARSET) ?
			(BYTE)ANSI_CHARSET : (BYTE)df.dfCharSet;

		DBMSG4(("dfPoints %d\n", df.dfPoints));
		DBMSG4(("  tmHeight          %d\n",((LPTEXTMETRIC)lpb)->tmHeight));
		DBMSG4(("  tmAscent          %d\n",((LPTEXTMETRIC)lpb)->tmAscent));
		DBMSG4(("  tmDescent         %d\n",((LPTEXTMETRIC)lpb)->tmDescent));
		DBMSG4(("  tmInternalLeading %d\n",((LPTEXTMETRIC)lpb)->tmInternalLeading));
		DBMSG4(("  tmExternalLeading %d\n",((LPTEXTMETRIC)lpb)->tmExternalLeading));

		break;
	}
}


/***************************************************************************
 * AliasFace
 *
 * this routine looks through a resource table that associates common
 * font names (Helv, TmsRmn, etc) with postscript font names.
 * user or app specified font names come in the logical font and the
 * device font (specified by lpFace) are matched with the table data
 * pointed to by lpFaces.
 *
 * this routine has been hacked for speed.  profiling revealed that this
 * guy gets called a lot!  the resource referances have been replaced
 * with strings hidden in our code segment. see comments below.
 *
 **************************************************************************/

BOOL PASCAL AliasFace(lpDevice, lpLogFont, lpFace)
LPDV		lpDevice;
LPLOGFONT	lpLogFont;	/* user specified font */
LPSTR		lpFace;		/* the device face name */
{
	WORD	numfaces;
	WORD	set, fam;
	int	one_in_set = -1;
	int	two_in_set = -1;
	LPSTR	lpLogFace = lpLogFont->lfFaceName;
	WORD	family = (lpLogFont->lfPitchAndFamily & 0xF0);
	LPSTR	lpFaces;

	extern void NumFaces(void);	/* slimy hack to reference data */
	extern void FaceTable(void);	/*  via far ptr in code segment */
	extern void DefaultFace(void);


	DBMSG1(("AliasFace(): log face:%ls device face:%ls\n",
	    (LPSTR)lpLogFace, (LPSTR)lpFace));

	/* NOTE: this code is like this to trick the C compiler into
	 * sticking _NumFaces segment into the segment of lpFaces
	 * DO NOT CHANGE THIS WITHOUT EXAMINING THE ASM OUTPUT TO
	 * MAKE SURE IT WORKED RIGHT! */

	numfaces = *(lpFaces = (LPSTR)NumFaces);

	lpFaces = (LPSTR)FaceTable;

	DBMSG1(("AliasFace(): default face:%ls num faces:%d\n",
	    (LPSTR)DefaultFace, numfaces));

	while (numfaces--) {
		fam = *lpFaces++;
		set = *lpFaces++;

		DBMSG1(("AliasFace(): num=%d, set=%d, fam=%d, 1:%d, 2:%d, %ls\n",
			numfaces, set, fam, one_in_set, two_in_set, lpFaces));

		if (fam == family) {
			DBMSG1(("AliasFace(): families match\n"));

			if (lstrcmpi(lpFaces, lpLogFace) == 0) {
				DBMSG1(("...first face matches\n"));

				if (two_in_set == (one_in_set = set)) {
					DBMSG1(("...both in the set\n"));
					break;
				}
			}

			if (lstrcmpi(lpFaces, lpFace) == 0) {
				DBMSG1(("...second face matches\n"));

				if (one_in_set == (two_in_set = set)) {
					DBMSG1(("...both in the set\n"));
					break;
				}
			}
		}

		while (*lpFaces)	/* Advance to next face. */
			++lpFaces;	
	}

	return (one_in_set > -1 && two_in_set == one_in_set);
}



/************************************************************
* Name: MapFont()
*
* Action: Search the font directory for the font that
*	  most closely matches the logical font.
*
* in:
*	lpdv		PDEVICE
*	lplf		logical font that we are trying to match
*	cbFontName	size of lszFont output buffer
*
* out:
*	lszFont		for resource name or SF load paths
*	
* returns:
*	the index of the font selected
*
*
*
*************************************************************/

int PASCAL MapFont(lpdv, lplf, lszFont, cbFontName)
LPDV		lpdv;
LPLOGFONT	lplf;		/* logical font to match */
LPSTR		lszFont;	/* return font name here */
int		cbFontName;
{
	LPFONTINFO lpdf;	/* Far ptr to the physical font record */
	short	cFonts;		/* The number of fonts in the directory */
	int	iPrecFont;	/* The highest precedence seen so far */
	int	iPrecCand;	/* The precedence of the candiate font */
	int	dfFont;		/* The currently selected font index */
	int	iFont;
	int	iFontCand;
	LPSTR	lpbDir;		/* Far ptr to the font directory */
	LPSTR	lpdfFaceName;
	char	szFontCand[90];	/* must be big enough for 2 path names!! */

	DBMSG1((">MapFont():\n"));

	/* Check if this is a vector font */
	/* what the hell is there here for? */
	if (lplf->lfCharSet == OEM_CHARSET) {
		return -1;
	}

	lpbDir = LockFontDir(lpdv->iPrinter);

	/* Search the font table for a font that comes closest to matching
	 * the desired attributes.	This font has highest precidence. */

	cFonts = *((short far * )lpbDir)++;

	/* copy the default font (Courier) name */

	lpdf = (LPFONTINFO) (lpbDir + 4);
	dfFont = *((short far * )(lpbDir + 2));

	lstrcpy(lszFont, ((LPSTR)lpdf) + dfFont);

	/* Courier */
	iFont = 0;

	DBMSG1((" MapFont: log face:%ls\n", (LPSTR)lplf->lfFaceName));

	for (iFontCand = 0; iFontCand < cFonts; ++iFontCand) {
		/* Get a ptr to the next entry in the font directory */

		dfFont = *((short far * )(lpbDir + 2));
		lpdf = (LPFONTINFO)(lpbDir + 4);

		lpdfFaceName = ((LPSTR)lpdf) + lpdf->dfFace;

		lstrcpy(szFontCand, ((LPSTR)lpdf) + dfFont);

		lpbDir += *((short far * )lpbDir);   /* Bump to next entry */

		/* Assume zero precidence on the candidate directory entry */
		iPrecCand = 0;

		/* Check for a typeface match */
		DBMSG1(("Font # %d %ls %ls\n", iFontCand, 
			(LPSTR)lpdfFaceName, (LPSTR)szFontCand));

		if (*(lplf->lfFaceName)) {

			if (!lstrcmpi(lplf->lfFaceName, lpdfFaceName)) {
				iPrecCand += FP_FACE;

			} else {
				/* If the the faces don't match try to find an alias */

				if (AliasFace(lpdv, lplf, lpdfFaceName)) {
					iPrecCand += (FP_FACE - 1);
					DBMSG1((" MapFont: facename (alternate face)\n"));
				}
			}
		}
		DBMSG1((" MapFont: char set:%d dev font:%ls log font:%ls %d\n",
		    lplf->lfCharSet, 
		    (LPSTR)lpdfFaceName, 
		    (LPSTR)lplf->lfFaceName,
		    iPrecCand));

		/* Check for fixed pitch (second highest precidence) */
		if (!(lpdf->dfPitchAndFamily & 1)) {
			if ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH) {
				iPrecCand += FP_FIXEDPITCH;
			}
		} else if ((lplf->lfPitchAndFamily & 0x03) == VARIABLE_PITCH) {
			iPrecCand += FP_VARIABLEPITCH;
		}
		/* Check for a family match */
		if ((lplf->lfPitchAndFamily & 0x0fc) == (lpdf->dfPitchAndFamily & 0x0fc)) {
			iPrecCand += FP_FAMILY;
		}
		DBMSG1((" MapFont: dfp=%d,lfp=%d,tally=%d\n",
		    lpdf->dfPitchAndFamily, lplf->lfPitchAndFamily, iPrecCand));

		/* Check for a boldface weight match */
		if ((lplf->lfWeight >= FW_BOLD) && lpdf->dfWeight >= FW_BOLD) {
			iPrecCand += FP_WEIGHT;
		} else if ((lplf->lfWeight < FW_BOLD) && lpdf->dfWeight < FW_BOLD) {
			iPrecCand += FP_WEIGHT;
		}
		DBMSG1((" MapFont: dfw=%d,lfw=%d,tally=%d\n",
		    lpdf->dfWeight, lplf->lfWeight, iPrecCand));

		/* Check for an italic font match */

		// this previously only looked at the low order bit for
		// truthfullness.  now any non zero is true

		if (lplf->lfItalic && lpdf->dfItalic) {
			iPrecCand += FP_ITALIC;
		} else if (!lplf->lfItalic && !lpdf->dfItalic) {
			iPrecCand += FP_ITALIC;
		}
		DBMSG1((" MapFont: dfi=%d,lfi=%d,tally=%d\n",
		    lpdf->dfItalic, lplf->lfItalic, iPrecCand));

		/* Select the current font if it has equal or higher precidence */

		if (iFontCand == iFont) {

			iPrecFont = iPrecCand;

		} else if (iPrecCand >= iPrecFont) {

			lstrcpy(lszFont, szFontCand);	/* save the name */
			iPrecFont = iPrecCand;
			iFont = iFontCand;
		}
	}

	UnlockFontDir(lpdv->iPrinter);

	DBMSG1(("<MapFont: iFont=%d,tally=%d\n", iFont, iPrecFont));

	return iFont;
}


/****************************************************************************
 * Name: EnumDFonts()
 *
 * Action: This routine is used to enumerate the fonts available
 *	  on the device. For each font, the callback function
 *	  is called with the information for that font.  The
 *	  callback function is called until there are no more
 *	  fonts to enumerate or until the callback function
 *	  returns zero.
 *
 * Note: All fonts are enumerated in a reasonable height (such
 *	as 12 points so that dumb apps that don't realize that
 *	they we can scale text will default to something reasonable.
 *
 ***************************************************************************/

int FAR PASCAL EnumDFonts(lpdv, lszFace,  lpfn, lpb)
LPDV	lpdv;	    /* ptr to the device descriptor */
LPSTR	lszFace;    /* ptr to the facename string (may be NULL) */
FARPROC lpfn;	    /* ptr to the callback function */
LPSTR	lpb;	    /* ptr to the client data (passed to callback) */
{
	/* The static variables prevent stack overflow on recursion */
	LOGFONT lf;
	TEXTMETRIC tm;

	/* add some arbitrary amount to make sure softfonts fit */

	char rgbFont[sizeof(FONTINFO) + 200];

	LPFONTINFO lpdf;
	int	idf;		/* The font index */
	int	cb;
	LPSTR	lpbSrc;
	LPSTR	lpbDst;
	LPSTR	lpbDir;
	short	cFonts;		/* count fonts */
	int	i;
	char	szLastFace[LF_FACESIZE];/* name of last font enumerated */
	int	iStatus = 1;		/* status=1 means failure! */


	DBMSG((">EnumDFont(): facename=%ls\n", lszFace));

	if ( !(lpbDir = LockFontDir(lpdv->iPrinter)) ) {

		/* Couldn't get font directory! */

		DBMSG(("<EnumDFont(): lock font failed\n"));
		return 1;
	}

	/* Scan through the fonts in the font directory */

	cFonts = *(LPSHORT)lpbDir;

	lpbDir += sizeof(short);	/* move past the font count */

	DBMSG((" EnumDFont(): cFonts=%d\n", cFonts));

	/* we will keep track of the last face name enumerated so that if
	 * lszFace == NULL we only enumerate one font of each face name */

	*szLastFace = 0;	/* make last face name invalid */

	for (idf = 0; idf < cFonts; ++idf) {

		cb = *(LPSHORT)lpbDir;	/* get the length of this dir entry */


		DBMSG1(("entry length: cb = %d\n", cb));


		lpdf = (LPDF)(lpbDir + 4);	/* point to the FONTINFO struct */

		DBMSG1(("font name: %ls\n", (LPSTR)lpdf + *(LPSHORT)(lpbDir + 2)));
		DBMSG1(("dfFace:%ls\n", (LPSTR)(lpdf) + lpdf->dfFace));
		DBMSG1(("dfDevice:%ls\n", (LPSTR)(lpdf) + lpdf->dfDevice));

		lpbDir += cb;		/* advance to the next dir entry */

		/* enumerate a font if:
		 * lszFace == NULL and we haven't enumed this face yet
		 * OR
		 * *lszFace == this font's face */

		if ((!lszFace && lstrcmpi(szLastFace, ((LPSTR)lpdf) + lpdf->dfFace)) ||
		    (lszFace && !lstrcmpi(lszFace, ((LPSTR)lpdf) + lpdf->dfFace))) {

			ASSERT((cb-4) < sizeof(rgbFont));

			lmemcpy((LPSTR)rgbFont, (LPSTR) lpdf, cb - 4);
			lpdf = (LPFONTINFO) rgbFont;

			ScaleFont(lpdv, lpdf, 0, 0);
			InfoToStruct(lpdf, TO_LOGFONT, (LPSTR)&lf);

			/* Copy the face name to the logical font limited by
			 * LF_FACESIZE */

			lpbSrc = ((LPSTR)lpdf) + lpdf->dfFace;
			lpbDst = (LPSTR)lf.lfFaceName;

			for (i = LF_FACESIZE - 1; i > 0 && *lpbSrc; --i) 
				*lpbDst++ = *lpbSrc++;
			*lpbDst = 0;

		    	/* save this face name (so we don't do it again) */

			lstrcpy(szLastFace, (LPSTR)lf.lfFaceName);

			InfoToStruct(lpdf, TO_TEXTMETRIC, (LPSTR)&tm);

			if (!(iStatus = (*lpfn)((LPLOGFONT)&lf, (LPTEXTMETRIC)&tm,
			    (int)DEVICE_FONTTYPE, lpb))) {
				break;
			}
		}
	}

	UnlockFontDir(lpdv->iPrinter);
	DBMSG(("<EnumDFont(): iStatus=%d\n", iStatus));

	return iStatus;
}


/**********************************************************
 * Name: ScaleFont()
 *
 * Action: This funtion scales all width and height values
 *	  in the FONTINFO record to match the specified
 *	  font height and width.  Note that the values
 *	  in a default FONTINFO record are based on a
 *	  scale from 0 to EM (defined in globals.h).
 *
 ************************************************************/

void PASCAL ScaleFont(lpdv, lpdf, cxFont, cyFont)
LPDV	lpdv;		/* Far ptr to the device descriptor */
LPFONTINFO lpdf; 	/* Far ptr to the extended device font */
int	cxFont;		/* The horizontal scale factor */
int	cyFont;		/* The vertical scale factor */
{
	int	sx;
	LPFX	lpfx;


	DBMSG(("ScaleFont(): x:%d y:%d\n", cxFont, cyFont));

#ifdef INTERNAL_LEADING

	if (cyFont < 0) {

		// negative height request:
		//     use the typographic height (Em square) of the font

		cyFont = -cyFont;	// just flip the sign

	} else {

		// positive height request:
		//     correct for the internal leading
		// this will give a smaller font

		cyFont = Scale(cyFont, EM, lpdf->dfInternalLeading + EM);
	}

#else
	/* Negative font height means exclude internal leading */
	if (cyFont < 0)
		cyFont = -cyFont;

#endif
	/* Default to a 10 point font */
	if (cyFont == 0)
		cyFont = Scale(10, lpdv->iRes, 72);

	if (cxFont == 0) {
		/*	Default case; scale x the same amount we scale y */
		cxFont = Scale(lpdf->dfAvgWidth, cyFont, EM);
		sx = cyFont;
	} else {
		/* The app requested a specific average character width, so we
		 * determine the correct scale factor to use in order to acheive
		 * the desired average width.
		 */
		if (cxFont < 0) 
			cxFont = -cxFont;
		sx = Scale(cxFont, EM, lpdf->dfAvgWidth);
	}

	/* Fill in the remaining structures using the x and y scale
	 * factors we just computed. */

	// set the point size for computations in SetLeading()

	lpdf->dfPoints = Scale(cyFont, 72, lpdv->iRes);

	/*	Set up internal and external leading */
	SetLeading(lpdf, cyFont, lpdv->iRes);

	lpdf->dfPixHeight = cyFont + lpdf->dfInternalLeading;
	lpdf->dfPixWidth  = cxFont;
	lpdf->dfVertRes   = lpdv->iRes;
	lpdf->dfHorizRes  = lpdv->iRes;
	lpdf->dfAscent    = Scale(cyFont, lpdf->dfAscent, EM);
	lpdf->dfAvgWidth  = Scale(sx, lpdf->dfAvgWidth, EM);
	lpdf->dfMaxWidth  = Scale(sx, lpdf->dfMaxWidth, EM);

	/* Save the scale factors for output */
	if (lpdf->dfDriverInfo) {
		lpfx = (LPFX) (((LPSTR)lpdf) + lpdf->dfDriverInfo);
		lpfx->sx = sx;
		lpfx->sy = cyFont;
	}

	DBMSG(("  dfPoints=%d\n", lpdf->dfPoints));
	DBMSG(("  dfPixHeight=%d\n", lpdf->dfPixHeight));
	DBMSG(("  dfAscent=%d\n", lpdf->dfAscent));
	DBMSG(("  dfExternalLeading=%d\n", lpdf->dfExternalLeading));
	DBMSG(("  dfInternalLeading=%d\n", lpdf->dfInternalLeading));
	DBMSG(("  dfAvgWidth=%d\n", lpdf->dfAvgWidth));
}


/***************************************************************
* Name: SetLeading()
*
* Action: Set the leading (inter-line spacing) value for a
*	  text font.  These values are based on some magic
*	  numbers supplied by Chris Larson who got them from
*	  somewhere in our publications department.  Note that
*	  these values are not a linear function of the font
*	  size.
*
*	  Prior to Chris Larson's mandate, the leading values
*	  were computed as 19.5% of the font height which produced
*	  output which closely matched the Macintosh output on the
*	  LaserWriter.
*
*	  Since Chris only had values for the Times-Roman, Helvetica,
*	  and Courier fonts, the LaserWriter fonts are broken out into
*	  four classes: Roman, Swiss, Modern, and other.  Each family
*	  (hopefully) has the same type of descenders, etc. so that
*	  the external leading is the same for all Roman fonts, etc.
*
********************************************************************
*/
void PASCAL SetLeading(lpdf, cyFont, iRes)
LPFONTINFO lpdf;    /* Far ptr to the device font */
int	cyFont;	    /* The font height in dots */
int	iRes;	    /* The device resolution */
{
	register int	ptLeadSuggest;	// suggest leading in points
	register int	iLeadTotal;	// total leading in device units

	// the max amount of leading is stored in dfInternalLeading

	iLeadTotal = Scale(lpdf->dfInternalLeading, cyFont, EM);

	// compute the suggested leading based on the type of font
	// first look at the family

	switch (lpdf->dfPitchAndFamily & 0x0f0) {

	case FF_ROMAN:		// Tms Rmn  type fonts
		ptLeadSuggest = 2;
		break;

	case FF_SWISS:		// Helv	type fonts

		if (lpdf->dfPoints <= 12) 
			ptLeadSuggest = 2;
		else if (lpdf->dfPoints < 14) 
			ptLeadSuggest = 3;
		else 
			ptLeadSuggest = 4;
		break;

	default:
		// default to 19.6%
		ptLeadSuggest = Scale(lpdf->dfPoints, 196, EM);
		break;
	}

	// for all fixed pitched fonts (Courier) use no leading

	if (lpdf->dfPitchAndFamily & 0x01) {

		// variable pitch

		// scale to device units

		lpdf->dfInternalLeading = Scale(lpdf->dfInternalLeading, cyFont, EM);
	} else {

		// fixed pitch

		lpdf->dfInternalLeading = 0;
		ptLeadSuggest = 0;
	}

#ifdef INTERNAL_LEADING

	// here we make sure the internal and external leading sum
	// to the recomended leading but don't allow external leading
	// to become negative.

	lpdf->dfExternalLeading = max(0, Scale(ptLeadSuggest, iRes, 72) - lpdf->dfInternalLeading);

#else
	lpdf->dfExternalLeading = Scale(ptLeadSuggest, iRes, 72);
#endif

}


