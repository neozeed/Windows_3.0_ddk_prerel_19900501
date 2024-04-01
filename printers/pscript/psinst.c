/*--------------------------------------------------------------------------*\
| psinst.c    Windows PostScript external printer installer
|
| History:
| 04/11/89 chrisg    Created
|
\*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*\
|                                                                            |
| i n c l u d e   f i l e s                                                  |
|                                                                            |
\*--------------------------------------------------------------------------*/

#include <windows.h>
#include <winexp.h>
#include "psdata.h"
#include "resource.h"
#include "debug.h"
#include "dmrc.h"


/*--------------------------------------------------------------------------*\
|                                                                            |
| g l o b a l   v a r i a b l e s                                            |
|                                                                            |
\*--------------------------------------------------------------------------*/


#define FILE_LEN	128

LPSTR lpIniFile;
LPSTR lpSrcPath;

char	szNull[] = "";
char	szRes[] = ".WPD";
int	nNewPrinter;		// external printer number of printer added

char    szIniName[] = "printers.ini";
char    def_drive[] = "A:\\";        /* default path in dialog */


/*--------------------------------------------------------------------------*\
|                                                                            |
| f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                            |
\*--------------------------------------------------------------------------*/

LONG PASCAL LZCopy(int doshSource, int doshDest);


WORD    FAR PASCAL GetPrivateProfileInt( LPSTR, LPSTR, int, LPSTR );
int    FAR PASCAL GetPrivateProfileString( LPSTR, LPSTR, LPSTR, LPSTR, int, LPSTR );
BOOL    FAR PASCAL WritePrivateProfileString( LPSTR, LPSTR, LPSTR, LPSTR );
BOOL    FAR PASCAL AddPrinter(HWND hwnd);

int    NEAR DosCopy(LPSTR szFile, LPSTR szPath);
BOOL     NEAR AlreadyInstalled(LPSTR szPrinter);

/*--------------------------------------------------------------------------*\
|
| l o c a l   p r o c s
|
\*--------------------------------------------------------------------------*/


#define SLASH(c)    ((c) == '/' || (c) == '\\')
#define CHSEPSTR    "\\"

void NEAR catpath(LPSTR path, LPSTR sz)
{
    if (!SLASH(path[lstrlen(path)-1]))
        lstrcat(path, CHSEPSTR);
    lstrcat(path, sz);
}

LONG PASCAL LZCopy(int doshSource, int doshDest)
{
	FARPROC	fp;
	HANDLE hLib;
	char buf[80];
	LONG l;

	LoadString(ghInst, IDS_LZLIB, buf, sizeof(buf));

	hLib = LoadLibrary(buf);

	if (hLib < 32)
		return -1L;

	LoadString(ghInst, IDS_LZCOPY, buf, sizeof(buf));

	fp = GetProcAddress(hLib, buf);

	if (!fp)
		return -1L;

	l = (LONG)(*fp)((int)doshSource, (int)doshDest);

	FreeLibrary(hLib);

	return l;
}



/*--------------------------------------------------------------------------*\
|
|   d i a l o g   p r o c s
|
\*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*\
|   fnGetIni (hDlg, uiMessage, wParam, lParam)
|
|   Description:
|
|   Arguments:                                     
|    hDlg        window handle of dialog window                 
|    uiMessage    message number                         
|    wParam        message-dependent                     
|    lParam        message-dependent                     
|
|   Returns:
|    TRUE if message has been processed, else FALSE
|
\*--------------------------------------------------------------------------*/

BOOL FAR PASCAL fnGetIni(hDlg, uiMessage, wParam, lParam)
HWND     hDlg;
unsigned    uiMessage;
WORD     wParam;
long    lParam;
{
    int    fh;
    char    last_char;

    switch (uiMessage) {
    case WM_COMMAND:
        switch (wParam) {

        case IDOK:

            GetDlgItemText(hDlg, ID_EDIT, lpSrcPath, FILE_LEN);

            last_char = lpSrcPath[lstrlen(lpSrcPath)-1];

            if (last_char != '\\')
                lstrcat(lpSrcPath, "\\");

            lstrcpy(lpIniFile, lpSrcPath);
            lstrcat(lpIniFile, szIniName);

            DBMSG(("looking for %ls...", (LPSTR)lpIniFile));

            if ((fh = _lopen(lpIniFile, READ)) <= 0) {
                DBMSG(("not found\n"));
                
                SetDlgItemText(hDlg, ID_EDIT, def_drive);

                break;    /* bogus file, don't EndDialog() */
            } else {
                DBMSG(("found!\n"));
                _lclose(fh);
            }

            /* fall through to cancel */

        case IDCANCEL:
            EndDialog(hDlg, wParam);
        }
        break;

    case WM_INITDIALOG:
        
        *lpSrcPath = 0;

        SendDlgItemMessage(hDlg, ID_EDIT, EM_LIMITTEXT, FILE_LEN - sizeof(szIniName), 0L);
        SetDlgItemText(hDlg, ID_EDIT, def_drive);
        SendDlgItemMessage(hDlg, ID_EDIT, EM_SETSEL, 0, MAKELONG(0, 3));

        SetFocus(GetDlgItem(hDlg, ID_EDIT));

        return FALSE;
    }
    return FALSE;
}



BOOL FAR PASCAL fnGetPrinters(hDlg, uiMessage, wParam, lParam)
HWND     hDlg;
unsigned    uiMessage;
WORD     wParam;
long    lParam;
{
    LPSTR	ptr;
    int    	iSel;
    char    	buf[60], ext_printers[20];
    int    	len, src_len;
    int    	i;
    char    	file[20];
    BOOL    	fExists;
    LPSTR   	szPrinters;
    char    	DstPath[130];
    int		fh_src, fh_dst;

#define SIZE_PRINTERS 1024

#define FALLOC(n)	(VOID FAR *)MAKELONG(0, GlobalAlloc(GPTR, (DWORD)n))
#define FFREE(n)	GlobalFree((HANDLE)HIWORD((LONG)n))


    switch (uiMessage) {
    case WM_INITDIALOG:

    	szPrinters = FALLOC(SIZE_PRINTERS);

        GetPrivateProfileString(szModule, NULL, szNull, szPrinters, SIZE_PRINTERS, lpIniFile);
        ptr = szPrinters;

        iSel = 0;

        while (*ptr) {
            GetPrivateProfileString(szModule, ptr, szNull, buf, sizeof(buf), lpIniFile);

            SendDlgItemMessage(hDlg, PRINTER_LIST, LB_INSERTSTRING, iSel, (LONG)(LPSTR)buf);
            ptr += (lstrlen(ptr) + 1);

            iSel++;
        }

	FFREE(szPrinters);

        SendDlgItemMessage(hDlg, PRINTER_LIST, LB_SETCURSEL, 0, 0L);


        return TRUE;

    case WM_COMMAND:
        switch (wParam) {

        case INSTALL:

            /* clear the status message field */
            SetDlgItemText(hDlg, STATUS_MSG, szNull);

            /* get the Src */

            iSel = (int)SendDlgItemMessage(hDlg, PRINTER_LIST, LB_GETCURSEL, 0, 0L);

            if (iSel == LB_ERR)
                break;

	    szPrinters = FALLOC(SIZE_PRINTERS);

            /* grab all of the printers from printers.ini */
            GetPrivateProfileString(szModule, NULL, szNull, szPrinters, SIZE_PRINTERS, lpIniFile);
            ptr = szPrinters;

            /* traverse the entries until we hit the one selected */
            while (iSel-- && *ptr)
                ptr += (lstrlen(ptr) + 1);

            lstrcpy(file, ptr);    /* get the file name alone */

	    FFREE(szPrinters);

            if (fExists = AlreadyInstalled(file)) {
            
		LoadString(ghInst, IDS_ALREADYINSTALLED, buf, sizeof(buf));
		LoadString(ghInst, IDS_ADDPRINTER, ext_printers, sizeof(ext_printers));

                if (MessageBox(hDlg, buf, ext_printers,
                    MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
                    return TRUE;
                }
            }

            len = lstrlen(file);

	    lstrcat(file, szRes);

            /* now get the Dst */

	    GetWindowsDirectory(DstPath, sizeof(DstPath));	// windows directory

	    src_len = lstrlen(lpSrcPath);

            catpath(lpSrcPath, file);
	    catpath(DstPath, file);

	    DBMSG(("src:%ls\n", (LPSTR)lpSrcPath));
	    DBMSG(("dst:%ls\n", (LPSTR)DstPath));

	    fh_src = _lopen(lpSrcPath, READ);
	    fh_dst = _lcreat(DstPath, 0);

	    DBMSG(("fh_src:%d\n", fh_src));
	    DBMSG(("fh_dst:%d\n", fh_dst));

            wsprintf(buf, "Copying %ls.", (LPSTR)lpSrcPath);
            SetDlgItemText(hDlg, STATUS_MSG, buf);

            if (LZCopy(fh_src, fh_dst) < 0) {
		LoadString(ghInst, IDS_INSTFAIL, buf, sizeof(buf));
                SetDlgItemText(hDlg, STATUS_MSG, buf);
                goto EXIT;
            }

	    LoadString(ghInst, IDS_INSTSUCCESS, buf, sizeof(buf));

            SetDlgItemText(hDlg, STATUS_MSG, buf);

            file[len] = 0;    /* wack of extension */

            LoadString(ghInst, IDS_EXTPRINTERS, ext_printers, sizeof(ext_printers));

            i = GetProfileInt(szModule, ext_printers, 0);

            /* if alread exists don't bump the printer count
             * or add a new win.ini line */

            if (!fExists) {
                i++;    /* adding a new printer */

		nNewPrinter = i;	// this is the ext printer number

                wsprintf(buf, "%d", i);

                WriteProfileString(szModule, ext_printers, buf);

                LoadString(ghInst, IDS_PRINTER, ext_printers, sizeof(ext_printers));

                wsprintf(buf, ext_printers, i);

                WriteProfileString(szModule, buf, file);
            }

EXIT:
	    _lclose(fh_src);
	    _lclose(fh_dst);

            /* clean the file names from the paths */

            lpSrcPath[src_len] = 0;

            break;


        case IDCANCEL:
            EndDialog(hDlg, nNewPrinter);
        }
        break;

    }
    return FALSE;
}


/* return TRUE if this printer has already been installed */

BOOL NEAR AlreadyInstalled(LPSTR szPrinter)
{
    int num_printers, i;
    char buf[20];
    char printer[20];
    char temp[80];
    char ext_printers[20];

    lstrcpy(temp, szPrinter);

    AnsiUpper(temp);

    LoadString (ghInst, IDS_EXTPRINTERS, ext_printers, sizeof(ext_printers));

    num_printers = GetProfileInt(szModule, ext_printers, 0);

    for (i = 1; i <= num_printers; i++) {

        LoadString(ghInst, IDS_PRINTER, printer, sizeof(printer));


        wsprintf(buf, printer, i);

        GetProfileString(szModule, buf, szNull, printer, sizeof(printer));
        // WinPrintf("GetProfile() -> %ls\n", (LPSTR)printer);
        AnsiUpper(printer);

        // WinPrintf("lstrcmpi(%ls,%ls)\n", (LPSTR)printer, (LPSTR)temp);

        if (!lstrcmpi(printer, temp)) {
            // WinPrintf("AlreadyInstalled() YES\n");
            return TRUE;
        }
    }

    // WinPrintf("AlreadyInstalled() NO\n");
    return FALSE;
}


/*****************************************************************
 * add a printer to the printer list (external printers)
 *
 *
 * returns:
 *	external printer number (1 == first, 2 == second)
 *	of last printer added (note, more than one can be
 *	added).  also, if this is an update (the external printer
 *	is already installed) we return zero (meaning no need to
 *	update teh list of printers).
 *
 *****************************************************************/

int FAR PASCAL AddPrinter(HWND hwnd)
{
    char    SrcPath[FILE_LEN];
    char    IniFile[FILE_LEN];

    lpSrcPath = SrcPath;		// put these buffers on the
    lpIniFile = IniFile;		// stack, not in our DS

    IniFile[0] = 0;

    nNewPrinter = 0;

    if (DialogBox(ghInst, "GI", hwnd, fnGetIni) == IDCANCEL)
        return 0;

    return DialogBox(ghInst, "GP", hwnd, fnGetPrinters);
}


