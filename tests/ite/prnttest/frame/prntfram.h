#include <drivinit.h>
#include "..\..\inc\isg_test.h"
#include "frame.h"

#define IDS_HEAD_TITLEPAGE       1
#define IDS_HEAD_PRINTAREA       2
#define IDS_HEAD_PRINTAREA1      3

#define IDS_TEST_RAST_NO        16
#define IDS_TEST_TEXT_STR1      17

#define IDS_TST_DSCR_CURV       32
#define IDS_TST_DSCR_RAST       33
#define IDS_TST_DSCR_LINE       34
#define IDS_TST_DSCR_POLY       35
#define IDS_TST_DSCR_TEXT       36
#define IDS_TST_HEAD_OBJT       37
#define IDS_TST_HEAD_GRAY       38
#define IDS_TST_HEAD_FUNC       39

#define IDS_STATUS_HEADER       48
#define IDS_STATUS_RASTER       49
#define IDS_STATUS_CURVE        50
#define IDS_STATUS_LINE         51
#define IDS_STATUS_POLYGON      52
#define IDS_STATUS_TEXT         53

/*----------------------------------------------*\
| Flags for the Justify/Text Functions used in   |
| the TEXT Tests.                                |
\*----------------------------------------------*/
#define ID_LEFT                 100
#define ID_RIGHT                101
#define ID_CENTER               102
#define ID_JUSTIFY              103

#define SYNFONT_ITALIC          0x0001
#define SYNFONT_UNDERLINED      0x0002
#define SYNFONT_STRIKEOUT       0x0004

/*----------------------------------------------*\
| Flags for the Raster functions used for        |
| determining the bitmap type.                   |
\*----------------------------------------------*/
#define BF_SOLID                  1
#define BF_HATCHED                2
