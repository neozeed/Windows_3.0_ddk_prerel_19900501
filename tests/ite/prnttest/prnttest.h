#include <drivinit.h>
#include "..\inc\isg_test.h"
#include "frame\frame.h"

/*---------------------------------------------------------------------------*\
| WINDOW CLASS/CREATE VALUES                                                  |
\*---------------------------------------------------------------------------*/
#define PRNTTESTCLASS "PRNTTEST"
#define PRNTTESTMENU  "PRNTTEST"
#define PRNTTESTNAME  "PRNTTEST"
#define PRNTTESTICON  "PRNTTEST"
#define PRNTTESTTITLE "Windows Printer Test Application"

/*---------------------------------------------------------------------------*\
| MENU COMMAND ID'S                                                           |
\*---------------------------------------------------------------------------*/
#define IDM_SETTINGS_HEADER     100
#define IDM_SETTINGS_TESTS      101
#define IDM_TEST_RUN            200
#define IDM_HELP_ABOUT          300
#define IDM_HELP_DESCR          301

/*---------------------------------------------------------------------------*\
| INTERFACE CONTROL ID'S                                                      |
\*---------------------------------------------------------------------------*/
#define IDD_INTRFACE_LIST       100
#define IDD_INTRFACE_TEST       101
#define IDD_INTRFACE_PROF       102
#define IDD_INTRFACE_NAME       103
#define IDD_INTRFACE_DRIV       104
#define IDD_INTRFACE_PORT       105
#define IDD_INTRFACE_MOD        106
#define IDD_INTRFACE_ADD        107
#define IDD_INTRFACE_REM        108
#define IDD_INTRFACE_SET        109
#define IDD_INTRFACE_TXT        110

/*---------------------------------------------------------------------------*\
| TESTS DIALOG ID'S                                                           |
\*---------------------------------------------------------------------------*/
#define IDD_TEST_TEXT           0x0004
#define IDD_TEST_BITMAPS        0x0008
#define IDD_TEST_POLYGONS       0x0010
#define IDD_TEST_CURVES         0x0020
#define IDD_TEST_LINES          0x0040

/*---------------------------------------------------------------------------*\
| HEADER DIALOG ID's                                                          |
\*---------------------------------------------------------------------------*/
#define IDD_HEAD_YES            0x0002
#define IDD_HEAD_EXP            0x0004
#define IDD_HEAD_CON            0x0008
#define IDD_HEAD_CAPS           0x0010
#define IDD_HEAD_FONT           0x0020
#define IDD_HEAD_BRSH           0x0040
#define IDD_HEAD_PEN            0x0080

/*---------------------------------------------------------------------------*\
| OBJECT DIALOG ID's                                                          |
\*---------------------------------------------------------------------------*/
#define IDD_OBJT_PENLIST        100
#define IDD_OBJT_BRSHLIST       101
#define IDD_OBJT_FONTLIST       102
#define IDD_OBJT_PENALL         103
#define IDD_OBJT_BRSHALL        104
#define IDD_OBJT_FONTALL        105

/*---------------------------------------------------------------------------*\
| MASK VALUES - These are the same as there Dialog Equivalents                |
\*---------------------------------------------------------------------------*/
#define PH_PRINTHEADER          (IDD_HEAD_YES)
#define PH_EXPANDED             (IDD_HEAD_EXP)
#define PH_CONDENSED            (IDD_HEAD_CON)
#define PH_CAPABILITIES         (IDD_HEAD_CAPS)
#define PH_FONTS                (IDD_HEAD_FONT)
#define PH_BRUSHES              (IDD_HEAD_BRSH)
#define PH_PENS                 (IDD_HEAD_PEN)

#define PT_TEXT                 (IDD_TEST_TEXT)
#define PT_BITMAPS              (IDD_TEST_BITMAPS)
#define PT_POLYGONS             (IDD_TEST_POLYGONS)
#define PT_CURVES               (IDD_TEST_CURVES)
#define PT_LINES                (IDD_TEST_LINES)

/*---------------------------------------------------------------------------*\
| STRING TABLE IDENTIFIERS - Seperated into logical 16 value blocks.          |
\*---------------------------------------------------------------------------*/
#define IDS_ERROR_STARTDOC        1
#define IDS_ERROR_GETDC           2
#define IDS_ERROR_GETCAPS         3
#define IDS_TEST_JOBTITLE         4

#define IDS_INTRFACE_PROF        16
#define IDS_INTRFACE_NAME        17
#define IDS_INTRFACE_DRIV        18
#define IDS_INTRFACE_PORT        19
#define IDS_INTRFACE_ADD         20
#define IDS_INTRFACE_MOD         21
#define IDS_INTRFACE_REM         22
#define IDS_INTRFACE_SET         23

/*---------------------------------------------------------------------------*\
| FUNCTION DECLARATIONS (By Segment)                                          |
\*---------------------------------------------------------------------------*/

/*----------------------------------------------*\
| _INIT SEGMENT FUNCTIONS.                       |
\*----------------------------------------------*/
BOOL FAR RegisterPrntTestClass(HANDLE);
HWND FAR CreatePrntTestWindow(HANDLE);

/*----------------------------------------------*\
| _TEXT SEGMENT FUNCTIONS.                       |
\*----------------------------------------------*/
BOOL     AddProfiles(HWND);
BOOL     InitializeInterface(HWND);
BOOL     ModifyProfiles(HWND);
BOOL     PaintPrntTestWindow(HWND);
BOOL     ParseDeviceString(LPSTR,LPSTR,LPSTR,LPSTR,LPSTR);
BOOL     ProcessPrntTestCommands(HWND,WORD);
BOOL     RemoveProfiles(HWND);
BOOL     SearchProfileString(LPSTR,LPSTR);
BOOL     SetupPrinter(HWND);
BOOL     UpdateSelectionChange(HWND);

/*----------------------------------------------*\
| _HEADER SEGMENT FUNCTIONS.                     |
\*----------------------------------------------*/
BOOL FAR PrintHeader(HWND,HDC);

/*----------------------------------------------*\
| _INFO SEGMENT FUNCTIONS.                       |
\*----------------------------------------------*/
BOOL FAR GetPrinterInformation(HDC,LPPRINTER,LPSTR,LPSTR);

/*----------------------------------------------*\
| WINDOW CALL BACK FUNCTIONS.                    |
\*----------------------------------------------*/
BOOL FAR PASCAL AbortDlg(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL AboutDlg(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL PrintAbortProc(HDC,short);
BOOL FAR PASCAL PrntTestDlg(HWND,unsigned,WORD,LONG);
LONG FAR PASCAL PrntTestProc(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL SetupHeaderDlg(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL SetupTestsDlg(HWND,unsigned,WORD,LONG);
BOOL FAR PASCAL SetupObjectsDlg(HWND,unsigned,WORD,LONG);
BOOL            InitObjectsFontList(HWND);
