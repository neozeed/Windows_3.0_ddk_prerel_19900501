/**[f******************************************************************
 * control2.c
 *
 *
 **f]*****************************************************************/

/*********************************************************************
 * 8Jan89	chrisg	created (moved from enable.c)
 *
 *
 *
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "driver.h"
#include "utils.h"
#include "debug.h"
#include "resource.h"
#include "defaults.h"
#include "profile.h"
#include "getdata.h"
#include "psdata.h"
#include "control2.h"
#include "dmrc.h"

/*--------------------------- local functions -------------------------*/

short PASCAL CompareImageableAreas(LPRECT, LPRECT, LPBOOL);
short PASCAL SearchPaperMetrics(PPAPER, LPRECT, LPSHORT, LPBOOL, short, LPPSDEVMODE);

#define ERROR     (-2)
#define NOTFND    (-1)


/*--------------------------------------------------------------------------
 * This routine is called by the ENUMPAPERMETRICS Escape. It is designed to 
 * return an array of RECTS whose values representing the imageable areas of
 * the supported paper types.
 * Returns: a positive value if successful or -1 otherwise.
 *
 * if mode == 0 then we stuff the # of rects we have into lprcMetrics (short)
 * if mode == 1 then we fill lprcMetrics with all the rects that we support
 *
 * Digby Horner - 9/15/88 Adobe Systems Inc.                                
 *
 *------------------------------------------------------------------------*/

short FAR PASCAL EnumPaperMetrics(lpdv, lprcMetrics, mode, fTypesetter)
LPDV	lpdv;
LPRECT	lprcMetrics;	// return stuff here
short	mode;		// modes: 0=INFORM 1=PERFORM */
short	fTypesetter;	// if TRUE support even negative top left (?)
{
	#define PERFORM 1
	PPRINTER pPrinter;
	PPAPER pPaper;    /* ptr to the array of paper metrics */
	short j, top, left, result = NOTFND;
	short rect_count;
	int iPaper;

	ASSERT(lpdv);
	ASSERT(lprcMetrics);


	pPrinter = GetPrinter(lpdv->iPrinter);


	if (!pPrinter) {
		result = ERROR;
		goto END_NOFREE;
	}

	pPaper = GetPaperMetrics(pPrinter);

	if (!pPaper) {
		result = ERROR;
		goto END;
	}

	DBMSG(("EnumPaperMetrics() %ls num_papers %d\n", 
		mode == PERFORM ? (LPSTR)"PERFORM" : (LPSTR)"INFORM",
		pPrinter->iNumPapers
	));
#if 0
	if (mode == PERFORM)
		rect_count =  0;	// we are going to copy out
	else
		rect_count =  1;	// we are just going to count
#else
	rect_count =  0;	// none found yet
#endif

	for (iPaper = 0; iPaper < pPrinter->iNumPapers; iPaper++) {

		DBMSG(("iPaper %d\n", iPaper));

		top = Scale(pPaper[iPaper].cyMargin, lpdv->iRes, DEFAULTRES);
		left = Scale(pPaper[iPaper].cxMargin, lpdv->iRes, DEFAULTRES);

		if ((top > 0 || left > 0) || (fTypesetter == 1)) {
			if (mode == PERFORM) {

				/* imageable area with default margins */

				lprcMetrics[rect_count].top = top;
				lprcMetrics[rect_count].left = left;
				lprcMetrics[rect_count].right = 
				    Scale(pPaper[iPaper].cxPage + pPaper[iPaper].cxMargin, lpdv->iRes, DEFAULTRES);
				lprcMetrics[rect_count].bottom = 
				    Scale(pPaper[iPaper].cyPage + pPaper[iPaper].cyMargin, lpdv->iRes, DEFAULTRES);

				DBMSG(("  %d %d %d %d\n", 
					lprcMetrics[rect_count].left, lprcMetrics[rect_count].top,  
					lprcMetrics[rect_count].right, lprcMetrics[rect_count].bottom
				));

				rect_count++;

				/* imageable area with no margins */

				lprcMetrics[rect_count].top = lprcMetrics[rect_count].left = 0;
				lprcMetrics[rect_count].right = Scale(pPaper[iPaper].cxPaper, lpdv->iRes, DEFAULTRES);
				lprcMetrics[rect_count].bottom = Scale(pPaper[iPaper].cyPaper, lpdv->iRes, DEFAULTRES);

				DBMSG(("  %d %d %d %d\n", 
					lprcMetrics[rect_count].left, lprcMetrics[rect_count].top,  
					lprcMetrics[rect_count].right, lprcMetrics[rect_count].bottom
				));

				rect_count++;
			} else
				rect_count += 2;
			result = 1;
		}
	}
	if (mode == PERFORM) {
#if 0
		/* imageable area with tiled margins */
		lprcMetrics[rect_count].top = lprcMetrics[rect_count].left = -6825;
		lprcMetrics[rect_count].right = lprcMetrics[rect_count].bottom = 13650;
#endif
	} else
		*((short far * )lprcMetrics) = rect_count;	// return # of rects we have

	DBMSG(("  rect_count %d\n", rect_count));

	LocalFree((HANDLE)pPaper);
END:
	FreePrinter(pPrinter);
END_NOFREE:
	return result;
}

/*--------------------------------------------------------------------------
   Compares two RECTS representing imageable areas - returning TRUE if a
   match occurs FALSE otherwise. Sets.dm.dmOrientation == DMORIENT_LANDSCAPE according to r2's
   orientation. 

   assumptions: 
     - the rect r1 will always be in portrait mode 
     - r2 could be in either portrait or landscape orientation
 *--------------------------------------------------------------------------*/

short PASCAL CompareImageableAreas(r1, r2, lpfLandscape)
LPRECT r1, r2;
LPBOOL lpfLandscape;
{
	if (r1->top == r2->top && r1->left == r2->left
	     && r1->right == r2->right && r1->bottom == r2->bottom) {
		*lpfLandscape = FALSE;
		return TRUE;
	}
	if (r1->top == r2->left && r1->left == r2->top
	     && r1->right == r2->bottom && r1->bottom == r2->right) {
		*lpfLandscape = TRUE;
		return TRUE;
	}
	return FALSE;
}


/*--------------------------------------------------------------------------*/
short PASCAL SearchPaperMetrics(pPaper, lpRect, margins, lpfLandscape, res, lpdm)
PPAPER  pPaper;
LPRECT	lpRect;
LPSHORT margins;
LPBOOL  lpfLandscape;
short	res;
LPPSDEVMODE lpdm;
{
	short	j;
	RECT	r;

	for (j = 0; pPaper[j].iPaper; j++) {

		r.top = Scale(pPaper[j].cyMargin, res, DEFAULTRES);
		r.left = Scale(pPaper[j].cxMargin, res, DEFAULTRES);
		if (r.top || r.left) {
			r.right = Scale(pPaper[j].cxPage + pPaper[j].cxMargin, res, DEFAULTRES);
			r.bottom = Scale(pPaper[j].cyPage + pPaper[j].cyMargin, res, DEFAULTRES);
			if (CompareImageableAreas(lpRect, &r, lpfLandscape)) {
				*margins = DEFAULT_MARGINS;
				return (j);
			}
			r.top = r.left = 0;
			r.right = Scale(pPaper[j].cxPaper, res, DEFAULTRES);
			r.bottom = Scale(pPaper[j].cyPaper, res, DEFAULTRES);
			if (CompareImageableAreas(lpRect, &r, lpfLandscape)) {
				*margins = ZERO_MARGINS;
				return (j);
			}
		}
	}

#if 0
	/* tiling has been removed from the driver */

	/* check for tiled margin state */

	if (lpRect->left == -6825 && lpRect->top == -6825
	     && lpRect->right == 6825 && lpRect->bottom == 6825) {

		*margins = TILE_MARGINS;
		*lpfLandscape = lpdm->dm.dmOrientation == DMORIENT_LANDSCAPE;
		return lpdm->dm.dmPaperSize;
	}
#endif
	return NOTFND;
}


/*--------------------------------------------------------------------------
 * service routine for the GETSETPAPERMETRICS escape - sets the current paper
 * type and imageable area taking into account margin state and orientation.
 *
 * this updates *lpdm and *lpdv with new paper metrics
 *
 *-------------------------------------------------------------------------*/

short FAR PASCAL SetPaperMetrics(lpdv, lpdm, lpbIn)
LPDV lpdv;
LPPSDEVMODE lpdm;
LPRECT lpbIn;
{
	PPRINTER pPrinter;
	PPAPER	pPaper;	/* ptr to the array of paper metrics */
	short	j, margins, result = ERROR;
	BOOL	fLandscape;

	ASSERT(lpdv);
	ASSERT(lpbIn);


	pPrinter = GetPrinter(lpdm->iPrinter);

	if (!pPrinter) {
		result = ERROR;
		goto END_NOFREE_PRINTER;
	}

	pPaper = GetPaperMetrics(pPrinter);

	if (!pPaper) {
		result = ERROR;
		goto END_NOFREE_PAPER;
	}

	j = SearchPaperMetrics(pPaper, lpbIn, &margins, &fLandscape, lpdv->iRes, lpdm);

	if (j == NOTFND) {
		result = NOTFND;
		goto END;
	}

	lpdv->marginState = lpdm->marginState = margins;
	lpdv->paper.iPaper = lpdm->dm.dmPaperSize = pPaper[j].iPaper;
	lpdm->rgiPaper[pPrinter->defFeed] = pPaper[j].iPaper;
	lpdv->fLandscape = lpdm->dm.dmOrientation == DMORIENT_LANDSCAPE;

	lpdv->paper.cxPage = fLandscape ? 
	    Scale(pPaper[j].cyPage, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cxPage, lpdv->iRes, DEFAULTRES);

	lpdv->paper.cyPage = fLandscape ? 
	    Scale(pPaper[j].cxPage, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cyPage, lpdv->iRes, DEFAULTRES);

	switch (lpdv->marginState) {
	case ZERO_MARGINS:
		lpdv->paper.cxMargin = 0;
		lpdv->paper.cyMargin = 0;
		lpdv->paper.cxPaper = fLandscape ?
		    Scale(pPaper[j].cyPaper, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cxPaper, lpdv->iRes,
		     DEFAULTRES);
		lpdv->paper.cyPaper = fLandscape ?
		    Scale(pPaper[j].cxPaper, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cyPaper, lpdv->iRes,
		     DEFAULTRES);
		break;
	case DEFAULT_MARGINS:
		lpdv->paper.cxMargin = fLandscape ?
		    Scale(pPaper[j].cyMargin, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cxMargin, lpdv->iRes,
		     DEFAULTRES);
		lpdv->paper.cyMargin = fLandscape ?
		    Scale(pPaper[j].cxMargin, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cyMargin, lpdv->iRes,
		     DEFAULTRES);
		lpdv->paper.cxPaper = fLandscape ?
		    Scale(pPaper[j].cyPaper, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cxPaper, lpdv->iRes,
		     DEFAULTRES);
		lpdv->paper.cyPaper = fLandscape ?
		    Scale(pPaper[j].cxPaper, lpdv->iRes, DEFAULTRES) : Scale(pPaper[j].cyPaper, lpdv->iRes,
		     DEFAULTRES);
		break;
	}
	result = 1;
END:

	LocalFree((HANDLE)pPaper);
END_NOFREE_PAPER:
	FreePrinter(pPrinter);
END_NOFREE_PRINTER:
	return result;
}


