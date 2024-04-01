#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "apd.h"
#include "..\paper.h"
#include "globals.h"
#include "apderror.h"

/****************************************************************************/
/*#define DEBUG_ON*/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/

int Scale(int,int,int);
int Data2File(char *,char *,int,int);
void APDError(int,char*,...);

BOOL APDPass1(void);

BOOL APDPass1()
{
	POINT *paperSizes;
	int numPapers;
	int i;
	PAPER *zeroPaper;
	PAPER *tilePaper;

	fprintf(stderr,"APD Compiler:  Pass 1.\n");
	printf("APD Compiler:  Pass 1.\n");

	numPapers=0;
	/* find the number of papers */
	while(!(gPaperSizes[numPapers].x==0.0)) numPapers++;

	/* allocate the various structures */
	if(!(paperSizes=(POINT*)malloc(numPapers*sizeof(POINT)))){
		APDError(E_MEMORY1,"");
		goto ERROR;
	}
	for(i=0;i<numPapers;i++){
		DBMSG(("BEFORE:x=%10.4f,y=%10.4f\n",
			gPaperSizes[i].x,gPaperSizes[i].y));
		paperSizes[i].x=(short)
			(((gPaperSizes[i].x*(double)(DEFAULTRES))/72.0)+.5);
		paperSizes[i].y=(short)
			(((gPaperSizes[i].y*(double)(DEFAULTRES))/72.0)+.5);
		DBMSG(("AFTER: x=%d,y=%d\n",paperSizes[i].x,paperSizes[i].y));
	}

#if 0
	numPapers=0;
	/* convert paper sizes from pts to 1/DEFAULTRES ths of an inch */
	while(!(gPaperSizes[numPapers].x==0.0)){
		DBMSG(("BEFORE:x=%10.4f,y=%10.4f\n",
			gPaperSizes[numPapers].x,gPaperSizes[numPapers].y));
		paperSizes[numPapers].x=(short)
			(((gPaperSizes[numPapers].x*(double)(DEFAULTRES))/72.0)+.5);
		paperSizes[numPapers].y=(short)
			(((gPaperSizes[numPapers].y*(double)(DEFAULTRES))/72.0)+.5);
		DBMSG(("AFTER: x=%d,y=%d\n",
			paperSizes[numPapers].x,paperSizes[numPapers].y));
		numPapers++;
	}
#endif

	/* remember to subtract the entry for the default */
	numPapers--;
	DBMSG(("numPapers=%d\n",numPapers));

	if(!(zeroPaper=(PAPER*)malloc(numPapers*sizeof(PAPER)))){
		APDError(E_MEMORY1,"");
		goto ERROR;
	}
	if(!(tilePaper=(PAPER*)malloc(numPapers*sizeof(PAPER)))){
		APDError(E_MEMORY1,"");
		goto ERROR;
	}

	for(i=0;i<numPapers;i++){
		DBMSG(("BEFORE:x=%10.4f,y=%10.4f\n",
			gPaperSizes[i+1].x,gPaperSizes[i+1].y));
		zeroPaper[i].iPaper=IDS_PP_MIN+i;
		zeroPaper[i].cxPaper=(short)
			(((gPaperSizes[i+1].x*(double)(DEFAULTRES))/72.0)+.5);
		zeroPaper[i].cyPaper=(short)
			(((gPaperSizes[i+1].y*(double)(DEFAULTRES))/72.0)+.5);
		zeroPaper[i].cxPage=zeroPaper[i].cxPaper;
		zeroPaper[i].cyPage=zeroPaper[i].cyPaper;
		zeroPaper[i].cxMargin=0;
		zeroPaper[i].cyMargin=0;
		DBMSG(("AFTER: i=%d: %d %d, %d %d, %d %d\n",
			IDS_PP_MIN+i,zeroPaper[i].cxPaper,zeroPaper[i].cyPaper,
			zeroPaper[i].cxPage,zeroPaper[i].cyPage,
			zeroPaper[i].cxMargin,zeroPaper[i].cyMargin));
	}

	for(i=0;i<numPapers;i++){
		DBMSG(("BEFORE:x=%10.4f,y=%10.4f\n",
			gPaperSizes[i+1].x,gPaperSizes[i+1].y));
		tilePaper[i].iPaper=IDS_PP_MIN+i;
		tilePaper[i].cxPaper=(short)
			(((gPaperSizes[i+1].x*(double)(DEFAULTRES))/72.0)+.5);
		tilePaper[i].cyPaper=(short)
			(((gPaperSizes[i+1].y*(double)(DEFAULTRES))/72.0)+.5);
		tilePaper[i].cxPage=PA;
		tilePaper[i].cyPage=PA;
		tilePaper[i].cxMargin=-(PA/2);
		tilePaper[i].cyMargin=-(PA/2);
		DBMSG(("AFTER: i=%d: %d %d, %d %d, %d %d\n",
			IDS_PP_MIN+i,tilePaper[i].cxPaper,tilePaper[i].cyPaper,
			tilePaper[i].cxPage,tilePaper[i].cyPage,
			tilePaper[i].cxMargin,tilePaper[i].cyMargin));
	}

	/* Remember the first entry is the default image area...
	 * the 2nd, 3rd, ... entries are the actual list.
	 */
	if(!Data2File("papersiz.dta",(char*)&paperSizes[1],sizeof(POINT),
		numPapers)
	){
		goto ERROR;
	}

	/* little lie approach:  the idea is not to report the real printable
	 * area, because conversion to scanlines happens with alarming frequency,
	 * and is deadly.  Besides, it's too hard to keep up with printable areas
	 * for each printer. Uses win.ini flag--"margins=no".
	 */
	if(!Data2File("nomargin.dta",(char*)zeroPaper,sizeof(PAPER),numPapers)){
		goto ERROR;
	}
	/* big lie approach:  for doing tiling, you want to image an area much
	 * larger than the physical page.  So, we tell gdi that this is the case.
	 * the majority of apps do strange things, so we make it depend on a
	 * win.ini flag--"Tile Mode=yes" invalidates "margins=no".
	 */
	if(!Data2File("ngmargin.dta",(char*)tilePaper,sizeof(PAPER),numPapers)){
		goto ERROR;
	}
	free((char*)paperSizes);
	free((char*)zeroPaper);
	free((char*)tilePaper);

	return(TRUE);

ERROR:
	return(FALSE);
}

