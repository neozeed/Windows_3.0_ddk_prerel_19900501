/**[f******************************************************************
 * getdata.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * GETDATA.C
 *
 * 20Aug87	sjp		Creation: Extracted from RESET.C.
 * 03Nov87	sjp		Added APD stuff...DumpPSS().
 **********************************************************************/

#define DEBUG3_ON

#include "pscript.h"
#include <winexp.h>
#include "devmode.h"
#include "debug.h"
#include "channel.h"
#include "resource.h"
#include "psdata.h"
#include "getdata.h"
#include "profile.h"
#include "utils.h"

/*
 * BOOL FAR PASCAL DumpPSS(lpdv,resID,dirID,pssID)
 *
 * lpdv		ptr to device structure
 * resID	resource # for PR_PSS resource type
 * dirID	directory index (used to offset from beginning of resource)
 * pssID	another offset? make < 0 if not used.
 *
 * returns TRUE for success, FALSE on error.
 *
 * NOTE: iPrinter is assumed to be in the range of 0 - (NUMPRINTERS - 1)
 *	 this is different from GetPriterCaps()!
 *
 */

BOOL FAR PASCAL DumpPSS(lpdv, iPrinter, dirID, pssID)
LPDV	lpdv;
short	iPrinter;	/* the printer number */
short	dirID;
short	pssID;
{
	long	startOffset;
	long	lOffset;
	short	len;
	short	numToRead;
	short	fh;
	HANDLE	hRes;
	char	buf[128];
	OFSTRUCT os;
	PS_RES_HEADER header;

	DBMSG((">DumpPSS(%lp, %d, %d, %d)\n", lpdv, iPrinter, dirID, pssID));

	if (iPrinter >= EXT_PRINTER_MIN) {


		if ((fh = GetExternPrinter(iPrinter - EXT_PRINTER_MIN + 1)) == -1) {
			DBMSG(("external printer failed\n"));
			goto ERROR0;

		}

		if (_lread(fh, (LPSTR)&header, sizeof(header)) != sizeof(header)) {
			DBMSG(("external printer read failed\n"));
			return FALSE;
		}

		_llseek(fh, header.pss_loc, 0);	// position the file pointer


		DBMSG(("PSS file:%ls fh: %d\n", (LPSTR)buf, fh));
		

	} else {

		if (!(hRes = FindResource(ghInst, MAKEINTRESOURCE(iPrinter),
		    MAKEINTRESOURCE(PR_PSS)))) {
			DBMSG((" DumpPSS(): FindRes ERROR\n"));
			goto ERROR1;
		}

		if ((fh = AccessResource(ghInst, hRes)) < 0) {
			DBMSG((" DumpPSS(): AccessRes ERROR\n"));
			goto ERROR1;
		}
	}

	startOffset = _llseek(fh, 0L, SEEK_CUR);

	/* access the directory and get the offset to the PSS/PSS list*/
	lOffset = (long)(dirID * sizeof(long));

	DBMSG((" DumpPSS(): offset=%ld\n", lOffset));

	_llseek(fh, lOffset, SEEK_CUR);

	if (_lread(fh, (LPSTR)&lOffset, sizeof(long)) != sizeof(long)) {
		DBMSG((" DumpPSS(): lread ERROR\n"));
		goto ERROR0;
	}

	DBMSG((" DumpPSS(): offset=%ld\n", lOffset));

	if (!lOffset)
		goto END;

	/* if the desired PSS is part of a list */
	if (pssID >= 0) {
		lOffset += (long)(pssID * sizeof(long));

		DBMSG((" DumpPSS(): offset=%ld\n", lOffset));

		/* access the string's offset in the PSS list */
		_llseek(fh, startOffset, SEEK_SET);
		_llseek(fh, lOffset, SEEK_CUR);

		if (_lread(fh, (LPSTR)&lOffset, sizeof(long)) != sizeof(long))
			goto ERROR0;

		DBMSG((" DumpPSS(): offset=%ld\n", lOffset));
		if (lOffset <= 0 || lOffset > 1000000L)
			goto END;
	}
	/* access the string */
	_llseek(fh, startOffset, SEEK_SET);
	_llseek(fh, lOffset, SEEK_CUR);

	/* get the length of the string */
	if (_lread(fh, (LPSTR)&len, sizeof(short)) != sizeof(short))
		goto ERROR0;

	DBMSG((" DumpPSS(): len=%d\n", len));
	if (!len)
		goto END;

	while (len > 0) {
		numToRead = len > sizeof(buf) ? sizeof(buf) : len;

		if (_lread(fh, buf, numToRead) != numToRead)
			goto ERROR0;

#ifdef DEBUG1_ON
		{
			short	i;
			for (i = 0; i < numToRead; i++) {
				DBMSG(("%c", buf[i]));
				if (!((i + 1) % 64)) 
					DBMSG(((LPSTR)"\n"));
			}
		}
#endif
		WriteChannel(lpdv, buf, numToRead);
		len -= numToRead;
	}
	PrintChannel(lpdv, "\n");

END:
	_lclose(fh);
	DBMSG(("<DumpPSS()\n"));
	return TRUE;

ERROR0:
	DBMSG(("<DumpPSS(): ERROR0\n"));
	_lclose(fh);

ERROR1:
	DBMSG(("<DumpPSS(): ERROR1\n"));
	return FALSE;
}


/********************************************************************/

BOOL FAR PASCAL DumpResourceString(lpdv, resType, resID)
LPDV lpdv;
short	resType;
short	resID;
{
	HANDLE hRes;
	short	fh;
	short	cbRead;
	short	cbFile;
	char	buf[128];

	DBMSG((">DumpResourceString(): rT=%d,RID=%d\n", resType, resID));

	/* possibly shortcircuit the downloading of certain stuff */

	if (resType == PS_DATA) {
		switch (resID) {
		case PS_UNPACK:

			if (lpdv->DLState & DL_UNPACK)
				return TRUE;

			lpdv->DLState |= DL_UNPACK;
			break;

		case PS_CIMAGE:

			if (lpdv->DLState & DL_CIMAGE)
				return TRUE;

			lpdv->DLState |= DL_CIMAGE;
			break;

		case PS_FONTS:

			if (lpdv->DLState & DL_FONTS)
				return TRUE;

			lpdv->DLState |= DL_FONTS;
			break;
		}
	}

	if (!(hRes = FindResource(ghInst, MAKEINTRESOURCE(resID),
	    MAKEINTRESOURCE(resType)))) {
		goto ERROR1;
	}

	DBMSG1((" DumpResourceString(): -AccessResource()\n"));

	if ((fh = AccessResource(ghInst, hRes)) < 0)
		goto ERROR1;

	DBMSG1((" DumpResourceString(): +AccessResource()\n"));

	if (_lread(fh, (LPSTR)&cbFile, sizeof(cbFile)) == sizeof(cbFile)) {

		DBMSG1((" DumpResourceString: total=%d\n", cbFile));
		while (cbFile > 0) {
			cbRead = cbFile > sizeof(buf) ? sizeof(buf) : cbFile;
			DBMSG1((" DumpResourceString: numToRead=%d\n", cbRead));
			if (_lread(fh, buf, cbRead) != cbRead) 
				goto ERROR0;

			DBMSG1((" DumpResourceString: -WriteChannel t=%d,n=%d\n",
			    cbFile, cbRead));
			WriteChannel(lpdv, buf, cbRead);
			cbFile -= cbRead;
			DBMSG1((" DumpResourceString: +WriteChannel t=%d,n=%d\n",
			    cbFile, cbRead));
		}
	}
	_lclose(fh);

	DBMSG(("<DumpResourceString()\n"));
	return TRUE;

ERROR0:
	_lclose(fh);
ERROR1:
	DBMSG(("<DumpResourceString(): ERROR\n"));
	return FALSE;
}


/********************************************************************
 * LPSTR FAR PASCAL GetResourceData(lphData,lpName,lpType)
 *
 * find and load a resource identified by lpName lpType.
 *
 * returns a LOCKED pointer to data
 * stuffs the handle of the resource in *lphData (for future referance)
 *
 * if resource not found or some other error returns NULL
 *
 ********************************************************************/

LPSTR FAR PASCAL GetResourceData(lphData, lpName, lpType)
LPHANDLE lphData;
LPSTR lpName;
LPSTR lpType;
{
	HANDLE hInfo;
	LPSTR lpData = NULL;

	if ((hInfo = FindResource(ghInst, lpName, lpType)) && 
	    (*lphData = LoadResource(ghInst, hInfo))) {
		if (!(lpData = LockResource(*lphData))) {

			DBMSG3(("LockResource() failed in GetResourceData()!\n"));
			FreeResource(*lphData);
		}
	}
	return lpData;
}


/********************************************************************
 * BOOL FAR PASCAL UnGetResourceData(hData)
 *
 * unlocks and frees resource referanced by hData
 *
 ** can FreeResource fail if LoadResource worked? 
 *
 ********************************************************************/

BOOL FAR PASCAL UnGetResourceData(hData)
HANDLE hData;
{
	GlobalUnlock(hData);

	/* remember backwards logic on FreeResource() */
	return(!FreeResource(hData));
}



BOOL	FAR PASCAL GetImageRect(PPRINTER pPrinter, int iPaper, LPRECT lpRect)
{
	int i;

	for (i = 0; i < pPrinter->iNumPapers; i++) {
		if (pPrinter->Paper[i].iPaperType == iPaper) {
			*lpRect = pPrinter->Paper[i].rcImage;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL	FAR PASCAL PaperSupported(PPRINTER pPrinter, int iPaper)
{
	int i;

	for (i = 0; i < pPrinter->iNumPapers; i++) {
		if (pPrinter->Paper[i].iPaperType == iPaper)
			return TRUE;
	}
	return FALSE;
}



/********************************************************************
 * PPRINTER FAR PASCAL GetPrinter(iPrinter)
 *
 * in:
 *	iPrinter	printer #
 *
 * out:
 *	lpPrinter	gets PRINTER caps structure
 *
 * returns:
 *	TRUE	success
 *	FALSE	failure
 *
 ********************************************************************/

PPRINTER FAR PASCAL GetPrinter(iPrinter)
short	iPrinter;
{
	char buf[40];
	int fh;
	int cb;
	PPRINTER pPrinter;
	OFSTRUCT os;
	PS_RES_HEADER header;

	DBMSG(("GetPrinter(%d)\n", iPrinter));

	if (iPrinter >= EXT_PRINTER_MIN) {
	
		if ((fh = GetExternPrinter(iPrinter - EXT_PRINTER_MIN + 1)) == -1)
			return NULL;

		if (_lread(fh, (LPSTR)&header, sizeof(header)) != sizeof(header)) {
			DBMSG(("external printer read failed\n"));
			return NULL;
		}

		cb = header.cap_len; 		// get the cap size
		_llseek(fh, header.cap_loc, 0);	// position the file pointer

		DBMSG(("cap file:%ls size:%d\n", (LPSTR)buf, cb));

		if (!(pPrinter = (PPRINTER)LocalAlloc(LPTR, cb)))
			goto DONE_CLOSE;
		
		cb = _lread(fh, (LPSTR)pPrinter, cb);
DONE_CLOSE:
		_lclose(fh);

		return pPrinter;	// this will be NULL on error
	}


	/* get the selected printer's capabilities structure */
	if (iPrinter >= INT_PRINTER_MIN && iPrinter <= INT_PRINTER_MAX) {
		int size;
		HANDLE hRes;
		int fh;

		hRes = FindResource(ghInst, MAKEINTRESOURCE(iPrinter), MAKEINTRESOURCE(PR_CAPS));

		if (!hRes) {
			DBMSG(("Couldn't find printer CAP resource\n"));
			return NULL;
		}

		size = SizeofResource(ghInst, hRes);

		DBMSG(("Size of CAP resource %d\n", size));

		if (!(pPrinter = (PPRINTER)LocalAlloc(LPTR, size)))
			goto DONE_CLOSE1;

		fh = AccessResource(ghInst, hRes);
		_lread(fh, (LPSTR)pPrinter, size);
DONE_CLOSE1:
		_lclose(fh);
	} else
		return NULL;

	return pPrinter;	// will be NULL on error
}


