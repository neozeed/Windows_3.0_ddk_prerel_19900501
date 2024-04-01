/**[f******************************************************************
 * fontman.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

typedef enum { fontname, fontescape, fontpfmfile, fontdlfile } stringType;

typedef struct {
    short offset;		/* offset to font in resource file */
    SYMBOLSET symbolSet;	/* symbol set (USASCII, Roman8) */
    short fontID;		/* ID of soft font (-1 if not soft) */
    short indName;		/* Index to Font name */
    short indEscape;		/* Index to escape invoking font */
				/* -1 if soft, downloadable font */
    short indPFMPath;		/* Index to path name (soft font) */
				/* -1 if ROM/cartridge font */
    short indDLPath;		/* Index to path name (download soft font) */
				/* -1 if permanent downloaded font */
    short indPFMName;		/* Index to file name (soft font) */
				/* -1 if ROM/cartridge font */
    short indDLName;		/* Index to file name (download soft font) */
				/* -1 if permanent downloaded font */
    long lPCMOffset;		/* offset of PFM in PCM file */
				/* 0 if soft font PFM file (ie, no offset) */
    BOOL ZCART_hack;		/* Z cartridge hack */
    BOOL QUOTE_hack;		/* Typographic quotes hack */
    short LRUcount;		/* Least Recently used count */
    long memUsage;		/* Memory use of this fonts' bitmaps */
    BYTE onPage;		/* TRUE if font on page */
    short indPrevSoft;		/* Index to prev soft font in list */
    short indNextSoft;		/* Index to next soft font in list */
    short int dfType;		/* Font Metrics that we need ... */
    short int dfPoints;
    short int dfVertRes;
    short int dfHorizRes;
    short int dfAscent;
    short int dfInternalLeading;
    short int dfExternalLeading;
    BYTE dfItalic;
    BYTE dfUnderline;
    BYTE dfStrikeOut;
    short int dfWeight;
    BYTE dfCharSet;
    short int dfPixWidth;
    short int dfPixHeight;
    BYTE dfPitchAndFamily;
    short int dfAvgWidth;
    short int dfMaxWidth;
    BYTE dfFirstChar;
    BYTE dfLastChar;
    BYTE dfDefaultChar;
    BYTE dfBreakChar;
    HANDLE hExtMetrics;         /* Handles to tables we may load ... */
    HANDLE hWidthTable;
    HANDLE hPairKernTable;
    HANDLE hTrackKernTable;
} FONTSUMMARY;

typedef struct {
    short numOpenDC;            /* # DC's using this struct */
    short len;                  /* Number of fontSummary items */
    short firstSoft;            /* First soft font in list */
    BOOL newFS;                 /* TRUE if first time struct created */
    WORD softfonts;		/* Number of key words listed in win.ini */
    PCLDEVMODE environ; 	/* Environment used to create fontSum */
    FONTSUMMARY f[1];           /* Array of fonts */
} FONTSUMMARYHDR;

typedef FONTSUMMARY far *LPFONTSUMMARY;
typedef FONTSUMMARYHDR far *LPFONTSUMMARYHDR;


#ifdef FONTMAN_UTILS
/*  Constants used by LoadPFMStruct
 */
#define FNTLD_WIDTHS        1
#define FNTLD_EXTMETRICS    2
#define FNTLD_PAIRKERN      3
#define FNTLD_TRACKKERN     4

BOOL far PASCAL LoadFontString(LPDEVICE, LPSTR, short, stringType, short);
LPSTR far PASCAL LoadWidthTable(LPDEVICE, short);
void far PASCAL UnloadWidthTable(LPDEVICE, short);
#endif

#ifdef FONTMAN_ENABLE
HANDLE far PASCAL GetFontSummary(LPSTR, LPSTR, LPPCLDEVMODE, HANDLE);
#endif

#ifdef FONTMAN_DISABLE
HANDLE far PASCAL FreeFontSummary(LPDEVICE);
#endif

#ifdef DEBUG
void FAR PASCAL DBGdumpFontSummary(LPFONTSUMMARYHDR, short);
#endif
