/**[f******************************************************************
 * stubs.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/**********************************************************************
 *
 * 20 mar 90	clarkc 		Check if bitmap in Pixel() TEXTBAND test
 * 02 oct 89	clarkc 		EnumObj moved to EnumObj.c
 * 30 aug 89	clarkc 		Replaced FatalExit(6) with return in Output()
 * 06 may 89	peterbe		Add () pair in #define MAXSTYLEERR
 * 27 apr 89	peterbe		Tabs @ 8 spaces, other cleanup.
 *   1-17-89    jimmat  Added PCL_* entry points to lock/unlock data seg.
 */

/*
 *  NOTE:  Currently in this module only the Output routine has a PCL_*
 *     entry point to lock/unlock the driver's data segment.  The
 *     other exported routines were not given these entry points
 *     because at this time they do not appear to use the data seg.
 *     If that should change (like uncommenting the debug msgs),
 *     they should then lock/unlock the data seg.
 *
 */

#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#include "fontman.h"


/*  debugs
 */
#define DBGtrace(msg) DBMSG(msg)

/* char *faceescape = "\033(s0T";
 */

/*  these output routines call the brute to do all the work; they
 *  1.  fake the lpDevice as a memory bitmap
 *  2.  change x, y coordinates to support banding
 */
#define HYPOTENUSE	14
#define YMAJOR		10
#define XMAJOR		10
#define MAXSTYLEERR	(HYPOTENUSE*2)

ASPECT aspect = { MAXSTYLEERR, HYPOTENUSE, XMAJOR, YMAJOR };

/*  Forward Ref */
static short fake(LPDEVICE far *, short far *, short far *);
int    FAR PASCAL Output(LPDEVICE,short,short,LPPOINT,long,long,long,long);

far PASCAL Bitblt(lpDevice, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
	xExt, yExt, Rop, lpBrush, lpDrawmode)
    LPDEVICE lpDevice;		/* --> to destination bitmap descriptor */
    short DstxOrg;		/* Destination origin - x coordinate    */
    short DstyOrg;		/* Destination origin - y coordinate    */
    BITMAP far *lpSrcDev;	/* --> to source bitmap descriptor      */
    short SrcxOrg;		/* Source origin - x coordinate         */
    short SrcyOrg;		/* Source origin - y coordinate         */
    short xExt;			/* x extent of the BLT                  */
    short yExt;			/* x extent of the BLT                  */
    long Rop;			/* Raster operation descriptor          */
    long lpBrush;		/* --> to a physical brush (pattern)    */
    long lpDrawmode;
    {
    /*  DBGtrace(("In BitBlt,xExt=%d,yExt=%d\n",xExt,yExt));
     */

    /*          only allow memory bitmap as source DC
     *  if (lpSrcDev && lpSrcDev->bmType != DEV_MEMORY)
     *          return 0;
     */
    if (!fake(&lpDevice, &DstxOrg, &DstyOrg))
	return FALSE;

    return dmBitblt(lpDevice, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
	xExt, yExt, Rop, lpBrush, lpDrawmode);
    }

far PASCAL Pixel(lpDevice, x, y, Color, lpDrawMode)
    LPDEVICE lpDevice;
    short x;
    short y;
    long Color;
    long lpDrawMode;
    {
    BITMAP FAR *lpBitmap;
    short status;
    short nWidth = FALSE;

    /*  If the application is using the BANDINFO escape and this is a
     *  text band, then the only graphics that could be coming from
     *  GDI would be those necessary to simulate text (i.e., vector
     *  fonts) or a text attribute (i.e., strike-through).  If this is
     *  the case, then we have to set a special flag (epGDItext) which
     *  indicates to the banding code that we will have to ask the
     *  application to send down text on the graphic bands, just so
     *  we can band through GDI's simulations.  If the application is
     *  not using the BANDINFO escape, then the state of this flag does
     *  not matter because the application will be sending text and
     *  graphics on every band.
     *  
     *  Verify that its not a bitmap first.  20 March 1990  Clark Cyr
     */
    if (lpDevice->epType && (lpDevice->epNband == TEXTBAND))
	lpDevice->epGDItext = TRUE;

/*  HACK OF DOOM, really ugly deception follows:
    If in Landscape mode and on the final band, the bitmap may not be
       byte aligned, i.e. bmWidth != bmWidthBytes * 8.  This currently
       causes problems with display drivers clipping some of the pixels
       set because the display drivers ASSUME that the bitmap is using
       bits 0 thru bmWidth-1, which is not the case here.  So be sneaky
       (read that ugly) and temporarily tell the driver that the width
       IS byte aligned, and no clipping takes place.  If we can pursuade
       the display drivers to check vs. bmWidthBytes instead of bmWidth,
       we'll save a lot of time since they'd only add 3 SHL instructions
       instead of the painful stuff below.  27 Nov 1989   Clark Cyr */

    if ((lpDevice->epType == (short)DEV_LAND) &&
                             (lpDevice->epNband >= lpDevice->epNumBands - 1))
      {
        lpBitmap = &lpDevice->epBmpHdr;
        nWidth = lpBitmap->bmWidth;
        lpBitmap->bmWidth = lpBitmap->bmWidthBytes * 8;
      }

    if (!fake(&lpDevice, &x, &y))
	return FALSE;

    /*  DBGtrace(("Setting Pixel,x=%d,y=%d\n", x, y));
     */

    status = dmPixel(lpDevice, x, y, Color, lpDrawMode);
    if (nWidth)
      {
        lpBitmap->bmWidth = nWidth;
      }
    return status;
//  return dmPixel(lpDevice, x, y, Color, lpDrawMode);
    }

/*  PCL_Output
 *
 *  Output entry point to lock/unlock the data segment.
 */

int FAR PASCAL
PCL_Output(lpDevice,style,count,lpPoints,lpPPen,lpPBrush,lpDrawMode,lpClipRect)
LPDEVICE lpDevice;
short style, count;
LPPOINT lpPoints;
long lpPPen, lpPBrush, lpDrawMode, lpClipRect;
{
    int rc;

    LockSegment(-1);

    rc = Output(lpDevice, style, count, lpPoints, lpPPen, lpPBrush,
	      lpDrawMode, lpClipRect);

    UnlockSegment(-1);

    return rc;
}

far PASCAL Output(lpDevice, style, count, lpPoints, lpPPen, lpPBrush,
	lpDrawMode, lpClipRect)
    LPDEVICE lpDevice;		/* --> to the destination */
    short style;		/* Output operation                   */
    short count;		/* # of points                        */
    LPPOINT lpPoints;		/* --> to a set of points             */
    long lpPPen;		/* --> to physical pen                */
    long lpPBrush;		/* --> to physical brush              */
    long lpDrawMode;		/* --> to a Drawing mode              */
    long lpClipRect;		/* --> to a clipping rectange if <> 0 */
    {
    short status;
    HANDLE hPoints = 0;

    if (lpDevice->epType)
	{
	register short i;
	register short far *p;

	/* set flag to indicate graphics on PAGE */
	lpDevice->epMode |= ANYGRX;

	if (lpDevice->epNband == TEXTBAND)
	    return TRUE;

	if (lpDevice->epMode & DRAFTFLAG)
	    return FALSE;

	/* make our own copy of the points
         */
	if (!(hPoints = (HANDLE)GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)(count * sizeof(POINT)))))
	    return FALSE;

	if (!(p = (short far *)GlobalLock(hPoints)))
	    {
	    GlobalFree(hPoints);
	    return FALSE;
	    }

	lmemcpy((LPSTR)p, (LPSTR)lpPoints, count * sizeof(POINT));
	lpPoints = (LPPOINT)p;

	if (style == OS_SCANLINES)
	    {
	    p++;
	    *p++ -= lpDevice->epYOffset >> lpDevice->epScaleFac;
	    i = (count << 1) - 2;

	    for ( ; i; --i)
		*p++ -= lpDevice->epXOffset >> lpDevice->epScaleFac;
	    }
	else
	    {
#if NEVER
/* It's not a good move to call FatalExit() because you didn't get what you
 * expect.  Added features scream.  It's worse to fall through after you've *
 * expected death.  If it's unexpected, return FALSE.    30 Aug 89  clarkc
 */
	    FatalExit(0x06);
#endif
	    return FALSE;
	    }

	/* set flag to indicate graphics in BAND */
	lpDevice->epMode |= GRXFLAG;

	/* point to location of actual storage for bitmap */
	lpDevice->epBmpHdr.bmBits = lpDevice->epBmp;
	lpDevice = (LPDEVICE) &lpDevice->epBmpHdr;
	}

    /*  DBGtrace(("    calling dmOutput(), count=%d\n", count));
     */

    status = dmOutput(lpDevice, style, count, lpPoints, lpPPen, lpPBrush,
	lpDrawMode, *((long *) &aspect));

    if (hPoints)
	{
	GlobalUnlock(hPoints);
	GlobalFree(hPoints);
	}

    return status;
    }

int far PASCAL DeviceBitmap(lpDevice, command, lpBitmap, lpBits)
    LPDEVICE lpDevice;
    int command;
    BITMAP far *lpBitmap;
    BYTE far *lpBits;
    {
    return (0);
    }


int far PASCAL FastBorder(lpRect, borderWidth, borderDepth,
	rasterOp, lpDevice, lpPBrush, lpDrawMode, lpClipRect)
    LPRECT lpRect;
    WORD borderWidth;
    WORD borderDepth;
    DWORD rasterOp;
    LPDEVICE lpDevice;
    LPSTR lpPBrush;
    LPDRAWMODE lpDrawMode;
    LPRECT lpClipRect;
    {
    return (0);
    }


int far PASCAL SetAttribute(lpDevice, stateNum, index, attribute)
    LPDEVICE lpDevice;
    int stateNum;
    int index;
    int attribute;
    {
    return (0);
    }


far PASCAL ScanLR(lpDevice, x, y, Color, DirStyle)
    LPDEVICE lpDevice;
    short x;
    short y;
    long Color;
    short DirStyle;
    {
    BITMAP FAR *lpBitmap = &lpDevice->epBmpHdr;
    short status;
    short nWidth = FALSE;

    /* DBGtrace(("In ScanLR\n"));
    */

    if (!fake(&lpDevice, &x, &y))
	return FALSE;

/*  HACK OF DOOM, see commentary in Pixel() routine above.
                                               27 Nov 1989   Clark Cyr */

    if ((lpDevice->epNband == lpDevice->epNumBands - 1) &&
                             (lpDevice->epType == (short)DEV_LAND))
      {
        lpBitmap = &lpDevice->epBmpHdr;
        nWidth = lpBitmap->bmWidth;
        lpBitmap->bmWidth = lpBitmap->bmWidthBytes * 8;
      }
    status = dmScanLR(lpDevice, x, y, Color, DirStyle);
    if (nWidth)
      {
        lpBitmap->bmWidth = nWidth;
      }
    return status;
//  return dmScanLR(lpDevice, x, y, Color, DirStyle);
    }

#if 0
/* Moved to EnumObj.c.     2 Oct 1989  Clark Cyr     */

far PASCAL EnumObj(lpDevice, style, lpCallbackFunc, lpClientData)
    LPDEVICE lpDevice;
    short style;
    long lpCallbackFunc;
    long lpClientData;
    {

    /* DBGtrace(("In EnumObj\n"));
    */

    return dmEnumObj((lpDevice->epType ? (LPDEVICE) & lpDevice->epBmpHdr :
	lpDevice), style, lpCallbackFunc, lpClientData);
    }
#endif


far PASCAL ColorInfo(lpDevice, ColorIn, lpPhysBits)
    LPDEVICE lpDevice;
    long ColorIn;
    long lpPhysBits;
    {
    return dmColorInfo((lpDevice->epType ? (LPDEVICE) & lpDevice->epBmpHdr :
	lpDevice), ColorIn, lpPhysBits);
    }

static short fake(lplpDevice, x, y)
    LPDEVICE far *lplpDevice;
    short far *x, far *y;
    {
    register LPDEVICE lpDevice;
    lpDevice = *lplpDevice;

    if (lpDevice->epType)
	{
	lpDevice->epMode |= ANYGRX;

	if (lpDevice->epMode & DRAFTFLAG)
	    return FALSE;

	if (lpDevice->epNband == TEXTBAND)
	    return TRUE;

	lpDevice->epBmpHdr.bmBits = lpDevice->epBmp;
	    /*point to lpDevice
                location for storing the bitmap*/

	if (y)
	    *y -= lpDevice->epYOffset >> lpDevice->epScaleFac;

	if (x)
	    *x -= lpDevice->epXOffset >> lpDevice->epScaleFac;

	lpDevice->epMode |= GRXFLAG;
	*lplpDevice = (LPDEVICE) & lpDevice->epBmpHdr;
	    /*fake bitmap within
                    LPDEVICE as the actual bitmap*/
	}

    return TRUE;
    }
