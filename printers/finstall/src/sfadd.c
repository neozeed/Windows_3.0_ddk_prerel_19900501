/**[f******************************************************************
 * sfadd.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/********************************   sfadd.c   ******************************/
//
//  SFadd:  Module for adding fonts.
//
/***************************************************************************/
//
//  History
//
// 29 nov 89	peterbe		Changed MB_ICONQUESTION to MB_ICONEXCLAMATION
// 22 nov 89	peterbe		Use status line to indicate entering filename
//				for directory, etc. in SearchDisk().
// 09 nov 89	peterbe		Can read (by default) either FINSTALL.DIR or
//				SFINSTAL.DIR. In Smart mode, any file name may
//				be used.  Also, in Smart mode, only the
//				selected directory file name is used.
//
// 28 sep 89	peterbe		In AddFontsMode(), send WM_SETREDRAW messages
//				to disable/enable redraw of window while
//				searching disk.
//				Also, turn cursor to hourglass while waiting.
// 27 sep 89	peterbe		In MergePath(), don't append backslash to path
//				which is just a drive spec.
// 20 sep 89	peterbe		Just added some DBG messages().
// 03 aug 89	peterbe		Modify so only cartridge fonts are displayed
//				if (gNoSoftFonts != FALSE) -- see isDLfile().
// 01 aug 89	peterbe		Adding calls to LZEXPAND.MOD.
//				Use 'hsrcFile' for source file handle name
//				whenever possible.
//
//   1-26-89    jimmat  Adjustments for changes in resource file.
//

#ifdef SFDEBUG
#define DEBUG
#endif

#include "nocrap.h"
#undef NOOPENFILE
#undef NOMSG
#undef NOCTLMGR
#undef NOWINMESSAGES
#undef NOSCROLL
#undef NOMEMMGR
#undef NOMB
#include "windows.h"
#include "neededh.h"
#include "resource.h"
#include "strings.h"
#include "pfm.h"
#include "sfadd.h"
#include "sfdir.h"
#include "sflb.h"
#include "sfpfm.h"
#include "sffile.h"
#define NOBLDDESCSTR
#include "sfutils.h"
#include "sfinstal.h"
#include "dlgutils.h"
#include "dosutils.h"
#include "fntutils.h"
#include "expand.h"


/*  DEBUG switches
 */
#ifdef DEBUG
#define DBGmergepath(msg)	DBMSG(msg)
#define DBGx(msg)	 	DBMSG(msg)
#define DBGdlgfn(msg)    	DBMSG(msg)
#else
#define DBGmergepath(msg)	/*nulla*/
#define DBGx(msg)		/*nada*/
#define DBGdlgfn(msg)	    	/*nechto*/
#endif


#define LOCAL static


/*  Structure used by SearchDisk().
 */
typedef struct {
    SFDIRFILE SFfile;		    /* MUST be first */
    char more_s[128];		    /* MUST be second */
    char file[128];		    /* MUST be third */
    PFMHEADER pfmHead;
    EXTTEXTMETRIC extText;
    OFSTRUCT ofstruct;
    WORD state;
    char point[32];
    char bold[32];
    char italic[32];
    char scan[32];
    char buf[512];
    } SRCHREC;
typedef SRCHREC FAR *LPSRCHREC;

/*  Forward references
 */
int FAR PASCAL AddDlgFn(HWND, unsigned, WORD, LONG);
int FAR PASCAL tDirDlgFn(HWND, unsigned, WORD, LONG);
LOCAL void ErrNoFile(HWND, HANDLE, WORD, LPSTR, int);
LOCAL HANDLE SearchDisk(HWND, HANDLE, WORD, WORD FAR *);
LOCAL int isDLfile(HANDLE, LPSRCHREC);
LOCAL int isPCMFile(LPSRCHREC);
LOCAL BOOL VerifyDir(HWND, HANDLE, LPSTR, int);
LOCAL BOOL createDir(LPSTR);

extern BOOL gNoSoftFonts;	// Indicates only cartridge fonts

LOCAL HANDLE gHMd = 0;
LOCAL char gPath[64];
LOCAL char gDirFile[64];	// 3.0 "FINSTALL.DIR"
LOCAL char gDirFileOld[64];	// 2.0 "SFINSTAL.DIR"
LOCAL LPSTR lpDirFile;		// points to name of file we actually opened
LOCAL char gTmpFile[64];
LOCAL BOOL gReportErr;
LOCAL BOOL gSmartMode;

/**************************************************************************/
/****************************   Global Procs   ****************************/


/*  AddFontsMode
 */
HANDLE FAR PASCAL AddFontsMode(hDB, hMd, idLB, lpCount, smartMode)
    HWND hDB;
    HANDLE hMd;
    WORD idLB;
    WORD FAR *lpCount;
    BOOL smartMode;
    {
    FARPROC lpDlgFunc;
    HANDLE hSFlb = 0;
    int response;

    DBGdlgfn(("AddFontsMode(%d,%d,%d,%d)\n",
	(WORD)hDB, (WORD)hMd, idLB, smartMode));

    /*  Assign globals.
     */
    gHMd = hMd;
    gSmartMode = smartMode;
    gReportErr = smartMode;
    *lpCount = 0;

    /*  Pick up default strings.
     */
    if (!LoadString(hMd, SFADD_DEFPATH, gPath, sizeof(gPath)) ||
	!LoadString(hMd, SFADD_DEFDIRF, gDirFile, sizeof(gDirFile)) ||
	!LoadString(hMd, SFADD_OLDDIRF, gDirFileOld, sizeof(gDirFileOld)) ||
	!LoadString(hMd, SFADD_DEFERRF, gTmpFile, sizeof(gTmpFile)))
	{
	return (0);
	}

    /*  Pull up "add fonts" dialog and find out from where we're
     *  supposed to read the fonts.
     */
    response = MyDialogBox(hMd,smartMode ? SMARTSFADD : SFADDFONT,
			   hDB, AddDlgFn);

    if (response == IDOK)
	{
	OFSTRUCT ofStruct;
	int hsrcFile = -1;
	int k = lstrlen(gTmpFile) + 1;

	DBGx(( (gSmartMode) ?
		"AddFontsMode(): Try to open <%ls>\n" :
		"AddFontsMode(): Try to open <%ls> or <%ls>)\n",
		(LPSTR)gDirFile, (LPSTR)gDirFileOld));

	// try to open FINSTALL.DIR
	if ((hsrcFile=lzOpenFile(gDirFile,&ofStruct,OF_READ)) != -1)
	    {	// we opened FINSTALL.DIR, set pointer to filename
	    lpDirFile = (LPSTR) gDirFile;
	    DBGx((".. Opened %ls\n", (LPSTR)gDirFile));
	    }
	else if (!gSmartMode)
	    { // failing, try to open SFINSTAL.DIR (unless smart mode)
	    hsrcFile=lzOpenFile(gDirFileOld,&ofStruct,OF_READ);
	    if (hsrcFile != -1)
		{	// we opened SFINSTAL.DIR, set pointer to filename
		lpDirFile = (LPSTR)  gDirFileOld;
		DBGx((".. Opened the old 2.0 SFINSTAL.DIR\n"));
		}
	    }

	// now we check for a *.LBL file on the diskette..

	if ((hsrcFile == -1) &&					// no .DIR
	    existLBL(hMd,-1,&gTmpFile[k],sizeof(gTmpFile)-k))	// has .LBL
	    {
	    //  There was a .LBL file on this diskette, but FINSTALL.DIR
	    //  (or SFINSTAL.DIR) wasn't there, so this must not be the
	    //  first disk.  We need the user to insert another disk.
	    //  Loop until we successfully read the *INSTAL*.DIR file or
            //  the user cancels.
            //
	    while (TRUE)
		{
		response = MyDialogBox(hMd,NOSFDIRFILE,hDB,GenericWndProc);

		if (response == IDOK)
		    {
		    // try to find FINSTALL.DIR -- break out if we find it.
		    if ((hsrcFile=lzOpenFile(gDirFile,&ofStruct,OF_READ)) != -1)
			{
			DBGx((".. Found %ls here.\n", (LPSTR)gDirFile));
			lpDirFile = (LPSTR) gDirFile;
			break;
			}
		    // we failed, so we try to find the old SFINSTAL.DIR --
		    // break out if we find THAT instead.
		    if (!gSmartMode)	// only 1 filename in 'smart' mode
			{
			hsrcFile= lzOpenFile(gDirFileOld,&ofStruct,OF_READ);
			if (hsrcFile != -1)
			    {
			    DBGx((".. Found the OLD 2.0 SFINSTAL.DIR here.\n"));
			    lpDirFile = (LPSTR) gDirFileOld;
			    break;
			    }
			}
		    }
		else if (response == IDCANCEL)
		    return (0);			// we give up.
		else
		    break;			// just search for fonts.
		}
	    }

	// disable redraw and turn cursor to hourglass while updating

	SendMessage(GetDlgItem(hDB,idLB), WM_SETREDRAW, FALSE, 0L);
	SetCursor(LoadCursor(NULL,IDC_WAIT));

	if (hsrcFile != -1)	// Either FINSTALL.DIR or SFINSTALL.DIR
	    {
	    hSFlb = LoadSFdirFile(hDB, hMd, idLB, hsrcFile, lpDirFile,
		gTmpFile, gReportErr, lpCount);
	    DBGx(("AddFontsMode(): lzClose(%d)", hsrcFile));
	    lzClose(hsrcFile);
	    DBGx(("AddFontsMode(): .. Closed\n"));
	    hsrcFile = -1;
	    }
	else
	   {
	    /*  Search the disk for downloadable files.
             */
	    hSFlb = SearchDisk(hDB, hMd, idLB, lpCount);
	    }

	if (hSFlb)
	    {
	    /*  Put path at top of listbox.  Precede the path with
             *  the word "Drive " -- there is a keyboard speedup in
             *  one of the letters that makes it unique with the rest
             *  of the speedups in the installer dialog.
             */
	    if (LoadString(hMd,SFADD_DRIVETEXT,gTmpFile,sizeof(gTmpFile)))
		{
		int i = lstrlen(gTmpFile);
		lmemcpy(&gTmpFile[i], gPath, sizeof(gTmpFile)-i);
		gTmpFile[sizeof(gTmpFile)-1] = '\0';
		SetDlgItemText(hDB, SF_PRINTER_RIGHT, trimLBcaption(gTmpFile));
		}
	    else
		SetDlgItemText(hDB, SF_PRINTER_RIGHT, gPath);

	    /*  Use gTmpFile as a general buffer.
             */
	    if (!LoadString(hMd,SFADD_CLOSESTR,gTmpFile,sizeof(gTmpFile)))
		gTmpFile[0] = '\0';

	    /*  Change button to "Close drive."
             */
	    SetDlgItemText(hDB, SF_ADD_RIGHT, gTmpFile);

	    if (!LoadString(hMd,SFADD_ADDTEXT,gTmpFile,sizeof(gTmpFile)))
		gTmpFile[0] = '\0';

	    /*  Change move button to "Add."
             */
	    SetDlgItemText(hDB, SF_MOVE, gTmpFile);

	    /*  Shift contents to top and enable listbox.
             */
	    DBGx(("AddFontsMode(): .. setting scroll to top\n"));
	    //SendMessage(GetDlgItem(hDB,idLB), WM_SETREDRAW, FALSE, 0L);
	    SendMessage(GetDlgItem(hDB,idLB), WM_VSCROLL, SB_TOP, 0L);
	    EnableWindow(GetDlgItem(hDB, idLB), TRUE);
	    //SendMessage(GetDlgItem(hDB,idLB), WM_SETREDRAW, TRUE, 0L);
	    //InvalidateRect(GetDlgItem(hDB,idLB), (LPRECT)0L, FALSE);
	    }
	}
    #ifdef DEBUG
    if (hSFlb)
	{
	DBGdumpSFbuf(0L);
	}
    #endif

    // make new stuff visible and restore cursor to pointer
    SetCursor(LoadCursor(NULL,IDC_ARROW));
    SendMessage(GetDlgItem(hDB,idLB), WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(GetDlgItem(hDB,idLB), (LPRECT)0L, FALSE);

    return (hSFlb);

    }	// AddFontsMode()

/*  EndAddFontsMode
 */
HANDLE FAR PASCAL EndAddFontsMode(hDB, hMd, hSFlb, idLB)
    HWND hDB;
    HANDLE hMd;
    HANDLE hSFlb;
    WORD idLB;
    {
    LPSFLB lpSFlb = 0L;
    int ind;

    /*  Roll through list deleting the entries from the SF directory
     *  (if an entry was copied, it would have more than one owner so
     *  it will not actually be deleted).
     */
    if (hSFlb && (lpSFlb = (LPSFLB)GlobalLock(hSFlb)))
	{
	for (ind=0; ind < lpSFlb->free; ++ind)
	    {
	    delSFdirEntry(0L, lpSFlb->sflb[ind].indSFfile);
	    }
	lpSFlb->free = 0;

	GlobalUnlock(hSFlb);
	lpSFlb = 0L;
	}

    /*  Free up the listbox struct.
     */
    if (hSFlb)
	{
	GlobalFree(hSFlb);
	hSFlb = 0;
	}

    /*  Remove path at top of listbox.
     */
    SetDlgItemText(hDB, SF_PRINTER_RIGHT, (LPSTR)"");

    /*  Use gTmpFile as a general buffer.
     */
    if (!LoadString(hMd,SFADD_ADDSTR,gTmpFile,sizeof(gTmpFile)))
	gTmpFile[0] = '\0';

    /*  Change button back to "Add fonts."
     */
    SetDlgItemText(hDB, SF_ADD_RIGHT, gTmpFile);

    if (!LoadString(hMd,SFADD_MOVETEXT,gTmpFile,sizeof(gTmpFile)))
	gTmpFile[0] = '\0';

    /*  Change move button back to "Move."
     */
    SetDlgItemText(hDB, SF_MOVE, gTmpFile);

    /*  Clear and disable listbox.
     */
    SendDlgItemMessage(hDB, idLB, LB_RESETCONTENT, 0, 0L);
    EnableWindow(GetDlgItem(hDB, idLB), FALSE);

    return (hSFlb);

    }	// EndAddFontsMode()

/*  MergePath
 *
 *  Merge the path/drive from which we're supposed to read fonts with
 *  a file name.
 */
BOOL FAR PASCAL MergePath(lpPath, lpFile, fbufsz, stripPath)
    LPSTR lpPath;
    LPSTR lpFile;
    int fbufsz;
    BOOL stripPath;
    {
    LPSTR s, t;
    int j, k;

    /*  Use global path if no path provided.
     */
    if (!lpPath)
	lpPath = (LPSTR)gPath;

    /*  Make sure there is a colon in the path.  Otherwise fail.
     */
    for (s = (LPSTR)lpPath + lstrlen(lpPath);
	(s > (LPSTR)lpPath) && (s[-1] != ':'); --s)
	;
    if (s == (LPSTR)lpPath)
	return FALSE;

    //  Make sure the path ends with a slash, unless it's just a
    //  drive spec.
    //
    {
    char ch;

    if (((ch = lpPath[lstrlen(lpPath)-1]) != '\\') && (ch != ':'))
	lstrcat(lpPath, (LPSTR)"\\");
    }

    /*  Strip the path or at least the drive off the file spec.
     */
    if (stripPath)
	{
	/*  Step to the beginning of the file name.
         */
	for (t = lpFile + lstrlen(lpFile);
	    (t > lpFile) && (t[-1] != ':') && (t[-1] != '\\'); --t)
	    ;
	}
    else
	{
	/*  Step to the beginning of the path.
         */
	for (t = lpFile + lstrlen(lpFile);
	    (t > lpFile) && (t[-1] != ':'); --t)
	    ;
	if (*t == '\\')
	    ++t;
	}
    j = lstrlen(lpPath);
    k = (int)(t - lpFile);

    if (k < j)
	{
	/*  Push the file name to the right to make room
         *  for the path.
         */
	if ((j + (k=lstrlen(t)) + 1) > fbufsz)
	    {
	    DBMSG(("MergePath(): not enough room!!!\n"));
	    return FALSE;
	    }

	for (t+=k, s=lpFile+k+j; k >= 0; --k, --s, --t)
	    {
	    *s = *t;
	    }
	}
    else if (k > j)
	{
	/*  Pull the filename to the left so there won't
         *  be any gaps between it and the path.
         */
	lstrcpy(&lpFile[j], t);
	}

    /*  Insert the path with the rest of the file name.
     */
    lmemcpy(lpFile, lpPath, j);

    //DBGx(("MergePath(): ..lpFile is %ls\n", lpFile));

    return TRUE;
    }	// MergePath()

/*  GetTargDir
 */
BOOL FAR PASCAL GetTargDir(hDB, hMd, lpBuf, bufsz, lpAppNm)
    HWND hDB;
    HANDLE hMd;
    LPSTR lpBuf;
    int bufsz;
    LPSTR lpAppNm;
    {
    FARPROC lpDlgFunc;
    int response;

    DBGdlgfn(("GetTargDir(%d,%d,%lp,%d,%lp): %ls\n",
	hDB, hMd, lpBuf, bufsz, lpAppNm, lpAppNm));

    gTmpFile[0] = '\0';

    /*  Read target directory from win.ini file.
     */
    LoadString(hMd, SFADD_DIRKEYNM, lpBuf, bufsz);
    GetProfileString(lpAppNm, lpBuf, gTmpFile, gTmpFile, sizeof(gTmpFile));

    /*  Not in win.ini, get default from resources.
     */
    if (gTmpFile[0] == '\0')
	{
	LoadString(gHMd, SFADD_DEFTARG, gTmpFile, sizeof(gTmpFile));
	}

    /*  Prompt for the user to verify/change the target directory.
     */
    gHMd = hMd;
    response = MyDialogBox(hMd,SFTARGDIR,hDB,tDirDlgFn);

    /*  If successful, the return directory is in gTmpFile.
     */
    if (gTmpFile[0] != '\0')
	{
	WriteProfileString(lpAppNm, lpBuf, gTmpFile);
	lmemcpy(lpBuf, gTmpFile, bufsz);
	lpBuf[bufsz-1] = '\0';
	}
    else
	lpBuf[0] = '\0';

    DBGdlgfn(("...end GetTargDir, lpBuf=%ls\n", lpBuf));

    return ((BOOL)*lpBuf);

    }	// GetTargDir()

/*  existLBL
 *
 *  Look for a label file on the target disk.  Return TRUE if one
 *  exists.
 */
BOOL FAR PASCAL existLBL(hMd, indLOGdrv, lpBuf, bufsz)
    HANDLE hMd;
    int indLOGdrv;
    LPSTR lpBuf;
    int bufsz;
    {
    LPSFDIRLOGDRV lpSFdrv = 0L;
    DIRDATA dirdata;

    *lpBuf = '\0';

    if (indLOGdrv > -1 &&
	(lpSFdrv=(LPSFDIRLOGDRV)lockSFdirEntry(0L,indLOGdrv)))
	{
	/*  Pick up a specific logical drive file name out of the
         *  SF directory.
         */
	if (lpSFdrv->offsLabel &&
	    lstrlen(&lpSFdrv->s[lpSFdrv->offsLabel]) < bufsz)
	    {
	    lstrcpy(lpBuf, &lpSFdrv->s[lpSFdrv->offsLabel]);
	    }
	unlockSFdirEntry(indLOGdrv);
	}
    else
	{
	/*  Look for any logical drive (*.LBL) file.
         */
	LoadString(hMd, SFADD_LBLSPEC, lpBuf, bufsz);
	}

    if (*lpBuf)		// if we got the "*.LBL" string
	{		// or there was a label entry in sfdir
	MergePath(0L, lpBuf, bufsz, FALSE);

	if (dos_opend(&dirdata, lpBuf, 0x01) == 0)
	    {
	    return TRUE;
	    }
	}

    return FALSE;

    }	// existLBL()

//  AddDlgFn
//
// This dialog prompts for the path for the font directory FINSTALL.DIR
// or for the soft fonts.
// If Ctrl and Shift are held down when it's evoked, then we get the
// 'Smart' version of the dialog, which allows one to type in the names
// of the directory and error files.
// In Smart mode, we only use the file name that's entered in the
// dialog (see tests on gSmartMode above).
// In normal mode, we search for both FINSTALL.DIR and SFINSTAL.DIR.

BOOL FAR PASCAL AddDlgFn(hDB, wMsg, wParam, lParam)
    HWND hDB;
    unsigned wMsg;
    WORD wParam;
    LONG lParam;
    {
    char szDirFile[64];

    switch (wMsg)
	{
	case WM_INITDIALOG:
	    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): WM_INITDIALOG\n",
		hDB, wMsg, wParam, lParam));
	    CenterDlg(hDB);

	    SetDlgItemText(hDB, SFADD_PATH, gPath);

	    if (gSmartMode)
		{
		// save original for comparison.
		lstrcpy ((LPSTR) szDirFile, (LPSTR) gDirFile);

		SetDlgItemText(hDB, SFADD_DIRFILE, gDirFile);
		SetDlgItemText(hDB, SFADD_ERRFILE, gTmpFile);
		}
	    CheckDlgButton(hDB, SFADD_RPTERR, gReportErr);
	    break;

	case WM_COMMAND:
	    if (HIWORD(lParam) == LBN_ERRSPACE)
		EndDialog(hDB,-1);

	    switch (wParam)
		{
		case SFADD_PATH:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): SFADD_PATH\n",
			hDB, wMsg, wParam, lParam));
		    break;

		case SFADD_DIRFILE:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): SFADD_DIRFILE\n",
			hDB, wMsg, wParam, lParam));
		    break;

		case SFADD_ERRFILE:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): SFADD_ERRFILE\n",
			hDB, wMsg, wParam, lParam));
		    break;

		case SFADD_RPTERR:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): SFADD_RPTERR\n",
			hDB, wMsg, wParam, lParam));
		    CheckDlgButton(hDB, SFADD_RPTERR,
					(gReportErr = !gReportErr));
		    break;

		case IDCANCEL:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): IDCANCEL\n",
				hDB, wMsg, wParam, lParam));
		    EndDialog(hDB, wParam);
		    break;

		case IDOK:
		    DBGdlgfn(("AddDlgFn(%d,%d,%d,%ld): IDOK\n",
				hDB, wMsg, wParam, lParam));

		    GetDlgItemText(hDB, SFADD_PATH, gPath, sizeof(gPath));
		    AnsiUpper(gPath);

		    if (gSmartMode)
			{
			GetDlgItemText(hDB, SFADD_DIRFILE, gDirFile,
					sizeof(gDirFile));
			GetDlgItemText(hDB, SFADD_ERRFILE, gTmpFile,
					sizeof(gTmpFile));
			AnsiUpper(gDirFile);
			AnsiUpper(gTmpFile);
			}

		    if (gDirFile[0] != '\0')
			{
			// merge the path with both filenames
			MergePath(0L, gDirFile, sizeof(gDirFile), TRUE);
			MergePath(0L, gDirFileOld, sizeof(gDirFileOld), TRUE);
			SetDlgItemText(hDB, SFADD_PATH, gPath);
			}

		    if (gSmartMode)
			{
			// put the paths into the dialog
			SetDlgItemText(hDB, SFADD_DIRFILE, gDirFile);
			SetDlgItemText(hDB, SFADD_ERRFILE, gTmpFile);
			}

		    if (gPath[0] == '\0')
			{
			ErrNoFile(hDB, gHMd, SFADD_NOPTHMSG, gTmpFile,
					sizeof(gTmpFile));
			}
		    else if (gSmartMode && gDirFile[0] == '\0')
			{
			ErrNoFile(hDB, gHMd, SFADD_NODIRFMSG, gTmpFile,
					sizeof(gTmpFile));
			}
		    else if (gSmartMode && gReportErr && gTmpFile[0] == '\0')
			{
			ErrNoFile(hDB, gHMd, SFADD_NOERRFMSG, gTmpFile,
					sizeof(gTmpFile));
			}
		    else
			EndDialog(hDB, wParam);
		    break;
		}
	    break;

	default:
	    return FALSE;
	}

    return TRUE;

    }	// AddDlgFn()

/*  tDirDlgFn
 */
BOOL FAR PASCAL tDirDlgFn(hDB, wMsg, wParam, lParam)
    HWND hDB;
    unsigned wMsg;
    WORD wParam;
    LONG lParam;
    {
    switch (wMsg)
	{
	case WM_INITDIALOG:
	    DBGdlgfn(("tDirDlgFn(%d,%d,%d,%ld): WM_INITDIALOG\n",
			hDB, wMsg, wParam, lParam));
	    CenterDlg(hDB);
	    SetDlgItemText(hDB, SFADD_TARGDIR, gTmpFile);
	    gTmpFile[0] = '\0';
	    break;

	case WM_COMMAND:
	    switch (wParam)
		{
		case IDCANCEL:
		    DBGdlgfn(("tDirDlgFn(%d,%d,%d,%ld): IDCANCEL\n",
				hDB, wMsg, wParam, lParam));
		    gTmpFile[0] = '\0';
		    EndDialog(hDB, wParam);
		    break;

		case IDOK:
		    DBGdlgfn(("tDirDlgFn(%d,%d,%d,%ld): IDOK\n",
				hDB, wMsg, wParam, lParam));

		    GetDlgItemText(hDB,SFADD_TARGDIR,gTmpFile,sizeof(gTmpFile));
		    AnsiUpper(gTmpFile);

		    if (VerifyDir(hDB, gHMd, gTmpFile, sizeof(gTmpFile)))
			EndDialog(hDB, wParam);
		    else
			gTmpFile[0] = '\0';
		    break;
		}
	    break;

	default:
	    return FALSE;
	}

    return TRUE;
    }	// tDirDlgFn()

/**************************************************************************/
/*****************************   Local Procs   ****************************/


/*  ErrNoFile
 */
LOCAL void ErrNoFile(hDB, hMd, msgID, lpBuf, bufsz)
    HWND hDB;
    HANDLE hMd;
    WORD msgID;
    LPSTR lpBuf;
    int bufsz;
    {
    int ind;

    if (LoadString(hMd, SFADD_NOPTHCAP, lpBuf, bufsz) &&
	(ind=lstrlen(lpBuf)+1) &&
	LoadString(hMd, msgID, &lpBuf[ind], bufsz-ind))
	{
	MessageBox(hDB, &lpBuf[ind], lpBuf, MB_OK | MB_ICONEXCLAMATION);
	}
    }	// ErrNoFile()

/*  VerifyDir
 */
LOCAL BOOL VerifyDir(hDB, hMd, lpPath, pthsz)
    HWND hDB;
    HANDLE hMd;
    LPSTR lpPath;
    int pthsz;
    {
    LPSTR s;
    DIRDATA dirdata;
    BOOL success = FALSE;
    BOOL validpath = FALSE;
    char buf[80];
    int k, m;

    DBGdlgfn(("VerifyDir(%d,%d,%lp,%d): %ls\n",
	hDB, hMd, lpPath, pthsz, lpPath));

    /*  Test the path name for validity -- should include a drive
     *  id and should not have two backslashes in a row.
     */
    for (s=lpPath; *s && (*s != ':'); ++s)
	;

    if (*s == ':')
	{
	for (s=lpPath; *s; ++s)
	    {
	    if (*s == '\\' && s[1] == '\\')
		break;
	    }

	if (!(*s))
	    validpath = TRUE;
	}

    k = lstrlen(lpPath);

    if (validpath && (k < pthsz - 5))
	{
	/*  Check to see if the directory already exists.
         */
	if (lpPath[k-1] != '\\')
	    lstrcat(lpPath, (LPSTR)"\\");
	lstrcat(lpPath, (LPSTR)"*.*");

	m = dos_opend(&dirdata,lpPath,0x01);

	lpPath[k] = '\0';

	if (m == 0 || m == DOS_NOFILES)
	    {
	    /*  Directory exists.
             */
	    success = TRUE;
	    }
	else
	    {
	    /*  Directory does not exist, ask the user if a new
             *  directory should be created.
             */
	    LoadString(hMd, SFINSTAL_NM, buf, sizeof(buf));
	    k = lstrlen(buf) + 1;
	    LoadString(hMd, SFADD_NEWDIR, &buf[k], sizeof(buf)-k);
	    m = k + lstrlen(&buf[k]);
	    lmemcpy(&buf[m], lpPath, sizeof(buf)-m);
	    buf[sizeof(buf)-1] = '\0';
	    m = k + lstrlen(&buf[k]);
	    LoadString(hMd, SF_QUESTION, &buf[m], sizeof(buf)-m);

	    if ((MessageBox(hDB,&buf[k],buf,
		MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK))
		{
		/*  User clicked "OK" so attempt to make new directory.
                 */
		if (createDir(lpPath))
		    {
		    /*  New directory created.
                     */
		    success = TRUE;
		    }
		else if (LoadString(hMd,SFADD_BADDIRCAP,buf,sizeof(buf)) &&
		    (k=lstrlen(buf)+1) &&
		    LoadString(hMd,SFADD_BADDIRMSG,&buf[k],sizeof(buf)-k))
		    {
		    /*  Failed to create new directory.
                     */
		    MessageBox(hDB, &buf[k], buf, MB_OK | MB_ICONEXCLAMATION);
		    }
		}
	    }
	}
    else if (LoadString(hMd,SFADD_BADDIRCAP,buf,sizeof(buf)) &&
	(k=lstrlen(buf)+1) &&
	LoadString(hMd,SFADD_BADDIRMSG,&buf[k],sizeof(buf)-k))
	{
	/*  No drive id on the path name, err up front.
         */
	MessageBox(hDB, &buf[k], buf, MB_OK | MB_ICONEXCLAMATION);
	}

    return (success);

    }	// VerifyDir()

/*  createDir
 */
LOCAL BOOL createDir(lpPath)
    LPSTR lpPath;
    {
    DIRDATA dirdata;
    LPSTR s = lpPath + lstrlen(lpPath);
    BOOL success = FALSE;
    char tmp[4];
    int result = -1;

    DBGdlgfn(("createDir(%lp): %ls\n", lpPath, lpPath));

    do {
	/*  Step backward in the path name until we find a directory
         *  which exists.
         */
	for (--s; (s > lpPath) && (s[-1] != '\\'); --s)
	    ;

	if (s[-1] == '\\')
	    {
	    /*  Test to see if this directory exists.
             */
	    tmp[0] = s[0];
	    tmp[1] = s[1];
	    tmp[2] = s[2];
	    tmp[3] = s[3];
	    s[0] = '*';
	    s[1] = '.';
	    s[2] = '*';
	    s[3] = '\0';

	    result = dos_opend(&dirdata, lpPath, 0x01);
	    DBGdlgfn(("dos_opend(%ls) return %d\n", lpPath, result));

	    s[0] = tmp[0];
	    s[1] = tmp[1];
	    s[2] = tmp[2];
	    s[3] = tmp[3];
	    }
	else
	    result = 0;

	} while ((result != 0) && (result != DOS_NOFILES) && (s > lpPath));

    if (s[-1] == '\\')
	{
	/*  Step forward through the path making a new directory
         *  wherever necessary to get the desired path.
         */
	do {
	    for (++s; *s && (*s != '\\'); ++s)
		;

	    tmp[0] = *s;
	    *s = '\0';

	    result = dos_mkdir(lpPath);
	    DBGdlgfn(("dos_mkdir(%ls) return %d\n", lpPath, result));

	    *s = tmp[0];

	    } while ((result == 0) && *s);

	if ((result == 0) && !(*s))
	    success = TRUE;
	}

    return (success);

    }	// createDir()

/*  SearchDisk
 *
 *  Roll through all the files on the target disk and assume each is
 *  a Printer Cartridge Metric file or
 *  a downloadable font file.  If it follows the format of a font file,
 *  pick up its name, point size, and orientation and fill in a
 *  SFDIRFILE struct.  Enter the struct into the SF directory and put
 *  the font in the listbox.  If it's a PCM, read the cartridge title.
 *
 *  This routine uses a temporary data structure called SRCHREC.  Sometimes
 *  the structure contains a string, other times it contains the SFDIRFILE
 *  struct -- its meaning changes from line to line.  Each function usually
 *  takes the data it needs from the structure, then writes the results over
 *  top of it.
 */
LOCAL HANDLE SearchDisk(hDB, hMd, idLB, lpCount)
    HWND hDB;
    HANDLE hMd;
    WORD idLB;
    WORD FAR *lpCount;
    {
    MSG msg;
    LPSRCHREC lpBuf = 0L;
    DIRDATA dirdata;
    HANDLE hBuf = 0;
    HANDLE hSFlb = 0;
    BOOL merged = TRUE;
    int tmp;
    int doscode = 0;
#ifdef DEBUG
    int nfiles;

    nfiles = 0;
#endif

    if ((hBuf=GlobalAlloc(GMEM_FIXED,(DWORD)sizeof(SRCHREC))) &&
	(lpBuf=(LPSRCHREC)GlobalLock(hBuf)) &&
	LoadString(hMd, SF_POINT, lpBuf->point, sizeof(lpBuf->point)) &&
	LoadString(hMd, SF_BOLD, lpBuf->bold, sizeof(lpBuf->bold)) &&
	LoadString(hMd, SF_ITALIC, lpBuf->italic, sizeof(lpBuf->italic)) &&
	LoadString(hMd, SFADD_SCAN, lpBuf->scan, sizeof(lpBuf->scan)) &&
	LoadString(hMd, SFADD_ALLFSPEC, lpBuf->file, sizeof(lpBuf->file)) &&
	(merged=MergePath(0L, lpBuf->file, sizeof(lpBuf->file), FALSE)) &&
	((doscode=dos_opend(&dirdata, lpBuf->file, 0x01)) == 0) &&
	(lstrlen(dirdata.name) > 0))
	{
	SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");
	EnableWindow(GetDlgItem(hDB, SF_MOVE), FALSE);
	EnableWindow(GetDlgItem(hDB, SF_COPY), FALSE);
	EnableWindow(GetDlgItem(hDB, SF_ERASE), FALSE);
	EnableWindow(GetDlgItem(hDB, SF_EDIT), FALSE);
	CheckRadioButton(hDB, SF_PERM_LEFT, SF_TEMP_LEFT, 0);
	EnableWindow(GetDlgItem(hDB, SF_PERM_LEFT), FALSE);
	EnableWindow(GetDlgItem(hDB, SF_TEMP_LEFT), FALSE);
	EnableWindow(GetDlgItem(hDB, SF_ADD_RIGHT), FALSE);

	/*  Change exit button to Cancel.
         */
	if (LoadString(hMd,SF_CNCLSTR,lpBuf->buf,sizeof(lpBuf->buf)))
	    {
	    SetDlgItemText(hDB, SF_EXIT, lpBuf->buf);
	    gSF_FLAGS |= SF_NOABORT;
	    }

	*lpCount = 0;

	do {
	    //DBGx(("SearchDisk(): file # %d.", ++nfiles));

	    lstrcpy(lpBuf->file, dirdata.name);
	    //DBGx((".. <%ls>\n", (LPSTR)(lpBuf->file) ));

	    /*  Update dialog status line.
             */
	    lstrcpy(lpBuf->buf, lpBuf->scan);
	    lstrcat(lpBuf->buf, lpBuf->file);
	    SetDlgItemText(hDB, SF_STATUS, lpBuf->buf);

	    /*  Read the file and extract the PFM information if it
             *  is a downloadable font file.
             */

	    if (MergePath(0L,lpBuf->file,sizeof(lpBuf->file),FALSE) &&
		// is it a Cartridge (.PCM) file or a download font file?
		((tmp=isPCMFile(lpBuf)) || (tmp=isDLfile(hMd,lpBuf))) &&
		(tmp=addSFdirEntry(0L,(LPSTR)lpBuf,lpBuf->state,tmp)) &&
		(hSFlb=addSFlistbox(hDB,hSFlb,idLB,-1,tmp,
		lpBuf->SFfile.fIsPCM?SFLB_PERM|SFLB_CART:0,
		    (LPSTR)lpBuf,sizeof(SRCHREC),0L)))
		{
		++(*lpCount);
		}

	    /*  Process any messages to the installer's dialog box
             *  so we can detect the cancel button.
             */
	    while (PeekMessage(&msg, hDB, NULL, NULL, TRUE) &&
		IsDialogMessage(hDB, &msg))
		;

	    // keep the cursor an hourglass
	    SetCursor(LoadCursor(NULL,IDC_WAIT));

	    } while (dos_readd(&dirdata) == 0 && (gSF_FLAGS & SF_NOABORT));


	/*  Restore exit button.
         */
	if (LoadString(hMd,SF_EXITSTR,lpBuf->buf,sizeof(lpBuf->buf)))
	    {
	    SetDlgItemText(hDB, SF_EXIT, lpBuf->buf);
	    gSF_FLAGS &= ~(SF_NOABORT);
	    }

	if (!hSFlb &&
	    LoadString(hMd,SF_NOFNTFOUND,lpBuf->buf,sizeof(lpBuf->buf)))
	    {
	    SetDlgItemText(hDB, SF_STATUS, lpBuf->buf);
	    }
	else
	    SetDlgItemText(hDB, SF_STATUS, (LPSTR)"");

	EnableWindow(GetDlgItem(hDB, SF_ADD_RIGHT), TRUE);
	}
    else if (doscode || !merged)
	{
	if (!merged)
	    {
	    // "Please specify the complete path."
	    if (LoadString(hMd,SFADD_BADDIRMSG,lpBuf->buf,sizeof(lpBuf->buf)))
		SetDlgItemText(hDB, SF_STATUS, lpBuf->buf);

//	    if (LoadString(hMd,SFINSTAL_NM,lpBuf->buf,sizeof(lpBuf->buf)) &&
//		(tmp = lstrlen(lpBuf->buf) + 1) &&
//		LoadString(hMd,SFADD_BADDIRMSG,&lpBuf->buf[tmp],
//				sizeof(lpBuf->buf)-tmp))
//		{
//		MessageBox(hDB, &lpBuf->buf[tmp], lpBuf->buf, MB_OK);
//		}

	    }
	else if (doscode == DOS_NOFILES)
	    {
	    // we found the directory, but "No fonts were found."
	    if (LoadString(hMd,SF_NOFNTFOUND,lpBuf->buf,sizeof(lpBuf->buf)))
		SetDlgItemText(hDB, SF_STATUS, lpBuf->buf);
	    }
	else
	    {
	    // "The drive\directory you specified does not exist or
	    // is not a directory."
	    if (LoadString(hMd,SF_DIRNOTFOUND,lpBuf->buf,sizeof(lpBuf->buf)))
		SetDlgItemText(hDB, SF_STATUS, lpBuf->buf);
	
//	    if (LoadString(hMd,SFINSTAL_NM,lpBuf->buf,sizeof(lpBuf->buf)) &&
//		(tmp = lstrlen(lpBuf->buf) + 1) &&
//		LoadString(hMd,SF_DIRNOTFOUND,&lpBuf->buf[tmp],
//				sizeof(lpBuf->buf)-tmp))
//		{
//		MessageBox(hDB, &lpBuf->buf[tmp], lpBuf->buf, MB_OK);
//		}
	    }
	}

    if (lpBuf)
	{
	GlobalUnlock(hBuf);
	lpBuf = 0L;
	}

    if (hBuf)
	{
	GlobalFree(hBuf);
	hBuf = 0;
	}

    return (hSFlb);

    }	// SearchDisk()

/*
 *  isPCMFile
 *
 *  Fill in the SFDIRFILE struct from a PCM file, if valid, returning the
 *  size thereof.
 */

LOCAL int isPCMFile(LPSRCHREC lpBuf)
{
    LPSFDIRFILE lpSFfile = (LPSFDIRFILE)&lpBuf->SFfile;
    LPSTR lpFile = lpBuf->file;
    LPSTR pch;
    int cch;

    lpBuf->state=SF_FILE|SF_CART;
    lpSFfile->fIsPCM=1;
    lpSFfile->orient=0;
    lpSFfile->indDLpath = lpSFfile->indLOGdrv = lpSFfile->indScrnFnt =-1;
    lpSFfile->offsDLname=0;

    cch=GetCartName(lpFile,lpSFfile->s,sizeof(lpBuf->more_s));

    if (!cch)
	return 0;

    /* search the filename for a path */
    for (pch = lpFile+lstrlen(lpFile);
	pch>lpFile && pch[-1]!=':' && pch[-1]!='\\'; --pch)
	;

    /* if a path is found, insert the path */
    if (pch > lpFile)
	{
	char chSave=*pch;

	*pch=0;

	lpSFfile->indPFMpath =
	    addSFdirEntry(0L,lpFile-2,SF_PATH,lstrlen(lpFile)+3);

	*pch=chSave;
	}
    else
	lpSFfile->indPFMpath = -1;

    /* append the file name to the SFDIRFILE structure */
    lstrcpy(&lpSFfile->s[cch], pch);
    lpSFfile->offsPFMname=cch;

    /* adjust the size of the SFDIRFILE structure */
    cch += lstrlen(pch) + sizeof(SFDIRFILE);

    return cch;

}	// isPCMFile()

//  isDLfile
//
//  Fill in the SFDIRFILE struct (at the top of the SRCHREC struct) by
//  reading the PFM header and EXTTEXTMETRIC information from the download
//  file.  Return the size of the SFDIRFILE struct.
//
//  Always  return 0 if (gNoSoftFonts != FALSE)

LOCAL int isDLfile(hMd, lpBuf)
    HANDLE hMd;
    LPSRCHREC lpBuf;
    {
    LPSFDIRFILE lpSFfile = (LPSFDIRFILE)&lpBuf->SFfile;
    LPPFMHEADER lpPFMhead = (LPPFMHEADER)&lpBuf->pfmHead;
    LPEXTTEXTMETRIC lpExtText = (LPEXTTEXTMETRIC)&lpBuf->extText;
    LPSTR lpFile = lpBuf->file;		// file name
    LPSTR s;
    int hsrcFile = -1;
    int len = 0;

    if (gNoSoftFonts)
	return 0;

    lmemset((LPSTR)&lpBuf->ofstruct, 0, sizeof(OFSTRUCT));

    DBGx(("isDLFile(): lzOpenFile(%ls)\n", (LPSTR)lpFile));
    if ((hsrcFile=lzOpenFile(lpFile,&lpBuf->ofstruct,OF_READ)) != -1)
	{
	DBGx((".. opened, handle = %d\n", hsrcFile));
	if (DLtoPFM(hsrcFile, hMd, TRUE, lpPFMhead, 0L, lpExtText, 0L,
	    lpSFfile->s, sizeof(lpBuf->more_s)+1, lpBuf->buf,
	    sizeof(lpBuf->buf)))
	    {
	    //DBGx((".. DLtoPFM() returned TRUE.\n"));
	    /*  Make sure file name is upper case, for display.
             */
	    AnsiUpper(lpFile);
	    // DBGx((".. AnsiUpper() executed.\n"));

	    /*  Set the state for addSFdirEntry().
             */
	    lpBuf->state = SF_FILE;
	    if (!lpBuf->ofstruct.fFixedDisk)
		lpBuf->state |= SF_MOVEABLE;

	    /*  Pick up orientation.
             */
	    lpSFfile->orient = (BYTE) lpExtText->emOrientation;
	    lpSFfile->fIsPCM=0;

	    //DBGx(("isDLFile(): check face name."));
	    if (lpSFfile->s[0] == '\0')
		{
		// DBGx((".. No face name, use download name"));
		/*  No face name, use the name of the download file.
                 */
		for (s=lpFile+lstrlen(lpFile);
		    s > lpFile && s[-1] != '\\' && s[-1] != ':'; --s)
		    ;
		lmemcpy(lpSFfile->s, s, 13);
		lpSFfile->s[12] = '\0';

		// DBGx((".. lmemcpy() succeeded\n"));
//
//              LoadString(hMd,SF_NODESCSTR,lpSFfile->s,
//			    sizeof(lpBuf->more_s)+1);
//

		}
#ifdef DEBUG
	    //else
		//DBGx((".. Face name OK.\n"));
#endif

	    /*  DLtoPFM placed the face name in lpSFfile->s, now we
             *  must add point size and bold/italic as necessary.
             */
	    // DBGx(("isDLfile(): lstrlen() and makeDesc().\n"));
	    len = lstrlen(lpSFfile->s);
	    makeDesc((LPSTR)lpPFMhead, &lpSFfile->s[len],
		sizeof(lpBuf->more_s) - len + 1,
		lpBuf->point, lpBuf->bold, lpBuf->italic);

	    /*  Pick up the length of the face name string
             *  (including terminating NULL).
             */
	    len = lstrlen(lpSFfile->s) + 1;

	    /*  Step backward through the file name stopping at the end
             *  of the path.
             */
	    // DBGx(("isDLfile(): stripping down to path.\n"));
	    for (s = lpFile + lstrlen(lpFile);
		(s > lpFile) && (s[-1] != ':') && (s[-1] != '\\'); --s)
		;

	    /*  If there is a path, insert it.
             */
	    // DBGx(("isDLfile(): inserting path.\n"));
	    if (s > lpFile)
		{
		/*  Turn the end character into a null.
                 */
		char savec = *s;
		*s = '\0';

		/*  Insert string path name, allow two bytes before the
                 *  string for use by the SF directory utilities and
                 *  one byte at the end for the null-terminator.
                 */
		lpSFfile->indDLpath =
		    addSFdirEntry(0L, lpFile-2, SF_PATH, lstrlen(lpFile)+3);

		*s = savec;
		}
	    else
		lpSFfile->indDLpath = -1;

	    /*  Copy the file name to after the face name in the
             *  SFDIRFILE struct (note that its safe to do a strcpy
             *  because the 'file' field follows the 'more_s' field
             *  in the SRCHREC, so at worse we'll overwrite something
             *  we don't need anymore).
             */
	    lstrcpy(&lpSFfile->s[len], s);
	    lpSFfile->offsDLname = len;

	    /*  Adjust length to reflect size of whole structure
             *  (including terminating NULL on DL file name).
             */
	    len += lstrlen(s) + sizeof(SFDIRFILE);

	    /*  Zero out the rest of the fields.
             */
	    lpSFfile->indLOGdrv = -1;
	    lpSFfile->indScrnFnt = -1;
	    lpSFfile->indPFMpath = -1;
	    lpSFfile->offsPFMname = 0;
	    }

	DBGx(("isDLfile(): lzClose(%d)\n", hsrcFile));
	lzClose(hsrcFile);
	}

    DBGx(("isDLfile(): Exit, return length = %d.\n", len));

    return (len);

    }	// isDLfile()
