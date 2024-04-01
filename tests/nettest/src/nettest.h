/*  .h Include file for NetTest  - Windows 3.0 test app   */

extern HWND hParWnd;


/*
 * Global Types
 */


/*
 * Global Variables
 */
extern HANDLE hInst;
extern char   szLogFile[];


/*
 *    Define initial windows dimensions
 */

#define X_START 	40
#define Y_START 	200
#define WIDTH_START	550
#define HEIGHT_START	200


/*
 * Define misc constants
 */

#define FILE_BUFF	79	   /* the size of the file buffer for the function file */


/*
 * Define Menu options
 */

#define IDM_HELP	 100
#define IDM_EXIT	 101
#define IDM_ABOUT	 102
#define IDM_BATCH	 103
#define IDM_ADDNET	 104
#define IDM_LISTNET	 105
#define IDM_REMOVENET	 106
#define IDM_ABORTJOB	 107
#define IDM_CANCELJOB	 108
#define IDM_CLOSEJOB	 109
#define IDM_HOLDJOB	 110
#define IDM_LOCKQ	 111
#define IDM_OPENJOB	 112
#define IDM_RELEASEJOB	 113
#define IDM_SETCOPIES	 114
#define IDM_STOPWATCHQ	 115
#define IDM_UNLOCKQ	 116
#define IDM_WATCHQ	 117
#define IDM_BROWSE	 118
#define IDM_DRIVEDLG	 119
#define IDM_USERNAME	 120
#define IDM_NETCAPS	 121
#define IDM_GETERRCODE	 122
#define IDM_GETERRTEXT	 123
#define IDM_NETBIOS	 124
#define IDM_FILECOUNT	 125

/*
 *  Function Templates
 */

/* Exported Functions */
BOOL FAR PASCAL AddConnectionDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL AboutDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL BatchFileDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL HelpDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL ListConnsDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL NetCapsDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
long FAR PASCAL NetTestWndProc(HWND hWnd, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL RemoveConnectionDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL GetErrorTextDlgProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam);

/* Nonexported Functions */
BOOL FAR PASCAL BrowseDialog(HWND hWnd);
BOOL	 PASCAL GetErrorCode(void);
BOOL FAR PASCAL GetUserName(HWND hWnd);
BOOL FAR PASCAL ProcessWMCommand(HWND hWnd, unsigned message, WORD wParam, LONG lParam);
BOOL FAR PASCAL NetTestInit(HANDLE hInstance);
BOOL FAR PASCAL NetBiosTest(HWND hWnd);
BOOL	 PASCAL FileCount(void);
 /* file logging functions */
BOOL	 PASCAL WriteLog(char szBuff1[], char szBuff2[]);
PSTR	 PASCAL GiveMeTime();
