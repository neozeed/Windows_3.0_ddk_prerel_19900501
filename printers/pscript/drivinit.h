/*
 *  drivinit.h
 *
 *  Header file for printer driver initialization.
 *
 *  History:
 *	2-6-89	    craigc	Initial
 *	2-7-89	    jimmat	Added DM*_FIRST/LAST defines
 *
 *  NOTE: the DMBIN_* and DMPAPER_* values need to be merged with the
 * 	official version of this.
 *
 */

/* size of a device name string */
#define CCHDEVICENAME 32

/* current version of specification */
#define DM_SPECVERSION 0x300

/* field selection bits */
#define DM_ORIENTATION	    0x0000001L
#define DM_PAPERSIZE	    0x0000002L
#define DM_PAPERLENGTH	    0x0000004L
#define DM_PAPERWIDTH	    0x0000008L
#define DM_SCALE	    0x0000010L
#define DM_COPIES	    0x0000100L
#define DM_DEFAULTSOURCE    0x0000200L
#define DM_PRINTQUALITY     0x0000400L
#define DM_COLOR	    0x0000800L
#define DM_DUPLEX	    0x0001000L

/* orientation selections */
#define DMORIENT_PORTRAIT   1
#define DMORIENT_LANDSCAPE  2

/* paper selections */
#define DMPAPER_FIRST	    DMPAPER_LETTER
#define DMPAPER_LETTER	    1
#define DMPAPER_LETTERSMALL 2
#define DMPAPER_TABLOID     3
#define DMPAPER_LEDGER	    4
#define DMPAPER_LEGAL	    5
#define DMPAPER_STATEMENT   6
#define DMPAPER_EXECUTIVE   7
#define DMPAPER_A3	    8
#define DMPAPER_A4	    9
#define DMPAPER_A4SMALL     10
#define DMPAPER_A5	    11
#define DMPAPER_B4	    12
#define DMPAPER_B5	    13
#define DMPAPER_FOLIO	    14
#define DMPAPER_QUARTO	    15
#define DMPAPER_10X14	    16
#define DMPAPER_11X17	    17
#define DMPAPER_NOTE	    18
#define DMPAPER_ENV_9	    19
#define DMPAPER_ENV_10	    20
#define DMPAPER_ENV_11	    21
#define DMPAPER_ENV_12	    22
#define DMPAPER_ENV_14	    23
#define DMPAPER_C	    24
#define DMPAPER_D	    25
#define DMPAPER_E	    26
#define DMPAPER_LAST	    DMPAPER_E

#define DMPAPER_ENVELOPE    256

#define DMPAPER_USER	    512


#define DMPAPER_USER_FIRST  50

#define DMPAPER_LETTER_EXTRA	50
#define DMPAPER_LEGAL_EXTRA 	51
#define DMPAPER_TABLOID_EXTRA	52
#define DMPAPER_A4_EXTRA     	53

#define DMPAPER_USER_LAST   DMPAPER_A4_EXTRA


/* bin selections */
#define DMBIN_FIRST	    DMBIN_UPPER
#define DMBIN_UPPER	    1
#define DMBIN_ONLYONE	    2
#define DMBIN_LOWER	    3
#define DMBIN_MIDDLE	    4
#define DMBIN_ENVELOPE	    5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO	    7
#define DMBIN_TRACTOR	    8
#define DMBIN_SMALLFMT	    9
#define DMBIN_LARGEFMT	    10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_ANYSMALLFMT   12
#define DMBIN_ANYLARGEFMT   13
#define DMBIN_CASSETTE      14
#define DMBIN_MANUAL	    15
#define DMBIN_LAST	    DMBIN_MANUAL

#define DMBIN_USER	    256     /* device specific bins start here */

/* print qualities */
#define DMRES_DRAFT	    (-1)
#define DMRES_LOW	    (-2)
#define DMRES_MEDIUM	    (-3)
#define DMRES_HIGH	    (-4)

/* color enable/disable for color printers */
#define DMCOLOR_MONOCHROME  1
#define DMCOLOR_COLOR	    2

/* duplex enable */
#define DMDUP_SIMPLEX	 1
#define DMDUP_VERTICAL	 2
#define DMDUP_HORIZONTAL 3

typedef struct _devicemode {
    char dmDeviceName[CCHDEVICENAME];
    WORD dmSpecVersion;
    WORD dmDriverVersion;
    WORD dmSize;
    WORD dmDriverExtra;
    DWORD dmFields;
    short dmOrientation;
    short dmPaperSize;
    short dmPaperLength;
    short dmPaperWidth;
    short dmScale;
    short dmCopies;
    short dmDefaultSource;
    short dmPrintQuality;
    short dmColor;
    short dmDuplex;
} DEVMODE;

typedef DEVMODE * PDEVMODE, NEAR * NPDEVMODE, FAR * LPDEVMODE;

/* mode selections for the device mode function */
#define DM_UPDATE	    1
#define DM_COPY 	    2
#define DM_PROMPT	    4
#define DM_MODIFY	    8

/* device capabilities indices */
#define DC_FIELDS	    1
#define DC_PAPERS	    2
#define DC_PAPERSIZE	    3
#define DC_MINEXTENT	    4
#define DC_MAXEXTENT	    5
#define DC_BINS 	    6
#define DC_DUPLEX	    7
#define DC_SIZE 	    8
#define DC_EXTRA	    9
#define DC_VERSION	    10
#define DC_DRIVER	    11

/* export ordinal definitions */
#define GPA_EXTDEVICEMODE   MAKEINTRESOURCE(22)
#define GPA_DEVICECAPABILITIES MAKEINTRESOURCE(23)
#define GPA_OLDDEVICEMODE   MAKEINTRESOURCE(13)

/* type of pointer returned by GetProcAddress() for ExtDeviceMode */
typedef WORD FAR PASCAL FNDEVMODE(HWND, HANDLE, LPDEVMODE, LPSTR, LPSTR,
    LPDEVMODE, LPSTR, WORD);

typedef FNDEVMODE FAR * LPFNDEVMODE;

/* type of pointer returned for DeviceCapabilities */
typedef DWORD FAR PASCAL FNDEVCAPS(LPSTR, LPSTR, WORD, LPSTR, LPDEVMODE);

typedef FNDEVCAPS FAR * LPFNDEVCAPS;
