/**[f******************************************************************
 * control.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * CONTROL.C
 *
 * 87-02-11	sec	"letter" -> a4,b5 for apple printers
 * 9Mar87	sjp	IBM printer support
 * 7Apr87	sjp	added DataProducts LZR 2665 (1st cut)
 * 8Apr87	sjp	added DECLN03R ScriptPrinter
 * 17Apr87	sjp	disabled SELECTPAPERSOURCE Escape,added PrintServer40
 *			paper selection.
 * 3Jun87	sjp	disabled ENABLERELATIVEWIDTHS escape.
 * 3Jun87	sjp	added GETSETPRINTERORIENTATION escape.
 * 4Jun87	sjp	added GETSETPAPERBINS escape, and escape numbers
 *			for above 2 escapes and ENABLEDUPLEX
 * 18Jun87	sjp	Added initialization of "spare" output data fields
 *			for GETSETPAPERBINS escape.
 * 20Jul87	sjp	Bug fix for Digital LPS40 (PRINTSERVER40).
 * 26Oct88	msd	added support for OLIVETTI_CHARSET
 *
 * note:
 *   escapes in the range 256 (AldusMin) to 767 (AldusPureOutMax) lpInData
 *   get modified to point to EXTTEXTDATA structure that has info
 *   on the currently selected font (and other stuff).  thus, the lpInData
 *   param that the app passes is ignored. (also note that some ranges
 *   of escapes only go to metafiles, and some only go to real devices).
 *   
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "driver.h"
#include "pserrors.h"
#include "psdata.h"
#include "utils.h"
#include "debug.h"
#include "channel.h"
#include "pss.h"
#include "resource.h"
#include "atprocs.h"
#include "atstuff.h"
#include "profile.h"
#include "etm.h"
#include "text.h"
#include "exttext.h"
#include "getdata.h"
#include "spooler.h"
#include "control2.h"
#include "escapes.h"
#include "mgesc.h"
#include "graph.h"

/* from mgesc.h */
int	FAR PASCAL MGControl(LPDV, int, LPSTR, LPSTR);

typedef struct {
	short ScaleMode;
	short dx, dy;
} DIBPARAMS;

typedef DIBPARAMS FAR *LPDIBPARAMS;

/*---------------------------- local data ----------------------------------*/

char clip_box[] = "%d %d 0 0 CB\n";

/*------------------------- local functions --------------------------------*/

void PASCAL MakeBinList(PPRINTER, LPSHORT);
void PASCAL CalcBreaks(LPJUSTBREAKREC, short, WORD);
BOOL PASCAL PaperRef(LPDV);
BOOL FAR PASCAL DumpPSSSafe(LPDV, short, short, short);


/* these functions are placed in a different segment to reduce the size of
 * _CNTRL (make it < 4k) */

BOOL FAR PASCAL OlivettiPrinter(LPDV);	/* in segment _STUBS */
BOOL FAR PASCAL InitDev(LPDV, LPSTR);	/* in segment _INITDEV */
void FAR PASCAL TermDev(LPDV);		/* in segment _INITDEV */

#pragma alloc_text(_INITDEV, InitDev, TermDev)
#pragma alloc_text(_STUBS, OlivettiPrinter)


#define ESCAPE_PORTRAIT		1
#define ESCAPE_LANDSCAPE	2



#if 0 /* DISABLED */

/* The paper metrics structure for the SELECTPAPERSOURCE escape */
typedef struct  {
	WORD x, y;
	RECT rcPage;
	WORD orientation;
} PM;
typedef PM FAR *LPPM;

#endif

/* this structure is for the SETALLJUSTVALUES escape...
 * added by Aldus Corporation--19Jan87--sjp
 */
typedef struct {
	short	nCharExtra;
	WORD	nCharCount;
	short	nBreakExtra;
	WORD	nBreakCount;
} ALLJUSTREC;
typedef ALLJUSTREC FAR *LPALLJUSTREC;



/* These next 2 typedefs are specially for
 * the GETSETPAPERBINS output data structures.
 * The 2nd one is only used for debug purposes
 * since the escape doesn't make assumptions on
 * the actual number of elements in the structure.
 */
typedef struct {
	short	binNum;
	short	numOfBins;
	short	spare1, spare2, spare3, spare4;
} GSPBOD;
typedef GSPBOD FAR *LPGSPBOD;



BOOL FAR PASCAL DumpPSSSafe(LPDV lpdv, short iPrinter, short dirID, short pssID)
{
	register BOOL ret;

	PrintChannel(lpdv, "{");
	ret = DumpPSS(lpdv, iPrinter, dirID, pssID);
	PrintChannel(lpdv, "}stopped pop\n");
	return ret;
}


/*********************************************************************
 * Name: PaperRef()
 *
 * Action: This routine is called to refresh the paper-
 * handling characteristics for each output page.
 *
 *********************************************************************/

BOOL PASCAL PaperRef(LPDV lpdv)
{
	PPRINTER pPrinter;

	DBMSG1((">PaperRef(): iP=%d\n", lpdv->iPrinter));

	if (!(pPrinter = GetPrinter(lpdv->iPrinter)))
		goto ERROR;
	 
	/* Is manual feed supported by this printer? ... if so then
	 * make sure that manual feed is turned off or on depending
	 * on the state of iPaperSource, i.e. if manual feed is selected. */

	if (pPrinter->feed[DMBIN_MANUAL - DMBIN_FIRST]) {
		DumpPSS(lpdv, lpdv->iPrinter, PSS_MANUALSWITCH, (lpdv->iPaperSource == DMBIN_MANUAL));
	}

#if 0
	/* this sets the paper source */
	/* right now we do this within the escape */

	DumpPSS(lpdv, lpdv->iPrinter, PSS_INPUTSLOT, lpdv->iPaperSource - DMBIN_FIRST);
#endif

	FreePrinter(pPrinter);

	DBMSG1(("<PaperRef()\n"));
	return TRUE;

ERROR:
	DBMSG1(("<PaperRef():  ERROR\n"));
	return FALSE;
}


/*********************************************************************
* Name: InitDev()
*
* Action: This routine is called to initialize the output
*	  device when the printer is enabled.
*
* this dumps the header
*
**********************************************************************/

BOOL FAR PASCAL InitDev(lpdv, lszJob)
LPDV lpdv;	    /* Far ptr to the device descriptor */
LPSTR lszJob;	    /* Far ptr to the print job name */
{
	int	iPrinter, i;
	int	paperID;
	PPRINTER pPrinter;
	char	userName[40];		/* used with AppleTalk */
	char	buf[80];
	short	resID;
	int	x0, y0, x1, y1;

	DBMSG1((">InitDev()\n"));

	ASSERT(lpdv);
	ASSERT(ghInst);

	if (!(pPrinter = GetPrinter(lpdv->iPrinter)))
		return FALSE;

	iPrinter = lpdv->iPrinter;

	/* Terminate any previous job before the new one,
	 * however, a few printers do not allow EOF! */

#ifdef APPLE_TALK
	if (pPrinter->fEOF && !lpdv->fDoEps && !ATState())
#else
	if (pPrinter->fEOF && !lpdv->fDoEps)
#endif
		WriteChannelChar(lpdv, EOF);

	/* Print minimum Adobe Header Comments */
	if (lpdv->fDoEps) {
		LoadString(ghInst, IDS_EPSHEAD, buf, sizeof(buf));
		PrintChannel(lpdv, buf);
		LoadString(ghInst, IDS_EPSBBOX, buf, sizeof(buf));

		/* translate bounding rect to EPS bounding rect in
		 * user coords (1/72 of an inch) and user space
		 * (0, 0 == lower left) */

		x0 = lpdv->EpsRect.left;
		y0 = lpdv->paper.cyPaper - lpdv->EpsRect.bottom;
		x1 = lpdv->EpsRect.right;
		y1 = lpdv->paper.cyPaper - lpdv->EpsRect.top;

		PrintChannel(lpdv, buf, 
			Scale(x0, 72, lpdv->iRes),
			Scale(y0, 72, lpdv->iRes),
			Scale(x1, 72, lpdv->iRes),
			Scale(y1, 72, lpdv->iRes));

	} else {
		LoadString(ghInst, IDS_PSHEAD, buf, sizeof(buf));
		PrintChannel(lpdv, buf);
	}

	LoadString(ghInst, IDS_PSTITLE, buf, sizeof(buf));
	PrintChannel(lpdv, buf, (LPSTR)lszJob);
	LoadString(ghInst, IDS_PSTIMEOUT, buf, sizeof(buf));
	PrintChannel(lpdv, buf, lpdv->iJobTimeout);

	if (lszJob) {

		LoadString(ghInst, IDS_PSJOB, buf, sizeof(buf));
		PrintChannel(lpdv, buf, (LPSTR)lszJob);

#ifdef APPLE_TALK
		if (ATState()) {
			LoadString(ghInst, IDS_PREPARE, userName, sizeof(userName));
			ATMessage(-1, 0, (LPSTR)userName);
		}
#endif
	}

	/*
	 * #ifndef ADOBE_11_1_88
	 * Make a decision here as to what type of header to download.
	 * There will be one of three choices:
	 *
	 * - if fHeader = TRUE download the PS header.
	 * - if fHeader = FALSE don't download the PS header just send code
	 *   to make sure its already there outside the exitserver loop.
	 * - if fEps is set download only the control portions of the header
	 *   setting the CTM and a clip rect to the imageable area.
	 */
	DBMSG1((" InitDev: fHeader=%d\n", lpdv->fHeader));
	DBMSG1((" InitDev: fEps=%d\n", lpdv->fEps));

	resID = (lpdv->fEps) ? PS_2 : (lpdv->fHeader) ? PS_HEADER : PS_1;

	DBMSG((" InitDev: resID=%d\n", resID));

	DumpResourceString(lpdv, PS_DATA, resID);

	/*
	 * #ifdef OLIVETTI_CHARSET
	 *
	 *	msd - 10/26/88:  If the printer is an Olivetti printer, then
	 *	download the PostScript stub which appends Olivetti's special
	 *	characters to the Windows ANSI character set.
	 */
	if (OlivettiPrinter(lpdv))
	    DumpResourceString(lpdv, PS_DATA, PS_OLIVCHSET);


	if (!lpdv->fDoEps) {

		/* the default is NOT manual feed */
		paperID = PSS_NORMALIMAGE;

#if 0
		/* if this printer has intrinsic "auto" capabilities and
	 	* "auto" is selected, then send the auto error-check routine
	 	* if available */

		if (pPrinter->feed[DMBIN_AUTO - DMBIN_FIRST] &&
	     	    lpdv->iPaperSource == DMBIN_AUTO)
		{
			DumpPSS(lpdv, iPrinter, PSS_WINDOWSAUTO, -1);
		}
#endif

		/* If this printer has "auto" capabilities (implicit or explicit)
	 	* then try to explicitly set the desired tray
	 	* N.B.:Manual will always give a null string and
	 	*	Auto will give a null string with "implicit" auto
	 	*	Auto will yield a PS string with "explicit" auto
	 	*/

		DumpPSSSafe(lpdv, iPrinter, PSS_INPUTSLOT, lpdv->iPaperSource - DMBIN_FIRST);
			    
		/* select the manual image PostScript for manual feed
	 	* or, if this printer has auto capabilities and auto is
	 	* not selected */

		if ((pPrinter->feed[DMBIN_MANUAL - DMBIN_FIRST] &&
	     	    (lpdv->iPaperSource == DMBIN_MANUAL))
// only do this for manual
//		    ||
//	     	    (pPrinter->feed[DMBIN_AUTO - DMBIN_FIRST]	 &&
//	     	    (lpdv->iPaperSource != DMBIN_AUTO))
		    )
		{
			paperID = PSS_MANUALIMAGE;
		}
		DBMSG1((" InitDev: tP.p [ (iP=%d)-(MIN=%d)=%d ] = %d pID=%d\n",
		    	lpdv->paper.iPaper, DMPAPER_FIRST, lpdv->paper.iPaper - DMPAPER_LAST,
		    	PaperSupported(pPrinter, lpdv->paper.iPaper), paperID));

		DBMSG((" InitDev: iPaper:%d\n", lpdv->paper.iPaper));


		/* only try to send a PostScript image/tray setup if
		 * the paper is available to the printer */

		if (PaperSupported(pPrinter, lpdv->paper.iPaper)) {

			/* search here to get the index of the PSS table entry
		 	* that matches the index of the Paper[] array for the
		 	* current printer */
		 
			for (i = 0; i < pPrinter->iNumPapers; i++)
				if (pPrinter->Paper[i].iPaperType == lpdv->paper.iPaper)
					break;

			DBMSG(("Paper index %d DMPAPER_ val %d\n", i, pPrinter->Paper[i].iPaperType));

			DumpPSSSafe(lpdv, iPrinter, paperID, i);
		}

		/* dump the PostScript transfer function */
		DumpPSS(lpdv, iPrinter, PSS_TRANSFER, -1);

		PrintChannel(lpdv, "settransfer\n");
	}

	PrintChannel(lpdv, "%d %d %d %d %d %d SM\n",
	    lpdv->fLandscape,
	    lpdv->paper.cxMargin,
	    lpdv->paper.cyMargin,
	    lpdv->paper.cxPaper,
	    lpdv->paper.cyPaper,
	    lpdv->iRes);

	DBMSG1(("<InitDev()\n"));

	FreePrinter(pPrinter);

	return TRUE;
}


/*********************************************************************
 * Name: TermDev()
 *
 * Action: This routine is called at the end of a print job.
 *
 *********************************************************************/

void FAR PASCAL TermDev(lpdv)
LPDV lpdv;
{
	PPRINTER pPrinter;

	DBMSG1((">TermDev()\n"));
	ASSERT(lpdv);

	PrintChannel(lpdv, "end\n");	/* match the Win33Dict begin */

	/* see if we should dump EOF */

	if (!(pPrinter = GetPrinter(lpdv->iPrinter)))
		return;

#ifdef APPLE_TALK
	if (pPrinter->fEOF && !lpdv->fDoEps && !ATState())
#else
	if (pPrinter->fEOF && !lpdv->fDoEps)
#endif
		WriteChannelChar(lpdv, EOF);	/* yes */

	CloseChannel(lpdv);
	KillAT();

	FreePrinter(pPrinter);

	DBMSG1(("<TermDev()\n"));

	return;
}


/*********************************************************************
 * CalcBreaks
 *
 * fill in the JUSTBREAKREC according to BreakExtra and Count
 *
 * note: BreakExtra can be negative.
 *
 * Aldus Corporation--19January87
 *********************************************************************/

void PASCAL CalcBreaks (lpJustBreak, BreakExtra, Count)
LPJUSTBREAKREC	lpJustBreak;
short		BreakExtra;
WORD		Count;
{
	DBMSG(("***CalcBreaks(): Count=%d,BreakExtra=%d\n",
	    Count, BreakExtra));

	if (Count > 0) {
		/* Fill in JustBreak values.  May be positive or negative.
		 */
		lpJustBreak->extra = BreakExtra / (short)Count;
		lpJustBreak->rem = BreakExtra % (short)Count;
		lpJustBreak->err = (short)Count / 2 + 1;
		lpJustBreak->count = Count;
		lpJustBreak->ccount = 0;

		DBMSG(((LPSTR)">0 e=%d,r=%d,e=%d,c=%d,cc=%d\n",
		    lpJustBreak->extra, lpJustBreak->rem, lpJustBreak->err,
		    lpJustBreak->count, lpJustBreak->ccount));

		/* Negative justification:  invert rem so the justification
		 * algorithm works properly.
		 */
		if (lpJustBreak->rem < 0) {
			--lpJustBreak->extra;
			lpJustBreak->rem += (short)Count;

			DBMSG(((LPSTR)"Neg.Just. e=%d,r=%d\n",
			    lpJustBreak->extra, lpJustBreak->rem));
		}
	} else {
		/* Count = zero, set up justification rec so the algorithm
		 * always returns zero adjustment.
		 */
		lpJustBreak->extra = 0;
		lpJustBreak->rem = 0;
		lpJustBreak->err = 1;
		lpJustBreak->count = 0;
		lpJustBreak->ccount = 0;

		DBMSG(((LPSTR)"=0 e=%d,r=%d,e=%d,c=%d,cc=%d\n",
		    lpJustBreak->extra, lpJustBreak->rem, lpJustBreak->err,
		    lpJustBreak->count, lpJustBreak->ccount));
	}
}



/*********************************************************************/

void PASCAL MakeBinList(pPrCaps, lpBinList)
PPRINTER pPrCaps;
LPSHORT   lpBinList;
{
	short	i;
	short	j = 0;

	DBMSG2((">MakeBinList()\n"));

	for (i = 0; i < NUMFEEDS; i++) {

		if (pPrCaps->feed[i]) {
			DBMSG2(((LPSTR)" MakeBinList(): i=%d, j=%d,feed=%d\n",
			    i, j, pPrCaps->feed[i]));

			/* add the bin number to the list */
			*(lpBinList + j) = i;
			DBMSG2((" MakeBinList(): bL[%d]=%d\n", j, *(lpBinList + j)));

			/* get ready for the next one */
			j++;
		}
	}
	DBMSG2(("<MakeBinList()\n"));
}


/*********************************************************************
 * Name: Control()
 *
 * Action: This routine is the low-level version of the
 *	  device Escape function.  It handles a hodgepodge
 *	  of somewhat unrelated functions.  Its main purpose
 *	  is to allow enhancements to the printer driver
 *	  functionality without altering the GDI interface.
 *
 * some of these escapes modify the current enviornment (defined by
 * the PSDEVMODE struct).  these changes are for the most part saved
 * permenently (in win.ini).  these changes are also updated in the
 * PDEVICE (DV) struct when necessary.  this routine and Enable() are
 * the only places where data is copied from PSDEVMODE to PDEVICE.
 *
 * note: many of these routines were added by many different people
 *	 (not all of them having a good understanding for this code).
 *	 much of this code should be reviewd for correctness and
 *	 efficiency.
 *
 *********************************************************************/

int FAR PASCAL Control(lpdv, ifn, lpbIn, lpbOut)
LPDV	lpdv;	    /* ptr to the device descriptor */
int	ifn;	    /* the escape function to execute */
LPSTR	lpbIn;	    /* ptr to input data for the escape function */
LPSTR	lpbOut;	    /* ptr to the output buffer for the esc function*/
{
	register int	iResult;
	short	i;

	DBMSG((">Control():  escFunc=%d\n", ifn));
	ASSERT(lpdv);

	if (ifn >= 4096 && ifn <= 4109)
		return MGControl(lpdv, ifn, lpbIn, lpbOut);

	switch (ifn) {

	case QUERYESCSUPPORT:
		switch (*(LPSHORT)lpbIn) {
		case NEXTBAND:
		case NEWFRAME:
		case ABORTDOC:
		case QUERYESCSUPPORT:
		case FLUSHOUTPUT:
		case STARTDOC:
		case ENDDOC:
		case GETPRINTINGOFFSET:
		case GETPHYSPAGESIZE:
		case SETCOPYCOUNT:
		case PASSTHROUGH:
		case GETEXTENDEDTEXTMETRICS:
		case GETEXTENTTABLE:
		case GETPAIRKERNTABLE:
		case SETKERNTRACK:
		case SETCHARSET:
		case ENABLEPAIRKERNING:
		case GETTECHNOLOGY:
		case SETLINECAP:
		case SETLINEJOIN:
		case SETMITERLIMIT:
		case GETSETPRINTERORIENTATION:
		case GETSETPAPERBINS:
		case ENUMPAPERBINS:
		case SETALLJUSTVALUES:	/* Aldus Corporation--19Jan87--sjp */
		case GETFACENAME:	/* added by ADOBE_11_1_88 */
		case DOWNLOADFACE:
		case EPSPRINTING:
		case ENUMPAPERMETRICS:
		case GETSETPAPERMETRICS:
		case GETVERSION:
		case SETDIBSCALING:

		case BEGIN_PATH:
		case CLIP_TO_PATH:
		case END_PATH:
		case EXT_DEVICE_CAPS:
		case SET_ARC_DIRECTION:
		case SET_POLY_MODE:
		case RESTORE_CTM:
		case SAVE_CTM:
		case TRANSFORM_CTM:
		case SET_CLIP_BOX:
		case SET_BOUNDS:
		case SET_SCREEN_ANGLE:
		case SETABORTPROC:

#ifdef PS_IGNORE
		case POSTSCRIPT_DATA:
		case POSTSCRIPT_IGNORE:
#endif

			DBMSG(("Query # %d supported\n", *(LPSHORT)lpbIn));
			return TRUE;
			break;

		default:
			DBMSG(("Query # %d not supported\n", *(LPSHORT)lpbIn));
			return FALSE;
			break;
		}
		break;


	case SETDIBSCALING:
		i = ((LPDIBPARAMS)lpbIn)->ScaleMode;

		if (i >= 0 && i <= 2) {

			DBMSG(("SETDIBSCALING mode:%d dx:%d dy:%d\n", 
				((LPDIBPARAMS)lpbIn)->ScaleMode,
				((LPDIBPARAMS)lpbIn)->dx,
				((LPDIBPARAMS)lpbIn)->dy));

			iResult = lpdv->ScaleMode;

			lpdv->ScaleMode = (char)i;
			lpdv->dx = ((LPDIBPARAMS)lpbIn)->dx;
			lpdv->dy = ((LPDIBPARAMS)lpbIn)->dy;

		} else
			iResult = -1;


		return iResult;

	/* this escape is required by many apps that call next band without
	 * first QUERYESCSUPPORTing it eventhough we are not a banding device */

	case NEXTBAND:

		SetRectEmpty((LPRECT)lpbOut);
		if (lpdv->fh >= 0) {
			if (lpdv->iBand == 0) {
				((LPRECT) lpbOut)->bottom = lpdv->paper.cyPaper;
				((LPRECT) lpbOut)->right = lpdv->paper.cxPaper;
				lpdv->iBand = 1;	/* we've done a band */
			} else {
				lpdv->iBand = 0;

				Control(lpdv, NEWFRAME, 0L, 0L);
			}
		}


		DBMSG(("NEXTBAND %d %d %d %d result:%d\n", 
			((LPRECT)lpbOut)->left, 
			((LPRECT)lpbOut)->top,
			((LPRECT)lpbOut)->right,
			((LPRECT)lpbOut)->bottom, iResult));

		return lpdv->fh;
		break;

	case NEWFRAME:
		/* invalidate pens and brushes and fonts */

		lpdv->fPenSelected = FALSE;
		lpdv->GraphData = GD_NONE;
		lpdv->DLState = DL_NOTHING;	/* reset download state */
		lpdv->lidFont = -1L;
		lpdv->nextSlot = -1;
		lpdv->FillMode = 0;	/* reset fill mode */
		lpdv->FillColor = -1L;

		// get all this stuff out before we EndSpoolPage()
		// so that we can begin printing.

		if (!lpdv->fDoEps) {

			/* # of copies */
			PrintChannel(lpdv, "%d #C\n", lpdv->iCopies);

			/* refresh paper source */
			PaperRef(lpdv);
		}

		/* EJ=eject, RS=RestoreState, SS=SaveState */

		PrintChannel(lpdv, "EJ RS SS\n");

		/* ADOBE_11_1_88 */

		if (lpdv->fEps) {
			PrintChannel(lpdv, clip_box,
			    lpdv->paper.cxPage,
			    lpdv->paper.cyPage);
		}

		lpdv->fDirty = FALSE;		/* current page is empty */

		if (lpdv->fh >= 0) {

			/* let the spooler start printing */
			if ((iResult = EndSpoolPage(lpdv->fh)) < 0)
				return iResult;

			if ((iResult = StartSpoolPage(lpdv->fh)) < 0)
				return iResult;
		}

		return lpdv->fh;	// this will < 0 on SP_ error condition
		break;

	case ABORTDOC:
		TermDev(lpdv);
		break;

	case GETEXTENTTABLE:
		return(GetExtTable(lpdv, (LPEXTTEXTDATA)lpbIn, (LPSHORT)lpbOut));
		break;

/* shut this escape off for now...
 *	case ENABLERELATIVEWIDTHS:
 *		if (lpbIn)
 *			lpdv->fIntWidths = !*lpbIn;
 *		break;
 */
	case GETEXTENDEDTEXTMETRICS:
		return(GetEtm(lpdv, (LPEXTTEXTDATA)lpbIn, lpbOut));
		break;

	/* ADOBE_11_1_88 */
	case GETFACENAME:
		return(GetPSName(lpdv, (LPEXTTEXTDATA)lpbIn, lpbOut));
		break;

	case DOWNLOADFACE:
		if ((*((short far * )lpbOut) == 1) && (lpdv->fh <= 0))
			return(-1);
		else
			return(PutFontInline(lpdv, (LPEXTTEXTDATA)lpbIn, *((short far * )lpbOut)));
		break;

	/* this escape makes the driver output a minimal header.  this
	 * is intended for apps that produce their own postscript */

	case EPSPRINTING:
		/* check to see if we need to toggle */
		if (iResult = lpdv->fEps != *(LPSHORT)lpbIn)
			lpdv->fEps = *(LPSHORT)lpbIn;

		return iResult ? 1 : -1;
		break;

	case ENUMPAPERMETRICS:
		{
		short	rc;

		DBMSG(("ENUMPAPERMETRICS in:%lx out:%lx\n", lpbIn, lpbOut));

		rc = EnumPaperMetrics(lpdv, (LPRECT)lpbOut, *((short far * )lpbIn), 0);
		if (rc == -1)
			rc = EnumPaperMetrics(lpdv, (LPRECT)lpbOut, *((short far * )lpbIn), 1);
		DBMSG(("    return %d\n", rc));
		return rc;
		}
		break;

	case GETSETPAPERMETRICS:
		{
		short	rc;
		PSDEVMODE originalDM;
		PSDEVMODE newDM;

		DBMSG(("GETSETPAPERMETRICS in:%lx out:%lx\n", lpbIn, lpbOut));

		if (lpbOut) {
			SetRect((LPRECT)lpbOut,
				lpdv->paper.cxMargin,
				lpdv->paper.cyMargin,
				lpdv->paper.cxMargin + lpdv->paper.cxPage,
				lpdv->paper.cyMargin + lpdv->paper.cyPage);

			DBMSG(("    GET rc: %d %d %d %d\n", 
				((LPRECT)lpbOut)->left, ((LPRECT)lpbOut)->top, ((LPRECT)lpbOut)->right, ((LPRECT)lpbOut)->bottom));

			rc = 1;
		}

		if (lpbIn) {

			/* get the current env */

			MakeEnvironment(lpdv->szDevType,
			    lpdv->szFile, &originalDM, NULL);

			newDM = originalDM;	/* make new copy */

			DBMSG(("    SET rc: %d %d %d %d\n", ((LPRECT)lpbIn)->left, ((LPRECT)lpbIn)->top, ((LPRECT)lpbIn)->right, ((LPRECT)lpbIn)->bottom));

			rc = SetPaperMetrics(lpdv, &newDM, (LPRECT)lpbIn);

			if (rc != -1) {

				// save changes to newDM and lpdv in win.ini

				SaveEnvironment(lpdv->szDevType,
				    	lpdv->szFile, &newDM, 
				    	&originalDM, NULL, TRUE, TRUE);
			}
		}

		DBMSG(("    return %d\n", rc));

		return rc;
		}
		break;

	/* Adobe added this.  this probally isn't that good of an idea.
	 * if apps querey a driver for a version # they usualy are going
	 * to assume something.  be careful here */

	case GETVERSION:		/* from Adobe 3.1+ */
		return 100;


	case GETPAIRKERNTABLE:
		return(GetPairKern(lpdv, (LPEXTTEXTDATA)lpbIn, lpbOut));
		break;

	case FLUSHOUTPUT:
		if (lpdv->fh >= 0)
			FlushChannel(lpdv);
		break;

	case STARTDOC:
		DBMSG2((" Control(): STARTDOC\n"));

		iResult = SP_ERROR;	/* general error -1 */

		/* Do an explicit ENDDOC if the user forgot to do it */
		if (lpdv->fh >= 0) 
			Control(lpdv, ENDDOC, 0L, 0L);
#ifdef APPLE_TALK
		ATQuery(lpdv->szFile);

		if (lpdv->fDoEps) {

			ATChangeState(FALSE);	/* turn AppleTalk off if
			 			 * doing EPS output */
		}

		if (iResult = TryAT()) {
			if (iResult > 0) 
				iResult = SP_USERABORT;
			goto END_STARTDOC;
		}
#endif

		/* Open the output channel: note lpbIn is the document name */
		if ((iResult = OpenChannel(lpdv, lpbIn)) < 0) 
			goto END_STARTDOC;

		/* Initialize the device */
		if (!InitDev(lpdv, lpbIn)) {
			iResult = SP_ERROR;
			goto END_STARTDOC;
		}

		/* 87-1-13 sec: save at the beginning of the 1st page
		 * SS=SaveState */

		PrintChannel(lpdv, "SS\n");

		/* ADOBE_11_1_88 */
		if (lpdv->fEps) {
			PrintChannel(lpdv, clip_box, lpdv->paper.cxPage, lpdv->paper.cyPage);
		}

		/* clear the temporary softfonts array */
		lpdv->nextSlot = -1;


		/* everything is A.O.K. */
		iResult = 1;

END_STARTDOC:
		DBMSG2((" Control(): STARTDOC exit\n"));
		return(iResult);
		break;

	case ENDDOC:

		if (lpdv->fDirty) 
			Control(lpdv, NEWFRAME, 0L, 0L);

		/* RS=RestoreState */
		PrintChannel(lpdv, "RS\n");
		TermDev(lpdv);
		break;

	case SETABORTPROC:
		lpdv->hdc = *(HDC FAR * )lpbIn;
		break;

	case GETPHYSPAGESIZE:
		((LPPOINT)lpbOut)->xcoord = lpdv->paper.cxPaper;
		((LPPOINT)lpbOut)->ycoord = lpdv->paper.cyPaper;

		DBMSG(("GETPHYSPAGESIZE %d %d\n", lpdv->paper.cxPaper,
						 lpdv->paper.cyPaper));
		break;

	case GETPRINTINGOFFSET:
		((LPPOINT)lpbOut)->xcoord = lpdv->paper.cxMargin;
		((LPPOINT)lpbOut)->ycoord = lpdv->paper.cyMargin;
		DBMSG(("GETPRINTINGOFFSET %d %d size %d %d\n",
			lpdv->paper.cxMargin, lpdv->paper.cyMargin,
			lpdv->paper.cxPage, lpdv->paper.cyPage));
		break;

	case GETSCALINGFACTOR:
		((LPPOINT)lpbOut)->xcoord = ((LPPOINT)lpbOut)->ycoord = 0;
		break;

	case SETCOPYCOUNT:
		i = lpdv->iCopies;

		if (lpbIn)
			lpdv->iCopies = *((LPSHORT)lpbIn);

		if (lpbOut)
			*((LPSHORT)lpbOut) = i;

		iResult = TRUE;
		break;

#if 0	/* DISABLED */
	case SELECTPAPERSOURCE:
		DBMSG1(((LPSTR)"***SELECTPAPERSOURCE\n"));
		if (!SetPaperSource(lpdv, *((short far * )lpbIn))) 
			return(-1);

		if (lpbOut) {
			((LPPM)lpbOut)->x = lpdv->paper.cxPaper;
			((LPPM)lpbOut)->y = lpdv->paper.cyPaper;

#if 0
			/* big lie: */
			((LPPM)lpbOut)->rcPage.top = 0;
			((LPPM)lpbOut)->rcPage.left = 0;
			((LPPM)lpbOut)->rcPage.bottom = lpdv->paper.cyPage;
#endif
			((LPPM)lpbOut)->rcPage.top = lpdv->paper.cyMargin;
			((LPPM)lpbOut)->rcPage.left = lpdv->paper.cxMargin;
			((LPPM)lpbOut)->rcPage.bottom = lpdv->paper.cyMargin + 
			    lpdv->paper.cyPage;

			((LPPM)lpbOut)->orientation = lpdv->fLandscape ? 2 : 1;
		}
		DBMSG1(("SELECTPAPERSOURCE***\n"));
		break;
#endif

#ifdef PS_IGNORE
	case POSTSCRIPT_IGNORE:

		i = lpdv->fSupressOutput;
		lpdv->fSupressOutput = (BOOL)*lpbIn;

		return i;	// return previous value
		break;

	case POSTSCRIPT_DATA:
#endif
	case PASSTHROUGH:
		{
		unsigned int cb;

		if (lpbIn) {
			cb = *((unsigned int far * )lpbIn)++;

			WriteChannel(lpdv, lpbIn, cb);

			if (lpdv->fh >= 0) 
				return cb;
			else 
				return lpdv->fh;
		}
		return lpdv->fh;
		break;
		}

	case SETKERNTRACK:
		i = lpdv->iTrack;
		lpbIn = (LPSTR) ((LPETD)lpbIn)->lpInData;
		if (lpbIn) 
			lpdv->iTrack = *((short far * )lpbIn);
		if (lpbOut) 
			*((short far * )lpbOut) = i;
		break;

	case SETCHARSET:

		// this is a bogus undocumented escape that is used
		// by various unnamed apps to "switch" between the standard
		// ANSI char set and a special publishing char set.
		// if *lpbIn == 1 we use the publishing set
		//	char 145 single open quote
		//	char 146 single close quote
		//	char 147 double open quote
		//	char 148 double close quote
		//	char 149 bullet
		//	char 150 En dash
		//	char 151 Em dash
		//	char 160 non breaking space
		// if *lpbIn == 0 we use standard ANSI.
		//
		// the way this works is these chars (in the publishing set)
		// are always their.  so this just serves as a means
		// to inform the app that we support this stuff

		DBMSG(("SETCHARSET %d\n", *((WORD FAR *)lpbIn)));

		// instead of actually checking for valid parameters
		// we assume the caller knows what he is doing (this is
		// undocumented anyway) and just say, hey! cool man, go
		// for it.

#if 0
		// valid params?

		if (*((WORD FAR *)lpbIn) < 2) 
			return TRUE;
		else 
			return FALSE;
#else
		return TRUE;	// what I said above
#endif
		break;

		/* added by Aldus Corporation--19Jan87--sjp */
	case SETALLJUSTVALUES:
		{
		LPEXTTEXTDATA lpExtText = (LPEXTTEXTDATA)lpbIn;
		LPALLJUSTREC lpAllJust = (LPALLJUSTREC)lpExtText->lpInData;
		LPDRAWMODE lpDrawMode = lpExtText->lpDrawMode;
		LPFONTINFO lpFont = lpExtText->lpFont;

		DBMSG(((LPSTR)"SETALLJUSTVALUES\n"));
		DBMSG(((LPSTR)"TBreakExtra=%d\n", lpDrawMode->TBreakExtra));
		DBMSG(((LPSTR)"BreakExtra=%d\n", lpDrawMode->BreakExtra));
		DBMSG(((LPSTR)"BreakErr=%d\n", lpDrawMode->BreakErr));
		DBMSG(((LPSTR)"BreakRem=%d\n", lpDrawMode->BreakRem));
		DBMSG(((LPSTR)"BreakCount=%d\n", lpDrawMode->BreakCount));
		DBMSG(((LPSTR)"CharExtra=%d\n", lpDrawMode->CharExtra));

		lpDrawMode->BreakErr = 1;
		lpDrawMode->TBreakExtra = 0;
		lpDrawMode->BreakExtra = 0;
		lpDrawMode->BreakRem = 0;
		lpDrawMode->BreakCount = 0;
		lpDrawMode->CharExtra = 0;

		DBMSG(((LPSTR)"TBreakExtra=%d\n", lpDrawMode->TBreakExtra));
		DBMSG(((LPSTR)"BreakExtra=%d\n",  lpDrawMode->BreakExtra));
		DBMSG(((LPSTR)"BreakErr=%d\n",    lpDrawMode->BreakErr));
		DBMSG(((LPSTR)"BreakRem=%d\n",    lpDrawMode->BreakRem));
		DBMSG(((LPSTR)"BreakCount=%d\n",  lpDrawMode->BreakCount));
		DBMSG(((LPSTR)"CharExtra=%d\n",   lpDrawMode->CharExtra));

		if (lpFont->dfCharSet == OEM_CHARSET) {
		 /*	Vector font: disable ALLJUSTVALUES and
		  *	return false.
		  */
			lpdv->epJust = fromdrawmode;
			DBMSG(((LPSTR)" Control(): vector font disable and return\n"));
			return(0);
		}

		if (lpbIn) {

			CalcBreaks (&lpdv->epJustWB, lpAllJust->nBreakExtra,
			    lpAllJust->nBreakCount);
			CalcBreaks (&lpdv->epJustLTR, lpAllJust->nCharExtra,
			    lpAllJust->nCharCount);

			if (lpdv->epJustWB.extra || lpdv->epJustWB.rem || 
			    lpdv->epJustLTR.extra || lpdv->epJustLTR.rem
			    ) {
				if (lpdv->epJustLTR.rem) {
					lpdv->epJust = justifyletters;
				} else {
					lpdv->epJust = justifywordbreaks;
				}
			} else {
				/* Zero justification == shut off ALLJUSTVALUES */
				lpdv->epJust = fromdrawmode;
				DBMSG(((LPSTR)"zero just.\n"));
			}
		}

		}
		break;

	case ENABLEPAIRKERNING:
		i = lpdv->fPairKern;
		lpbIn = (LPSTR) ((LPETD)lpbIn)->lpInData;
		if (lpbIn) 
			lpdv->fPairKern = *((short far * )lpbIn);
		if (lpbOut) 
			*((short far * )lpbOut) = i;
		break;

	case GETTECHNOLOGY:

		lstrcpy(lpbOut, "PostScript");

#if 0

		/* is this a valid binary port? */
		if (lpdv->fBinary) {
			lstrcat(lpbOut, "binary");
		}
#endif

		/* always a double NULL at end of string */
		lpbOut[lstrlen(lpbOut)+1] = '\0';
		break;

	case SETLINECAP:
		i = lpdv->iNewLineCap;
		if (lpbIn)
			lpdv->iNewLineCap = *((short far * ) lpbIn);
		if (lpbOut)
			*((short far * ) lpbOut) = i;
		break;

	case SETLINEJOIN:
		i = lpdv->iNewLineJoin;
		if (lpbIn)
			lpdv->iNewLineJoin = *((short far * ) lpbIn);
		if (lpbOut)
			*((short far * ) lpbOut) = i;
		break;

	case SETMITERLIMIT:
		i = lpdv->iNewMiterLimit;
		if (lpbIn)
			lpdv->iNewMiterLimit = *((short far * ) lpbIn);
		if (lpbOut)
			*((short far * ) lpbOut) = i;
		break;

	case GETSETPRINTERORIENTATION:
		 {
		/* lpInData is set up in the following way...
		 * struct { short orientation,spare1,spare2,spare3; } lpInData;
		 */
		PSDEVMODE oldDM;
		short	orientation;
		short	rc = ESCAPE_PORTRAIT;

		DBMSG2(((LPSTR)" Control(): GETSETPRINTERORIENTATION rc=%d\n", rc));

		/* always return the present value of the orientation...
		 * unless there is an error
		 */
		if (lpdv->fLandscape)
			rc = ESCAPE_LANDSCAPE;
		DBMSG2(((LPSTR)" Control(): rc=%d\n", rc));

		/* if NULL then return the current orientation
		 * (previously set)...if NOT NULL process...
		 */
		if (lpbIn) {
			orientation = *((short far * )lpbIn);

			DBMSG2(((LPSTR)" Control(): CHANGE o=%d\n", orientation));

			/* check to make sure the orientation is withing the
			 * available bounds
			 */
			if (orientation <= ESCAPE_LANDSCAPE && orientation >= ESCAPE_PORTRAIT) {

				/* make the environment...doesn't fail */
				MakeEnvironment(lpdv->szDevType,
				    lpdv->szFile, &oldDM, NULL);

				if (orientation == ESCAPE_LANDSCAPE) 
					oldDM.dm.dmOrientation = DMORIENT_LANDSCAPE;
				else 
					oldDM.dm.dmOrientation = DMORIENT_PORTRAIT;

				/* Update the environment with the new info and inform
			 	 * everyone.
				 */
				SaveEnvironment(lpdv->szDevType,
					lpdv->szFile, &oldDM,
					NULL, NULL, FALSE, TRUE);
			} else {
				rc = -1; /* invalid selection */
				DBMSG2(((LPSTR)" Control(): ERROR rc=%d\n", rc));
			}
		}
		DBMSG2(("<Control(): GETSETPRINTERORIENTATION exit rc=%d\n",
		    rc));
		return rc;
		}
		break;

	/* 88Jan10 chrisg added support for change of paper bin while
	 * printing */

	case GETSETPAPERBINS:
		{
		PSDEVMODE oldDM;	/* used to get/save env */
		PSDEVMODE newDM;	/* used to get/save env */
		PPRINTER pPrinter;      /* temp printer struct */
		short	bin[NUMFEEDS];	/* bin array */
		short	num, i;
		BOOL	fSave;

		DBMSG((" Control():>GETSETPAPERBINS\n"));


		/* get the bin data for this printer */

		if (!(pPrinter = GetPrinter(lpdv->iPrinter)))
			return FALSE;

		MakeBinList(pPrinter, bin);

		if (lpbOut) {	/* GET */

			/* calc the number of bins used by this printer */
			num = 0;
			for (i = 0; i < NUMFEEDS; i++)
				if (pPrinter->feed[i]) 
					num++;

			((LPGSPBOD)lpbOut)->binNum = lpdv->iPaperSource;
			((LPGSPBOD)lpbOut)->numOfBins = num;

			DBMSG((" Control():>GETSETPAPERBINS GET bin num:%d  num bins:%d\n",
				((LPGSPBOD)lpbOut)->binNum,
				((LPGSPBOD)lpbOut)->numOfBins));
		}


		if (lpbIn) {	/* SET */

			num = *(LPSHORT)lpbIn;	/* bin number */


			fSave = !(num & 0x8000);/* high bit means temp change */

			if (!fSave) {
				num &= 0x7FFF;	/* keep low bits */
			}
			DBMSG((" Control():>GETSETPAPERBINS SET bin = %d Save = %d\n", num, fSave));

			/* make sure bin # exists before setting */

			for (i = 0; i < NUMFEEDS; i++) {
				if (num == bin[i]) {
					lpdv->iPaperSource = num + DMBIN_FIRST;
					break;
				}
			}

			MakeEnvironment(lpdv->szDevType, lpdv->szFile, &oldDM, NULL);
			newDM = oldDM;
			newDM.dm.dmDefaultSource = lpdv->iPaperSource;

			DBMSG((" GETSETPAPERBINS dmDefaultSource %d\n", newDM.dm.dmDefaultSource));

			SaveEnvironment(lpdv->szDevType, lpdv->szFile, &newDM,
				&oldDM, NULL, fSave, fSave);

			DumpPSSSafe(lpdv, lpdv->iPrinter, PSS_INPUTSLOT, 
				lpdv->iPaperSource - DMBIN_FIRST);
		}

		FreePrinter(pPrinter);

		}
		return TRUE;
		break;

	case ENUMPAPERBINS:
		{
		short	binList[NUMFEEDS];
		short	numOfBins;
		short	offset;/* for indexing into the array of bins names */
		short	i;
		short	numOfElements;
		LPSHORT lpBinList;
		char	paperBin[BINSTRLEN];
		PPRINTER pPrinter;

		DBMSG((" Control():>ENUMPAPERBINS\n"));

		if (!lpbIn || !lpbOut)
			return FALSE;

		if (!(pPrinter = GetPrinter(lpdv->iPrinter)))
			return -1;

		/* generate the list of bins supported by this printer */
		MakeBinList(pPrinter, binList);

		/* calculate the number of bins used by this printer */
		numOfBins = 0;
		for (i = 0; i < NUMFEEDS; i++) {
			if (pPrinter->feed[i]) 
				numOfBins++;
		}

		/* get the number of elements in the ouput data structure */

		numOfElements = *(LPSHORT)lpbIn;
		numOfBins = min(numOfBins, numOfElements);

		DBMSG((" Control(): ENUMPAPERBINS numOfElements=%d\n", numOfElements));

		/* get 1st array element */
		lpBinList = (LPSHORT)lpbOut;

		/* offset to 1st paper name position */
		offset = numOfElements  * sizeof(short);

		for (i = 0; i < numOfBins; i++) {

			/* add the bin number to the list */
			lpBinList[i] = binList[i];

			/* add the bin string to the list */
			LoadString(ghInst, binList[i] + DMBIN_FIRST + DMBIN_BASE, paperBin, BINSTRLEN);
			lstrcpy(lpbOut + offset, paperBin);

			DBMSG((" Control(): lpBinList[%d]=%d", i, lpBinList[i]));
			DBMSG((" lpbOut + %d=%ls\n", offset, lpbOut + offset));

			offset += BINSTRLEN;
		}

		FreePrinter(pPrinter);

		DBMSG((" Control():<ENUMPAPERBINS ret TRUE\n"));
		return TRUE;

		}
		break;

	default:
		return FALSE;
		break;
	}

	DBMSG2(("<Control(): Normal exit\n"));

	return TRUE;
}



/*
 * #ifdef OLIVETTI_CHARSET
 */

/*
 * OlivettiPrinter()
 *
 * msd - 10/26/88:  
 * special case hack.  check to see if this is an Olivetti LP 5000
 * to support it's fucked up charaterset.  note, Olivetti does not
 * support this printer anymore.
 */

BOOL FAR PASCAL OlivettiPrinter(lpdv)
LPDV lpdv;
{
	PPRINTER pPrinter;
	char buf[60];
	BOOL res = FALSE;

	if (pPrinter = GetPrinter(lpdv->iPrinter)) {

		LoadString(ghInst, IDS_OLIV, buf, sizeof(buf));

		res = !lstrcmpi(pPrinter->Name, buf);

		FreePrinter(pPrinter);
	}

	return res;
}

