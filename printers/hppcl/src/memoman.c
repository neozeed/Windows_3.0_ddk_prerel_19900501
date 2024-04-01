/**[f******************************************************************
 * memoman.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.  
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/*******************************   memoman.c   *******************************/
/*
 *	Printer Memory Manager Routines
 *
 *	rev:
 *
 *	12-05-86	msd		changed some == to = for proper execution; added
 *							push/pop cursor position in CopySoft()
 *	11-29-86	skk		module creation
 */

#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#include "fontman.h"
#include "utils.h"
#define SEG_PHYSICAL
#include "memoman.h"


/*	utilities
 */
#include "lockfont.c"
#include "makefsnm.c"


/*	Local debug structure.
 */
#ifdef DEBUG
	#define LOCAL_DEBUG
#endif

#ifdef LOCAL_DEBUG
	#define DBGTrace(msg) DBMSG(msg)
	#define DBGErr(msg)	DBMSG(msg)
	#define DBGUpdateLRU(msg) DBMSG(msg)
#else
	#define DBGErr(msg)  /*null*/
	#define DBGTrace(msg) /*null*/
	#define DBGUpdateLRU(msg) /*null*/
#endif

/*declaration of static routines*/
static BOOL UnloadSofts(LPDEVICE,LPFONTSUMMARYHDR,long);
static BOOL CopySoft(LPDEVICE, LPSTR);
static void AddSoft(LPDEVICE, short);
static void DeleteSoft(LPDEVICE, short);
static BOOL UnloadSofts(LPDEVICE, LPFONTSUMMARYHDR, long);


BOOL FAR PASCAL DownLoadSoft(lpDevice, fontInd)
	LPDEVICE lpDevice;
	short fontInd;
	{
	BOOL retvalue=TRUE;
	int err=SUCCESS;
	ESCtype escape;
	char DLfilename[128];
	LPFONTSUMMARYHDR lpFontSummary;
	LPFONTSUMMARY lpSummary;
	short len, fontID;
	long newmem;

	DBGTrace(("In DownLoadSoft, fontInd=%d, epPgSoftNum=%d, epMaxMgSoft=%d, epFreeMem=%d\n",
		fontInd, lpDevice->epPgSoftNum, lpDevice->epMaxPgSoft, lpDevice->epFreeMem));
	if (lpDevice->epPgSoftNum >= lpDevice->epMaxPgSoft)	{
		DBGTrace(("more than max softs already on page\n"));
		retvalue=FALSE;
	}
	else if (!(lpFontSummary = lockFontSummary(lpDevice)))
		{
		/*if fontsummary can't be accessed then exit*/
		DBGTrace(("DownLoadSoft: can't access fontsummary table\n"));
		retvalue=FALSE;
		}
	else {
		lpSummary = &lpFontSummary->f[fontInd];

		if (lpDevice->epFreeMem < ((newmem=lpSummary->memUsage)+MINMEM)
			|| (lpDevice->epTotSoftNum >= lpDevice->epMaxSoft))
			{
	 		if (!(UnloadSofts(lpDevice,lpFontSummary,newmem)))
	 			retvalue=FALSE;
			}

		if (retvalue && !MakeFontSumFNm (lpFontSummary, lpSummary,
				DLfilename, sizeof(DLfilename), FALSE))
			retvalue = FALSE;

		if (retvalue) {
			/*send escape to set the font ID*/
			fontID=lpSummary->fontID;
			err=myWrite(lpDevice, (LPSTR)&escape,
				MakeEscape((lpESC)&escape,SET_FONT,fontID));
			/*download the font*/
			CopySoft(lpDevice, DLfilename);
			/**/
			lpDevice->epFreeMem-=newmem;
			DBGTrace(("free mem is %ld\n",lpDevice->epFreeMem));
			UpdateSoftInfo(lpDevice,lpFontSummary,fontInd);
			lpDevice->epTotSoftNum++;
			/*add to soft font list*/
			AddSoft(lpDevice, fontInd);
			/*send escape to select the font*/
			err = myWrite(lpDevice, (LPSTR)&escape,
				MakeEscape((lpESC)&escape,DES_FONT, fontID));
		}
		unlockFontSummary(lpDevice);
	}
	return retvalue;
}


void FAR PASCAL UpdateSoftInfo(lpDevice,lpFontSummary,fontind)
	LPDEVICE	lpDevice;
	LPFONTSUMMARYHDR lpFontSummary;
	short	fontind;  /*index of current font*/
	/*Assumes that FontSummary is locked. LRU counts for temporary soft
	fonts are updated (the LRU count for the current
	font is 0 and the LRU count of all other fonts is incremented) The
	onPage flag is also set for the font and lpDevice->epPgFontNum is
	incremented*/
	{
	short  ind,len;
	LPFONTSUMMARY lpSummary;

	lpSummary=&lpFontSummary->f[0];
	len = lpFontSummary->len;
	for (ind=0; ind < len; ++ind, ++lpSummary) {
		if (ind==fontind) {
			if (lpSummary->indDLName != -1)
				lpSummary->LRUcount = 0;
			if (!(lpSummary->onPage)) {
				lpSummary->onPage = TRUE;
				lpDevice->epPgSoftNum++;
			}
		}
		else
			if ((lpSummary->indDLName != -1) && (lpSummary->LRUcount != -1))
				lpSummary->LRUcount++;
	}
}

	
static BOOL CopySoft(lpDevice, DLfilename)
	LPDEVICE lpDevice;
	LPSTR 	DLfilename;
	{
	int fh;
	char fbuf[BLOCK_SIZE];
	BOOL morebytes=TRUE;
	WORD numread=0;
	DBGTrace(("IN CopySoft, DLfilename=%ls\n",DLfilename));
	if ((fh = _lopenp(DLfilename, OF_READ)) > 0) {
		DBGTrace(("file opened\n"));
		/* save current cursor position */
		myWrite(lpDevice,PUSH_POSITION);
		while (morebytes) {
			if ((numread =_lread(fh, (LPSTR) fbuf, BLOCK_SIZE))!=BLOCK_SIZE)
				morebytes=FALSE;
			if (numread >0) {
				myWrite(lpDevice,(LPSTR)fbuf,(short)numread);
				myWriteSpool(lpDevice);
			}
		}
		_lclose(fh);
		/* restore cursor position */
		myWrite(lpDevice,POP_POSITION);
		return TRUE;
	}
	else
		return FALSE;
	}
	



static void AddSoft(lpDevice, fontInd)
	LPDEVICE lpDevice;
	short fontInd;
/*add soft font to soft font list*/
{
	
}

static void DeleteSoft(lpDevice,fontInd)
	LPDEVICE lpDevice;
	short fontInd;
/*delete soft font from soft font list*/
 {
 	
 }

static BOOL UnloadSofts(lpDevice,lpFontSummary,memNeeded)
	LPDEVICE lpDevice;
	LPFONTSUMMARYHDR lpFontSummary;
	long memNeeded;
/*note:assumes FONTSUMMARY is locked*/
/*based on LRU count, memUsage & onPage, decide which downloaded fonts to boot*/
{
	int err;
	short fontInd;
	ESCtype escape;
	short  ind,len;
	LPFONTSUMMARY lpSummary;
	short lastLRU=-1;

	DBGTrace(("IN UnloadSofts, memneeded=%ld\n",memNeeded));
	lpSummary=&lpFontSummary->f[0];
	len = lpFontSummary->len;
	for (ind=0; ind < len; ++ind, ++lpSummary) {
		if (!lpSummary->onPage && (lpSummary->memUsage>=memNeeded) &&
			(lpSummary->LRUcount>lastLRU)) {
				fontInd=ind;
				lastLRU=lpSummary->LRUcount;
			}
	}
	if (lastLRU==-1)
		return FALSE;
	
	DeleteSoft(lpDevice,fontInd);
	/*send escapes to delete the font*/
	lpFontSummary->f[fontInd].LRUcount=-1;
	err=myWrite(lpDevice, (LPSTR)&escape,
		MakeEscape((lpESC)&escape,DES_FONT,lpFontSummary->f[fontInd].fontID));
	err=myWrite(lpDevice,DEL_FONT);
	lpDevice->epFreeMem += memNeeded - lpFontSummary->f[fontInd].memUsage;
	lpDevice->epTotSoftNum--;
	lpDevice->epPgSoftNum--;
	return TRUE;
}


