/*---------------------------------------------------------------------------*\
| EXTERNAL GLOBAL VARIABLE DECLARATIONS                                       |
\*---------------------------------------------------------------------------*/
HANDLE          hInst;
HWND            hIntrDlg;
DEVINFO         dcDevCaps;
HDEVOBJECT      hDevFonts,hDevPens,hDevBrushes;
WORD            wTestsSet;
int             nTimerSpeed;
char            szLogFile[LOGFILE_SIZE];
FARPROC         lpIntrDlg;
char            szChrisWil[] = "Copyright © 1989, Microsoft Windows";
TEST            tlTest;
