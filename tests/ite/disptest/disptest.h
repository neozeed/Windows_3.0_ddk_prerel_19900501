#include "..\inc\isg_test.h"

/*---------------------------------------------------------------------------*\
| WINDOW CLASS/CREATE VALUES                                                  |
\*---------------------------------------------------------------------------*/
#define DISPTESTCLASS "DISPTEST"
#define DISPTESTMENU  "DISPTEST"
#define DISPTESTNAME  "DISPTEST"
#define DISPTESTICON  "DISPTEST"
#define DISPTESTTITLE "Windows Display Test Application"


/*---------------------------------------------------------------------------*\
| CONSTANTS                                                                   |
\*---------------------------------------------------------------------------*/
#define LOGFILE_SIZE    80

#define STAT_AUTORUN    0x0001


/*---------------------------------------------------------------------------*\
| MENU COMMAND ID'S                                                           |
\*---------------------------------------------------------------------------*/
#define IDM_SETTINGS_OBJECTS    100
#define IDM_TEST_RUN            200
#define IDM_TEST_STOP           201
#define IDM_TEST_RESET          202
#define IDM_HELP_ABOUT          300
#define IDM_HELP_DESCR          301

/*---------------------------------------------------------------------------*\
| TESTS DIALOG ID'S                                                           |
\*---------------------------------------------------------------------------*/
#define IDD_TEST_BITMAPS        10
#define IDD_TEST_CURVES         11
#define IDD_TEST_LINES          12
#define IDD_TEST_POLYGONS       13
#define IDD_TEST_TEXT           14

#define DT_RAST                 0x0004
#define DT_CURV                 0x0008
#define DT_LINE                 0x0010
#define DT_POLY                 0x0020
#define DT_TEXT                 0x0040

/*---------------------------------------------------------------------------*\
| INTERFACE DIALOG ID'S                                                       |
\*---------------------------------------------------------------------------*/
#define IDD_INTR_LOGC           100
#define IDD_INTR_MSTC           101
#define IDD_INTR_CPYC           102
#define IDD_INTR_LOGE           103
#define IDD_INTR_MSTE           104
#define IDD_INTR_CPYE           105
#define IDD_INTR_RAST           106
#define IDD_INTR_CURV           107
#define IDD_INTR_LINE           108
#define IDD_INTR_POLY           109
#define IDD_INTR_TEXT           110
#define IDD_INTR_TIME           111
#define IDD_INTR_STAT           112
#define IDD_INTR_SEC1           113
#define IDD_INTR_SEC2           114

/*---------------------------------------------------------------------------*\
| OBJECT DIALOG ID's                                                          |
\*---------------------------------------------------------------------------*/
#define IDD_OBJT_PENLIST        100
#define IDD_OBJT_BRSHLIST       101
#define IDD_OBJT_FONTLIST       102
#define IDD_OBJT_PENALL         103
#define IDD_OBJT_BRSHALL        104
#define IDD_OBJT_FONTALL        105

#define IDS_TEXT_TESTSTRING     100

/*----------------------------------------------*\
| Structures.                                    |
\*----------------------------------------------*/
typedef struct
     {
          FARPROC      lpProc;
          PSTR         szText;
     } TESTFUNCTION;
typedef TESTFUNCTION FAR *LPTESTFUNCTION;

typedef struct
     {
          BITMAP       bm;
          GLOBALHANDLE hBits;
     } BITMAPHEADBITS;
typedef BITMAPHEADBITS FAR *LPBITMAPHEADBITS;

/*----------------------------------------------*\
| _INIT SEGMENT FUNCTIONS.                       |
\*----------------------------------------------*/
BOOL FAR RegisterDispTestClass(HANDLE);
HWND FAR CreateDispTestWindow(HANDLE);

/*----------------------------------------------*\
| _TEXT SEGMENT FUNCTIONS.                       |
\*----------------------------------------------*/
BOOL FAR PaintDispTestWindow(HWND);
BOOL FAR ProcessDispTestCommands(HWND,WORD);
BOOL FAR DestroyDispWindow(HWND);

/*----------------------------------------------*\
| _RASTER SEGMENT FUNCTIONS.                     |
\*----------------------------------------------*/
BOOL VerifyBitBltROP(HDC,int,int,HBRUSH,HBRUSH,HBRUSH,DWORD);
BOOL VerifyExtTextOutClip(HDC,short,LPRECT,LPSTR,short,LPINT);
BOOL FAR FindBitmapBit(LPSTR,int);
BOOL FAR CheckROPBit(BOOL,BOOL,BOOL,BOOL,WORD);

HBRUSH GetNextTestBrush(HDEVOBJECT,HDEVOBJECT,LPINT);

BOOL DisplayExtTextOut(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPSTR);
BOOL DisplayBitmaps(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPSTR);
BOOL DisplayPolygons(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPSTR);
BOOL DisplayPolylines(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPSTR);
BOOL DisplayEllipses(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPSTR);
BOOL DisplayColorMapping(HWND,HDC,LPDEVINFO,LPSTR);

/*----------------------------------------------*\
| WINDOW CALL BACK FUNCTIONS.                    |
\*----------------------------------------------*/
BOOL FAR PASCAL AboutDlg(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL InterfaceDlg(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispTestProc(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispBitmProc(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispCurvProc(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispLineProc(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispTextProc(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL DispPolyProc(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL SetupObjectsDlg(HWND,unsigned,WORD,LONG);
BOOL InitObjectsFontList(HWND);

LOCALHANDLE FAR AllocTextBuffer(LPSTR,WORD);
BOOL FAR        FreeTextBuffer(LOCALHANDLE);

