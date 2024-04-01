/*---------------------------------------------------------------------------*\
| WINDOWS INTEGRATED TEST ENVIRONMENT HEADER FILE                             |
\*---------------------------------------------------------------------------*/

#define POLYGON_STAR       100
#define ELLIPSE            101
#define POLYLINE_TRAPEZOID 102

/*---------------------------------------------------------------------------*\
| ENUMERATE STRUCTURE                                                         |
|   This structure is used to identify data for Fonts, Pens, and Brushes for  |
|   a Display or Printer device.  The modules (FONTS,BRUSHES,PENS) contain    |
|   the routines which act upon this structure.                               |
\*---------------------------------------------------------------------------*/
typedef struct
     {
          HDC          hDC;                      /* Handle to device context */
          GLOBALHANDLE hGMem;                    /* Handle to enumed structs */
          GLOBALHANDLE hGFlg;                    /* Handle to enumed flags   */
          short        nCount;                   /* Count of objects in array*/
     } ENUMERATE;
typedef ENUMERATE      *PENUMERATE;
typedef ENUMERATE NEAR *NPENUMERATE;
typedef ENUMERATE FAR  *LPENUMERATE;

/*---------------------------------------------------------------------------*\
| FONT STRUCTURE                                                              |
|   This structure is used to identify the fields in the enumerate structure  |
|   global array (ENUMERATE).  These coorespond to the structures passed to   |
|   the call-backs by Windows.                                                |
\*---------------------------------------------------------------------------*/
typedef struct
     {
          short      nFontType;                  /* Font type (GDI/DEV)      */
          LOGFONT    lf;                         /* LogFont structure        */
          TEXTMETRIC tm;                         /* TextMetric structure     */
     } FONT;
typedef FONT      *PFONT;
typedef FONT NEAR *NPFONT;
typedef FONT FAR  *LPFONT;

typedef struct
     {
          short nIndex;
          PSTR  szType;
     } PRINTCAPS;
typedef PRINTCAPS FAR *LPPRINTCAPS;

typedef struct
     {
          char szProfile[80];
          char szName[80];
          char szDriver[40];
          char szPort[40];
          char szSystemVer[10];
          char szDriverVer[10];
     } PRINTER;
typedef PRINTER FAR *LPPRINTER;

typedef struct
     {
          short nDriverVersion;
          short nTechnology;
          short nHorzSizeMM;
          short nVertSizeMM;
          short nHorzRes;
          short nVertRes;
          short nLogPixelsX;
          short nLogPixelsY;
          short nBitsPixel;
          short nPlanes;
          short nBrushes;
          short nPens;
          short nFonts;
          short nColors;
          short nAspectX;
          short nAspectY;
          short nAspectXY;
          short nPDeviceSize;
          WORD  wClipCaps;
          WORD  wRasterCaps;
          WORD  wCurveCaps;
          WORD  wLineCaps;
          WORD  wPolygonCaps;
          WORD  wTextCaps;
     } DEVCAPABILITIES;
typedef DEVCAPABILITIES FAR *LPDEVCAPABILITIES;


/*---------------------------------------------------------------------------*\
| BITMAPS FUNCTIONS                                                           |
\*---------------------------------------------------------------------------*/
HBITMAP FAR PASCAL CreateColorBitmap(HDC,short,short,HBRUSH);
BOOL    FAR PASCAL DrawBitmapToDevice(HDC,short,short,HBITMAP,DWORD);
BOOL    FAR PASCAL PrintBitBltRop(HDC,short,short,short,short,HBRUSH,HBRUSH,HBRUSH,DWORD);

/*---------------------------------------------------------------------------*\
| TEXT FUNCTIONS                                                              |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL ExtTextOutClip(HDC,short,LPRECT,LPSTR,short,LPINT);

/*---------------------------------------------------------------------------*\
| POLYGON FUNCTIONS                                                           |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PolygonTest(HDC,short,short,short,short,int);

/*---------------------------------------------------------------------------*\
| CURVE FUNCTIONS                                                             |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL EllipseTest(HDC,short,short,short,short,int);

/*---------------------------------------------------------------------------*\
| LINE FUNCTIONS                                                              |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PolylinePenTest(HDC,short,short,short,short,LPLOGPEN,int);


/*---------------------------------------------------------------------------*\
| LINE FUNCTIONS                                                              |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL OutputObject(HDC,short,short,short,short,int);

/*---------------------------------------------------------------------------*\
| TEST FUNCTIONS                                                              |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL DisplayResourceMessage(HWND,HANDLE,WORD);
BOOL FAR PASCAL GetDeviceCapabilities(HDC,LPDEVCAPABILITIES);
BOOL FAR PASCAL GetDeviceFonts(HDC,HANDLE,LPENUMERATE);
BOOL FAR PASCAL GetDeviceBrushes(HDC,HANDLE,LPENUMERATE);
BOOL FAR PASCAL GetDevicePens(HDC,HANDLE,LPENUMERATE);
int  FAR PASCAL EnumAllFontFaces(LPLOGFONT,LPTEXTMETRIC,short,LPENUMERATE);
int  FAR PASCAL EnumAllFonts(LPLOGFONT,LPTEXTMETRIC,short,LPENUMERATE);
int  FAR PASCAL EnumAllBrushes(LPLOGBRUSH,LPENUMERATE);
int  FAR PASCAL EnumAllPens(LPLOGPEN,LPENUMERATE);
int  FAR PASCAL CreateLogFile(LPSTR);
int  FAR PASCAL WriteLogFile(LPSTR,LPSTR);
void FAR PASCAL GetTime(LPINT,LPINT,LPINT);
void FAR PASCAL GetDate(LPINT,LPINT,LPINT,LPINT);
BOOL FAR PASCAL OutputGrayScale(HDC,short,short,short,short);
