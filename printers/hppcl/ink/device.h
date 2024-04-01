/**[f******************************************************************
 * device.h -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 *             All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 *
 *  05 sep 89	peterbe	Bring some comments up to date.
 *			Added epLineBuf.
 *  17 apr 89	peterbe	Change tabs, cleanup.
 *   1-18-89    jimmat  Now space for epBuf is only allocated if the printer
 *          is in landscape mode.
 */

/* hplaserjet's own device.h */
#define BLOCK_SIZE	512
#define LINE_LEN	80

#define DEV_PORT    0x8888
#define DEV_LAND    0x8889

#define DRAFTFLAG   0x01
#define BREAKFLAG   0x02
#define GRXFLAG	    0x04    /* flag to see if any graphics output to the buffer */
#define LOWRES	    0x08
#define TEXTFLAG    0x10    /* text output to the buffer */
#define INFO	    0x20
#define ANYGRX	    0x40    /* any graphics in the entire page */


typedef TEXTXFORM far * LPTEXTXFORM;
typedef LOGFONT	  far * LPLOGFONT;
typedef FONTINFO  far * LPFONTINFO;
typedef DRAWMODE  far * LPDRAWMODE;
typedef TEXTMETRIC far * LPTEXTMETRIC;

//      Font weights lightest to darkest.
#define FW_DONTCARE		0
#define FW_THIN			100
#define FW_EXTRALIGHT		200
#define FW_LIGHT		300
#define FW_NORMAL		400
#define FW_MEDIUM		500
#define FW_SEMIBOLD		600
#define FW_BOLD			700
#define FW_EXTRABOLD		800
#define FW_HEAVY		900

#define FW_ULTRALIGHT		FW_EXTRALIGHT
#define FW_REGULAR		FW_NORMAL
#define FW_DEMIBOLD		FW_SEMIBOLD
#define FW_ULTRABOLD		FW_EXTRABOLD
#define FW_BLACK		FW_HEAVY

BOOL FAR PASCAL TextOut(HANDLE, short, short, LPSTR, short);

int	    FAR PASCAL SetRect(LPRECT, int, int, int, int);
int	    FAR PASCAL SetRectEmpty(LPRECT);
int	    FAR PASCAL CopyRect(LPRECT, LPRECT);
int	    FAR PASCAL InflateRect(LPRECT, int, int);
int	    FAR PASCAL IntersectRect(LPRECT, LPRECT, LPRECT);
int	    FAR PASCAL UnionRect(LPRECT, LPRECT, LPRECT);
int	    FAR PASCAL OffsetRect(LPRECT, int, int);
BOOL	    FAR PASCAL IsRectEmpty(LPRECT);
BOOL	    FAR PASCAL PtInRect(LPRECT, LPPOINT);



/* heap structure :

    base                                                    base + start
    |                                                            |
   \|/                                                          \|/
    string chars (ELEMENT)--->                              index ---->
*/

typedef struct
    {
    FONTINFO fontInfo;
    short indFontSummary;	/* ind of selected font */
    SYMBOLSET symbolSet;	/* symbol set: USASCII or Roman 8 */
    BOOL ZCART_hack;		/* Z cartridge hack */
    BOOL QUOTE_hack;		/* Typographic quotes hack */
    BOOL isextpfm;		/* TRUE if the PFM is not in a resource */
    } PRDFONTINFO, far * LPPRDFONTINFO;

typedef struct {
    char InitStyleError;
    char Hypoteneuse;
    char XMajorDistance;
    char YMajorDistance;
    } ASPECT;

typedef enum {
    fromdrawmode,
    justifywordbreaks,
    justifyletters
    } JUSTBREAKTYPE;

typedef struct {
    short extra;
    short rem;
    short err;
    WORD count;
    WORD ccount;
    } JUSTBREAKREC;

typedef JUSTBREAKTYPE FAR *LPJUSTBREAKTYPE;
typedef JUSTBREAKREC FAR *LPJUSTBREAKREC;


/*****************************************************************/
/*  *** REMEMBER *** IF YOU PUT IT HERE, PUT IT IN DEVICE.I TOO ***
 */

typedef struct {
    short epType;		/* DEV_LAND means landscape device
                                        DEV_PORT means portrait device */
    BITMAP epBmpHdr;		/* bitmap structure */
    PAPERFORMAT epPF;
    short ephDC;		/* apps's callback abort proc */
    short epMode;		/* draft mode */
    short epNband;		/* nth band */
    short epXOffset;
    short epYOffset;
    short epJob;		/* job number */
    short epDoc;		/* current status of the document */
    unsigned epPtr;		/* spool buffer pointer */
    short epXerr;		/* running round off err */
    short epYerr;		/* running round off err */
    short ephMd;		/* module handle */
    short epECtl;		/* last escape control selected */
    short epCurx;
    short epCury;
    short epNumBands;		/* number of print bands on page */
    short epLastBandSz;	    	/* size of last band (does not have to be 64) */
    short epScaleFac;		/* 0=300 dpi ptr res, 1= 150, 2 = 75 */
    short epCopies;	    	/* number of copies */
    short epTray;		/* chosen tray */
    short epPaper;		/* chosen paper size:  20=LETTER, 21=DINA4,
				   22=LEGAL, 23=B5, 24=EXEC, 25=A3, 26=LEDGER */
    BYTE epFontSub;	    	/* set if warning message has already been given
                                        for substituted fonts on the page*/
    short epPgSoftNum;	    	/* number of soft fonts used on current page */
    short epTotSoftNum;	    	/* # soft fonts downloaded, incl. permanent */
    short epMaxPgSoft;	    	/* maximum soft fonts per page */
    short epMaxSoft;		/* maximum soft fonts */
    BYTE epGDItext;	    	/* set if GDI simulates text attributes */
    BYTE epOpaqText;		/* set if we got an opaque rectangle */
    RECT epGrxRect;	    	/* enclosing rect for page graphics */
    short epCaps;		/* printer capabilites */
    short epOptions;		/* options bits from options dialog */
    short epDuplex;	    	/* 0=no duplex, 1=duplex printing */
    short epPageCount;
    long epAvailMem;		/* initial available memory at reset time */
    long epFreeMem;
    short epTxWhite;		/* white text intensity */
    HANDLE epHFntSum;		/* handle fontSummary struct */
    LPSTR epLPFntSum;		/* locked pointer fontSummary struct */
    JUSTBREAKTYPE epJust;   	/* kind of justification */
    JUSTBREAKREC epJustWB;  	/* justification rec for word breaks */
    JUSTBREAKREC epJustLTR; 	/* justification rec for letters */
    HANDLE epHWidths;		/* widths for ExtTextOut() */
    char epDevice[NAME_LEN];
    char epPort[NAME_LEN];
    } DEVICEHDR;


// NOTE:  This data structure (LPDEVICE) is duplicated in "device.i" for
//        use by "dumputil.a" and "lasport.a".  ANY changes to this
//	  structure should also be made in "device.inc".
//

typedef struct {
    short epType;		/* DEV_LAND means landscape device
                                       DEV_PORT means portrait device */
    BITMAP epBmpHdr;		/* bitmap structure */
    PAPERFORMAT epPF;
    short ephDC;		/* apps's callback abort proc */
    short epMode;		/* draft mode */
    short epNband;		/* nth band */
    short epXOffset;
    short epYOffset;
    short epJob;		/* job number */
    short epDoc;		/* current status of the document */
    unsigned epPtr;	    	/* spool buffer pointer */
    short epXerr;
    short epYerr;
    short ephMd;		/* module handle */
    short epECtl;		/* last escape control selected */
    short epCurx;
    short epCury;
    short epNumBands;		/* number of print bands on page */
    short epLastBandSz;	    	/* size of last band (does not have to be 64) */
    short epScaleFac;		/* 0=300 dpi ptr res, 1= 150, 2 = 75 */
    short epCopies;	    	/* number of copies */
    short epTray;
    short epPaper;		// chosen paper size:  20=LETTER, 21=DINA4,
                                // 22=LEGAL, 23=B5, 24=EXEC, 25=A3, 26=LEDGER 
    BYTE epFontSub;	    	// set if warning message has already been given
                                //      for substituted fonts on the page 
    short epPgSoftNum;	    	/* number of soft fonts used on current page */
    short epTotSoftNum;		// # soft fonts downloaded, including permanent 
    short epMaxPgSoft;	    	/* maximum soft fonts per page */
    short epMaxSoft;		/* maximum soft fonts */
    BYTE epGDItext;	    	/* set if GDI simulates text attributes */
    BYTE epOpaqText;		/* set if we got an opaque rectangle */
    RECT epGrxRect;	    	/* enclosing rect for page graphics */
    short epCaps;		/* printer capabilites */
    short epOptions;		/* options bits from options dialog */
    short epDuplex;	    	/* 0=no duplex, 1=duplex printing */
    short epPageCount;
    /*memory usage variables*/
    long epAvailMem;		/* initial available memory at reset time */
    long epFreeMem;
    short   epTxWhite;		/* white text intensity */
    HANDLE epHFntSum;		/* handle fontSummary struct */
    LPSTR epLPFntSum;		/* locked pointer fontSummary struct */
    JUSTBREAKTYPE epJust;   	/* kind of justification */
    JUSTBREAKREC epJustWB;  	/* justification rec for word breaks */
    JUSTBREAKREC epJustLTR; 	/* justification rec for letters */
    HANDLE epHWidths;		/* widths for ExtTextOut() */
    char epDevice[NAME_LEN];
    char epPort[NAME_LEN];
    char epSpool[SPOOL_SIZE];
    short epBuf;	    	// OFFSET in DEVICE struct to landscape mode
				// buffer (transposed slice of epBmp[]).
    short epLineBuf;		// OFFSET in DEVICE struct to special graphics
				// buffer for 1 scanline.
    char epBmp[1];		/* size of bitmap determined at run time */
    } DEVICE, far *LPDEVICE;


far PASCAL  dmBitblt(LPDEVICE, short, short, BITMAP far *, short, short, short, short, long, long, long);
far PASCAL  dmColorInfo(LPDEVICE, long, long);
far PASCAL  dmEnumDFonts(LPDEVICE, long, long, long);
far PASCAL  dmEnumObj(LPDEVICE, short, long, long);
far PASCAL  dmOutput(LPDEVICE, short, short, LPPOINT, long, long, long, long );
far PASCAL  dmPixel(LPDEVICE, short, short, long,long);
far PASCAL  dmRealizeObject(LPDEVICE, short, LPSTR, LPSTR, LPSTR);
LONG far PASCAL	 dmStrBlt(LPDEVICE, short, short, LPRECT, LPSTR, short, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM);
far PASCAL  dmScanLR(LPDEVICE, short, short, long, short);
far PASCAL  dmTranspose(LPSTR, LPSTR, short);
