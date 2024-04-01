/**[f******************************************************************
 * paper.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.  
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/*******************************   paper.c   *******************************/
/*
 *  Service routines for reading paper formats from resources.
 *
 *  22 mar 90	clarkc  (-sizeof(struct) + ulVar) --> (ulVar - sizeof(struct))
 *			This makes it work under both c5 and c6, return value
 *			from sizeof() changed from signed int to unsigned int.
 *			
 *  15 sep 89	peterbe	Simplified and corrected ComputeLineBufSize().
 *			
 *  05 sep 89	peterbe	Added ComputeLineBufSize() with 5/4 worst-case
 *			expansion.
 *			Speed up NumBytes().
 *   2-07-89    jimmat  Driver initialization changes.
 */

#include "generic.h"
#include "resource.h"
#include "paper.h"

#define LOCAL static

LOCAL WORD NumBytes (short);
LOCAL void SetBandCount(LPDEVICE, short);

/**************************************************************************/
/****************************   Global Procs   ****************************/

/*  GetPaperFormat
 *
 *  Read the paper format from the resources.
 *
 *  This function called by Enable() to set up the page and image area
 *  dimensions.
 */

BOOL FAR PASCAL
GetPaperFormat(PAPERFORMAT FAR *lpPF, HANDLE hModule, short paperInd,
	       short paperSize, short Orientation) {

    PAPERHEAD paperHead;
    PAPERLIST paperList;
    PAPERLISTITEM FAR *p;
    HANDLE hResInfo;
    LONG startpos;
    WORD paperid;
    BOOL success = FALSE;
    int hFile;
    int i;

    if ((hResInfo=FindResource(hModule,(LPSTR)PAPER1,(LPSTR)PAPERFMT)) &&
	(hFile=AccessResource(hModule,hResInfo)) >= 0) {

	if ((_lread(hFile,(LPSTR)&paperHead,sizeof(PAPERHEAD)) ==
	    sizeof(PAPERHEAD)) &&
            (_llseek(hFile, paperHead.offsLists - sizeof(PAPERHEAD) +
	    (paperInd*sizeof(PAPERLIST)),1) > 0L) &&
	    (_lread(hFile,(LPSTR)&paperList,sizeof(PAPERLIST)) ==
	    sizeof(PAPERLIST))) {

	    startpos = -paperHead.offsLists-sizeof(PAPERLIST)-
		       (paperInd*sizeof(PAPERLIST));

	    switch (paperSize) {

		case DMPAPER_LETTER:	paperid = PAPERID_LETTER;   break;
		case DMPAPER_LEGAL:	paperid = PAPERID_LEGAL;    break;
		case DMPAPER_EXECUTIVE: paperid = PAPERID_EXEC;     break;
		case DMPAPER_LEDGER:	paperid = PAPERID_LEDGER;   break;
		case DMPAPER_A3:	paperid = PAPERID_A3;	    break;
		case DMPAPER_A4:	paperid = PAPERID_A4;	    break;
		case DMPAPER_B5:	paperid = PAPERID_B5;	    break;
		default:		paperid = 0;		    break;
	    }

            for (i=0, p=paperList.p; i < paperList.len &&
                !(p->id & paperid); ++i, ++p)
                ;

	    if (i < paperList.len) {

		if (Orientation == DMORIENT_LANDSCAPE)
                    i = p->indLandPaperFormat;
                else
                    i = p->indPortPaperFormat;

		if ((_llseek(hFile,startpos+paperHead.offsFormats+
		    (i*sizeof(PAPERFORMAT)),1) > 0L) &&
		    (_lread(hFile,(LPSTR)lpPF,sizeof(PAPERFORMAT)) ==
		    sizeof(PAPERFORMAT))) {

                    success = TRUE;
		}
	    }
	}

        _lclose(hFile);
    }

#ifdef DEBUG
    if (success)
        {
        DBMSG(("GetPaperFormat(): paper %d, paperInd %d, ind %d\n",
	       paperSize, paperInd, i));
        DBMSG(("   xPhys=%d\n", lpPF->xPhys));
        DBMSG(("   yPhys=%d\n", lpPF->yPhys));
        DBMSG(("   xImage=%d\n", lpPF->xImage));
        DBMSG(("   yImage=%d\n", lpPF->yImage));
        DBMSG(("   xPrintingOffset=%d\n", lpPF->xPrintingOffset));
        DBMSG(("   yPrintingOffset=%d\n", lpPF->yPrintingOffset));
        DBMSG(("   select=%ls\n", (LPSTR)lpPF->select));
        }
    else
        {
        DBMSG(("GetPaperFormat(): paper %d, paperInd %d, FAILED\n",
	       paperSize, paperInd));
        }
#endif

    return (success);
}

/*  GetPaperBits
 *
 *  Get the paper lists from the resources and merge the supported
 *  papers for each list into one WORD per list.
 *
 *  This function called by DeviceMode() to get the list of possible
 *  paper combinations (each printer has an index into this list).
 */
BOOL FAR PASCAL GetPaperBits (hModule, lpPaperBits)
    HANDLE hModule;
    WORD FAR *lpPaperBits;
    {
    PAPERHEAD paperHead;
    PAPERLIST paperList;
    PAPERLISTITEM FAR *p;
    HANDLE hResInfo;
    short i, j;
    int hFile;

    if ((hResInfo=FindResource(hModule,(LPSTR)PAPER1,(LPSTR)PAPERFMT)) &&
        (hFile=AccessResource(hModule,hResInfo)) >= 0)
        {
        if ((_lread(hFile,(LPSTR)&paperHead,sizeof(PAPERHEAD)) ==
		sizeof(PAPERHEAD)) &&
            (paperHead.offsLists == (DWORD)sizeof(PAPERHEAD)))
            {
            if (paperHead.numLists > MAX_PAPERLIST)
                paperHead.numLists = MAX_PAPERLIST;

            for (i=0; i < paperHead.numLists; ++i, ++lpPaperBits)
                {
                *lpPaperBits = 0;

                if (_lread(hFile,(LPSTR)&paperList,sizeof(PAPERLIST)) ==
			sizeof(PAPERLIST))
                    {
                    for (j=0, p=paperList.p; j < paperList.len; ++j, ++p)
                        *lpPaperBits |= p->id;
                    }

                #ifdef DEBUG
                {
                WORD bits = *lpPaperBits;
                DBMSG(("Paper bits at ind %d\n", i));
                if (bits & PAPERID_LETTER) 
		    { DBMSG(("    PAPERID_LETTER\n"));  }
                if (bits & PAPERID_LEGAL)
		    { DBMSG(("    PAPERID_LEGAL\n"));   }
                if (bits & PAPERID_EXEC)
		    { DBMSG(("    PAPERID_EXEC\n"));        }
                if (bits & PAPERID_LEDGER)
		    { DBMSG(("    PAPERID_LEDGER\n"));  }
                if (bits & PAPERID_A3)
		    { DBMSG(("    PAPERID_A3\n"));      }
                if (bits & PAPERID_A4)
		    { DBMSG(("    PAPERID_A4\n"));      }
                if (bits & PAPERID_B5)
		    { DBMSG(("    PAPERID_B5\n"));      }
                }
                #endif
                }
            }

        _lclose(hFile);
        }
    else
        return FALSE;

    return TRUE;
    }

/*  Paper2Bit
 */
WORD FAR PASCAL
Paper2Bit(short paper) {

    switch(paper) {

	case DMPAPER_LETTER:	return PAPERID_LETTER;
	case DMPAPER_LEGAL:	return PAPERID_LEGAL;
	case DMPAPER_EXECUTIVE: return PAPERID_EXEC;
	case DMPAPER_LEDGER:	return PAPERID_LEDGER;
	case DMPAPER_A3:	return PAPERID_A3;
	case DMPAPER_A4:	return PAPERID_A4;
	case DMPAPER_B5:	return PAPERID_B5;
    }

    return 0;
}

/*  ComputeBandBitmapSize
 *
 *  Compute the smallest size possible for the bitmap for banding.
 *
 *  This function called by Enable() to determine the size of the
 *  device header (which includes the banding bitmap).
 */

WORD FAR PASCAL
ComputeBandBitmapSize (PAPERFORMAT FAR *lpPF, LPPCLDEVMODE lpDevmode) {

    WORD size;
    short prtResFac = lpDevmode->prtResFac;

    if (lpDevmode->dm.dmOrientation == DMORIENT_LANDSCAPE)
        size = NumBytes(MAX_BANDDEPTH) * (lpPF->yImage >> prtResFac);
    else
        size = NumBytes(lpPF->xImage >> prtResFac) * MAX_BANDDEPTH;

    DBMSG(("ComputeBandBitmapSize(%lp,%lp): size=%d\n", lpPF, lpDevmode, size));
    return (size);

}	// ComputeBandBitmapSize()


// ComputeLineBufSize()
// Compute size of special scanline buffer (at offset lpDevice->epLineBuf)
// used for some printers.  We assume this printer needs such a buffer.

WORD FAR PASCAL
ComputeLineBufSize (PAPERFORMAT FAR *lpPF, LPPCLDEVMODE lpDevmode)
{
    WORD size;

    // figure basic size of scanline: scale and convert to bytes.
    size = NumBytes ( ((lpDevmode->dm.dmOrientation == DMORIENT_LANDSCAPE) ?
		(lpPF->yImage) : (lpPF->xImage)) >> (lpDevmode->prtResFac) );

    // correct for worst-case randomness (combo of small repeats and
    // short random patterns), round to even bytes.
    // always round UP!

    size = (5 * size + 3) / 4;		// times 5/4
    size = 2 * ((size + 1) / 2);	// round up to even value.

    return size;

}	// ComputeLineBufSize()

/*  ComputeBandingParameters
 *
 *  Set up the bitmap sizes and other banding information used by control
 *  to print bands.
 *
 *  This function is called by Enable() in reset.c to fill in the
 *  BITMAP structure.
 */
void FAR PASCAL ComputeBandingParameters (lpDevice, prtResFac)
    LPDEVICE lpDevice;
    short prtResFac;
    {
    PAPERFORMAT FAR *lpPF = &lpDevice->epPF;
    BITMAP FAR *lpBitmap = &lpDevice->epBmpHdr;

    if (lpDevice->epType == (short)DEV_LAND)
        {
        lpBitmap->bmWidth = MAX_BANDDEPTH;
        lpBitmap->bmHeight = lpPF->yImage >> prtResFac;
        SetBandCount(lpDevice, lpPF->xImage >> prtResFac);
        }
    else
        {
        lpBitmap->bmWidth = lpPF->xImage >> prtResFac;
        lpBitmap->bmHeight = MAX_BANDDEPTH;
        SetBandCount(lpDevice, lpPF->yImage >> prtResFac);
        }

    lpBitmap->bmWidthBytes = NumBytes(lpBitmap->bmWidth);
    lpBitmap->bmWidthPlanes = lpBitmap->bmWidthBytes * lpBitmap->bmHeight;
    lpBitmap->bmPlanes = 1;
    lpBitmap->bmBitsPixel = 1;

    DBMSG(("ComputeBandingParameters(%lp,%d):\n", lpDevice, prtResFac));
    DBMSG(("    lpBitmap->bmWidth = %d\n", lpBitmap->bmWidth));
    DBMSG(("    lpBitmap->bmHeight = %d\n", lpBitmap->bmHeight));
    DBMSG(("    lpBitmap->bmWidthBytes = %d\n", lpBitmap->bmWidthBytes));
    DBMSG(("    lpBitmap->bmWidthPlanes = %ld\n", lpBitmap->bmWidthPlanes));
    DBMSG(("    lpBitmap->bmPlanes = %d\n", (WORD)lpBitmap->bmPlanes));
    DBMSG(("    lpBitmap->bmBitsPixel = %d\n", (WORD)lpBitmap->bmBitsPixel));
    DBMSG(("    lpDevice->epNumBands = %d\n", lpDevice->epNumBands));
    DBMSG(("    lpDevice->epLastBandSz = %d\n", lpDevice->epLastBandSz));
    }

/*  ComputeBandStartPosition
 *
 *  Compute cursor positions to the band.  Normally, it is not necessary
 *  to do an explicit move to the start of a band because the current
 *  cursor position is already there (output of the previous band put it
 *  there).  This function is called when the previous graphics bands has
 *  been skipped.
 *
 *  In portrait, the starting cursor position is the top left corner of
 *  the band rectangle.  In landscape, it is the top right corner.
 *
 *  This function is called by the banding code for the NEXTBAND escape
 *  inside of control.c.
 */
void FAR PASCAL ComputeBandStartPosition(lpPos, lpDevice, bandnum)
    LPPOINT lpPos;
    LPDEVICE lpDevice;
    short bandnum;
    {
    lpPos->xcoord = 0;
    lpPos->ycoord = 0;

    /*  Text band: punt.
     */
    if (bandnum < 1)
        return;

    /*  Off of page, return coord of last band.
     */
    if (bandnum > lpDevice->epNumBands)
        bandnum = lpDevice->epNumBands;

    if (lpDevice->epType == (short)DEV_LAND)
        {
        lpPos->xcoord = (lpDevice->epPF.xImage >> lpDevice->epScaleFac) -
            (MAX_BANDDEPTH * (bandnum - 1));
        }
    else
        lpPos->ycoord = (bandnum - 1) * MAX_BANDDEPTH;

    /*  Shift out because we always report these numbers at
     *  300dpi, GDI scales them down if the resolution is
     *  less than 300dpi.
     */
    lpPos->xcoord <<= lpDevice->epScaleFac;
    lpPos->ycoord <<= lpDevice->epScaleFac;

    DBMSG(("ComputeBandStartPosition(%lp,%lp,%d): x=%d, y=%d\n",
        lpPos, lpDevice, bandnum, (WORD)lpPos->xcoord, (WORD)lpPos->ycoord));
    }

/*  ComputeNextBandRect
 *
 *  Compute the offsets and dimensions of the next banding rectangle.
 *  If we've banded the whole page, return FALSE.
 *
 *  We always report these numbers at 300dpi, Windows GDI scales them
 *  down if we're printing at 75 and 150dpi.  This accounts for the
 *  occasional loss of hairlines (1/300 inch lines) when printing at
 *  75dpi (the app prints them one pixel wide and GDI clips them out
 *  because four 300dpi scanlines are merged into one 75dpi scanline).
 *
 *  This function is called by the banding code for the NEXTBAND escape
 *  inside of control.c.
 */
BOOL FAR PASCAL ComputeNextBandRect(lpDevice, currentband, lpBandRect)
    LPDEVICE lpDevice;
    short currentband;
    LPRECT lpBandRect;
    {
    short bandsize = MAX_BANDDEPTH;

    if (currentband > lpDevice->epNumBands)
        {
        /*  No more bands.
         */
        SetRectEmpty(lpBandRect);
        return FALSE;
        }

    if (currentband < 1)
        {
        /*  Text band: punt.
         */
        lpDevice->epXOffset = 0;
        lpDevice->epYOffset = 0;
        SetRect(lpBandRect, 0, 0, lpDevice->epPF.xImage, lpDevice->epPF.yImage);
        return TRUE;
        }

    if (lpDevice->epType == (short)DEV_LAND)
        {
        /*     ________________
         *    |    :  :        |   Landscape page:
         *    |    :  :        |
         *  X |    :  :        |   Band across  (right to left)
         *    |____:__:________|
         *
         * (0,0)       Y
         */
        if (currentband == 1 || currentband == lpDevice->epNumBands)
            {
            BITMAP FAR *lpBitmap = &lpDevice->epBmpHdr;

            if (currentband == lpDevice->epNumBands)
                bandsize = lpDevice->epLastBandSz;

            lpBitmap->bmWidth = bandsize;
            lpBitmap->bmWidthBytes = NumBytes(lpBitmap->bmWidth);
            lpBitmap->bmWidthPlanes =
		lpBitmap->bmWidthBytes * lpBitmap->bmHeight;
            }

        lpDevice->epXOffset = (currentband == lpDevice->epNumBands) ? 0 :
            (lpDevice->epPF.xImage >> lpDevice->epScaleFac) -
            (MAX_BANDDEPTH * currentband);
        lpDevice->epXOffset <<= lpDevice->epScaleFac;

        SetRect(lpBandRect, lpDevice->epXOffset, 0,
            lpDevice->epXOffset + (bandsize << lpDevice->epScaleFac),
            lpDevice->epPF.yImage);

        if (bandsize < MAX_BANDDEPTH)
            {
            /*  If the last band's width does not equal a WORD boundary,
             *  modify epXOffset so the routines in stub.c will draw the
             *  image flush right within the band bitmap.
             */
            lpDevice->epXOffset -=
                ((bandsize + 15) / 16 * 16 - bandsize) << lpDevice->epScaleFac;

            DBMSG(
("ComputeNextBandRect(): epXOffset modified to %d\n", lpDevice->epXOffset));
            }
        }
    else
        {
        /*     _________
         *    |         |
         *    |         |   Portrait page:
         *    |- - - - -|
         *  X |- - - - -|   Band down
         *    |         |
         *    |         |
         *    |_________|
         *
         * (0,0)   Y
         */
        if (currentband == 1 || currentband == lpDevice->epNumBands)
            {
            BITMAP FAR *lpBitmap = &lpDevice->epBmpHdr;

            if (currentband == 1)
                bandsize = MAX_BANDDEPTH;
            else
                bandsize = lpDevice->epLastBandSz;

            lpBitmap->bmHeight = bandsize;
            lpBitmap->bmWidthPlanes =
		lpBitmap->bmWidthBytes * lpBitmap->bmHeight;
            }

        lpDevice->epYOffset = (currentband - 1) * MAX_BANDDEPTH;
        lpDevice->epYOffset <<= lpDevice->epScaleFac;

        SetRect(lpBandRect, 0, lpDevice->epYOffset, lpDevice->epPF.xImage,
            lpDevice->epYOffset + (bandsize << lpDevice->epScaleFac));
        }

    return TRUE;
    }

/**************************************************************************/
/*****************************   Local Procs   ****************************/


/*  NumBytes
 *
 *  Compute the number of bytes, but align it on word boundaries.
 *  (convert pixel count to even byte count)
 */
LOCAL WORD NumBytes (val)
    short val;
    {
    WORD num;

    num = (val + 15) / 16;

    //if ((num * 16) < val)
    //    ++num;

    return (num * 2);
    }


/*  SetBandCount
 *
 *  Set up the number of print bands on the page and compute the size
 *  of the last band (it does not necessarily have to be 64 scan lines).
 */
LOCAL void SetBandCount(lpDevice, depth)
    LPDEVICE lpDevice;
    short depth;
    {
    short tmp;

    lpDevice->epNumBands = depth / MAX_BANDDEPTH;

    if ((tmp=(lpDevice->epNumBands * MAX_BANDDEPTH)) < depth)
        {
        ++lpDevice->epNumBands;
        lpDevice->epLastBandSz = depth - tmp;
        }
    else
        lpDevice->epLastBandSz = MAX_BANDDEPTH;

    DBMSG(("SetBandCount(%lp,%d): numBands=%d, lastBandSz=%d\n",
        lpDevice, depth, lpDevice->epNumBands, lpDevice->epLastBandSz));
    }
