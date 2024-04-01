/**[f******************************************************************
 * fonts.c (soft fonts)
 *
 *
 **f]*****************************************************************/

/*********************************************************************
 * 8Jan89	chrisg	created (moved from enable.c and sucked in
 *			fontdir.c)
 *
 *********************************************************************/

#include "pscript.h"
#include <winexp.h>
#include "devmode.h"
#include "psdata.h"
#include "debug.h"
#include "utils.h"
#include "resource.h"
#include "etm.h"
#include "fonts.h"
#include "profile.h"

#define MAX_SOFT_FONTS 255	/* limit for softfonts=# win.ini entry */

#define SCALE_PRINTER(p)	((p) - INT_PRINTER_MIN)

typedef struct {
	int	LockCount;	/* lock count for printer font directory */
	HANDLE	FontsHandle;	/* handle for printer font data */
} PRINTER_FONTS;

/*---------------------------- local data ---------------------------*/

PRINTER_FONTS PrinterFonts[NUMPRINTERS];



/*--------------------------- local functions -------------------------*/


int PASCAL LoadSoftFonts(LPSTR, int far *, LPSTR);
int PASCAL LoadDirEntry(LPSTR, LPSTR);



/*********************************************************************
 * Name: LoadFontDir()
 *
 * Action: Load the specified font directory into memory from the
 *	  resource and append the font metrics for any
 *	  external softfonts if they exist.
 *
 * return TRUE on success
 * return FALSE on failure
 *
 *********************************************************************/

BOOL FAR PASCAL LoadFontDir(iPrinter, lszFile)
int	iPrinter;
LPSTR	lszFile;	    /* ptr to the output filename string */
{
	HANDLE	hres;	    /* The font directory resource */
	HANDLE	h;
	int	cSoftFonts;	    /* The number of soft fonts */
	int	cFonts; 	    /* The number of fonts */
	int	fh;		    /* The resource file handle */
	int	cb;		    /* Byte count */
	int	i;
	LPSTR	lpbDst;
     	LPSTR	lpbDir;
	char	buf[40];
	OFSTRUCT os;
	PS_RES_HEADER header;

	DBMSG(("LoadFontDir(): iPrinter:%d\n", iPrinter));


	/* If the font directory is already loaded, just return */

	if (lpbDir = LockFontDir(iPrinter)) {

		UnlockFontDir(iPrinter);

		PrinterFonts[SCALE_PRINTER(iPrinter)].LockCount++;

		return TRUE;
	}


	fh = 0;

	if (iPrinter >= EXT_PRINTER_MIN) {
	
		if ((fh = GetExternPrinter(iPrinter - EXT_PRINTER_MIN + 1)) == -1) {
			DBMSG(("external printer failed\n"));
			return FALSE;
		}

		if (_lread(fh, (LPSTR)&header, sizeof(header)) != sizeof(header)) {
			DBMSG(("external printer read failed\n"));
			return FALSE;
		}

		_llseek(fh, header.dir_loc, 0);	// position the file pointer

		DBMSG(("dir file:%ls fh:%d\n", (LPSTR)buf, fh));
	} else {

		/* Find the font directory */
		if (!(hres = FindResource(ghInst, 
			MAKEINTRESOURCE(iPrinter),
			MAKEINTRESOURCE(MYFONTDIR)))) {
			return FALSE;
		}
	}


	/* There will be two passes through the following code. */
	/* Once to compute the directory size, then again to load it */

AGAIN:

	if (fh) {
		cb = header.dir_len;		// get the dir size
	} else
		cb = SizeofResource(ghInst, hres);


	DBMSG(("size of font dir %d\n", cb));

	lpbDst = lpbDir;

	if (lpbDir) {
		/* Read the font directory into memory */
		if (!fh)	/* alread opened for external file? */
			fh = AccessResource(ghInst, hres);

		if (_lread(fh, lpbDir, cb) < 0) {
			cb = 0;
			DBMSG(("read failed\n"));
		}
		_lclose(fh);

		if (cb <= 0) {
			UnlockFontDir(iPrinter);
			DeleteFontDir(iPrinter);
			return FALSE;
		}

		/* Compute ptr to first "softfont" slot */
		cFonts = *((LPSHORT)lpbDst)++;

		DBMSG(("cFonts = %d\n", cFonts));

		for (i = 0; i < cFonts; ++i)
			lpbDst += *((LPSHORT)lpbDst);
	}

	/* Extend the font directory by adding any softfonts */
	cb += LoadSoftFonts(lpbDst, &cSoftFonts, lszFile);

	/* Update the font count at the beginning of the directory */
	if (lpbDir) 
		*((LPSHORT)lpbDir) = cFonts + cSoftFonts;

	/* At this point we may have only computed the font directory size.
	 * If so, then allocate memory for it and go back to load it. */

	if (!lpbDir) {
		if (h = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (long) cb)) {

			PrinterFonts[SCALE_PRINTER(iPrinter)].FontsHandle = h;
			PrinterFonts[SCALE_PRINTER(iPrinter)].LockCount = 1;
			if (lpbDir = LockFontDir(iPrinter))
				goto AGAIN;
			DeleteFontDir(iPrinter);
		}
		return FALSE;
	} else {
		UnlockFontDir(iPrinter);
	}
	return TRUE;
}


/***********************************************************
* Name: LoadDirEntry()
*
* Action: Calculate the size of the softfont directory and
*	  load it (if storage has been allocated for it).
*
************************************************************/

int PASCAL LoadDirEntry(lszFile, lpbDst)
LPSTR lszFile;		/* The PFM file name */
LPSTR lpbDst;		/* Ptr to place to load the entry */
{
	int	fh;
	int	cbEntry;	/* The size of the directory entry */
	int	cbdf;		/* The size of the device font header */
	int	cbDevice;	/* The size of the device name */
	int	cbFace; 	/* The size of the face name */
	int	cbFont; 	/* Size of the font name */
	LPSTR	lpbSrc;
	int	i;
	PFM	pfm;
	char	szDevType[32];
	char	szFace[32];
	LPSTR	ptr;
	char	old_char;
	LONG	lStart;

	/* traverse the softfont entry, and chop off the download file name
	 * if necessary. "softfontX=d:\path\file.pfm[,d:\path\file.pfb]" */

	ptr = lszFile;
	while (*ptr && *ptr != ',')
		ptr++;

	if (old_char = *ptr)
		*ptr = 0;	/* chop off extra stuff */


	fh = _lopen(lszFile, READ);

	/* look for softfont as a resource here */
	if (fh < 0) {

		HANDLE hData;

		DBMSG(("try softfont as resorce %ls\n", lszFile));

		hData = FindResource(ghInst, lszFile, MAKEINTRESOURCE(MYFONT));

		if (hData) {
			DBMSG(("softfont using resource!\n"));
			fh = AccessResource(ghInst, hData);
		}
	}

	if (fh < 0)
		return 0;

	/* get start file position incase we are reading as a resource */

	lStart = _llseek(fh, 0L, SEEK_CUR);

	DBMSG(("lStart %ld\n", lStart));

	if (_lread(fh, (LPSTR)&pfm, sizeof(PFM)) != sizeof(PFM))
		goto READERR;

	_llseek(fh, lStart, 0);		/* return to start */

	_llseek(fh, (long)pfm.df.dfFace, SEEK_CUR);
	if (_lread(fh, szFace, sizeof(szFace)) <= 0)
		goto READERR;

	_llseek(fh, lStart, 0);		/* return to start */

	_llseek(fh, (long)pfm.df.dfDevice, SEEK_CUR);
	if (_lread(fh, szDevType, sizeof(szDevType)) <= 0)
		goto READERR;

	_lclose(fh);


	*ptr = old_char;	/* resore ',' if necessary */

	cbdf     = ((LPSTR) & pfm.df.dfBitsPointer) - (LPSTR) & pfm;
	cbFace   = lstrlen(szFace) + 1;
	cbDevice = lstrlen(szDevType) + 1;
	cbFont   = lstrlen(lszFile) + 2;
	cbEntry  = cbdf + cbFace + cbDevice + cbFont + 4;

	if (lpbDst) {
		*((LPSHORT)lpbDst)++ = cbEntry;
		*((LPSHORT)lpbDst)++ = cbdf + cbFace + cbDevice;


		lpbSrc = (LPSTR) & pfm.df;
		pfm.df.dfFace = cbdf;
		pfm.df.dfDevice = cbdf + cbFace;
		for (i = 0; i < cbdf; ++i)
			*lpbDst++ = *lpbSrc++;

		/* Copy the face name into the font directory */
		lpbSrc = szFace;
		for (i = 0; i < cbFace; ++i)
			*lpbDst++ = *lpbSrc++;

		/* Copy the device name into the font directory */
		lpbSrc = szDevType;
		for (i = 0; i < cbDevice; ++i)
			*lpbDst++ = *lpbSrc++;

		/* Copy the PFM file name into the font directory */
		*lpbDst++ = '$';	 /* Mark this as a softfont */
		lpbSrc = lszFile;
		for (i = 0; i < cbFont; ++i)
			*lpbDst++ = *lpbSrc++;
	}
	return(cbEntry);

READERR:
	_lclose(fh);
	return(0);
}



/****************************************************************
* Name: LoadSoftFonts()
*
* Action: Either load the soft fonts into memory or get information
*	  about them depending on the value of the destination pointer.
*	  If the destination pointer is NULL, then this routine just
*	  computes the size and number of softfonts specified in
*	  win.ini.  If the destination pointer is not NULL, then
*	  the soft fonts are actually loaded.
* in:
*	lpbDir		font directory, if NULL just compute size
*	lszFile		port name, used to read win.ini
*
* out:
*	*lpcFonts	gets the number of softfonts loaded
*	
* returns:
*	the size of the softfonts in bytes
*
*********************************************************************/

int PASCAL LoadSoftFonts(lpbDir, lpcFonts, lszFile)
LPSTR lpbDir;		/* ptr to font directory */
int far *lpcFonts;	/* output # of soft fonts found */
LPSTR lszFile;		/* port name for reading win.ini */
{
	char	szField[20];
	char	szFile[90];	/* must hold stuff after "softfontX=" */
	int	cFonts;
	int	i;
	int	cbDir;
	int	cbEntry;
	char	key[50];

	/* I'll bet this isn't the right way to do it...the name of the driver
	 * should come from somewhere central... */

	SetKey(key, lszFile);

	DBMSG((">LoadSoftFonts(): key=%ls, ", (LPSTR)key));

	cbDir = 0;
	cFonts = GetProfileInt(key, "softfonts", 0);

	DBMSG(("# softfonts %d\n", cFonts));

	if (cFonts > MAX_SOFT_FONTS)
		cFonts = MAX_SOFT_FONTS;

	*lpcFonts = 0;		// start with no softfonts

	for (i = 1; i <= cFonts; ++i) {

		wsprintf(szField, "softfont%d", i);

		GetProfileString(key, szField, "", szFile, sizeof(szFile));

		DBMSG(("softfont%d=%ls\n", i, (LPSTR)szFile));

		cbEntry = LoadDirEntry(szFile, lpbDir);

		if (cbEntry) {			// only if successful

			(*lpcFonts)++;		// inc count of softfonts

			if (lpbDir)
				lpbDir += cbEntry;
			cbDir += cbEntry;
		}
	}

	DBMSG(("<LoadSoftFonts()\n"));
	return cbDir;
}



/********************************************************
 * Name: LockFontDir()
 *
 * iPrinter	printer who's font dir should be locked
 *
 * Action: Lock the specified font directory and return a pointer to it.
 *
 * Returns: A pointer to the locked font directory.
 *
 **********************************************************/

LPSTR FAR PASCAL LockFontDir(iPrinter)
int iPrinter;
{
	iPrinter = SCALE_PRINTER(iPrinter);

	if (iPrinter < NUMPRINTERS && PrinterFonts[iPrinter].FontsHandle)
		return GlobalLock(PrinterFonts[iPrinter].FontsHandle);
	else
		return NULL;
}


/**************************************************************
 * Name: UnlockFontDir()
 *
 * iPrinter	printer who's font dirctory should be unlocked
 *
 * Action: This routine unlocks the specified font directory.
 *
 ***************************************************************/

void FAR PASCAL UnlockFontDir(iPrinter)
int iPrinter;
{
	iPrinter = SCALE_PRINTER(iPrinter);

	if (iPrinter < NUMPRINTERS && PrinterFonts[iPrinter].FontsHandle)
		GlobalUnlock(PrinterFonts[iPrinter].FontsHandle);
}




/*******************************************************************
 * Name: DeleteFontDir()
 *
 * Action: Delete a font directory and free its memory if this is
 *	  the last reference to it.
 *
 *******************************************************************/

void FAR PASCAL DeleteFontDir(iPrinter)
int iPrinter;
{
	iPrinter = SCALE_PRINTER(iPrinter);

	if (iPrinter < NUMPRINTERS && 
	    PrinterFonts[iPrinter].FontsHandle) {
		if (--PrinterFonts[iPrinter].LockCount <= 0) {
			GlobalFree(PrinterFonts[iPrinter].FontsHandle);
	    		PrinterFonts[iPrinter].LockCount = 0;
		    	PrinterFonts[iPrinter].FontsHandle = NULL;
		}
	}
}





