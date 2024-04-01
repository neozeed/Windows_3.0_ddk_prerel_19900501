/**[f******************************************************************
 * reset.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/*******************************   reset.c   *******************************/
//
//  Reset: Enable and Disable driver, called on CreateIC/DC.
//
//  rev:
//
// 02 feb 90    clarkc  lmemcpy changed to lstrncpy when using LPSTR source
// 04 jan 90    clarkc  introduced USEEXTDEVMODE, using ExtDevMode to initialize
// 30 nov 89	peterbe	Visual edge calls in ifdef now.
// 19 sep 89	peterbe	Changed LITTLESPUD to HPLJIIP
// 15 sep 89	peterbe	It's a HP LJ II P if (lpStuff->prtCaps & HP LJ IIP)
// 06 sep 89	peterbe	Add code to allocate space for scanline buffer
//			and init. epLineBuf.  (LITTLESPUD)
// 07 aug 89	peterbe	Changed lstrcmp() to lstrcmpi().
//  11-14-86	msd	added calls to Init/FreeFontSummary
//   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
//   1-17-89	jimmat	Added PCL_* entry points to lock/unlock data seg.
//   1-18-89	jimmat	Space for the epBuf is only added to the DEVICE
//			struct when in LANDSCAPE (that's when it's used).
//   1-25-89	jimmat	Use global hLibInst instead of GetModuleHandle() -
//			No longer requires lclstr.h.
//   2-07-89	jimmat	Driver Initialization changes.
//   2-24-89	jimmat	Removed parameters to lp_enbl(), lp_disable().
//

#include "generic.h"
#include "resource.h"
#define FONTMAN_ENABLE
#define FONTMAN_DISABLE
#include "fontman.h"
#include "strings.h"
#include "environ.h"
#include "utils.h"
#include "dump.h"             /*  LaserPort  */
#include "paper.h"
#include "version.h"

#define USEEXTDEVMODE 1

#define LOCAL static


/*  Local debug structure.
 */

#ifdef DEBUG
    #define LOCAL_DEBUG
#endif

#ifdef LOCAL_DEBUG
    #define DBGsizeof(msg) DBMSG(msg)
    #define DBGentry(msg) /* DBMSG(msg) */
    #define DBGenable(msg) /* DBMSG(msg) */
#else
    #define DBGsizeof(msg) /*null*/
    #define DBGentry(msg) /*null*/
    #define DBGenable(msg) /*null*/
#endif

void PASCAL GetGdiInfo(GDIINFO FAR *, short, short);
short FAR PASCAL ExtDeviceMode(HWND, HANDLE, LPPCLDEVMODE,
	                       LPSTR, LPSTR, LPPCLDEVMODE, LPSTR, WORD);

/* need these to determine display driver abilities.
 */

HDC FAR PASCAL GetDC(HWND);
void FAR PASCAL ReleaseDC(HWND,HDC);
WORD FAR PASCAL GetDeviceCaps(HDC,WORD);

#define RASTERCAPS 38


/* The default portrait GDI info (for portrait mode) to describe the printer
 * to windows
 */
GDIINFO GDIdefault = { 0x300,	/* dpVersion */
    DT_RASPRINTER,              /* devices classification */
    0,                          /* dpHorzSize = page width in millimeters */
    0,                          /* dpVertSize = page height in millimeters */
    0,                          /* dpHorzRes = pixel width of page */
    0,                          /* dpVertRes = pixel height of page */
    1,                          /* dpBitsPixel = bits per pixel */
    1,				/* dpPlanes = # of bit planes */
    17, 			/* dpNumBrushes = # of brushes */
    2,				/* dpNumPens = # of pens on the device */
    0,                          /* futureuse (not documented) */
    4,				/* dpNumFonts = # of fonts for device */
    2,				/* dpNumColors = # of colors in color tbl */
    0,				/* dpDEVICEsize = size of device desciptor */
    CC_NONE,                    /* dpCurves = no curve capabilities */
    LC_NONE,    		/* dpLines = no line capabilities */
    PC_SCANLINE,                /* dpPolygonals = scanline capabilities */
    TC_OP_CHARACTER | TC_UA_ABLE, /* dpText */
    CP_NONE,                    /* dpClip = no clipping of output */
    RC_BITBLT | RC_BANDING |	/* dpRaster = (BitBlt only) */
    RC_SCALING | RC_GDI15_OUTPUT,
    XASPECT,                    /* dpAspectY = x major distance */
    YASPECT,                    /* dpAspectX = y major distance */
    XYASPECT,                   /* dpAspectXY = hypotenuse distance */
    MAXSTYLELEN,		/* dpStyleLen = Len of segment for line style */
	{ 254, 254 },		/* dpMLoWin: tenths of millimeter in an inch */
	{ HDPI, -VDPI },	/* dpMLoVpt: resolution in dots per inch */
	{ 2540, 2540 }, 	/* dpMHiWin: hundreths of millimeter in inch */
	{ HDPI, -VDPI },	/* dpMHiVpt: resolution in dots per inch */
	{ 100, 100 },		/* dpELoWin: hundreths of an inch in an inch */
	{ HDPI, -VDPI },	/* dpELoVpt: resolution in dots per inch */
	{ 1000, 1000 }, 	/* dpEHiWin: thousandths of inch in an inch */
	{ HDPI, -VDPI },	/* dpEHiVpt: resolution in dots per inch */
	{ 1440, 1440 }, 	/* dpTwpWin: twips in an inch */
	{ HDPI, -VDPI },	/* dpTwpVpt: resolution in dots per inch */
    HDPI,                       /* dpLogPixelsX */
    VDPI,                       /* dpLogPixelsY */
    DC_SPDevice,		/* dpDCManage: 1 PDevice needed per file */
    0, 0, 0, 0, 0               /* futureuse3 to futureuse 7 */
};

#if defined(DEBUG)
static void dumplpDevice(lpDevice)
    LPDEVICE lpDevice;
    {
    DBMSG(("lpDevice=%lp\n", lpDevice));
    DBMSG(("epType = %d\n", lpDevice->epType));
    DBMSG(("epBmpHdr = %p\n", lpDevice->epBmpHdr));
    DBMSG(("  epBmpHdr.bmType = %d\n", lpDevice->epBmpHdr.bmType));
    DBMSG(("  epBmpHdr.bmWidth = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmWidth));
    DBMSG(("  epBmpHdr.bmHeight = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmHeight));
    DBMSG(("  epBmpHdr.bmWidthBytes = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmWidthBytes));
    DBMSG(("  epBmpHdr.bmPlanes = %d\n", (BYTE)lpDevice->epBmpHdr.bmPlanes));
    DBMSG(("  epBmpHdr.bmBits = %lp\n", lpDevice->epBmpHdr.bmBits));
    DBMSG(("  epBmpHdr.bmWidthPlanes = %ld\n",
        (unsigned long)lpDevice->epBmpHdr.bmWidthPlanes));
    DBMSG(("  epBmpHdr.bmlpBDevice = %lp\n", lpDevice->epBmpHdr.bmlpPDevice));
/*  DBMSG(("  epBmpHdr.bmSegmentIndex = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmSegmentIndex));
    DBMSG(("  epBmpHdr.bmScanSegment = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmScanSegment));
    DBMSG(("  epBmpHdr.bmFillBytes = %d\n",
        (unsigned)lpDevice->epBmpHdr.bmFillBytes));
 */
    DBMSG(("  epBmpHdr.futureUse4 = %d\n",
        (unsigned)lpDevice->epBmpHdr.futureUse4));
    DBMSG(("  epBmpHdr.futureUse5 = %d\n",
        (unsigned)lpDevice->epBmpHdr.futureUse5));
    DBMSG(("epPF = %p\n", lpDevice->epPF));
    DBMSG(("ephDC = %d\n", lpDevice->ephDC));
    DBMSG(("epMode = %d\n", lpDevice->epMode));
    DBMSG(("epNband = %d\n", lpDevice->epNband));
    DBMSG(("epXOffset = %d\n", lpDevice->epXOffset));
    DBMSG(("epYOffset = %d\n", lpDevice->epYOffset));
    DBMSG(("epJob = %d\n", lpDevice->epJob));
    DBMSG(("epDoc = %d\n", lpDevice->epDoc));
    DBMSG(("epPtr = %d\n", (unsigned)lpDevice->epPtr));
    DBMSG(("epXerr = %d\n", lpDevice->epXerr));
    DBMSG(("epECtl = %d\n", lpDevice->epECtl));
    DBMSG(("epCurx = %d\n", lpDevice->epCurx));
    DBMSG(("epCury = %d\n", lpDevice->epCury));
    DBMSG(("epHFntSum = %d\n", lpDevice->epHFntSum));
    }
#endif

short FAR PASCAL Enable(LPDEVICE,short,LPSTR,LPSTR,LPPCLDEVMODE);
int   FAR PASCAL Disable(LPDEVICE);


/*
 *  PCL_Enable
 *
 *  Enable entry point to lock/unlock data segment.
 */

short FAR PASCAL
PCL_Enable(lpDevice, style, lpDeviceType, lpOutputFile, lpStuff)
LPDEVICE lpDevice;
short style;
LPSTR lpDeviceType;
LPSTR lpOutputFile;
LPPCLDEVMODE lpStuff;
{
    short rc;

    LockSegment(-1);

    rc = Enable(lpDevice, style, lpDeviceType, lpOutputFile, lpStuff);

    UnlockSegment(-1);

    return rc;
}


/*  Enable
 *
 *  GDI calls this proc when the application does a CreateIC or CreateDC.
 */
short FAR PASCAL
Enable(lpDevice, style, lpDeviceType, lpOutputFile, lpStuff)
LPDEVICE lpDevice;
short style;
LPSTR lpDeviceType;
LPSTR lpOutputFile;
LPPCLDEVMODE lpStuff;
{
    PAPERFORMAT pf;
    PCLDEVMODE tEnviron;
    extern HANDLE hLibInst;

    DBGentry(("Enable(%lp,%d,%lp,%lp,%lp)\n", lpDevice, style, lpDeviceType,
	     lpOutputFile, lpStuff));
    DBGenable(("Enable(): lpDeviceType=%ls\n", lpDeviceType));
    DBGenable(("Enable(): lpOutputFile=%ls\n", lpOutputFile));

    #ifdef LOCAL_DEBUG
    DBGenable(("Enable(): style=%d", style));
    if (style & InquireInfo)
        DBGenable((", InquireInfo"));
    if (style & EnableDevice)
        DBGenable((", EnableDevice"));
    if (style & InfoContext)
        DBGenable((", InfoContext"));
    DBGenable(("\n"));
    #endif

    /*	If the caller has passed in an environment, make sure it's valid.
     *	If one wasn't passed in, then build one.
     */

#if USEEXTDEVMODE
    if (ExtDeviceMode((HWND) NULL, (HANDLE) NULL, (LPPCLDEVMODE) &tEnviron,
	      lpDeviceType, (LPSTR) lpOutputFile, (LPPCLDEVMODE) lpStuff,
	      (LPSTR) NULL, (WORD) DM_COPY | DM_MODIFY) < 0)
	    return FALSE;
    lpStuff = &tEnviron;
#else
    if (lpStuff) {
	if (lpStuff->dm.dmSize != sizeof(DEVMODE) ||
	    lpStuff->dm.dmDriverVersion != VNUMint ||
	    lpStuff->dm.dmSpecVersion != DM_SPECVERSION ||
	    lpStuff->dm.dmDriverExtra != sizeof(PCLDEVMODE)-sizeof(DEVMODE) ||
	    lstrcmpi((LPSTR)lpDeviceType, (LPSTR)lpStuff->dm.dmDeviceName)) {
	    DBGenable(("Enable: returning FALSE, incomming "
		       "environment invalid\n"));
	    return FALSE;
	}
    } else {
	if (!GetEnvironment(lpOutputFile,(LPSTR)&tEnviron,sizeof(PCLDEVMODE)) ||
	        lstrcmpi(lpDeviceType, tEnviron.dm.dmDeviceName))
          {
	    MakeEnvironment(&tEnviron, lpDeviceType, lpOutputFile, NULL);
	    SetEnvironment(lpOutputFile, (LPSTR)&tEnviron, sizeof(PCLDEVMODE));
          }
        lpStuff = &tEnviron;
    }
#endif


    /*  Read the paper format resource.
     */
    if (!GetPaperFormat(&pf, hLibInst, lpStuff->paperInd,
			lpStuff->dm.dmPaperSize,lpStuff->dm.dmOrientation))
        return FALSE;

    if (style & InquireInfo) {

        /*  fill in GDIInfo structure
         */
	GetGdiInfo((GDIINFO FAR *)lpDevice, lpStuff->dm.dmOrientation,
            lpStuff->prtCaps);

        /* give GDIINFO needed size of LPDEVICE structure
	 */
        ((GDIINFO far *)lpDevice)->dpDEVICEsize = (style & InfoContext) ?
	    sizeof(DEVICEHDR) :
	    // basic DEVICE struct size + size of band buffer +
	    ( sizeof(DEVICE) + ComputeBandBitmapSize(&pf, lpStuff) +
	      // size of transpose buffer for landscape +
	      (lpStuff->dm.dmOrientation == DMORIENT_LANDSCAPE ?
	      MAX_BAND_WIDTH : 0) +
	      // size of scanline buffer for special printers
	      (lpStuff->prtCaps & HPLJIIP ?
		ComputeLineBufSize(&pf, lpStuff) : 0) );

	//////
        DBGsizeof(("sizeof LPDEVICE structure %d\n",
            ((GDIINFO far *)lpDevice)->dpDEVICEsize));
	if (lpStuff->prtCaps & HPLJIIP) 
	    {
	    DBGsizeof(("..It's a HP LJ II P printer..\n"));
	    DBGsizeof(("  Scale factor prtResFac %d\n",
		    lpStuff->prtResFac));
	    DBGsizeof(("  sizeof epLineBuf in LPDEVICE %d\n",
		    ComputeLineBufSize(&pf, lpStuff) ));
	    }
	//////

        /* Give the paper size in millimeters (25.4 mm/inch)
         */
        ((GDIINFO far *)lpDevice)->dpHorzSize =
            (short)ldiv(labdivc((long)pf.xImage,(long)254,(long)HDPI),(long)10);

        ((GDIINFO far *)lpDevice)->dpVertSize =
            (short)ldiv(labdivc((long)pf.yImage,(long)254,(long)VDPI),(long)10);

        /*  Give the image area in device units, i.e. dots
         */
        ((GDIINFO far *)lpDevice)->dpHorzRes = pf.xImage;
        ((GDIINFO far *)lpDevice)->dpVertRes = pf.yImage;

        DBGenable(("dpHorzRes is %d, dpVertRes %d\n", pf.xImage, pf.yImage));
        DBGenable(("Enable(): ...returning INFO\n"));

        return sizeof (GDIINFO);
    }

    #ifdef LOCAL_DEBUG

	// show bit in PCLDEVMODE.prtCaps
	if (lpStuff->prtCaps & HPLJIIP)
	    DBGsizeof(("--- PCLDEVMODE.prtCaps: printer is a HP LJ II P\n"));
	else
	    DBGsizeof(("--- PCLDEVMODE.prtCaps: printer is NOT a HP LJ IIP\n"));

    #endif

    #ifdef LOCAL_DEBUGer
    DBGenable(("Enable(): prtCaps(%d)...\n", (WORD)lpStuff->prtCaps));
    if (lpStuff->prtCaps & HPJET)
        DBMSG(("   HPJET       printer has capabilities of a laserjet\n"));
    if (lpStuff->prtCaps & HPPLUS)
        DBMSG(("   HPPLUS      printer has capabilities of a laserjet plus\n"));
    if (lpStuff->prtCaps & HP500)
        DBMSG(("   HP500       printer has capabilities of a laserjet 500\n"));
    if (lpStuff->prtCaps & LOTRAY)
        DBMSG(("   LOTRAY      lower tray is handled\n"));
    if (lpStuff->prtCaps & NOSOFT)
      DBMSG(("   NOSOFT      printer does *not* support downloadable fonts\n"));
    if (lpStuff->prtCaps & NOMAN)
        DBMSG(("   NOMAN       manual feed is *not* supported\n"));
    if (lpStuff->prtCaps & NOBITSTRIP)
      DBMSG(("   NOBITSTRIP  printer cannot support internal bit stripping\n"));
    if (lpStuff->prtCaps & HPEMUL)
        DBMSG(("   HPEMUL      printer emulates an hplaserjet\n"));
    if (lpStuff->prtCaps & ANYDUPLEX)
	DBMSG(("   ANYDUPLEX printer can print duplex\n"));
    if (lpStuff->prtCaps & AUTOSELECT)
      DBMSG((
    "   AUTOSELECT  printer selects paper bin based on paper size (auto select)\n"));
    if (lpStuff->prtCaps & BOTHORIENT)
        DBMSG(("   BOTHORIENT  printer can print fonts in any orientation\n"));
    if (lpStuff->prtCaps & HPSERIESII)
        DBMSG(("   HPSERIESII  printer has capabilities of a Series II\n"));
    #endif

    /*  fill in LPDEVICE structure
     */
    DBGenable(("Initializing LPDEVICE\n"));
    lmemset((LPSTR)lpDevice, 0, sizeof (DEVICEHDR));

    lmemcpy((LPSTR)&lpDevice->epPF, (LPSTR)&pf, sizeof(PAPERFORMAT));

    // fill in epType, and epBuf -- offset of landscape buffer
    if (lpStuff->dm.dmOrientation == DMORIENT_LANDSCAPE)
	{ // landscape: requires special buffer for transposed part of band
	lpDevice->epType = (short)DEV_LAND;
	if (!(style & InfoContext))
	    lpDevice->epBuf = sizeof(DEVICE) +
			      ComputeBandBitmapSize(&pf, lpStuff);

	#ifdef LOCAL_DEBUG
	if (!(style & InfoContext))
	    DBGsizeof(("__(Landscape) offset of epBuf in DEVICE %d\n",
						lpDevice->epBuf ));
	#endif
	}
    else
	{ // portrait: doesn't require buffer for transposed band.
	lpDevice->epType = (short)DEV_PORT;
	if (!(style & InfoContext))
	    lpDevice->epBuf = 0;

	#ifdef LOCAL_DEBUG
	if (!(style & InfoContext))
	    DBGsizeof(("__(Portrait) offset of epBuf in DEVICE %d\n",
						lpDevice->epBuf ));
	#endif
	}

    // fill in epLineBuf -- offset to special scanline buffer.

    if (!(style & InfoContext))
	{
	lpDevice->epLineBuf =
	    (lpStuff->prtCaps & HPLJIIP) ?	// needs scanline buffer?
		((lpStuff->dm.dmOrientation == DMORIENT_LANDSCAPE) ?
		    // Landscape: just after transpose buffer
		    (lpDevice->epBuf) + MAX_BAND_WIDTH :
		    // Portrait: just after banding buffer
		    sizeof(DEVICE) + ComputeBandBitmapSize(&pf, lpStuff)
		)  :
		0;				// no scanline buffer.

	DBGsizeof(("__Offset of epLineBuf in DEVICE %d\n",
						lpDevice->epLineBuf ));
	}

    /*  Set up banding bitmap.  This proc requires
     *  epType and epPF to be set.
     */
    ComputeBandingParameters (lpDevice, lpStuff->prtResFac);

    /*convert KB to bytes*/
    lpDevice->epAvailMem = lmul((long)lpStuff->availmem, ((long)1 << 10));

    if (lpStuff->dm.dmPrintQuality == DMRES_DRAFT)
	lpDevice->epMode |= DRAFTFLAG;

    lpDevice->epFreeMem = lpDevice->epAvailMem;
    lpDevice->epScaleFac = lpStuff->prtResFac;
    lpDevice->epCopies = lpStuff->dm.dmCopies;
    lpDevice->epTray = lpStuff->dm.dmDefaultSource;
    lpDevice->epPaper = lpStuff->dm.dmPaperSize;
    lpDevice->epMaxPgSoft = lpStuff->maxPgSoft;
    lpDevice->epMaxSoft = lpStuff->maxSoft;
    lpDevice->epCaps = lpStuff->prtCaps;
    lpDevice->epDuplex = lpStuff->dm.dmDuplex;
    lpDevice->epTxWhite = lpStuff->txwhite;
    lpDevice->epOptions = lpStuff->options;
    lpDevice->epJust = fromdrawmode;

    /*  Turn off the options bit for the DP-TEK LaserPort
     *  if the card is not present.
     */
#ifdef VISUALEDGE
    if ((lpDevice->epOptions & OPTIONS_DPTEKCARD) && !lp_enbl()) /* LaserPort */

	// if VISUALEDGE isn't defined, ALWAYS turn this bit off.

#endif

        lpDevice->epOptions &= ~(OPTIONS_DPTEKCARD);


    /*  Force soft fonts, even to a standard laserjet, if the
     *  option bit is set.  This exists so users can load up
     *  PFM files for cartridge fonts on their standard laserjet.
     */
    if (lpDevice->epOptions & OPTIONS_FORCESOFT)
        lpDevice->epCaps &= ~(NOSOFT);

    if (lpOutputFile)
        {
/* Changed to lstrncpy to avoid walking off the end of a segment.
 *                       2 February 1990  Clark R. Cyr
 *      lmemcpy(lpDevice->epPort, lpOutputFile, NAME_LEN);
 */
        lstrncpy(lpDevice->epPort, lpOutputFile, NAME_LEN);
        lpDevice->epPort[NAME_LEN-1] = '\0';
        }
    else
	LoadString(hLibInst, NULL_PORT, (LPSTR)lpDevice->epPort, NAME_LEN);

/* See comment above
 *  lmemcpy(lpDevice->epDevice, lpDeviceType, NAME_LEN);
 */
    lstrncpy(lpDevice->epDevice, lpDeviceType, NAME_LEN);
    lpDevice->epDevice[NAME_LEN-1] = '\0';

    lpDevice->epLPFntSum = 0L;
    lpDevice->epHWidths = 0;

    if (lpDevice->epHFntSum =
	GetFontSummary(lpOutputFile, lpDeviceType, lpStuff, hLibInst))
        {
        DBGenable(("Enable(): ...returning ENABLED\n"));
        return TRUE;
        }
    else
        {
        DBGenable(("Enable(): ...returning *not* ENABLED\n"));
        return FALSE;
        }
    }


/*
 *  PCL_Disable
 *
 *  Disable entry point to lock/unlock data segment.
 */

int FAR PASCAL
PCL_Disable(lpDevice)
LPDEVICE lpDevice;
{
    int rc;

    LockSegment(-1);

    rc = Disable(lpDevice);

    UnlockSegment(-1);

    return rc;
}


/*  Disable                                                
 */
far PASCAL Disable(lpDevice)
    LPDEVICE lpDevice;
    {
    DBGentry(("Disable(%lp)\n", lpDevice));

#ifdef VISUALEDGE
    if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
	lp_disable();				    /* LaserPort */
#endif

    if (lpDevice->epHWidths)
        {
        GlobalFree(lpDevice->epHWidths);
        lpDevice->epHWidths = 0;
        }

    FreeFontSummary(lpDevice);

    return TRUE;
    }


/*  GetGdiInfo
 *
 *  Get the default GDIINFO structure.
 */
void PASCAL GetGdiInfo(lpdp, orient, prtCaps)
    GDIINFO FAR *lpdp;
    short orient;
    short prtCaps;
    {
    HDC hdcScreen;

    lmemcpy((LPSTR)lpdp, (LPSTR) &GDIdefault, sizeof (GDIINFO));

    /* determine if the display driver (ie, the brute routines)
     * will support bitmaps larger than 64k.  If so, we'll set the
     * bit for it.
     */
    hdcScreen = GetDC(NULL);
    if (GetDeviceCaps(hdcScreen,RASTERCAPS) & RC_BITMAP64)
	lpdp->dpRaster |= RC_BITMAP64;
    ReleaseDC(NULL,hdcScreen);

    if (orient == DMORIENT_LANDSCAPE)
        {
        /*  reverse necessary horizontal and vertical fields
         */
        PTTYPE far *iptr;
        short temp, index;

        if (HDPI != VDPI)
            {
            /*  exchange x and y values
             */
            temp = lpdp->dpAspectX;
            lpdp->dpAspectX = lpdp->dpAspectY;
            lpdp->dpAspectY = temp;
            }

        /*  exchange x and y settings for page measurement values
         *  invert 1 <--> 2, 3 <--> 4 for landscape
         */

        /*  exchange x and y values
         */
        for (iptr = &(lpdp->dpMLoWin), index = 0;
            index < 10;
            iptr++, index++)
            {
            if (iptr->ycoord >= 0)
                {
                temp = iptr->ycoord;
                iptr->ycoord = iptr->xcoord;
                iptr->xcoord = temp;
                }
            else
                {
                temp = -iptr->ycoord;
                iptr->ycoord = -iptr->xcoord;
                iptr->xcoord = temp;
                }
            }
        }

    /*  We can do strike-out if the printer is anything other
     *  than a normal laserjet.
     */
    if (!(prtCaps & HPJET))
        lpdp->dpText |= TC_SO_ABLE;
    }
