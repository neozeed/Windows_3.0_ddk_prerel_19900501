/*---------------------------------------------------------------------------*\
| GLOBAL VARIABLES                                                            |
\*---------------------------------------------------------------------------*/
HANDLE          hInst;
DEVINFO         dcDevCaps;
HDEVOBJECT      hFonts,hPens,hBrushes;
PRINTER         pPrinter;
BOOL            bAbort,bAutoRun;
HWND            hAbortDlg,hPrntDlg;
FARPROC         lpAbortProc,lpAbortDlg;
WORD            wHeaderSet,wTestsSet;
char            szChrisWil[] = "Copyright © 1989, Microsoft Windows - ChrisWil";
TEST            tlTest;
