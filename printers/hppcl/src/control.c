/**[f******************************************************************
 * control.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   control.c   ******************************/
/*
 *  Control: Driver ESCAPE functions.
 *
 * 05 apr 90    clarkc  If in DRAFT mode, set n = lpDevice->epNumBands + 1
 * 30 jan 90    clarkc  Set ANYGRX if not in 300 dpi.
 * 30 nov 89	peterbe	Visual edge calls in ifdef now.
 * 19 sep 89	peterbe	Moved IIP support from include file.
 * 13 jul 89	peterbe	Moved #include newpattr.h to newprint subdirectory.
 * 20 jun 89	peterbe	Added #include for new stuff in DRAWPATTERNRECT code.
 * 08 may 89	peterbe	Cosmetics
 *   2-07-89	jimmat	Driver Initialization changes.
 *   2-22-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
 */

#include "generic.h"
#include "resource.h"
#include "strings.h"
#include "spool.h"
#define FONTMAN_UTILS
#include "fontman.h"
#include "memoman.h"
#include "environ.h"
#include "utils.h"
#include "dump.h"
#include "extescs.h"
#include "qsort.h"
#include "paper.h"


/*  Utilities
 */
#include "message.c"
#include "lockfont.c"
#include "makefsnm.c"
#include "loadfile.c"


/*  Debug
 */
#define DBGtrace(msg) DBMSG(msg)
#define DBGErr(msg)   DBMSG(msg)
#define DBGMem(msg)   /*DBMSG(msg)*/
#define DBGBandInfo(msg) /*DBMSG(msg)*/

#define LOCAL static

/*  Constants used by ChangeEnvironment().
 */
#define CHG_ORIENT   1
#define CHG_PAPERBIN 2

/*  set this bit to change current rather than default paper bin
 */
#define GSPB_CURRENT 0x8000
#define GSPB_DEFAULT 0x0000

/*  For DRAWPATTERNRECT
 */
typedef struct {
    POINT prPosition;
    POINT prSize;
    WORD  prStyle;
    WORD  prPattern;
    } DRAWPATRECT;
typedef DRAWPATRECT FAR * LPDRAWPATRECT;


/*  For SETALLJUSTVALUES
 */
typedef struct {
    short nCharExtra;
    WORD nCharCount;
    short nBreakExtra;
    WORD nBreakCount;
    } ALLJUSTREC;
typedef ALLJUSTREC FAR * LPALLJUSTREC;

/*  BinInfo data structures for GETSETPAPERBINS.
 */
typedef struct {
    short binNum;
    short numOfBins;
    short reserve1;
    short reserve2;
    short reserve3;
    short reserve4;
    } BININFO;
typedef BININFO FAR * LPBININFO;

#define MAXBINS ((DMBIN_LAST - DMBIN_FIRST) + 2)
#define BINSTRLEN 24

typedef struct {
    short binList[1];
    char paperNames[BINSTRLEN];
    } BINLIST;
typedef BINLIST FAR * LPBINLIST;


extern HANDLE hLibInst; 	/* driver's instance handle */


/*  Forward procs.
 */
LOCAL void CalcBreaks (LPJUSTBREAKREC, short, WORD);
LOCAL void AbortJob (LPDEVICE);
LOCAL BOOL SpoolerON (LPDEVICE);
LOCAL void ChangeEnvironment(LPDEVICE, WORD, short);
LOCAL BOOL EnumPaperBins(LPDEVICE, LPBINLIST, LPBININFO, short far *, short);
LOCAL long ScanSofts(LPDEVICE);
LOCAL void InitStartDoc(LPDEVICE);
LOCAL BOOL LoadPFMStruct(LPDEVICE, LPFONTINFO, LPSTR, WORD);


#ifdef DEBUG
#define DBGepmode(mode) dump_epmode(mode)
LOCAL void dump_epmode(short);
LOCAL void dump_epmode(mode)
    short mode;
    {
    if (mode & DRAFTFLAG)
        { DBGtrace((" DRAFTFLAG")); }
    if (mode & BREAKFLAG)
        { DBGtrace((" BREAKFLAG")); }
    if (mode & GRXFLAG)
        { DBGtrace((" GRXFLAG")); }
    if (mode & LOWRES)
        { DBGtrace((" LOWRES")); }
    if (mode & TEXTFLAG)
        { DBGtrace((" TEXTFLAG")); }
    if (mode & INFO)
        { DBGtrace((" INFO")); }
    if (mode & ANYGRX)
        { DBGtrace((" ANYGRX")); }
    }
#else
#define DBGepmode(mode) /*null*/
#endif

/*  Control II
 *
 *  Windows Escape() function, except QUERYESCSUPPORT.
 */
int far PASCAL Control_II(lpDevice, function, lpInData, lpOutData)
    LPDEVICE lpDevice;
    short function;
    LPSTR lpInData;
     LPPOINT lpOutData;
    /************************************************************************
    DESCRIPTION:
    IN:
    OUT:
    ************************************************************************/   
    {
    unsigned i;
    register char far *p;
    short n;
    ESCtype escstr;

    switch (function)
        {
        case NEXTBAND:
            DBMSG(("NEXTBAND\n"));
            if (lpDevice->epDoc != TRUE)
                {
                DBGErr(("lpDevice->epDoc != TRUE, goto SpoolFail\n"));
                goto SpoolFail;
                }

            n = lpDevice->epNband++;

            DBGtrace(("epNband = %d, epMode = %d",
                lpDevice->epNband, lpDevice->epMode));
            DBGepmode(lpDevice->epMode);
            DBGtrace(("\n"));

            switch (n)
                {
                case 0:
                    i = 0;
                    i |= lpDevice->epMode & DRAFTFLAG;  /*set draftflag*/
                    lpDevice->epMode = i;

                    if ((lpDevice->epDoc =
                        StartSpoolPage(lpDevice->epJob)) != TRUE)
                        {
                        DBGErr(
			    ("Failed to StartSpoolPage(), goto SpoolFail\n"));
                        goto SpoolFail;
                        }

                    lpDevice->epECtl = -1;
                    lpDevice->epCurx = lpDevice->epCury = -1;
                    lpDevice->epXerr = 0;
                    lpDevice->epYerr = 0;
                    lpDevice->epGDItext = FALSE;
                    lpDevice->epOpaqText = FALSE;
                    lpDevice->epXOffset = 0;
                    lpDevice->epYOffset = 0;

                    /*  Increment page count and, if the first page,
                     *  send down job control commands (reset, number
                     *  of copies, simplex/duplex).
                     */
		    if (++lpDevice->epPageCount == 1) {

                        if (lpDevice->epOptions & OPTIONS_RESETJOB)
                            myWrite(lpDevice, RESET);

                        myWrite(lpDevice, (LPSTR)&escstr,               
			    MakeEscape((lpESC)&escstr,HP_COPIES,
				       lpDevice->epCopies));

			if (lpDevice->epCaps & ANYDUPLEX)

                            myWrite(lpDevice, (LPSTR)&escstr,               
				MakeEscape((lpESC)&escstr,HP_DUPLEX,
				(lpDevice->epDuplex == DMDUP_SIMPLEX) ?
				0 : ((lpDevice->epDuplex == DMDUP_VERTICAL) ?
				1 : 2)));

			else
			    lpDevice->epDuplex = DMDUP_SIMPLEX;
		    }

                    DBGtrace(("epPageCount=%d\n", lpDevice->epPageCount));

                    /*  Set first band to whole page.
                     */
                    SetRect((LPRECT)lpOutData, 0, 0, lpDevice->epPF.xPhys,
                        lpDevice->epPF.yPhys);

                    /*  Set graphics rect to whole page, BANDINFO will
                     *  change it if the app uses it.
                     */
                    lpDevice->epGrxRect = *((RECT FAR *)lpOutData);

                    /*  Set page formatting stuff.
                     */
		    if ((lpDevice->epDuplex == DMDUP_SIMPLEX) ||
			(lpDevice->epPageCount % 2))
			{
                        /*  initialize the page (orientation, size, offset)
                         */
			if ((lpDevice->epTray == DMBIN_ENVELOPE) &&
                            (lpDevice->epCaps & HPSERIESII))
                            {
                            /*  Envelope initialization -- note we always
                             *  assume Commercial-10 size envelopes.
                             */
                            if (lpDevice->epType == (short)DEV_LAND)
                                myWrite(lpDevice, LAND_COM10_RESET);
                            else
                                myWrite(lpDevice, PORT_COM10_RESET);
                            }
                        else
                            {
                            /*  Normal page initialization.
                             */
                            myWrite(lpDevice, (LPSTR)lpDevice->epPF.select,
                                lstrlen((LPSTR)lpDevice->epPF.select));
                            }

                        /*  set the paper tray
                         */
                        switch (lpDevice->epTray)
                            {
			    case DMBIN_LOWER:
                                myWrite(lpDevice, (LPSTR)LOWER_TRAY);
                                break;
			    case DMBIN_MANUAL:
                                myWrite(lpDevice, (LPSTR)MAN_FEED);
                                break;
			    case DMBIN_AUTO:
                                myWrite(lpDevice, (LPSTR)PAPER_DECK);
                                break;
			    case DMBIN_ENVELOPE:
                                if (lpDevice->epCaps & NEWENVFEED)
                                    myWrite(lpDevice, ENV_TRAY);
                                else
                                    myWrite(lpDevice, MAN_ENVFEED);
                                break;
                            default:
                                myWrite(lpDevice, (LPSTR)UPPER_TRAY);
                                break;
                            }
                        }

setnoscale:
                    ((LPPOINT)lpInData)->xcoord = ((LPPOINT)lpInData)->ycoord
                        = 0;
                    break;

                case 1:
/* If scaling exists, the graphics were scaled but the clipping region
 * wasn't.  Set the flag as if we had indeed scene graphics.  30 Jan 90
 */
                    if (lpDevice->epScaleFac)
                        lpDevice->epMode |= ANYGRX;

                    if ((lpDevice->epMode & DRAFTFLAG) ||
                        (   !(lpDevice->epMode & ANYGRX) &&
                            !lpDevice->epGDItext)   )
                        {
                        /*  don't band further if in draftmode or if no
                         *  graphics on the page
                         */
                        n = lpDevice->epNumBands + 1;
                        goto setnoscale;
                        }

                    /* Set desired printer resolution */
                    if (!(lpDevice->epScaleFac))
                        myWrite(lpDevice, (LPSTR) HP_SET_300RES);
                    else
                        {
                        if (lpDevice->epScaleFac==1)
                            myWrite(lpDevice, (LPSTR) HP_SET_150RES);
                        else
                            myWrite(lpDevice, (LPSTR) HP_SET_75RES);
                        }

                    /*  Fall through.  Note that GRXFLAG should never be
                     *  set at this point.
                     */
                default:
                    /*  Send current band to the spooler if it contains
                     *  graphics.
                     */
                    if (lpDevice->epMode & GRXFLAG)
                        {
                        /*  If we have skipped previous bands because they
                         *  did not contain graphics, then advance the cursor
                         *  position so we'll place this band correctly.
                         */
                        if (lpDevice->epMode & SKIPFLAG)
                            {
                            POINT pos;
                            ComputeBandStartPosition(&pos, lpDevice, n-1);

                            if (lpDevice->epType == (short)DEV_LAND)
                                xmoveto(lpDevice, pos.xcoord);
                            else
                                ymoveto(lpDevice, pos.ycoord);
                            }

                        /*  Send the band to the printer.
                         */
                        dump(lpDevice);

                        /*  Reset the current position because it
                         *  is unknown after sending the bitmap.
                         */
                        lpDevice->epCurx = -1;
                        lpDevice->epCury = -1;

                        /*  Clear the bitmap.
                         */
                        p = (LPSTR)lpDevice->epBmp;
                        for (i = 0;
			   i < (unsigned)lpDevice->epBmpHdr.bmWidthPlanes; i++)
                            *p++ = (char)0xFF;

                        /*  Clear previous flags.
                         */
                        lpDevice->epMode &= ~(GRXFLAG | SKIPFLAG);
                        }
                    else
                        {
                        /*  Skip this band because it does not contain
                         *  graphics.
                         */
                        DBGtrace(("Skip Band\n"));
                        lpDevice->epMode |= SKIPFLAG;
                        }

                    ((LPPOINT)lpInData)->xcoord = ((LPPOINT)lpInData)->ycoord
                        = lpDevice->epScaleFac;
                    break;
                }

            /*  Calculate coordinates for the next band.
             */
            if (!ComputeNextBandRect(lpDevice,n,(LPRECT)lpOutData) ||
                (n > 0 && !(lpDevice->epMode & ANYGRX) && !lpDevice->epGDItext))
                {
                /* no next band */
                lpDevice->epNband = 0;
                /* reset to default font */
                myWrite(lpDevice, FONT_DEFAULT);
                myWrite(lpDevice, HP_FF);
                myWriteSpool(lpDevice);

                if (lpDevice->epDoc == TRUE)
                    lpDevice->epDoc = EndSpoolPage(lpDevice->epJob);
#ifdef VISUALEDGE
                /* Eject page to LaserPort */
                if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                    lp_ff(lpDevice);     /* LaserPort */
#endif
                SetRectEmpty((LPRECT)lpOutData);
                ((LPPOINT)lpInData)->xcoord = ((LPPOINT)lpInData)->ycoord = 0;
                }
            else if (n == 1)
                {
                /*  Initialize the band bitmap.
                 */
                p = (LPSTR)lpDevice->epBmp;
                for (i = 0; i < (unsigned)lpDevice->epBmpHdr.bmWidthPlanes; i++)
                    *p++ = (char)0xFF;
                }

            if (lpDevice->epDoc != TRUE)
SpoolFail:
                {
                SetRectEmpty((LPRECT)lpOutData);

                DBGErr(("SpoolFail:  calling AbortJob()\n"));
                AbortJob(lpDevice);
                }

            /* fall through and return the status
             */
            DBGtrace(("Band Rect is left=%d,top=%d,right=%d,bottom=%d\n",
		    (*(LPRECT)lpOutData).left,(*(LPRECT)lpOutData).top,
		    (*(LPRECT)lpOutData).right,(*(LPRECT)lpOutData).bottom));
            DBGtrace(("NEXTBAND returning %d\n",lpDevice->epDoc));
            return (lpDevice->epDoc);
        case NEWFRAME:
            DBMSG(("NEW FRAME\n"));
            lpDevice->epFreeMem = lpDevice->epAvailMem-ScanSofts(lpDevice);
            return (lpDevice->epDoc);
        case GETSCALINGFACTOR:
            DBGtrace(("GETSCALINGFACTOR,returns %d\n",lpDevice->epScaleFac));
            ((LPPOINT)lpOutData)->xcoord = ((LPPOINT)lpOutData)->ycoord =
                    lpDevice->epScaleFac;
            break;
        case DRAFTMODE:
            DBGtrace(("DRAFTMODE\n"));
            if (*(short far *)lpInData)
                lpDevice->epMode |= DRAFTFLAG;
            else
                lpDevice->epMode &= ~(DRAFTFLAG);
            break;
        case STARTDOC:
            DBGtrace(("STARTDOC: %ls\n", (LPSTR)lpInData));

            if (lpDevice->epDoc != 0)
                {
                /*  There is another job spooling!  Return an "out of space"
                 *  err to indicate that the job cannot be processed.
                 */
                DBGErr(("...aborting job because another job spooling!\n"));
                return SP_OUTOFDISK;
                }

            InitStartDoc(lpDevice);

#ifdef VISUALEDGE
            if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                lp_Reset(lpDevice);     /* LaserPort */
#endif

            lpDevice->epFreeMem = lpDevice->epAvailMem - ScanSofts(lpDevice);
            lpDevice->epPageCount = 0;

            /*check that minimum memory is available*/
            DBGMem(("free mem is %ld\n",lpDevice->epFreeMem));
            if (lpDevice->epFreeMem < MINMEM)
                {
                ErrorMsg(lpDevice, MAX_PERM_DL);
                return FALSE;
                }

            if ((lpDevice->epJob = OpenJob((LPSTR)lpDevice->epPort,
                    lpInData, lpDevice->ephDC)) > 0)
                {
                lpDevice->epDoc = TRUE;
                }
            #ifdef DEBUG
            else
                {
                DBGErr(("...failed to OpenJob()\n"));
                }
            #endif

            return (lpDevice->epJob);

        case ENDDOC:
            DBGtrace(("ENDDOC\n"));

            /*  If printing duplex and we output an uneven number
             *  of pages, then send down an extra page eject.
             */
	    if ((lpDevice->epCaps & ANYDUPLEX) &&
		(lpDevice->epDuplex != DMDUP_SIMPLEX) &&
                (lpDevice->epPageCount % 2) && (lpDevice->epDoc == TRUE) &&
                (lpDevice->epDoc = StartSpoolPage(lpDevice->epJob)))
                {
                DBGErr(("UNEVEN NUMBER OF PAGES -- SENDING DOWN PAGE EJECT\n"));
                myWrite(lpDevice, (LPSTR)lpDevice->epPF.select,
                    lstrlen((LPSTR)lpDevice->epPF.select));
                ++lpDevice->epPageCount;
                myWriteSpool(lpDevice);
                lpDevice->epDoc = EndSpoolPage(lpDevice->epJob);
                }


            if ((lpDevice->epOptions & OPTIONS_RESETJOB) &&
                (lpDevice->epDoc == TRUE) &&
                (lpDevice->epDoc = StartSpoolPage(lpDevice->epJob)))
                {
                myWrite(lpDevice, RESET);
                myWriteSpool(lpDevice);
                lpDevice->epDoc = EndSpoolPage(lpDevice->epJob);
                }

#ifdef VISUALEDGE
            if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                lp_Reset(lpDevice);     /* LaserPort */
#endif

            if (lpDevice->epDoc == TRUE)
                CloseJob(lpDevice->epJob);
            lpDevice->epDoc = 0;
            return TRUE;
        case GETPHYSPAGESIZE:
            DBGtrace(("GETPHYSPAGESIZE\n"));
            lpOutData->xcoord = lpDevice->epPF.xPhys;
            lpOutData->ycoord = lpDevice->epPF.yPhys;
            break;
        case GETPRINTINGOFFSET:
            DBGtrace(("GETPRINTINGOFFSET\n"));
            lpOutData->xcoord = lpDevice->epPF.xPrintingOffset;
            lpOutData->ycoord = lpDevice->epPF.yPrintingOffset;
            break;
        case SETABORTPROC:
            DBGtrace(("SETABORTPROC\n"));
            lpDevice->ephDC = *(HANDLE far *)lpInData;
            break;
        case ABORTDOC:
            DBGtrace(("ABORTDOC\n"));
            AbortJob(lpDevice);
            break;
        case SETCOPYCOUNT:
            DBGtrace(("SETCOPYCOUNT\n"));

            /* MSD 11/7/88 don't send the escape here -- it is sent at the
             *  beginning of the print job.
             *
             * if (lpInData!=NULL)
             *      myWrite(lpDevice, (LPSTR)&escstr,               
             *          MakeEscape((lpESC)&escstr,HP_COPIES, *lpInData));
             */
            lpDevice->epCopies = *lpInData;
            *((LPSTR)lpOutData) = *lpInData;
            break;
        case BANDINFO:
            DBMSG(("BANDINFO\n"));
            if (lpInData!=NULL) {
                if ((lpDevice->epNband-1)==0) {
                    if (*((BOOL far *)lpInData)++) { 
                        /*  set flag to indicate graphics on page
                         */
                        lpDevice->epMode |= ANYGRX;
                        DBGBandInfo(("graphics on page\n"));
                    }
                }
                if (*((BOOL far *)lpInData)++) {
                    DBGBandInfo(("text on page\n"));
                }
                lpDevice->epGrxRect = *((RECT FAR *)lpInData);
                DBGBandInfo(
			("BandInfo Rect is left=%d,top=%d,right=%d,bottom=%d\n",
                        lpDevice->epGrxRect.left,lpDevice->epGrxRect.top,
                        lpDevice->epGrxRect.right,lpDevice->epGrxRect.bottom));
                }
            if ((lpDevice->epNband-1)==0) {
                *((BOOL far *)lpOutData)++=FALSE; /*no graphics on first band*/
                *((BOOL far *)lpOutData)=TRUE; /*text on first band*/
            }
            else {
                *((BOOL far *)lpOutData)++=TRUE; /*graphics on all other bands*/
                if (lpDevice->epGDItext || lpDevice->epOpaqText) 
		    // text 'cause vector fonts used 
		    *((BOOL far *)lpOutData)=TRUE;
                else
                    *((BOOL far *)lpOutData)=FALSE;  /*no text*/
            }
            break;

        case DEVICEDATA:
            DBGtrace(("DEVICEDATA\n"));
            if (lpInData!=NULL) {
                short bytenum;

                /*  First clear buffer, then output stuff, then
                 *  flush the buffer again and return the number
                 *  of bytes written.
                 */
                bytenum = *((short far *)lpInData)++;

                if (bytenum > 0)
                    {
#ifdef VISUALEDGE
                    /* LaserPort */
                    if ( !(lpDevice->epOptions & OPTIONS_DPTEKCARD) ||
                      !lp_DeviceData(lpDevice,(LPSTR)lpInData,(short)bytenum) )
#endif
                        {
                        myWriteSpool(lpDevice);
                        myWrite(lpDevice,(LPSTR)lpInData,(short)bytenum);
                        myWriteSpool(lpDevice);
                        }
                        return(bytenum);
                    }
                else
                    return(-1);
            }
            return(-1);
            break;

        case SETALLJUSTVALUES:
            {
            LPEXTTEXTDATA lpExtText = (LPEXTTEXTDATA)lpInData;
            LPALLJUSTREC lpAllJust = (LPALLJUSTREC)lpExtText->lpInData;
            LPDRAWMODE lpDrawMode = lpExtText->lpDrawMode;
            LPFONTINFO lpFont = lpExtText->lpFont;

            DBGtrace(("SETALLJUSTVALUES\n"));

            lpDrawMode->TBreakExtra = 0;
            lpDrawMode->BreakExtra = 0;
            lpDrawMode->BreakErr = 1;
            lpDrawMode->BreakRem = 0;
            lpDrawMode->BreakCount = 0;
            lpDrawMode->CharExtra = 0;

            if (lpFont->dfCharSet == OEM_CHARSET)
                {
                /*  Vector font: disable ALLJUSTVALUES and
                 *  return false.
                 */
                lpDevice->epJust = fromdrawmode;
                return FALSE;
                }

            if (lpInData)
                {
                CalcBreaks (&lpDevice->epJustWB, lpAllJust->nBreakExtra,
                    lpAllJust->nBreakCount);
                CalcBreaks (&lpDevice->epJustLTR, lpAllJust->nCharExtra,
                    lpAllJust->nCharCount);

                if (lpDevice->epJustWB.extra || lpDevice->epJustWB.rem ||
                    lpDevice->epJustLTR.extra || lpDevice->epJustLTR.rem)
                    {
                    if (lpDevice->epJustLTR.rem)
                        lpDevice->epJust = justifyletters;
                    else
                        lpDevice->epJust = justifywordbreaks;
                    }
                else
                    /*  Zero justification == shut off ALLJUSTVALUES.
                     */
                    lpDevice->epJust = fromdrawmode;
                }
            break;
            }

        case DRAWPATTERNRECT:
            {
            LPDRAWPATRECT lpPatRect = (LPDRAWPATRECT)lpInData;
            WORD pattern, style;

            DBGtrace(("DRAWPATTERNRECT\n"));

            /* LaserPort does not currently support patterns */
            if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                                return FALSE;       /* LaserPort */

            if (lpDevice->epCaps & HPJET)
                return FALSE;       /* Printer cannot do rules */

            if (lpDevice->epNband != TEXTBAND)
                return TRUE;        /* Do not draw rules on non-text bands */

            /*  Range-check the pattern based upon kind of rule.
             */
            switch (style = lpPatRect->prStyle)
                {
                case 0:			// black fill
                    pattern = 0;
                    break;

		case 1:			// White (erase) fill (II P only)
			// device must have white rules
			if (!(lpDevice->epCaps & HPLJIIP))
			    return FALSE;

			pattern = 0;
			break;

                case 2:			// Shaded gray fill
                    if ((pattern = lpPatRect->prPattern) < 1)
                        pattern = 1;
                    if (pattern > 100)
                        pattern = 100;
                    break;

                case 3:			// HP-defined pattern fill
                    if ((pattern = lpPatRect->prPattern) < 1)
                        pattern = 1;
                    if (pattern > 6)
                        pattern = 6;
                    break;

                default:
                    pattern = 0;
                    style = 0;
                    break;
                }

            /*  Move to output position.
             */
            xmoveto(lpDevice, lpPatRect->prPosition.xcoord);
            ymoveto(lpDevice, lpPatRect->prPosition.ycoord);

            /*  Set size of rule.
             */
            myWrite(lpDevice, (LPSTR)&escstr,               
                MakeEscape((lpESC)&escstr,DOT_HRPS,lpPatRect->prSize.xcoord));
            myWrite(lpDevice, (LPSTR)&escstr,               
                MakeEscape((lpESC)&escstr,DOT_VRPS,lpPatRect->prSize.ycoord));

            /*  Set the pattern if applicable.
             */
            if (pattern)
                {
                myWrite(lpDevice, (LPSTR)&escstr,               
                    MakeEscape((lpESC)&escstr,PAT_ID,pattern));
                }

            /*  Output pattern/rule.
             */
            myWrite(lpDevice, (LPSTR)&escstr,               
                MakeEscape((lpESC)&escstr,PAT_PRINT,style));
            }
            break;

        case ENABLEDUPLEX:
            DBGtrace(("ENABLEDUPLEX\n"));

            if (lpInData && (lpDevice->epCaps & ANYDUPLEX))
                {
                lpDevice->epDuplex = *(short far *)lpInData;

		if (lpDevice->epDuplex == 1)
		    lpDevice->epDuplex = DMDUP_VERTICAL;
		else if (lpDevice->epDuplex == 2)
		    lpDevice->epDuplex == DMDUP_HORIZONTAL;
		else
		    lpDevice->epDuplex == DMDUP_SIMPLEX;

                /*  If doing duplex on a LaserJet IID in landscape, swap the
                 *  duplex bits so it follows the same rules as a LaserJet 2000.
		 */
		if ((lpDevice->epDuplex != DMDUP_SIMPLEX) &&
                    (lpDevice->epCaps & HPIIDDUPLEX) &&
		    (lpDevice->epType == (short)DEV_LAND))

		    lpDevice->epDuplex =
			    (lpDevice->epDuplex == DMDUP_VERTICAL) ?
			    DMDUP_HORIZONTAL : DMDUP_VERTICAL;
                }
            else
                return FALSE;
            break;

        case GETEXTENDEDTEXTMETRICS:
            DBGtrace(("GETEXTENDEDTEXTMETRICS\n"));
            i = FNTLD_EXTMETRICS;
            goto loadPFM;
        case GETPAIRKERNTABLE:
            DBGtrace(("GETPAIRKERNTABLE\n"));
            i = FNTLD_PAIRKERN;
            goto loadPFM;
        case GETTRACKKERNTABLE:
            DBGtrace(("GETTRACKKERNTABLE\n"));
            i = FNTLD_TRACKKERN;
loadPFM:
            {
            LPFONTINFO lpFont = ((LPEXTTEXTDATA)lpInData)->lpFont;

            return (LoadPFMStruct(lpDevice, lpFont, (LPSTR)lpOutData, i));
            }

        case GETSETPRINTORIENT:
            {
            /*  For this escape, 1 = PORTRAIT, 2 = LANDSCAPE.
             */
            short oldOrient;

            DBGtrace(("GETSETPRINTORIENT\n"));

            if (lpDevice->epType == (short)DEV_LAND)
                oldOrient = 2;
            else if (lpDevice->epType == (short)DEV_PORT)
                oldOrient = 1;
            else
                oldOrient = -1;

            if ((oldOrient > 0) && lpInData)
                {
                short orient = lpDevice->epType;

                if (*(short far *)lpInData == 2)
                    orient = (short)DEV_LAND;
                else if (*(short far *)lpInData == 1)
                    orient = (short)DEV_PORT;
                else
                    oldOrient = -1;

                if (orient != lpDevice->epType)
                    {
                    ChangeEnvironment(lpDevice, CHG_ORIENT,
			(orient == (short)DEV_LAND) ? DMORIENT_LANDSCAPE :
						      DMORIENT_PORTRAIT);
                    }
                }

            return (oldOrient);
            }

        case GETSETPAPERBINS:
            DBGtrace(("GETSETPAPERBINS\n"));

            if (lpInData)
                {
                BININFO binInfo;
                LPBININFO lpBinInfo;
                short binNum = (~GSPB_CURRENT) & *((short far *)lpInData);
                short fDefault = !(GSPB_CURRENT & *((short far *)lpInData));
                short xbin[MAXBINS];

                if (lpOutData)
                    lpBinInfo = (LPBININFO)lpOutData;
                else
                    lpBinInfo = &binInfo;

                if (EnumPaperBins(lpDevice,0L,lpBinInfo,xbin,MAXBINS) &&
                    (binNum >= 0) && (binNum < lpBinInfo->numOfBins))
                    {
                    if (fDefault && xbin[binNum] != lpDevice->epTray)
                        /* set the default environment */
                        ChangeEnvironment(lpDevice,CHG_PAPERBIN,xbin[binNum]);
                    else
                        /* set the bin for the current job */
                        lpDevice->epTray = xbin[binNum];
                    }
                else
                    return FALSE;
                }
            else if (lpOutData)
                return(EnumPaperBins(
			lpDevice,0L,(LPBININFO)lpOutData,0L,MAXBINS));
            else
                return FALSE;
            break;

        case ENUMPAPERBINS:
            DBGtrace(("ENUMPAPERBINS\n"));

            if (lpInData && lpOutData)
                {
                short maxBins = *((short far *)lpInData);

                if (maxBins > 0)
                    return(EnumPaperBins(
			lpDevice,(LPBINLIST)lpOutData,0L,0L,maxBins));
                else
                    return FALSE;
                }
            else
                return FALSE;
            break;

        case GETTECHNOLOGY:
            if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                lmemcpy((LPSTR)lpOutData, (LPSTR)"PCL\0LaserPort\0\0", 15);
            else
                lmemcpy((LPSTR)lpOutData, (LPSTR)"PCL\0\0", 5);
            break;

        default:
            return FALSE;
        }

    return TRUE;  /*return true if Escape is successful*/
    }

/*  CalcBreaks
 */
LOCAL void CalcBreaks (lpJustBreak, BreakExtra, Count)
    LPJUSTBREAKREC lpJustBreak;
    short BreakExtra;
    WORD Count;
    {
    if (Count > 0)
        {
        /*  Fill in JustBreak values.  May be positive or negative.
         */
        lpJustBreak->extra = BreakExtra / (short)Count;
        lpJustBreak->rem = BreakExtra % (short)Count;
        lpJustBreak->err = (short)Count / 2 + 1;
        lpJustBreak->count = Count;
        lpJustBreak->ccount = 0;

        /*  Negative justification:  invert rem so the justification algorithm
         *  works properly.
         */
        if (lpJustBreak->rem < 0)
            {
            --lpJustBreak->extra;
            lpJustBreak->rem += (short)Count;
            }
        }
    else
        {
        /*  Count = zero, set up justification rec so the algorithm
         *  always returns zero adjustment.
         */
        lpJustBreak->extra = 0;
        lpJustBreak->rem = 0;
        lpJustBreak->err = 1;
        lpJustBreak->count = 0;
        lpJustBreak->ccount = 0;
        }
    }

/*  AbortJob
 *
 *  Abort the print job -- send down an escape-E to reset the printer
 *  if its okay to do that.
 */
LOCAL void AbortJob (lpDevice)
    LPDEVICE lpDevice;
    {
    DBGtrace(("AbortJob()\n"));

    if (lpDevice->epJob)
        {
        DBGtrace(("AbortJob(): epJob == TRUE\n"));

        if (SpoolerON(lpDevice))
            {
            /*  Spooler on, DeleteJob() will discard the current
             *  page without sending any miscellaneous data to
             *  the printer.
             */
            DBGtrace(("AbortJob(): DeleteJob (spooler ON)\n"));
            DeleteJob(lpDevice->epJob, 0);
            }
        else
            {
            /*  Spooler off, output a RESET and end the job normally,
             *  this will flush the job out of the printer (the last
             *  page will probably come out garbaged).
             */
            if ((lpDevice->epOptions & OPTIONS_RESETJOB) &&
                (lpDevice->epDoc == TRUE) &&
                (lpDevice->epDoc = StartSpoolPage(lpDevice->epJob)))
                {
                DBGtrace(
		  ("AbortJob(): StartSpoolPage, send RESET, EndSpoolPage\n"));

                myWrite(lpDevice, RESET);
                myWriteSpool(lpDevice);
                lpDevice->epDoc = EndSpoolPage(lpDevice->epJob);
                }

#ifdef VISUALEDGE
            if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
                lp_Reset(lpDevice);     /* LaserPort */
#endif

            DBGtrace(("AbortJob(): CloseJob (spooler OFF)\n"));
            CloseJob(lpDevice->epJob);
            }

        lpDevice->epDoc = 0;
        lpDevice->epJob = 0;
        }
    }

/*  SpoolerON
 *
 *  Return TRUE if spooler=yes in the win.ini file.
 */
LOCAL BOOL SpoolerON (lpDevice)
    LPDEVICE lpDevice;
    {
    char nmWin[32];
    char nmSpl[32];
    char s[12];

    lmemset(s, 0, sizeof(s));

    if (LoadString(hLibInst, IDS_WINDOWS, nmWin, sizeof(nmWin)) &&
	LoadString(hLibInst, IDS_SPOOLER, nmSpl, sizeof(nmSpl)) &&
        GetProfileString(nmWin, nmSpl, s, s, sizeof(s)))
        {
        if ((s[0] == 'n') || (s[0] == 'N'))
            return FALSE;
        else
            return TRUE;
        }
    }

/*  ChangeEnvironment
 *
 *  An escape has been executed which changes the environment -- get the
 *  current environment and make the change, write it back, then broadcast
 *  a WM_DEVMODECHANGE message.  Responsible applications should then
 *  Disable() and then (re-) Enable() the driver to correctly effect the
 *  change.
 */
LOCAL void
ChangeEnvironment(LPDEVICE lpDevice, WORD field, short value) {

    PCLDEVMODE pclDevmode;

    /*  Get the current environment.
     */
    MakeEnvironment(&pclDevmode, lpDevice->epDevice, lpDevice->epPort, NULL);

    /*  Modify the specified field.
     */
    switch (field) {

        case CHG_ORIENT:
	    pclDevmode.dm.dmOrientation = value;
            break;

        case CHG_PAPERBIN:
	    pclDevmode.dm.dmDefaultSource = value;
            break;
    }

    /*  Update the environment with the change.
     */
    SetEnvironment(lpDevice->epPort, (LPSTR)&pclDevmode, sizeof(PCLDEVMODE));

    /*  Tell the world.
     */
    SendMessage(0xffff, WM_DEVMODECHANGE, 0, (LONG)(LPSTR)lpDevice->epDevice);
}

/*  EnumPaperBins
 *
 *  Count the number of paper bins that are currently in use.  If passed
 *  in lpBinInfo, then collect the number of bins and the bin number
 *  (0 through MAXBINS) of the currently selected paper bin.  If passed
 *  in xbin, then build a translation table which relates the bin number
 *  (0 through MAXBINS) to the devmode tray number (DMBIN_FIRST...).
 *  If passed in lpOut, then fill in a structure with bin numbers and
 *  corresponding paper description strings.
 */
LOCAL BOOL
EnumPaperBins(LPDEVICE lpDevice, LPBINLIST lpOut, LPBININFO lpBinInfo,
	      short far *lpXbin, short max) {

    int   i, tray;
    LPSTR lpPaperNames = 0L;
    short far *lpBinList = 0L;
    short cBins;

    DBGtrace(("EnumPaperBins(%lp,%lp,%lp,%lp, %d)\n",
        lpDevice, lpOut, lpBinInfo, lpXbin, max));

    if (lpOut) {
        lpBinList = lpOut->binList;
        lpPaperNames = (LPSTR)lpOut->binList + (max * 2);
    }

    if (lpBinInfo)
        lmemset((LPSTR)lpBinInfo, 0, sizeof(BININFO));

    for (cBins = 0, tray = DMBIN_FIRST; (tray <= DMBIN_LAST) &&
	(cBins < max); ++tray) {

/* initialized i instead of having default case.  This way the conditionals
 * for the other case statements don't need else statements.
 *                                2 February 1990   Clark R. Cyr
 */
	i = 0;
	switch (tray) {

	    case DMBIN_UPPER:
		i = IDS_UPPER;
                break;
	    case DMBIN_LOWER:
		if (lpDevice->epCaps & LOTRAY)
		    i = IDS_LOWER;
                break;
	    case DMBIN_MANUAL:
		if (!(lpDevice->epCaps & NOMAN))
		    i = IDS_MANUAL;
                break;
	    case DMBIN_ENVELOPE:
		if (lpDevice->epCaps & ANYENVFEED)
		    i = IDS_ENVELOPE;
                break;
	    case DMBIN_AUTO:
		if (lpDevice->epCaps & AUTOSELECT)
		    i = IDS_AUTO;
                break;
	}

	if (i) {

            /*  Bin translation table.
             */
            if (lpXbin)
		*(lpXbin++) = tray;

            /*  List of bin indices and corresponding description strings.
             */
	    if (lpOut) {
                *(lpBinList++) = cBins;
		LoadString(hLibInst, i, lpPaperNames, BINSTRLEN);
                lpPaperNames += BINSTRLEN;
	    }

	    if (lpBinInfo) {

                /*  Current tray selection.
                 */
                if (lpDevice->epTray == tray)
                    lpBinInfo->binNum = cBins;
	    }
	    cBins++;
	}
    }

    if (lpBinInfo)
        lpBinInfo->numOfBins = cBins;

    return (tray > DMBIN_LAST);
}

LOCAL long ScanSofts(lpDevice)
    LPDEVICE lpDevice;
    /*this is called at start of a page in order to find how much memory is
    occupied by downloaded fonts. The routine also clears the onPage flags
    and sets epTotSoftNum in lpDevice and returns the sum of memory used by all permanents. Memory is in
    bytes. It also resets the printer if the fontsummary information has been
    newly created in order that any previous temp soft fonts are deleted*/
    {
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY   lpSummary;  /*array of all fonts*/
    short  ind,len;
    long memused=0L;

    if (!(lpFontSummary = lockFontSummary(lpDevice)))
        {
        /*if fontsummary can't be accessed then exit*/
        DBGErr(("ScanSofts: can't access fontsummary table\n"));
        }
    else {
        lpSummary=&lpFontSummary->f[0];
        len = lpFontSummary->len;

        /*traverse the list of fonts and check for any that are downloaded*/

        for (ind=0; ind < len; ++ind, ++lpSummary) {
            if ((lpSummary->memUsage != 0) && (lpSummary->indDLName==-1 ||
                lpSummary->LRUcount!=-1)) {
                DBMSG(("ScanSofts:MEM for fontind %d is %ld\n",ind,
                    lpSummary->memUsage));
                memused += lpSummary->memUsage;
                lpSummary->onPage = FALSE;
                lpDevice->epTotSoftNum++;
            }
        }
        unlockFontSummary(lpDevice);
    }
    return memused; 
    }

LOCAL void InitStartDoc(lpDevice)
    LPDEVICE lpDevice;
    {
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY   lpSummary;  /*array of all fonts*/
    short  ind,len;
    long memused=0L;

    if (!(lpFontSummary = lockFontSummary(lpDevice)))
        {
        /*if fontsummary can't be accessed then exit*/
        DBGErr(("ScanPermSofts: can't access fontsummary table\n"));
        }
    else {
        lpSummary=&lpFontSummary->f[0];
        len = lpFontSummary->len;
        /*traverse the list of fonts clear onPage flags*/
        for (ind=0; ind < len; ++ind, ++lpSummary) {
            lpSummary->onPage = FALSE;
            lpSummary->LRUcount = -1;
        }
        unlockFontSummary(lpDevice);
    }
    lpDevice->epPgSoftNum=0;
    lpDevice->epFontSub=FALSE;
    }

/*  LoadPFMStruct
 *
 *  Load a printer font metrics structure, like width, pair-kern, or
 *  track-kern tables.
 */
LOCAL BOOL LoadPFMStruct(lpDevice, lpFont, lpDest, kind)
    LPDEVICE lpDevice;
    LPFONTINFO lpFont;
    LPSTR lpDest;
    WORD kind;
    {
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY lpSummary;
    int success = 0;
    short fontInd;

    DBGtrace(("LoadPFMStruct(%lp.%lp.%lp.%d)\n",
        lpDevice, lpFont, lpDest, (WORD)kind));

    if (lpFontSummary = lockFontSummary(lpDevice))
        {
        fontInd = ((LPPRDFONTINFO)lpFont)->indFontSummary;

        if ((fontInd >= 0) && (fontInd < lpFontSummary->len))
            {
            lpSummary = &lpFontSummary->f[fontInd];

            success = loadStructFromFile(lpFontSummary,lpSummary,lpDest,kind);

            /*  If pair kern table, sort it.
             */
            if (kind == FNTLD_PAIRKERN)
                pksort((LPKERNPAIR)lpDest, success);
            }

        unlockFontSummary(lpDevice);
        }

    return (success);
    }
