/**[f******************************************************************
 * windows.h - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

#include "style.h"

#ifndef NOMINMAX
#define max(a,b)	    ((a) > (b) ? (a) : (b))
#define min(a,b)	    ((a) < (b) ? (a) : (b))
#endif

#define INTENSITY(r,g,b)	(BYTE)(((WORD)((r) * 30) + (WORD)((g) * 59) + (WORD)((b) * 11))/100)
#define RGB(r,g,b)		((DWORD)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))

#define ODD(X)	((X) & 1)	/* Returns TRUE if X is odd */
#define EVEN(X) (!((X) & 1))	/* Returns TRUE if X is even */

#define TRUE	1
#define FALSE	0
#define NULL	0



/* Combo Box return Values */
#define CB_OKAY 	    0
#define CB_ERR		    (-1)
#define CB_ERRSPACE	    (-2)


/* Combo Box Notification Codes */
#define CBN_ERRSPACE	    (-1)
#define CBN_SELCHANGE	    1
#define CBN_DBLCLK	    2
#define CBN_SETFOCUS	    3
#define CBN_KILLFOCUS	    4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7


/* Combo Box messages */
#define CB_GETEDITSEL	   (WM_USER+0)
#define CB_LIMITTEXT	   (WM_USER+1)
#define CB_SETEDITSEL	   (WM_USER+2)
#define CB_ADDSTRING	   (WM_USER+3)
#define CB_DELETESTRING	   (WM_USER+4)
#define CB_DIR             (WM_USER+5)
#define CB_GETCOUNT	   (WM_USER+6)
#define CB_GETCURSEL	   (WM_USER+7)
#define CB_GETLBTEXT	   (WM_USER+8)
#define CB_GETLBTEXTLEN	   (WM_USER+9)
#define CB_INSERTSTRING    (WM_USER+10)
#define CB_RESETCONTENT	   (WM_USER+11)
#define CB_FINDSTRING	   (WM_USER+12)
#define CB_SELECTSTRING	   (WM_USER+13)
#define CB_SETCURSEL	   (WM_USER+14)
#define CB_RECALCULATEINTERNALS (WM_USER+15)
#define CB_MSGMAX          (WM_USER+16)

/* Dialog Box Command IDs */
#define IDOK		    1
#define IDCANCEL	    2
#define IDABORT 	    3
#define IDRETRY 	    4
#define IDIGNORE	    5
#define IDYES		    6
#define IDNO		    7


#define FAR	far
#define NEAR	near
#define PASCAL	pascal

typedef struct {
	int x;
	int y;
} POINT;
typedef POINT FAR *LPPOINT;

typedef struct tagRECT {
    int 	left;
    int 	top;
    int 	right;
    int 	bottom;
} RECT;

typedef RECT FAR *LPRECT;

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef long		LONG;
typedef unsigned long	DWORD;
typedef WORD		HANDLE;
typedef HANDLE		HDC;
typedef HANDLE		HCURSOR;
typedef HANDLE		HFONT;
typedef HANDLE		HICON;
typedef HANDLE FAR *	LPHANDLE;
typedef int		BOOL;
typedef int FAR	*	LPBOOL;
typedef short FAR *	LPSHORT;
typedef WORD FAR *	LPWORD;
typedef char FAR *	LPSTR;

typedef int (FAR PASCAL *FARPROC)();

typedef WORD HWND;
typedef WORD HINST;


#define MAKEINTRESOURCE(i)  (LPSTR)((DWORD)((WORD)(i)))
#define MAKELONG(a, b)	((long)(((unsigned)a) | ((unsigned long)((unsigned)b)) << 16))
#define LOWORD(l)	((WORD)(l))
#define HIWORD(l)	((WORD)(((DWORD)(l) >> 16) & 0xffff))

#define RGB(r,g,b)		((DWORD)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(rgb)		((BYTE)(rgb))
#define GetGValue(rgb)		((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)		((BYTE)((rgb)>>16))

#define RT_RCDATA	    MAKEINTRESOURCE(10)

#define LBN_SELCHANGE   1
#define LBN_DBLCLK      2

#define LB_INSERTSTRING  (2+WM_USER)
#define LB_DELETESTRING  (3+WM_USER)
#define LB_REPLACESTRING (4+WM_USER)
#define LB_RESETCONTENT  (5+WM_USER)
#define LB_SETCURSEL	 (7+WM_USER)
#define LB_GETCURSEL	 (9+WM_USER)

/* notification codes */
#define EN_SETFOCUS   0x0100
#define EN_KILLFOCUS  0x0200
#define EN_CHANGE     0x0300
#define EN_ERRSPACE   0x0500
#define EN_HSCROLL    0x0601
#define EN_VSCROLL    0x0602


#define MB_OK			0x0000
#define MB_OKCANCEL		0x0001
#define MB_ICONQUESTION		0x0020
#define MB_ICONEXCLAMATION	0x0030


/* Interface to global memory manager */
#define GMEM_FIXED	    0x0000
#define GMEM_MOVEABLE	    0x0002
#define GMEM_NOCOMPACT	    0x0010
#define GMEM_NODISCARD	    0x0020
#define GMEM_ZEROINIT	    0x0040
#define GMEM_MODIFY	    0x0080
#define GMEM_DISCARDABLE    0x0F00
#define GHND	(GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR	(GMEM_FIXED    | GMEM_ZEROINIT)
#define GMEM_SHARE          0x2000
#define GMEM_SHAREALL       0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_LOWER          0x1000
#define GMEM_NOTIFY         0x4000

#define WM_PSDEVMODECHANGE    0x001b
#define WM_INITDIALOG	    0x0110
#define WM_COMMAND	    0x0111

#define WM_USER 	 0x0400
#define EM_LIMITTEXT	 WM_USER+21


HANDLE	FAR PASCAL FindResource( HANDLE, LPSTR, LPSTR );
int	FAR PASCAL AccessResource( HANDLE, HANDLE );
HANDLE	FAR PASCAL LoadResource( HANDLE, HANDLE );
BOOL	FAR PASCAL FreeResource( HANDLE );
LPSTR	FAR PASCAL LockResource( HANDLE );
HANDLE	FAR PASCAL GetModuleHandle( LPSTR );
WORD	FAR PASCAL SizeofResource( HANDLE, HANDLE );


void	FAR PASCAL FatalExit( int );
HANDLE	FAR PASCAL FindResource( HANDLE, LPSTR, LPSTR );
HANDLE	FAR PASCAL LoadResource( HANDLE, HANDLE );
BOOL	FAR PASCAL FreeResource( HANDLE );
LPSTR	FAR PASCAL LockResource( HANDLE );
WORD	FAR PASCAL SizeofResource( HANDLE, HANDLE );

HANDLE	FAR PASCAL GlobalAlloc( WORD, DWORD );
HANDLE	FAR PASCAL GlobalFree( HANDLE );
LPSTR	FAR PASCAL GlobalLock( HANDLE );
BOOL	FAR PASCAL GlobalUnlock( HANDLE );
DWORD	FAR PASCAL GlobalCompact(DWORD);

HANDLE	FAR PASCAL LocalAlloc( WORD, WORD );
HANDLE	FAR PASCAL LocalFree( HANDLE );
BOOL	FAR PASCAL LocalUnlock( HANDLE );
char NEAR * FAR PASCAL LocalLock( HANDLE );

/* Local Memory Flags */
#define LMEM_FIXED	    0x0000
#define LMEM_MOVEABLE	    0x0002
#define LMEM_NOCOMPACT	    0x0010
#define LMEM_NODISCARD	    0x0020
#define LMEM_ZEROINIT	    0x0040
#define LMEM_MODIFY	    0x0080
#define LMEM_DISCARDABLE    0x0F00

#define LHND		    (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR		    (LMEM_FIXED | LMEM_ZEROINIT)


DWORD	    FAR PASCAL GlobalCompact( DWORD );
#define GlobalDiscard( h ) GlobalReAlloc( h, 0L, GMEM_MOVEABLE )
DWORD	    FAR PASCAL GlobalHandle( WORD );
HANDLE	    FAR PASCAL GlobalReAlloc( HANDLE, DWORD, WORD );
DWORD	    FAR PASCAL GlobalSize( HANDLE );
WORD	    FAR PASCAL GlobalFlags( HANDLE );

int	    FAR PASCAL LoadString( HANDLE, unsigned, LPSTR, int );
long	    FAR PASCAL SendMessage(HWND, unsigned, WORD, LONG);
short	    FAR PASCAL SetEnvironment(LPSTR, LPSTR, WORD);
short	    FAR PASCAL GetEnvironment(LPSTR, LPSTR, WORD);
HWND	    FAR PASCAL SetFocus(HWND);

int	    FAR PASCAL GetProfileInt( LPSTR, LPSTR, int );
int	    FAR PASCAL GetProfileString( LPSTR, LPSTR, LPSTR, LPSTR, int );
BOOL	    FAR PASCAL WriteProfileString( LPSTR, LPSTR, LPSTR );
FARPROC     FAR PASCAL MakeProcInstance(FARPROC, HANDLE);
void	    FAR PASCAL FreeProcInstance(FARPROC);

int	FAR PASCAL MessageBox(HWND, LPSTR, LPSTR, WORD);
int    	FAR PASCAL GetWindowText(HWND, LPSTR, int);
void   	FAR PASCAL SetWindowText(HWND, LPSTR);
BOOL 	FAR PASCAL ShowWindow(HWND, int);

/* ShowWindow() Commands */
#define SW_HIDE		    0
#define SW_SHOWNORMAL	    1
#define SW_RESTORE	    1
#define SW_NORMAL	    1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE	    3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW		    5
#define SW_MINIMIZE	    6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA	    8


/* Character Translation Routines */
BOOL  FAR PASCAL AnsiToOem(LPSTR, LPSTR);
BOOL  FAR PASCAL OemToAnsi(LPSTR, LPSTR);
LPSTR FAR PASCAL AnsiUpper(LPSTR);
LPSTR FAR PASCAL AnsiLower(LPSTR);
LPSTR FAR PASCAL AnsiNext(LPSTR);
LPSTR FAR PASCAL AnsiPrev(LPSTR, LPSTR);

WORD FAR PASCAL GetVersion(void);
BOOL FAR PASCAL IsRectEmpty(LPRECT);
int  FAR PASCAL IntersectRect(LPRECT, LPRECT, LPRECT);
int  FAR PASCAL SetRectEmpty(LPRECT);
void FAR PASCAL SetRect(LPRECT, int, int, int, int);

HWND FAR PASCAL FindWindow(LPSTR, LPSTR);
HCURSOR FAR PASCAL SetCursor(HCURSOR);
HCURSOR FAR PASCAL LoadCursor( HANDLE, LPSTR );
HWND FAR PASCAL GetFocus(void);
int   FAR PASCAL ReleaseDC(HWND, HDC);
HDC   FAR PASCAL GetDC(HWND);

HWND FAR PASCAL CreateDialog(HANDLE, LPSTR, HWND, FARPROC);
HWND FAR PASCAL CreateDialogIndirect(HANDLE, LPSTR, HWND, FARPROC);
int  FAR PASCAL DialogBox(HANDLE, LPSTR, HWND, FARPROC);
int  FAR PASCAL DialogBoxIndirect(HANDLE, HANDLE, HWND, FARPROC);
void FAR PASCAL EndDialog(HWND, int);
HWND FAR PASCAL GetDlgItem(HWND, int);
void FAR PASCAL SetDlgItemInt(HWND, int, WORD, BOOL);
WORD FAR PASCAL GetDlgItemInt(HWND, int, BOOL FAR *, BOOL);
void FAR PASCAL SetDlgItemText(HWND, int, LPSTR);
int  FAR PASCAL GetDlgItemText(HWND, int, LPSTR, int);
void FAR PASCAL CheckDlgButton(HWND, int, WORD);
void FAR PASCAL CheckRadioButton(HWND, int, int, int);
WORD FAR PASCAL IsDlgButtonChecked(HWND, int);
LONG FAR PASCAL SendDlgItemMessage(HWND, int, WORD, WORD, LONG);
HWND FAR PASCAL GetNextDlgGroupItem(HWND, HWND, BOOL);
HWND FAR PASCAL GetNextDlgTabItem(HWND, HWND, BOOL);
int  FAR PASCAL GetDlgCtrlID(HWND);

HANDLE	FAR PASCAL LoadLibrary(LPSTR);
HANDLE	FAR PASCAL FreeLibrary(HANDLE);

int	FAR PASCAL GetModuleFileName(HANDLE, LPSTR, int);
FARPROC FAR PASCAL GetProcAddress(HANDLE, LPSTR);

BOOL   FAR PASCAL Yield(void);
HANDLE FAR PASCAL GetCurrentTask(void);
int    FAR PASCAL SetPriority(HANDLE, int);


/* Message structure */
typedef struct tagMSG
  {
    HWND	hwnd;
    WORD	message;
    WORD	wParam;
    LONG	lParam;
    DWORD	time;
    POINT	pt;
  } MSG;
typedef MSG		    *PMSG;
typedef MSG NEAR	    *NPMSG;
typedef MSG FAR 	    *LPMSG;

/* Message Function Templates */
BOOL FAR PASCAL GetMessage(LPMSG, HWND, WORD, WORD);
BOOL FAR PASCAL TranslateMessage(LPMSG);
LONG FAR PASCAL DispatchMessage(LPMSG);
BOOL FAR PASCAL PeekMessage(LPMSG, HWND, WORD, WORD, WORD);
BOOL  FAR PASCAL PostMessage(HWND, WORD, WORD, LONG);
HICON FAR PASCAL LoadIcon(HANDLE, LPSTR);

BOOL FAR PASCAL DestroyWindow(HWND);
BOOL FAR PASCAL EnableWindow(HWND,BOOL);
BOOL FAR PASCAL IsWindowVisible(HWND);
void FAR PASCAL MoveWindow(HWND, int, int, int, int, BOOL);
int FAR PASCAL GetDeviceCaps(HDC, int);
BOOL FAR PASCAL MessageBeep(WORD);
void FAR PASCAL UpdateWindow(HWND);

/* undocumented exported windows functions from winexp.h */

int         far PASCAL OpenPathname( LPSTR, int );
int         far PASCAL DeletePathname( LPSTR );
int         far PASCAL _lopen( LPSTR, int );
void        far PASCAL _lclose( int );
int         far PASCAL _lcreat( LPSTR, int );
WORD        far PASCAL _ldup( int );
LONG        far PASCAL _llseek( int, long, int );
WORD        far PASCAL _lread( int, LPSTR, int );
WORD        far PASCAL _lwrite( int, LPSTR, int );

int FAR cdecl wsprintf  (LPSTR, LPSTR, ...);


#define  SEEK_SET 0	/* beginning of file */
#define  SEEK_CUR 1
#define  SEEK_END 2

#define READ        0   /* Flags for _lopen */
#define WRITE       1
#define READ_WRITE  2


/* User Button Notification Codes */
#define BN_CLICKED	   0
#define BN_PAINT	   1
#define BN_HILITE	   2
#define BN_UNHILITE	   3
#define BN_DISABLE	   4
#define BN_DOUBLECLICKED   5

/* Button Control Messages */
#define BM_GETCHECK	   (WM_USER+0)
#define BM_SETCHECK	   (WM_USER+1)
#define BM_GETSTATE	   (WM_USER+2)
#define BM_SETSTATE	   (WM_USER+3)
#define BM_SETSTYLE	   (WM_USER+4)


/*  Ternary raster operations */
#define SRCCOPY 	    (DWORD)0x00CC0020 /* dest = source			 */
#define SRCPAINT	    (DWORD)0x00EE0086 /* dest = source OR dest		 */
#define SRCAND		    (DWORD)0x008800C6 /* dest = source AND dest 	 */
#define SRCINVERT	    (DWORD)0x00660046 /* dest = source XOR dest 	 */
#define SRCERASE	    (DWORD)0x00440328 /* dest = source AND (NOT dest )	 */
#define NOTSRCCOPY	    (DWORD)0x00330008 /* dest = (NOT source)		 */
#define NOTSRCERASE	    (DWORD)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
#define MERGECOPY	    (DWORD)0x00C000CA /* dest = (source AND pattern)	 */
#define MERGEPAINT	    (DWORD)0x00BB0226 /* dest = (NOT source) OR dest	 */
#define PATCOPY 	    (DWORD)0x00F00021 /* dest = pattern 		 */
#define PATPAINT	    (DWORD)0x00FB0A09 /* dest = DPSnoo			 */
#define PATINVERT	    (DWORD)0x005A0049 /* dest = pattern XOR dest	 */
#define DSTINVERT	    (DWORD)0x00550009 /* dest = (NOT dest)		 */
#define BLACKNESS	    (DWORD)0x00000042 /* dest = BLACK			 */
#define WHITENESS	    (DWORD)0x00FF0062 /* dest = WHITE			 */


/* OpenFile() Structure */
typedef struct tagOFSTRUCT
  {
    BYTE	cBytes;
    BYTE	fFixedDisk;
    WORD	nErrCode;
    BYTE	reserved[4];
    BYTE	szPathName[128];
  } OFSTRUCT;
typedef OFSTRUCT	    *POFSTRUCT;
typedef OFSTRUCT NEAR	    *NPOFSTRUCT;
typedef OFSTRUCT FAR	    *LPOFSTRUCT;

/* OpenFile() Flags */
#define OF_READ 	    0x0000
#define OF_WRITE	    0x0001
#define OF_READWRITE	    0x0002
#define OF_PARSE	    0x0100
#define OF_DELETE	    0x0200
#define OF_VERIFY	    0x0400
#define OF_CANCEL	    0x0800
#define OF_CREATE	    0x1000
#define OF_PROMPT	    0x2000
#define OF_EXIST	    0x4000
#define OF_REOPEN	    0x8000

int  FAR PASCAL OpenFile(LPSTR, LPOFSTRUCT, WORD);
FARPROC FAR PASCAL SetResourceHandler(HANDLE, LPSTR, FARPROC);

/* Predefined Resource Types */
#define RT_CURSOR	    MAKEINTRESOURCE(1)
#define RT_BITMAP	    MAKEINTRESOURCE(2)
#define RT_ICON 	    MAKEINTRESOURCE(3)
#define RT_MENU 	    MAKEINTRESOURCE(4)
#define RT_DIALOG	    MAKEINTRESOURCE(5)
#define RT_STRING	    MAKEINTRESOURCE(6)
#define RT_FONTDIR	    MAKEINTRESOURCE(7)
#define RT_FONT 	    MAKEINTRESOURCE(8)
#define RT_ACCELERATOR	    MAKEINTRESOURCE(9)
#define RT_RCDATA	    MAKEINTRESOURCE(10)
#define RT_ERRTABLE	    MAKEINTRESOURCE(11)


/* Commands to pass WinHelp() */
#define HELP_QUIT	0x0002	 /* Terminate help */
#define HELP_INDEX	0x0003	 /* Display index */

BOOL FAR PASCAL WinHelp(HWND hwndMain, LPSTR lpszHelp, WORD usCommand, DWORD ulData);

int         FAR PASCAL lstrcmp( LPSTR, LPSTR );
int         FAR PASCAL lstrcmpi( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcpy( LPSTR, LPSTR );
LPSTR       FAR PASCAL lstrcat( LPSTR, LPSTR );
int         FAR PASCAL lstrlen( LPSTR );

void FAR PASCAL GetWindowsDirectory(LPSTR);
void FAR PASCAL GetSystemDirectory(LPSTR);
HWND FAR PASCAL GetActiveWindow(void);

#include "drivinit.h"
