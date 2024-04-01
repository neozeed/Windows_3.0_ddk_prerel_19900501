/**[f******************************************************************
 * paperfmt.h -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/*  The PAPERFORMATs are stored in a binary file that the driver
 *  accesses from the resources (HPPCL.RC).  It consists of a header
 *  that points to an array of lists and an array of paper formats.
 *  A list contains indexes to the paper types supported by one
 *  printer.  Each paper format contains the dimensions of the page
 *  and the selection string.
 *
 *  The printer strings in the resource file contain the index into
 *  the array of lists for each printer.
 */
#define PAPERID_LETTER	0x0001
#define PAPERID_LEGAL	0x0002
#define PAPERID_EXEC	0x0004
#define PAPERID_LEDGER	0x0008
#define PAPERID_A3	0x0010
#define PAPERID_A4	0x0020
#define PAPERID_B5	0x0040

#define MAXPAPERFORMAT	7

/*  This struct is duplicated in device.i */
typedef struct {
    short xPhys;		/* physical paper width */
    short yPhys;		/* physical paper height */
    short xImage;		/* image area width */
    short yImage;		/* image area height */
    short xPrintingOffset;  	/* printing offset in x direction */
    short yPrintingOffset;  	/* printing offset in y direction */
    BYTE select[16];		/* paper select string */
    } PAPERFORMAT;

typedef struct {
    short id;			/* paper type (letter, legal, etc) */
    short indPortPaperFormat;
    short indLandPaperFormat;
    } PAPERLISTITEM;

typedef struct {
    short len;			/* number of entries in the list */
    PAPERLISTITEM p[MAXPAPERFORMAT];
    } PAPERLIST;

typedef struct {
    short numLists;	    	/* number of paper list structs */
    short numFormats;		/* number of paper format structs */
    DWORD offsLists;		/* offset to array of lists */
    DWORD offsFormats;	    	/* offset to array of paper formats */
    } PAPERHEAD;


/*  ONLY MAKERES.C should define this symbol.
 */
#ifdef PAPER_DATA

/*  PAPERLIST
 *
 *  Each entry in this list contains the paper ids and offsets into
 *  the array of PAPERFORMATs for the corresponding portrait and
 *  landscape paper formats.
 *
 *  NOTE:  If you add an entry to the paper list, you must change the
 *  constant MAX_PAPERLIST in resource.h to reflect the number of
 *  entries.
 */
PAPERLIST PaperList[6] = {

/*  0 Standard LaserJet, LaserJet+, LaserJet 500+, Apricot, Epson,
 *  Kyocera, NEC, Okidata, QuadLaser, Tandy LP-1000, Tegra Genesis
 */
	{ 4,	PAPERID_LETTER,	 0,  4,
		PAPERID_LEGAL,	 1,  5,
		PAPERID_A4,	 2,  6,
		PAPERID_B5,	 3,  7	},

/*  1 LaserJet IIP, LaserJet IID
 */
	{ 4,	PAPERID_LETTER,	 8, 12,
		PAPERID_LEGAL,	 9, 13,
		PAPERID_EXEC,	10, 14,
		PAPERID_A4,	11, 15	},

/*  2 LaserJet 2000
 */
	{ 6,	PAPERID_LETTER, 16, 22,
		PAPERID_LEGAL,	17, 23,
		PAPERID_EXEC,	18, 24,
		PAPERID_LEDGER, 19, 25,
		PAPERID_A3,	20, 26,
		PAPERID_A4,	21, 27	},

/*  3 Toshiba PageLaser12
 */
	{ 5,	PAPERID_LETTER,	 8, 12,
		PAPERID_LEGAL,	 9, 13,
		PAPERID_EXEC,	10, 14,
		PAPERID_A4,	11, 15,
		PAPERID_B5,	 3,  7	},

/*  4 Wang LDP8
 */
	{ 4,	PAPERID_LETTER,	 8, 12,
		PAPERID_LEGAL,	 9, 13,
		PAPERID_A3,	20, 26,
		PAPERID_A4,	11, 15	},

/*  5 LaserJet Series II
 */
	{ 4,	PAPERID_LETTER,	28, 32,
		PAPERID_LEGAL,	29, 33,
		PAPERID_EXEC,	30, 34,
		PAPERID_A4,	31, 35	}

	};


/*  PAPER SIZES
 *
 *  xPhys, yPhys, xImage, yImage, xPrintingOffset,
 *  yPrintingOffset, select (string)
 */
PAPERFORMAT PaperFormat[36] = {

/*  LaserJet, LaserJet+, and compatibles.
 */
/*  0 P LETTER */  { 2550, 3300, 2400, 3150, 56, 59, "\033&l0o66p4d1e42F" },
/*  1 P LEGAL */   { 2550, 4200, 2400, 4080, 56, 59, "\033&l0o84p4d1e54F" },
/*  2 P A4 */	   { 2480, 3507, 2338, 3407, 48, 59, "\033&l0o70p4d1e45F" },
/*  3 P B5 */	   { 2150, 3036, 2008, 2917, 48, 59, "\033&l0o60p4d1e38F" },
/*  4 L LETTER */  { 3300, 2550, 3150, 2400, 59, 56, "\033&l66p1o1E"	  },
/*  5 L LEGAL */   { 4200, 2550, 4080, 2400, 59, 56, "\033&l84p1o1E"	  },
/*  6 L A4 */	   { 3507, 2480, 3407, 2338, 59, 48, "\033&l70p1o1E"	  },
/*  7 L B5 */	   { 3036, 2150, 2917, 2008, 59, 48, "\033&l60p1o1E"	  },

/*  LaserJet IIP, LaserJet IID, and compatibles.
 */
/*  8 P LETTER */  { 2550, 3300, 2400, 3200, 75, 50, "\033&l0o2a4d1e42F"  },
/*  9 P LEGAL */   { 2550, 4200, 2400, 4100, 75, 50, "\033&l0o3a4d1e54F"  },
/* 10 P EXEC */	   { 2175, 3150, 2025, 3050, 75, 50, "\033&l0o1a4d1e40F"  },
/* 11 P A4 */	   { 2480, 3507, 2338, 3407, 71, 50, "\033&l0o26a4d1e45F" },
/* 12 L LETTER */  { 3300, 2550, 3180, 2450, 60, 50, "\033&l1o2a1e48F"	  },
/* 13 L LEGAL */   { 4200, 2550, 4080, 2450, 60, 50, "\033&l1o3a1e48F"	  },
/* 14 L EXEC */	   { 3150, 2175, 3030, 2075, 60, 50, "\033&l1o1a1e40F"	  },
/* 15 L A4 */	   { 3507, 2480, 3389, 2380, 59, 50, "\033&l1o26a1e46F"	  },

/*  LaserJet 2000
 */
/* 16 P LETTER */  { 2550, 3300, 2400, 3200, 75, 50, "\033&l0o2a1e64F"	  },
/* 17 P LEGAL */   { 2550, 4200, 2400, 4100, 75, 50, "\033&l0o3a1e82F"	  },
/* 18 P EXEC */	   { 2175, 3150, 2025, 3050, 75, 50, "\033&l0o1a1e61F"	  },
/* 19 P LEDGER */  { 3300, 5100, 3150, 5000, 75, 50, "\033&l0o6a1e100F"	  },
/* 20 P A3 */	   { 3507, 4960, 3365, 4860, 71, 50, "\033&l0o27a1e97F"	  },
/* 21 P A4 */	   { 2480, 3507, 2338, 3407, 71, 50, "\033&l0o26a1e68F"	  },
/* 22 L LETTER */  { 3300, 2550, 3180, 2450, 60, 50, "\033&l1o2a1e49F"	  },
/* 23 L LEGAL */   { 4200, 2550, 4080, 2450, 60, 50, "\033&l1o3a1e49F"	  },
/* 24 L EXEC */	   { 3150, 2175, 3030, 2065, 60, 50, "\033&l1o1a1e41F"	  },
/* 25 L LEDGER */  { 5100, 3300, 4980, 3150, 60, 50, "\033&l1o6a1e70F"	  },
/* 26 L A3 */	   { 4960, 3507, 4842, 3407, 59, 50, "\033&l1o27a1e68F"	  },
/* 27 L A4 */	   { 3507, 2480, 3389, 2380, 59, 50, "\033&l1o26a1e47F"	  },

/*  LaserJet Series II
 */
/* 28 P LETTER */  { 2550, 3300, 2400, 3150, 56, 59, "\033&l0o2a4d1e42F"  },
/* 29 P LEGAL */   { 2550, 4200, 2400, 4080, 56, 59, "\033&l0o3a4d1e54F"  },
/* 30 P EXEC */	   { 2175, 3150, 2025, 3050, 56, 59, "\033&l0o1a4d1e40F"  },
/* 31 P A4 */	   { 2480, 3507, 2338, 3407, 48, 59, "\033&l0o26a4d1e45F" },
/* 32 L LETTER */  { 3300, 2550, 3150, 2400, 59, 56, "\033&l1o2a1e48F"	  },
/* 33 L LEGAL */   { 4200, 2550, 4080, 2400, 59, 56, "\033&l1o3a1e48F"	  },
/* 34 L EXEC */	   { 3150, 2175, 3030, 2075, 59, 56, "\033&l1o1a1e40F"	  },
/* 35 L A4 */	   { 3507, 2480, 3407, 2338, 59, 48, "\033&l1o26a1e46F"	  }

	};
#endif

