/************************************************************************
 * strblt.c	STRETCHBLT escape support
 *
 * mod history:
 *
 * 	88Jan02 chrisg	removed from bitblt.c
 * 			this code  resembles that in bitblt.
 *			mods made there should be reflected here.
 *
 *	88Jan10 chrisg	added non byte alligned source pixels
 *
 *	90Feb15	chrisg	> 64k bitmap support
 *
 ***********************************************************************/

#include "pscript.h"
#include "atstuff.h"
#include "debug.h"
#include "utils.h"
#include "channel.h"
#include "printers.h"
#include "getdata.h"
#include "resource.h"



int FAR PASCAL Output(LPPDEVICE, int, int, LPPOINT, LPPEN, LPBR, LPDRAWMODE, LPRECT);


#define ENCODE		/* enable RL encoding */
#define SHAPE_SOURCE	/* enable source shapping */

#define DOIT_GDI	-1	/* return code, GDI should do it */

extern char image_proc[];
extern char restore[];
extern char str_def[];

#ifdef ENCODE

typedef BYTE *PSTR;

void FAR PASCAL RunEncode(LPDV lpdv, PSTR lpbytes, int len);
#endif

void FAR PASCAL OpaqueBox(LPDV, int, int, int, int, RGB);	/* text.c */


#ifdef SHAPE_SOURCE

#define WHITE	0x0ff			/* white byte in a bitmap */

/*
 * BOOL PASCAL ShapeSource()
 *
 * adjust a rectangle in a bitmap so that it bounds all non white
 * data. this is to reduce output size for bitblt and strechblt.
 * this is for 1 bit per pixel bitmaps.
 *
 * in:
 *	bitmap and the dimentions of what is going to be blted
 *
 * out:
 *	adjusted values reduced to minimal bounding box that
 *	encloses the bitmap (to byte accuracy on left and right)
 *
 * returns:
 *	TRUE	there is some something in this bitmap (do output)
 *	FALSE	this is a blank bitmap (don't bother doing output)
 *
 * restrictions:
 *	the current implementation does not look at any pixels on
 *	the right margin that hang over less than a byte width.
 *	this means that if a bitmap contains a disconnected line up 
 *	the right side it may get thrown away.
 *
 */

BOOL PASCAL ShapeSource(
	LPBITMAP lpbm, 
	int FAR *lpSrcX, 
	int FAR *lpSrcY, 
	int FAR *lpSrcXE, 
	int FAR *lpSrcYE)
{
	int i, x, y;
	int ulx, uly, lrx, lry;		/* define region we are reducing */
	int dx_bytes;
	int width_bytes;
	BYTE FAR *lpbits;

	ulx = *lpSrcX;
	uly = *lpSrcY;

	lrx = *lpSrcX + *lpSrcXE - 1;
	lry = *lpSrcY + *lpSrcYE - 1;

	width_bytes = lpbm->bmWidthBytes;
	
	/* this should be rounded up but I am going to leave this to avoid
	 * having to do bit manipulation on the right margin (we may screw 
	 * up if there is only data on the right pixels that are being 
	 * skiped! (not likely)) */
	dx_bytes = *lpSrcXE / 8;


	/* problem here.  if the witdh is not byte rounded junk on the
	 * right edge will cause us to not trim properly.
	 *
	 * on far rigth byte we should use a right mask */

	/* try to move top down */

	for (y = uly; y < lry; y++) {

		lpbits = lpbm->bmBits + y * width_bytes + ulx / 8;

		for (i = 0; i < dx_bytes; i++)
			if (*lpbits++ != WHITE)
				goto DONE_WITH_TOP;
	}

DONE_WITH_TOP:

	if (y == lry) {		/* we made it to the bottom */

		DBMSG(("ShapeSource() Empty bitmap!\n"));

		return FALSE;	/* done, this is an empty bitmap */
	}

	uly = y;	/* update top boundary */

	/* from here on none of the outer loops should move one corner
	 * past the other.  best case is to move the corners so that they
	 * are equal (this is because we have found that this bitmap is 
	 * not empty) */

	/* try to move bottom up */

	for (y = lry; y > uly; y--) {

		lpbits = lpbm->bmBits + y * width_bytes + ulx / 8;

		for (i = 0; i < dx_bytes; i++)
			if (*lpbits++ != WHITE)
				goto DONE_WITH_BOTTOM;
	}

DONE_WITH_BOTTOM:

	lry = y;

	/* try to move left side to the right */

	for (x = ulx; x < lrx; x += 8) {
		lpbits = lpbm->bmBits + uly * width_bytes + x / 8;

		for (i = 0; i < (lry - uly); i++)
			if (*lpbits != WHITE)
				goto DONE_WITH_LEFT;
			else
				lpbits += width_bytes;
	}

DONE_WITH_LEFT:

	ulx = x;

	/* try to move the right side to the left */

	for (x = lrx; x > ulx; x -= 8) {
		lpbits = lpbm->bmBits + uly * width_bytes + x / 8;

		for (i = 0; i < (lry - uly); i++)
			if (*lpbits != WHITE)
				goto DONE_WITH_RIGHT;
			else
				lpbits += width_bytes;
	}

DONE_WITH_RIGHT:

	if (x < lrx)
		x += 8;

	lrx = x;


	*lpSrcX = ulx;		/* here is your new rectangle */
	*lpSrcY = uly;
	*lpSrcXE = lrx - ulx + 1;
	*lpSrcYE = lry - uly + 1;

	return TRUE;		/* we have adjusted the inputs */
}


#endif


/****************************************************************************
 * BOOL FAR PASCAL StrechBlt()
 *
 * This routine handles the the GDI StretchBlt routine. 
 * uses "image" operator to stretch a bitmap.
 *
 * if we return DOIT_GDI GDI will do the stretching for us.  this is
 * probally not a good thing.  so, we should probally tell GDI that
 * we do everything (except memory to memory stretching) and convert
 * to one of the rops we can do or faily quietly. if GDI has to do this
 * stuff we generate tons of data and everyone will be unhappy.
 *
 * in:
 *	lpdv	points to the device to recieve output
 *
 * restrictions:
 *	bmBitsPer pixel must be 1.  (use DIBs to get more)
 *
 *	dwRop supported:
 *		SRCCOPY		src -> dst
 *		NOTSRCCOPY	(NOT src) -> dst
 *		SRCPAINT	src OR dst -> dst
 *		MERGEPAINT	(NOT src) OR dst -> dst
 *
 * returns:
 *	TRUE		we did it
 *	FALSE		we couldn't do it
 *
 *	DOIT_GDI(-1)	if GDI should do this for us
 *
 ***************************************************************************/


int FAR PASCAL StretchBlt(
	LPPDEVICE lpdv,
	int DstX,
	int DstY,
	int DstXE,
	int DstYE,
	LPBITMAP lpbm,		/* source bitmap */
	int SrcX,
	int SrcY,
	int SrcXE,
	int SrcYE,
	DWORD rop,
	LPBR lpbr,
	LPDRAWMODE lpdm,	/* what do i do with this? */
	LPRECT lpClip)
{
	LPSTR   lpBits;
	int	byte_width;	/* bitmap width in bytes */
	int	pix_extra;	/* used for non byte alligned source */
	RECT	rect;		/* used for clipping */
	int	dx, dy;		/* pre Shapped values */
	int	x, y;
	HANDLE	hBuf;
	BYTE	*pBuf;
	unsigned scans_this_seg, scans;



	ASSERT(lpdv);

	// is DEST memory?

	if (!lpdv->iType)
		return DOIT_GDI;	/* let GDI do it */


	DBMSG(("StretchBlt() DstX:%d DstY:%d DstXE:%d DstYE:%d\n", 
		DstX, DstY, DstXE, DstYE));

#ifdef PS_IGNORE
       	if (lpdv->fSupressOutput)
       		return 1;
#endif

	// is SRC device?

	if (lpbm && lpbm->bmType) {
		DBMSG(("StretchBlt() source is device\n"));
		return FALSE;		// not even GDI can help us here
	}

	// from here down SRC must be memory or brush

	// special case these easy brushes and ROPs

	if (rop == BLACKNESS || rop == WHITENESS) {
		OpaqueBox(lpdv, DstX, DstY, SrcXE, SrcYE,
			(rop == WHITENESS) ? 0x00FFFFFF : 0L);
		return TRUE;
	}

	if (rop == PATCOPY) {

		rect.left = DstX;
		rect.top  = DstY;
		rect.right  = DstX + SrcXE;
		rect.bottom = DstY + SrcYE;

		Output(lpdv, OS_RECTANGLE, 2, (LPPOINT)&rect, NULL, lpbr, lpdm, NULL);

		return TRUE;
	}

	// from here down only accept memory sources

	if (!lpbm)
		return FALSE;	// no funny PAT stuff

	DBMSG(("StretchBlt(): src planes:%d bits:%d dx:%d dy:%d\n",
		lpbm->bmPlanes,
		lpbm->bmBitsPixel,
		lpbm->bmWidth,
		lpbm->bmHeight));

	// and no bogus memory stuff

	if (lpbm->bmBitsPixel != 1 || lpbm->bmPlanes != 1) {
		DBMSG(("More than one bit image!\n"));
		return FALSE;
	}

	// and hey! no silly stuff that I just can't do!

	if ((rop != SRCCOPY) && (rop != MERGEPAINT) &&
	    (rop != NOTSRCCOPY) && (rop != SRCPAINT)) {
	    	DBMSG(("warning, rop converted\n"));
	    	// rop = SRCCOPY;		/* convert to something we can handle */
	    	rop = SRCPAINT;		/* convert to something we can handle */
	}

	/* rect in defines destination */

	rect.left   = DstX;
	rect.top    = DstY;
	rect.right  = DstX + DstXE;
	rect.bottom = DstY + DstYE;

	/* merge destination and clip rect */

	if (IntersectRect(&rect, &rect, lpClip) == 0) {
		return TRUE;	/* empty clip rect */
	}

	ClipBox(lpdv, &rect);

	// wierd hack: the oqaque rect does not seem to get clipped the
	// same as the image operator.  it seems to draw slightly outside
	// the clip rect (define above).  to account for this bump it down
	// and shrink it by a pixel.

	if (rop == SRCCOPY || rop == NOTSRCCOPY)
		OpaqueBox(lpdv, rect.left + 1, rect.top + 1,
			rect.right - rect.left - 1, rect.bottom - rect.top - 1,
			lpdm->bkColor);

#ifdef SHAPE_SOURCE

	x = SrcX;
	y = SrcY;
	dx = SrcXE;
	dy = SrcYE;

	DBMSG(("Before Shape %d %d %d %d\n", SrcX, SrcY, SrcXE, SrcYE));

	if (!ShapeSource(lpbm, &SrcX, &SrcY, &SrcXE, &SrcYE)) {
		DBMSG(("empty bitmap\n"));
		goto EXIT;
	}

	DBMSG(("After  Shape %d %d %d %d\n", SrcX, SrcY, SrcXE, SrcYE));

#endif
	// calc the byte width accounting for non byte aligned start of
	// the scan and rounding up for the padding in the last byte
	// of the scan

	// round the last point up to byte boundry, the last down

	byte_width = ((((SrcX + SrcXE - 1) | 0x0007) - (SrcX & 0xFFF8)) + 7) / 8;

	DBMSG(("byte_width = %d\n", byte_width));

	/* compute # of non byte alligned pixels */

	pix_extra = SrcX % 8;

	DBMSG(("pix_extra = %d\n", pix_extra));

	/* this byte_width is how big the string (and encode buffer) are */

	PrintChannel(lpdv, str_def, byte_width);

#ifdef ENCODE

	if (!(hBuf = LocalAlloc(LPTR, byte_width)))
		goto ERR_EXIT;

	if (!(pBuf = LocalLock(hBuf))) {
		LocalFree(hBuf);
		goto ERR_EXIT;
	}

	DumpResourceString(lpdv, PS_DATA, PS_UNPACK);
#endif

	PrintChannel(lpdv, "%d %d %d sc\n",
		GetRValue(lpdm->TextColor),
		GetGValue(lpdm->TextColor),
		GetBValue(lpdm->TextColor));

	PrintChannel(lpdv, "save %d %d translate %d %d scale\n", DstX, DstY, DstXE, DstYE);

	// PrintChannel(lpdv, "%d %d ", byte_width * 8 + pix_extra, SrcYE);
	PrintChannel(lpdv, "%d %d ", SrcXE + pix_extra, SrcYE);

	PrintChannel(lpdv, rop == MERGEPAINT || rop == NOTSRCCOPY ?
		(LPSTR) "true" : (LPSTR) "false");

	/* this matrix maps the destination space into the coord system
	 * of the image.  this space is reduced by the scaling factor
	 * (DstXE, DstYE) and then transformed by the image matrix.
	 * we add an extra translation to the image matrix (pix_extra)
	 * to correct for non byte alligned source pixels (in X). */

#ifdef SHAPE_SOURCE

	/* since we reshaped the source we have to change the mapping
	 * so that it accounts for the offset of the smaller source
	 * (we translate by the difference between the old SrcX, SrcY
	 * and the new one (adjusted by ShapeSource)) */

	PrintChannel(lpdv, " [%d 0 0 %d %d %d] ", dx, dy,
		pix_extra + x - SrcX, y - SrcY);
#else
	PrintChannel(lpdv, " [%d 0 0 %d %d 0] ", SrcXE, SrcYE, pix_extra);
#endif


#ifdef ENCODE
	PrintChannel(lpdv, "{unpack} bind imagemask\n");
#else
	PrintChannel(lpdv, image_proc);
	PrintChannel(lpdv, "imagemask\n");
#endif

	/* calc offset into source bitmap */

	scans = SrcY;
	lpBits = lpbm->bmBits;

	// > 64k?

	if (lpbm->bmSegmentIndex) {

		// yes, offset the bits pointer to the proper segment

		while (scans >= lpbm->bmScanSegment) {
			scans -= lpbm->bmScanSegment;
			lpBits = (LPSTR)MAKELONG(0, HIWORD(lpBits) + lpbm->bmSegmentIndex);
		}

	}

	// use segment adjusted values to calc the starting point

	lpBits = lpBits + SrcX / 8 + scans * lpbm->bmWidthBytes;

	// now supports > 64k bitmaps

	scans = SrcYE;	// total number of scan lines to do

	while (scans) {

		if (lpbm->bmSegmentIndex)
			scans_this_seg = min(scans, lpbm->bmScanSegment) ;
		else
			scans_this_seg = scans;

		DBMSG(("scans_this_seg = %d\n", scans_this_seg));

		for (y = 0; y < scans_this_seg; y++) {

	#ifdef ENCODE
			lmemcpy(pBuf, lpBits, byte_width);
			RunEncode(lpdv, pBuf, byte_width);
			PrintChannel(lpdv, "\n");
			lpBits += lpbm->bmWidthBytes;
	#else

	#ifndef ENCODE	/* don't do binary stuff, unpack only accepts ascii */
			if (lpdv->fBinary) {
				WriteChannel(lpdv, lpBits, byte_width);
				lpBits += byte_width;

			} else {
	#endif
				for (x = 0; x < byte_width; x++) {
					PrintChannel(lpdv, "%02x", *lpBits++);
				}
				PrintChannel(lpdv, "\n");
	#ifndef ENCODE
			}
	#endif

			lpBits += lpbm->bmWidthBytes - byte_width;
	#endif
			if (lpdv->fh < 0)
				goto FREE_EXIT;
		}

		if (lpbm->bmSegmentIndex) {
			// or bump HIWORD(lpBits) += lpbm->bmSegmentIndex ?

			DBMSG(("lpBits before add = %lx\n", lpBits));

			// adding in bmSegIndex hits the first scan of the next
			// seg.  use the x position offset to get the start point right

			lpBits = (LPSTR)MAKELONG(SrcX / 8, HIWORD(lpBits) + lpbm->bmSegmentIndex);

			DBMSG(("lpBits after add = %lx\n", lpBits));
		}

		scans -= scans_this_seg;
	}

	PrintChannel(lpdv, restore);	/* match "save" in trans_scale */


#ifdef ENCODE
FREE_EXIT:
	LocalUnlock(hBuf);
	LocalFree(hBuf);
#endif
EXIT:
	ClipBox(lpdv, NULL);


	return TRUE;

ERR_EXIT:
	return 0;
}

