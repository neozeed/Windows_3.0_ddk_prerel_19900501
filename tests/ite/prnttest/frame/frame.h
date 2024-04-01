/*---------------------------------------------------------------------------*\
| HEADER FILE to include with applications IMPORTING this library.            |
\*---------------------------------------------------------------------------*/

/*----------------------------------------------*\
| _TEXTX FUNCTIONS                               |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintText(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPTEST,LPSTR);
BOOL            PrintExtTextOut(HDC,short,LPDEVINFO,HDEVOBJECT,LPTEST);
BOOL            PrintSYNExtTextOut(HDC,short,WORD,LPDEVINFO,HDEVOBJECT,LPTEST);

/*----------------------------------------------*\
| _RASTER FUNCTIONS                              |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintBitmaps(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPTEST,LPSTR);
BOOL            PrintStretchBlt(HDC,LPDEVINFO,LPTEST);
BOOL            PrintBitBlt(HDC,LPDEVINFO,HDEVOBJECT,LPTEST);
HBRUSH          GetNextTestBrush(HDEVOBJECT,HDEVOBJECT,LPINT);

/*----------------------------------------------*\
| _CURVE FUNCTIONS                               |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintCurves(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPTEST,LPSTR);
BOOL            PrintEllipses(HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,int,LPTEST);

/*----------------------------------------------*\
| _LINE FUNCTIONS                                |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintLines(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPTEST,LPSTR);
BOOL            PrintPolyLines(HDC,int,int,LPDEVINFO,HDEVOBJECT,LPTEST);

/*----------------------------------------------*\
| _POLYGON FUNCTIONS                             |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintPolygons(HWND,HDC,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,HDEVOBJECT,LPTEST,LPSTR);
BOOL            PrintPolygonStar(HDC,int,int,LPDEVINFO,HDEVOBJECT,HDEVOBJECT,LPTEST);

/*----------------------------------------------*\
| _HEADER FUNCTIONS                              |
\*----------------------------------------------*/
BOOL FAR PASCAL PrintTitlePage(HDC,LPPRINTER,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintFunctionSupport(HDC,LPPRINTER,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintPrintableArea(HDC,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintDeviceCapabilities(HDC,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintDeviceBrushes(HDC,HDEVOBJECT,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintDevicePens(HDC,HDEVOBJECT,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintDeviceFonts(HDC,HDEVOBJECT,LPDEVINFO,LPSTR);
BOOL FAR PASCAL PrintGrayScale(HDC,LPDEVINFO,LPSTR);
BOOL            PrintFontInfoExp(HDC,LPFONT,int,LPDEVINFO);
BOOL            PrintFontInfoCon(HDC,LPFONT,int,LPDEVINFO);

/*----------------------------------------------*\
| _TEXT FUNCTIONS                                |
\*----------------------------------------------*/
BOOL FAR PrintFooter(HDC,LPDEVINFO,LPSTR);
int  FAR PrintTestDescription(HDC,WORD,LPDEVINFO);
BOOL FAR EndOfPage(short,short,short);
