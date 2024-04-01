/**[f******************************************************************
 * fontman.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/******************************   fontman.c   ******************************/
//
//  Fontman: This module sets up the font summary data used by realize
//  and enumfonts to choose a font.
//
// 23 apr 90	clarkc  bldFSumFileName now uses Windows directory, not driver
//                      directory.  Bug 1741, problem when driver is on network.
// 16 feb 90	peterbe	Fixed bogus parameters to LoadString() at "Get
//			the name of the fontSummary..." in putFileFontSummary()
//			Changed some debug statements.
// 02 feb 90	clarkc  lmemcpy() to lstrncpy(), avoiding walking off segment.
// 07 aug 89	peterbe Changed lstrcmp() to lstrcmpi().
//   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
//   1-30-89	jimmat	Changes to support separate font installer DLL.
//   2-07-89	jimmat	Driver Initialization changes.
//   2-20-89	jimmat	Driver/Font Installer use same WIN.INI section (again)
//


#include "generic.h"
#include "resource.h"
#define FONTMAN_ENABLE
#define FONTMAN_DISABLE
#include "fontman.h"
#include "fonts.h"
#include "strings.h"
// #include "version.h"
#include "fontpriv.h"
#include "environ.h"
#include "utils.h"
#include "lclstr.h"

WORD FAR PASCAL GetWindowsDirectory(LPSTR,WORD);

/*  Utilities
 */
#include "lockfont.c"

#define USEWINDOWSDIRECTORY 1

/*  Local debug structure (mediumdumpfs must be
 *  enabled to get longdumpfs).
 */
#ifdef DEBUG
    #define LOCAL_DEBUG
    #undef DBGdumpfontsummary
#endif

#ifdef LOCAL_DEBUG
    #define DBGentry(msg) DBMSG(msg)
    #define DBGerr(msg) DBMSG(msg)
    #define DBGgetfontsum(msg) DBMSG(msg)
    #define DBGfntsumexists(msg) DBMSG(msg)
    #define DBGgetfile(msg) DBMSG(msg)
    #define DBGputfile(msg) DBMSG(msg)
    #define DBGfreefnt(msg) DBMSG(msg)
#else
    #define DBGentry(msg) /*null*/
    #define DBGerr(msg) /*null*/
    #define DBGgetfontsum(msg) /*null*/
    #define DBGfntsumexists(msg) /*null*/
    #define DBGgetfile(msg) /*null*/
    #define DBGputfile(msg) /*null*/
    #define DBGfreefnt(msg) /*null*/
#endif


typedef struct {
    char version[VERSION_LEN];		/* version of PCL driver */
    char port[PORT_LEN];		/* full name of port */
    WORD fsvers;			/* fontSummary version number */
    WORD envSize;			/* size of PCLDEVMODE struct */
    WORD softfonts;			/* number of softfonts in win.ini */
    WORD numEntries;			/* number of fontSums in file */
    DWORD offsetFirstEntry;		/* offset to directory */
} FSUMFILEHEADER;

typedef struct {
    PCLDEVMODE environ;			/* devmode for this fontSummary */
    WORD sizeofFontSum;			/* len of this fontSummary struct */
} FSUMFILEENTRY;


/*  Forward local procs.
 */
LOCAL HANDLE FontSumExists(LPPCLDEVMODE);
LOCAL HANDLE getFileFontSummary(LPPCLDEVMODE, LPSTR, HANDLE, LPHANDLE);
LOCAL void putFileFontSummary(HANDLE, LPPCLDEVMODE, LPSTR, HANDLE);
LOCAL BOOL SameEnvironments(LPPCLDEVMODE, LPPCLDEVMODE);
LOCAL BOOL mylmemcmp(LPSTR, LPSTR, short);
LOCAL void bldFSumFileName(LPSTR, LPSTR, HANDLE);
LOCAL void _lshuffle(int, DWORD, DWORD, WORD);


/*  gHFontSummary is used to share the same fontSummary struct
 *  across different instances -- this is safe because the
 *  fontSummary struct is used for read-only purposes.
 */
LOCAL HANDLE gHFontSummary = 0;

/***************************************************************************/
/**************************   Global Procedures   **************************/


/*  GetFontSummary
 *
 *  Lock down the fontSummary in memory if it exists, or see if it exists
 *  in a file.  Otherwise, build a new structure.
 */
HANDLE far PASCAL
GetFontSummary(LPSTR lpPortNm, LPSTR lpDeviceNm, LPPCLDEVMODE lpEnviron,
	       HANDLE hModule) {

    PCLDEVMODE environ;
    HANDLE hLS, hFontSummary;

    DBGentry(("GetFontSummary(%lp,%lp,%lp,%d): %ls, %ls\n",
	     lpPortNm, lpDeviceNm, lpEnviron, (HANDLE)hModule,
	     lpDeviceNm, lpPortNm));

    lmemset((LPSTR)&environ, 0, sizeof(PCLDEVMODE));

    /*  Get the environment (pcldevmode) structure.
     */
    if (lpEnviron) {

	/*  Environment passed in.
         */
	DBGgetfontsum(("GetFontSummary(): environment exists\n"));
	lmemcpy((LPSTR)&environ, (LPSTR)lpEnviron, sizeof(PCLDEVMODE));

    } else {

	/*  Environment not passed in, try to get it from Windows.
         */
	lstrcpy((LPSTR)&environ, lpDeviceNm);
	if (!GetEnvironment(lpPortNm, (LPSTR)&environ, sizeof(PCLDEVMODE))) {

	    /*  Environment not available, attempt to read it from the
             *  win.ini file or, if that fails, set the default values.
             */
	    DBGgetfontsum(("GetFontSummary(): getting default environment\n"));
	    MakeEnvironment(&environ, lpDeviceNm, lpPortNm, NULL);
	}
#ifdef LOCAL_DEBUG
	else {
	    DBGgetfontsum(("GetFontSummary(): got environment from Windows\n"));
	}
#endif
    }

    /*  Get/build the fontSummary structure:
     *
     *  1. Attempt to share it with existing identical IC, or
     *  2. Read it from a file (previously created), or
     *  3. Build it from scratch.
     */
    if (!(hFontSummary = FontSumExists(&environ)) &&
	!(hFontSummary = getFileFontSummary(&environ,lpPortNm,hModule,&hLS))) {

	/*  Build from scratch -- this may take a while.  Note that after
         *  the call to buildFontSummary(), hLS is invalid.
         */
	if (hFontSummary = buildFontSummary(&environ,hLS,lpPortNm,hModule))
	    putFileFontSummary(hFontSummary, &environ, lpPortNm, hModule);
	#ifdef DEBUG
	else
	  DBGgetfontsum(("..GetFontSummary(): buildFontSummary() failed.\n"));
	#endif
    }

    return (gHFontSummary = hFontSummary);
}

/*  FreeFontSummary
 *
 *  If this is the only DC using the font summary information, then
 *  erase the fontSummary struct.
 */
HANDLE far PASCAL FreeFontSummary (lpDevice)
    LPDEVICE lpDevice;
    {
    LPFONTSUMMARYHDR lpFontSummary = 0L;
    short numOpenDC;
    BOOL deleteit = FALSE;

    DBGentry(("FreeFontSummary(%lp): %d\n",
	lpDevice, (HANDLE)lpDevice->epHFntSum));

    /*  Lock down fontSummary struct if it exists.
     */
    if (lpFontSummary = lockFontSummary(lpDevice))
	{
	DBGfreefnt(("FreeFontSummary(): fontSummary %d locked\n",
	    (HANDLE)lpDevice->epHFntSum));

	/*  Decrement count of Display/Information Contexts sharing
         *  this structure -- if this is the last one, then mark the
         *  struct for delete.
         */
	if ((numOpenDC = --lpFontSummary->numOpenDC) <= 0)
	    {
	    LPFONTSUMMARY lpSummary;
	    short ind, len;

	    for (lpSummary=&lpFontSummary->f[ind=0], len=lpFontSummary->len;
		ind < len; ++ind, ++lpSummary)
		{
		/*  Free the handle to the width table if it exists.
                 */
		if (lpSummary->hWidthTable)
		    {
		    GlobalFree(lpSummary->hWidthTable);
		    lpSummary->hWidthTable = 0;
		    }
		}

	    /*  Set flag to delete fontSummary after
             *  it is unlocked.
             */
	    deleteit = TRUE;
	    }

	unlockFontSummary(lpDevice);
	lpFontSummary = 0L;

	if (deleteit)
	    {
	    DBGfreefnt(("FreeFontSummary(): fontSummary deleted\n"));
	    GlobalFree (lpDevice->epHFntSum);

	    if (lpDevice->epHFntSum == gHFontSummary)
		/* Zero global fontSummary handle */
		gHFontSummary = 0;

	    lpDevice->epHFntSum = 0;
	    }
	#ifdef LOCAL_DEBUG
	else {
	    DBGfreefnt((
		"FreeFontSummary(): fontSummary not deleted, %d DC using it\n",
		numOpenDC));
	    }
	#endif
	}

    DBGfreefnt(("...end of FreeFontSummary, return 0\n"));
    return (0);
    }

/***************************************************************************/
/**************************   Local Procedures   ***************************/


/*  FontSumExists
 *
 *  Look for the global fontSummary struct.  If it exists, lock it down and
 *  verify that it was built using environment parameters that match those
 *  of the environment passed in by the caller.  If the environments match,
 *  use the existing fontSummary.  If they don't, return failure.
 */
LOCAL HANDLE
FontSumExists(LPPCLDEVMODE lpEnviron) {

    LPPCLDEVMODE lpCmp;
    HANDLE hFontSummary = 0;
    LPFONTSUMMARYHDR lpFontSummary;

    DBGentry(("FontSumExists(%lp)\n", lpEnviron));

    if (gHFontSummary) {

	DBGfntsumexists(("FontSumExists(): gHFontSummary=%d exists\n",
	    (HANDLE)gHFontSummary));

	if (lpFontSummary = (LPFONTSUMMARYHDR)GlobalLock(gHFontSummary)) {

	    DBGfntsumexists(("FontSumExists(): gHFontSummary locked\n"));

	    lpCmp = &lpFontSummary->environ;

	    if (SameEnvironments(lpCmp, lpEnviron)) {

		/*  Environments are the same (i.e., font-related
                 *  information is the same).  We'll use this struct.
                 */
		DBGfntsumexists((
		    "FontSumExists(): environments are the same\n"));
		hFontSummary = gHFontSummary;
		++lpFontSummary->numOpenDC;
		lpFontSummary->newFS = FALSE;
	    }
#ifdef LOCAL_DEBUG
	    else {
		DBGfntsumexists((
		    "FontSumExists(): environments are *not* the same\n"));
	    }
#endif

	    GlobalUnlock (gHFontSummary);
	    lpFontSummary = 0L;
	}
    }

    return (hFontSummary);
}

/*  getFileFontSummary
 *
 *  Attempt to read the fontSummary structure from a file.
 */
LOCAL HANDLE getFileFontSummary(lpEnviron, lpPortNm, hModule, lpHLS)
    LPPCLDEVMODE lpEnviron;
    LPSTR lpPortNm;
    HANDLE hModule;
    LPHANDLE lpHLS;
    {
    FSUMFILEHEADER fsumFileHeader;
    FSUMFILEENTRY fsumFileEntry;
    HANDLE hFontSummary = 0;
    LPSTR lpFontSummary;
    LPLFS lpLS;
    DWORD seek;
    BOOL same;
    WORD fsvers;
    char fsumFileName[FSUM_FNMLEN];
    char version[VERSION_LEN];
    char buf[WRKBUF_LEN];
    int hFile = -1, err, count;

    DBGentry(("getFileFontSummary(%lp,%lp,%d,%lp): %ls, %ls\n",
      lpEnviron, lpPortNm, (HANDLE)hModule, lpHLS, (LPSTR)lpEnviron, lpPortNm));

    /*  Allocate LoadFontState struct.  We allocate it here (even though
     *  loadFontEntries() uses it) to have a place to store the list of
     *  soft font key names from the win.ini file.
     */
    if (!(*lpHLS = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
	(DWORD)sizeof(LOADFONTSTATE))))
	{
	DBGerr(("getFileFontSummary(): Could *not* alloc LoadFontState\n"));
	goto backout0;
	}
    if (!(lpLS = (LPLFS)GlobalLock(*lpHLS)))
	{
	DBGerr(("getFileFontSummary(): Could *not* lock LoadFontState\n"));
	goto backout1;
	}

    /*  Make application name (like "HPPCL,LPT1") for reading the
     *  win.ini entry.
     */
    MakeAppName(ModuleNameStr,lpPortNm,lpLS->appName,sizeof(lpLS->appName));

    /*  Get the fontSummary file version number from the win.ini
     *  file (or default to zero).
     */
    if (LoadString(hModule, WININI_FSVERS, buf, sizeof(buf)))
	fsvers = GetProfileInt(lpLS->appName, buf, 0);
    else
	fsvers = 0;

    /*  Get the number of soft fonts from win.ini (or default to zero).
     */
    if (LoadString(hModule, SF_SOFTFONTS, buf, sizeof(buf)))
	lpLS->softfonts = GetProfileInt(lpLS->appName, buf, 0);
    else
	lpLS->softfonts = 0;

    DBGgetfile(("getFileFontSummary(): appName=%ls, fsvers=%d, softfonts=%d\n",
	(LPSTR)lpLS->appName, fsvers, lpLS->softfonts));

    /*  Prepare to open fontSummary file.
     */
    lmemset(fsumFileName, 0, sizeof(fsumFileName));

    if (lstrlen(VNumStr) >= sizeof(version))
	lmemset(version, 0, sizeof(version));
    else
	lstrcpy(version, VNumStr);

    /*  Get the name of the fontSummary file from the win.ini file,
     *  open the file, read its header, and verify that its okay
     *  to use the file.
     */
    if (LoadString(hModule, FSUM_NAME, buf, sizeof(buf)) &&
	GetProfileString(lpLS->appName, buf, fsumFileName,
	    fsumFileName, sizeof(fsumFileName)) &&
	lstrlen(fsumFileName) &&
	((hFile = _lopenp(fsumFileName, OF_READ)) > 0) &&
	(err = _lread(hFile, (LPSTR) &fsumFileHeader,
	    sizeof(FSUMFILEHEADER))) &&
	(err == sizeof(FSUMFILEHEADER)) &&
	(lstrcmpi(version, fsumFileHeader.version) == 0) &&
	(fsvers == fsumFileHeader.fsvers) &&
	(lstrcmpi(lpPortNm, fsumFileHeader.port) == 0) &&
	(fsumFileHeader.envSize == sizeof(PCLDEVMODE)) &&
	(fsumFileHeader.softfonts == lpLS->softfonts))
	{
	DBGgetfile(("getFileFontSummary(): %ls=%ls\n",
	    (LPSTR)buf, (LPSTR)fsumFileName));
	DBGgetfile(
("                      hFile=%d, err=%d, version=%ls, fsvers=%d, port=%ls\n",
	    hFile, err, (LPSTR)version, fsvers, lpPortNm));
	DBGgetfile(
("                      envSize=%d, numEntries=%d, softfonts=%d\n",
	    fsumFileHeader.envSize, fsumFileHeader.numEntries,
	    fsumFileHeader.softfonts));

	/*  Search for a matching environment.
         */
	for (count = 0, same = FALSE, seek = fsumFileHeader.offsetFirstEntry;
	    (count < fsumFileHeader.numEntries) &&
	    (_llseek(hFile, seek, 0) == seek) &&
	    (err =
	     _lread(hFile, (LPSTR) &fsumFileEntry, sizeof(FSUMFILEENTRY))) &&
	    (err == sizeof(FSUMFILEENTRY)) &&
	    !(same = SameEnvironments(lpEnviron,(LPPCLDEVMODE)&fsumFileEntry));
	    ++count, seek += sizeof(FSUMFILEENTRY)+fsumFileEntry.sizeofFontSum)
	    ;

	if (same)
	    {
	    /*  A matching environment was found, we can use the
             *  fontSummary stored in the file.
             */
	    DBGgetfile(
		("getFileFontSummary(): environment %d matches\n", count));

	    /*  Allocate the fontSummary struct.
             *
             *  Make the structure fixed because this works best
             *  with LIM4.  The structure will go as low in memory
             *  as possible, so it won't clutter up the swap area
             *  and it will be shared across multiple instances
             *  of the driver.
             */
	    if (!(hFontSummary =
		GlobalAlloc(GMEM_FIXED | GMEM_LOWER | GMEM_DDESHARE,
		(DWORD)fsumFileEntry.sizeofFontSum)))
		{
		DBGerr(
		    ("getFileFontSummary(): Could *not* alloc fontSummary\n"));
		goto backout3;
		}

	    /*  Lock down fontSummary struct.
             */
	    if (!(lpFontSummary = GlobalLock(hFontSummary)))
		{
		DBGerr(
		    ("getFileFontSummary(): Could *not* lock fontSummary\n"));
		goto backout4;
		}

	    if ((err =
		_lread(hFile, lpFontSummary, fsumFileEntry.sizeofFontSum)) &&
		(err == fsumFileEntry.sizeofFontSum))
		{
		DBGgetfile(
		    ("getFileFontSummary(): fontSummary successfully read\n"));

		/*  Debug stuff: dump the whole damn thing.
                 */
		#ifdef DBGdumpfontsummary
		DBGdumpFontSummary((LPFONTSUMMARYHDR)lpFontSummary, -1);
		#endif

		GlobalUnlock(hFontSummary);
		lpFontSummary = 0L;

		if (count > 0)
		    {
		    _lclose(hFile);
		    hFile = 0;
		    if ((hFile = _lopenp(fsumFileName, OF_READWRITE)) > 0)
			{
			/*  Shuffle the directory item of the fontSummary
                         *  struct we just read to the top of the file.
                         *  This way we can use an easy LRU algorithm for
                         *  deleting items when the file grows too big.
                         */
			_lshuffle(hFile, fsumFileHeader.offsetFirstEntry, seek,
			    sizeof(FSUMFILEENTRY)+fsumFileEntry.sizeofFontSum);
			}
		    }
		goto backout3;
		}
	    #ifdef LOCAL_DEBUG
	    else
		{
		DBGgetfile(
		    ("getFileFontSummary(): *failed* to read fontSummary\n"));
		}
	    #endif
	    }
	else
	    {
	    DBGgetfile(
		("getFileFontSummary(): matching environment *not* found\n"));
	    goto backout3;
	    }
	}
    else
	{
	DBGgetfile(
	 ("getFileFontSummary(): failed to open/recognize fontSummary file\n"));
	goto backout3;
	}

    /*  Exit from the top if we allocated the fontSummary struct
     *  but failed to read it from the file.
     */
    GlobalUnlock(hFontSummary);
    lpFontSummary = 0L;
backout4:
    GlobalFree(hFontSummary);
    hFontSummary = 0;
backout3:
    if (hFile > 0)
	_lclose (hFile);
    hFile = -1;
    GlobalUnlock(*lpHLS);
    if (!hFontSummary)
	goto backout0;
backout1:
    GlobalFree(*lpHLS);
    *lpHLS = 0;
backout0:
    return (hFontSummary);
    }

/*  putFileFontSummary
 *
 *  Write the newly-created fontSummary structure to a file.
 */
LOCAL void putFileFontSummary(hFontSummary, lpEnviron, lpPortNm, hModule)
    HANDLE hFontSummary;
    LPPCLDEVMODE lpEnviron;
    LPSTR lpPortNm;
    HANDLE hModule;
    {
    FSUMFILEHEADER fsumFileHeader;
    FSUMFILEENTRY fsumFileEntry;
    LPFONTSUMMARYHDR lpFontSummary;
    WORD fsvers;
    char appName[APPNM_LEN];
    char fsumFileName[FSUM_FNMLEN];
    char fsumName[24];
    char buf[WRKBUF_LEN];
    char version[VERSION_LEN];
    int hFile = -1, err, ind, count;
    DWORD maxMem, seek;

    DBGentry(("putFileFontSummary(%d,%lp,%lp,%d): %ls, %ls\n",
	(HANDLE)hFontSummary, lpEnviron, lpPortNm,
	(HANDLE)hModule, (LPSTR)lpEnviron, lpPortNm));

    /*  Lock down the fontSummary struct.
     */
    if (!(lpFontSummary = (LPFONTSUMMARYHDR)GlobalLock(hFontSummary)))
	{
	DBGerr(
	  ("..putFileFontSummary(): could *not* lock fontSummary struct\n"));
	return;
	}

    /*  Make application name (like "HPPCL,LPT1") for reading the
     *  win.ini entry.
     */
    MakeAppName(ModuleNameStr,lpPortNm,appName,sizeof(appName));

    /*  Get the fontSummary file version number from the win.ini
     *  file (or default to zero).
     */
    if (LoadString(hModule, WININI_FSVERS, buf, sizeof(buf)))
	fsvers = GetProfileInt(appName, buf, 0);
    else
	fsvers = 0;

    /*  Determine the maximum amount of memory this file may use.
     */
    DBGputfile( ("..putFileFontSummary(): max length "));

    if (LoadString(hModule, FSUM_MEMLIMIT, buf, sizeof(buf)))
	{
	DBGputfile((" (Getting max size from WIN.INI) "));
	if ((maxMem=(DWORD)GetProfileInt(appName,buf,MAXFILE_MEM)) <= 0)
	    {
	    DBGputfile(
		(" -- user requests zero size file\n"));
	    goto backout0;
	    }

	if (maxMem > MAXFILE_MAXMEM)
	    maxMem = MAXFILE_MAXMEM;
	}
    else
	{
	DBGputfile((" (MAXFILE_MEM)"));
	maxMem = MAXFILE_MEM;
	}

    maxMem = lmul(maxMem, (long)1024);

    DBGputfile(("approx. %ld bytes\n", maxMem));

    /*  Prepare to open fontSummary file.
     */
    lmemset(fsumFileName, 0, sizeof(fsumFileName));
    lmemset((LPSTR) &fsumFileHeader, 0, sizeof(FSUMFILEHEADER));

    if (lstrlen(VNumStr) >= sizeof(version))
	lmemset(version, 0, sizeof(version));
    else
	lstrcpy(version, VNumStr);

    //  Get the name of the fontSummary file from the win.ini file.
    //  (fsumName is the item name in win.ini; fsumFileName is the
    //  filename which follows it).

    if (!(LoadString(hModule, FSUM_NAME, fsumName, sizeof(fsumName)) &&
	  GetProfileString(appName, fsumName, fsumFileName,
		fsumFileName, sizeof(fsumFileName)) ) )
	{
	/*  Failed to get file name, construct a name.
         */
	bldFSumFileName(fsumFileName, lpPortNm, hModule);

	DBGputfile(("putFileFontSummary(): built file name %ls\n",
	    (LPSTR)fsumFileName));
	}

    DBGputfile(("putFileFontSummary(): %ls=%ls\n",
	(LPSTR)fsumName, (LPSTR)fsumFileName));

    /*  Check to see if the fontSummary file already exists.
     */
    if ((hFile = _lopenp(fsumFileName, OF_READ)) > 0)
	{
	DBGputfile(("putFileFontSummary(): %ls open for read, hFile=%d\n",
	    (LPSTR)fsumFileName, hFile));

	if ((err = _lread(hFile, (LPSTR) &fsumFileHeader,
	    sizeof(FSUMFILEHEADER))) && (err == sizeof(FSUMFILEHEADER)))
	    {
	    /*  Successfully read header.
             */
	    _lclose (hFile);
	    hFile = 0;
	    DBGputfile(
	      ("                      ...%ls closed\n", (LPSTR)fsumFileName));

	    if ((lstrcmpi(version, fsumFileHeader.version) == 0) &&
		(fsvers == fsumFileHeader.fsvers) &&
		(lstrcmpi(lpPortNm, fsumFileHeader.port) == 0) &&
		(fsumFileHeader.envSize == sizeof(PCLDEVMODE)) &&
		(fsumFileHeader.softfonts == lpFontSummary->softfonts))
		{
		/*  We can use this file, so reopen it read/write.
                 */
		DBGputfile(
		  ("putFileFontSummary(): %ls is a valid fontSummary file\n",
		    (LPSTR)fsumFileName));
		hFile = _lopenp(fsumFileName, OF_READWRITE);

		#ifdef LOCAL_DEBUG
		if (hFile > 0) {
		    DBGputfile(
	    ("                      ...%ls reopened readwrite, hFile=%d\n",
			(LPSTR)fsumFileName, hFile));
		} else {
		    DBGputfile(
	("                      ...failed to reopen %ls readwrite, hFile=%d\n",
			(LPSTR)fsumFileName, hFile));
		}
		#endif
		}
	    #ifdef LOCAL_DEBUG
	    else
		{
		DBGputfile(("putFileFontSummary(): %ls header does not match\n",
		    (LPSTR)fsumFileName));
		}
	    #endif
	    }
	else
	    {
	    DBGputfile(("putFileFontSummary(): could not read %ls\n",
		(LPSTR)fsumFileName));
	    _lclose (hFile);
	    hFile = 0;
	    }
	}

    /*  If we failed to open an existing file, create a new one.
     */
    if (hFile <= 0)
	{
	lmemset((LPSTR) &fsumFileHeader, 0, sizeof(FSUMFILEHEADER));

	if ((hFile = _lopenp(fsumFileName, OF_CREATE | OF_READWRITE)) > 0)
	    {
	    DBGputfile(
	    ("putFileFontSummary(): %ls created for readwrite, hFile=%d\n",
		(LPSTR)fsumFileName, hFile));

	    /*  Fill in header.
             */
	    if (lstrlen(VNumStr) < sizeof(fsumFileHeader.version))
		lstrcpy(fsumFileHeader.version, VNumStr);
	    if (lstrlen(lpPortNm) < sizeof(fsumFileHeader.port))
		lstrcpy(fsumFileHeader.port, lpPortNm);
	    fsumFileHeader.fsvers = fsvers;
	    fsumFileHeader.envSize = sizeof(PCLDEVMODE);
	    fsumFileHeader.softfonts = lpFontSummary->softfonts;

	    fsumFileHeader.offsetFirstEntry +=
		_lwrite(hFile, (LPSTR) &fsumFileHeader, sizeof(FSUMFILEHEADER));

	    /*  Write a little message at the top of the file for
             *  curious users who open it.
             */
	    for (ind = 0; (ind < 9) && LoadString(hModule,
		FSUM_MESSAGE+ind, buf, sizeof(buf)); ++ind)
		{
		fsumFileHeader.offsetFirstEntry +=
		    _lwrite(hFile, buf, lstrlen(buf));
		}
	    }
	else
	    {
	    DBGerr(("putFileFontSummary(): %ls could not be created\n",
		(LPSTR)fsumFileName));
	    goto backout0;
	    }
	}

    DBGputfile(
    ("putFileFontSummary(): version=%ls, port=%ls, envSize=%d, softfonts=%d\n",
	(LPSTR) fsumFileHeader.version, (LPSTR) fsumFileHeader.port,
	fsumFileHeader.envSize, fsumFileHeader.softfonts));
    DBGputfile(
    ("                      fsvers=%d, numEntries=%d, offsetFirstEntry=%ld\n",
	fsumFileHeader.fsvers, fsumFileHeader.numEntries,
	fsumFileHeader.offsetFirstEntry));

    if (GlobalSize(hFontSummary) > maxMem)
	maxMem = 0L;
    else
	maxMem -= GlobalSize(hFontSummary);

    DBGputfile(("putFileFontSummary(): adjusted maxMem=%ld bytes\n", maxMem));

    /*  Search to the end of the file or to the size limit.
     */
    for (count = 0, seek = fsumFileHeader.offsetFirstEntry;
	(_llseek(hFile, seek, 0) == seek) &&
	(count < fsumFileHeader.numEntries) &&
	(err = _lread(hFile, (LPSTR) &fsumFileEntry, sizeof(FSUMFILEENTRY))) &&
	(err == sizeof(FSUMFILEENTRY)) &&
	(seek +sizeof(FSUMFILEENTRY) +fsumFileEntry.sizeofFontSum < maxMem);
	++count, seek += sizeof(FSUMFILEENTRY)+fsumFileEntry.sizeofFontSum)
	;

    #ifdef LOCAL_DEBUG
    if (count < fsumFileHeader.numEntries) {
	DBGputfile(("putFileFontSummary(): %d entries will be deleted\n",
	    (fsumFileHeader.numEntries - count)));
	}
    #endif

    DBGputfile(
    ("putFileFontSummary(): writing new fontSummary at %d\n", count));

    /*  Write out the new fontSummary struct and truncate the file.
     */
    lmemcpy((LPSTR) &fsumFileEntry.environ, (LPSTR) lpEnviron,
	    sizeof(PCLDEVMODE));

    fsumFileEntry.sizeofFontSum = (WORD)GlobalSize(hFontSummary);
    _llseek(hFile, seek, 0);
    _lwrite(hFile, (LPSTR) &fsumFileEntry, sizeof(FSUMFILEENTRY));
    _lwrite(hFile, (LPSTR)lpFontSummary, fsumFileEntry.sizeofFontSum);
    _lwrite(hFile, (LPSTR)NullStr, 0); /* truncate file */

    /*  Rewrite header to set the count correctly.
     */
    fsumFileHeader.numEntries = count + 1;
    _llseek(hFile, 0L, 0);
    _lwrite(hFile, (LPSTR) &fsumFileHeader, sizeof(FSUMFILEHEADER));

    if (count > 0)
	{
	/*  Move the new entry to to top of the file -- fontSummary
         *  entries are deleted via LRU (least-recently-used) method.
         */
	DBGputfile(("putFileFontSummary(): shuffling new fontSummary to 0\n"));

	_lshuffle(hFile, fsumFileHeader.offsetFirstEntry, seek,
	    sizeof(FSUMFILEENTRY)+fsumFileEntry.sizeofFontSum);
	}

    /*  Write the file name to the win.ini file.
     */
    if (lstrlen(appName) && lstrlen(fsumName) && lstrlen(fsumFileName))
	WriteProfileString(appName, fsumName, fsumFileName);

    /*  Close up shop.
     */
    _lclose(hFile);
backout0:
    GlobalUnlock(hFontSummary);
    }

/*  SameEnvironments
 *
 *  Compare those fields of the PCLDEVMODE struct which could affect font
 *  information -- return TRUE if they are identical.
 */
LOCAL BOOL
SameEnvironments(LPPCLDEVMODE lpEnvA, LPPCLDEVMODE lpEnvB) {

    BOOL same;

    DBGentry(("SameEnvironments(%lp,%lp): same=", lpEnvA, lpEnvB));

    same = (!lstrcmpi((LPSTR)lpEnvA->dm.dmDeviceName,
		     (LPSTR)lpEnvB->dm.dmDeviceName) &&
	   (lpEnvA->dm.dmOrientation == lpEnvB->dm.dmOrientation) &&
	   (lpEnvA->prtIndex == lpEnvB->prtIndex) &&
	   (lpEnvA->romind == lpEnvB->romind) &&
	   (lpEnvA->romcount == lpEnvB->romcount) &&
	   (lpEnvA->numCartridges == lpEnvB->numCartridges) &&
	   (mylmemcmp((LPSTR)lpEnvA->cartIndex,
		      (LPSTR)lpEnvB->cartIndex, DEVMODE_MAXCART*2)) &&
	   (mylmemcmp((LPSTR)lpEnvA->cartind,
		      (LPSTR)lpEnvB->cartind, DEVMODE_MAXCART*2)) &&
	   (mylmemcmp((LPSTR)lpEnvA->cartcount,
		      (LPSTR)lpEnvB->cartcount, DEVMODE_MAXCART*2)) &&
	   (lpEnvA->prtCaps == lpEnvB->prtCaps) &&
	   (lpEnvA->options == lpEnvB->options) &&
	   (lpEnvA->fsvers == lpEnvB->fsvers));

    #ifdef LOCAL_DEBUG
    if (same) {
	DBGentry(("TRUE\n"));
    } else {
	DBGentry(("FALSE\n"));
    }
    #endif

    return (same);
}

/*  mylmemcmp
 *
 *  Memcmp function.
 */
LOCAL BOOL mylmemcmp(a, b, len)
    LPSTR a;
    LPSTR b;
    short len;
    {
/*  DBGentry(("mylmemcmp(%lp,%lp,%d)\n", a, b, len));
 */

    while (len-- > 0)
	{
	if (*a++ != *b++)
	    return FALSE;
	}

    return TRUE;
    }

/*  bldFSumFileName
 *
 *  Build a file name for the file used to store the fontSummary struct.
 *  The file name consists of prefix+port+extension.
 */
LOCAL void bldFSumFileName(lpFSumFileName, lpPortNm, hModule)
    LPSTR lpFSumFileName;
    LPSTR lpPortNm;
    HANDLE hModule;
    {
    LPSTR s;
    char tempfile[32];

    /*  Load the file prefix from the resource file.
     */
    if (!LoadString(hModule, FSUM_FILEPREFIX, tempfile, 5))
	{
	DBGerr(("bldFSumFileName(): could not load file prefix\n"));
	lstrcpy(tempfile, "FS");
	}

    /*  Concat the port name to the file prefix.
     */
    if (!lpPortNm || !lstrlen(lpPortNm))
	{
	lstrcat(tempfile, "NONE");
	}
    else
	{
	s = lpPortNm + lstrlen(lpPortNm);

	/*  First get the filename part of the port (i.e., strip
         *  off any path names.
         */
	if (s[-1] == ':')
	    --s;
	for (; s > lpPortNm && s[-1] != ':' && s[-1] != '\\'; --s)
	    ;
	lstrncpy((LPSTR) &tempfile[lstrlen(tempfile)], s, 8);
//	lmemcpy((LPSTR) &tempfile[lstrlen(tempfile)], s, 8);
	tempfile[8] = '\0';

	/*  Truncate the name at any invalid file-name characters.
         */
	for (s = (LPSTR)tempfile + lstrlen(tempfile) - 1;
	    s > (LPSTR)tempfile; --s)
	    {
	    if (*s == ':' || *s == '\\' || *s == '.')
		*s = '\0';
	    }
	}
    tempfile[8] = '\0';

    if (!lstrlen(tempfile))
	{
	DBGerr(("bldFSumFileName(): screwed up building file name\n"));
	lstrcpy(tempfile, "FSNONE");
	}

    /*  Add extension to file name.
     */
    lstrcat(tempfile, ".");
    if (!LoadString(hModule, FSUM_FILEEXTENSION,
	    (LPSTR) &tempfile[lstrlen(tempfile)], 4))
	{
	DBGerr(("bldFSumFileName(): could not load file extension\n"));
	lstrcpy(tempfile, PclStr);
	}

#if USEWINDOWSDIRECTORY
    /*  Use the Windows directory path.
     */
    if (GetWindowsDirectory(lpFSumFileName, FSUM_FNMLEN))
	{
	s = lpFSumFileName + lstrlen(lpFSumFileName) - 1;
        if (*s++ != '\\')
            *s++ = '\\';
#else
    /*  Use the same path the driver file is in.
     */
    if (GetModuleFileName(hModule, lpFSumFileName, FSUM_FNMLEN))
	{
	for (s = lpFSumFileName + lstrlen(lpFSumFileName);
	    (s > lpFSumFileName) && (s[-1] != ':') && (s[-1] != '\\');
	    --s)
	    ;
#endif

	if (lstrlen(tempfile) < FSUM_FNMLEN - lstrlen(lpFSumFileName))
	    lstrcpy(s, tempfile);
	else
	    lstrcpy(lpFSumFileName, tempfile);
	}
    else
	{
	DBGerr(("bldFSumFileName(): could not get module file name\n"));
	lstrcpy(lpFSumFileName, tempfile);
	}
    }

/*  _lshuffle
 *
 *  Shuffle sizeofBuf bytes from sourcepos to destpos in the file
 *  pointed to by hFile.
 */
LOCAL void _lshuffle(hFile, destpos, sourcepos, bufSize)
    int hFile;
    DWORD destpos;
    DWORD sourcepos;
    WORD bufSize;
    {
    HANDLE hBuf;
    LPSTR lpSourceBuf, lpTransBuf;
    DWORD filepos, prevpos;
    WORD sizeofSourceBuf, sizeofTransBuf, spaceRemaining;

    DBGentry(("_lshuffle(%d,%ld,%ld,%d)\n",
	hFile, destpos, sourcepos, bufSize));

    if (destpos >= sourcepos)
	{
	DBGerr(("_lshuffle(): destpos >= sourcepos, *no* shuffle\n"));
	return;
	}

    if (!(hBuf = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(2*MAX_BUFSIZE))))
	{
	DBGerr(("_lshuffle(): Could *not* alloc buffer\n"));
	return;
	}

    if (!(lpSourceBuf = GlobalLock(hBuf)))
	{
	DBGerr(("_lshuffle(): Could *not* lock buffer\n"));
	GlobalFree(hBuf);
	return;
	}

    lpTransBuf = lpSourceBuf + MAX_BUFSIZE;

    do {
	/*  Set up size of buffer.
         */
	if (bufSize > MAX_BUFSIZE)
	    {
	    sizeofSourceBuf = sizeofTransBuf = MAX_BUFSIZE;
	    bufSize -= MAX_BUFSIZE;
	    }
	else
	    {
	    sizeofSourceBuf = sizeofTransBuf = bufSize;
	    bufSize = 0;
	    }

	/*  Read the source buffer.
         */
	_llseek(hFile, prevpos=filepos=sourcepos, 0);
	_lread(hFile, lpSourceBuf, sizeofSourceBuf);

	/*  Shift all the bytes at the destination to the source.
         */
	do {
	    if ((filepos - destpos) < (DWORD)sizeofTransBuf)
		{
		spaceRemaining = (WORD)(filepos - destpos);
		prevpos += (DWORD)(sizeofTransBuf - spaceRemaining);
		sizeofTransBuf = spaceRemaining;
		}
	    filepos -= sizeofTransBuf;

	    _llseek(hFile, filepos, 0);
	    _lread(hFile, lpTransBuf, sizeofTransBuf);
	    _llseek(hFile, prevpos, 0);
	    _lwrite(hFile, lpTransBuf, sizeofTransBuf);

	    } while ((prevpos=filepos) > destpos);

	/*  Write the source buffer at the destination.
         */
	_llseek(hFile, destpos, 0);
	_lwrite(hFile, lpSourceBuf, sizeofSourceBuf);

	/*  Update positions in case we loop.
         */
	destpos += sizeofSourceBuf;
	sourcepos += sizeofSourceBuf;

	} while (bufSize > 0);

    GlobalUnlock(hBuf);
    GlobalFree(hBuf);
    }
