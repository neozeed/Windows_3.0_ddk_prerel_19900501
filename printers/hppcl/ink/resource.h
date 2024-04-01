/**[f******************************************************************
 * resource.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 *
 *  10 feb 90	clarkc 	Increased MAX_PAPERLST to 6
 *
 *  27 nov 89	peterbe	Increased MAX_PRINTERS to 72
 *
 *  25 oct 89	peterbe	Increased MAX_PRINTERS to 70
 *
 *  19 sep 89	peterbe	Changed LITTLESPUD to HPLJIIP.
 *
 *  06 sep 89	peterbe	Changed #include "drivinit.h" to #include <drivinit.h>
 *
 *  29 aug 89	peterbe	Removed old, commented-out defs.
 *			Defined LITTLESPUD.
 *
 *  25 aug 89	craigc	Symbol font conversion. (added MATH8 set)
 *
 *  22 aug 89	peterbe	Increased MAX_PRINTERS to 60 again.
 *
 *  21 aug 89	peterbe	Increased MAX_PRINTERS to 60, then back to 50.
 *
 *  22 jun 89	peterbe	Added CAP_XXX, reserved for Microsoft use.
 *
 *  07 jun 89	peterbe	Added IDPORTLAND for orientation group's icon
 *			Also added IDHELP.
 *
 *  16 may 89	peterbe	Added () pair in #define OPTN_IDDPTEK.
 *
 *  15 apr 89	peterbe	Added IDABOUT for About pushbutton.
 *
 *  01 apr 89	peterbe	Added SFABOUT for About dialog.
 *
 *   1-26-89	jimmat	Many changes cause by the split-out of the font
 *			installer (FINSTALL).  I don't know why some of this
 *			stuff is in a file named resource.h, but...
 *   2-06-89	jimmat	Changes to DEVMODE structure for new driver
 *			initialization interface.
 *   2-21-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
 */

// get this from include directory, not current.
#include <drivinit.h>

/*  Compiler switches.
 */
#define SOFTFONTS_ENABLED
#define DIALOG_MESSAGES


/* Logical font constants */

#define DEFAULT_PITCH         0
#define FIXED_PITCH           1
#define VARIABLE_PITCH        2

/* GDI font families. */

#define FF_DONTCARE     (0<<4)  /* Don't care or don't know. */
#define FF_ROMAN        (1<<4)  /* Variable stroke width, serifed. */
                                /* Times Roman, Century Schoolbook, etc. */
#define FF_SWISS        (2<<4)  /* Variable stroke width, sans-serifed. */
                                /* Helvetica, Swiss, etc. */
#define FF_MODERN       (3<<4)  /* Constant stroke width, serifed or sans-serifed. */
                                /* Pica, Elite, Courier, etc. */
#define FF_SCRIPT       (4<<4)  /* Cursive, etc. */
#define FF_DECORATIVE   (5<<4)  /* Old English, etc. */


/*  Resource file families
 */

#define XFACES     256
#define TRANSTBL   257
#define PAPERFMT   258
#define MYFONT	   259
#define PCMFILE    260

/*  Resource constants
 */
#define XTBL_USASCII    1
#define XTBL_ROMAN8     2
#define XTBL_GENERIC7   3
#define XTBL_GENERIC8   4
#define XTBL_ECMA94	5
#define XTBL_MATH8	6

#define PAPER1          1

#define FACES1          1

/* dialogs */
#define DTMODE		1
#define OPTIONS 	2
#define LOTSOF_FONTSDLG 3

#define SFABOUT		4

/* printer capability flags in .epCaps */

#define HPJET	    0x0001  /*printer has capabilities of a 'basic' laserjet*/
#define HPPLUS	    0x0002  /*printer has capabilities of a laserjet plus*/
#define HP500	    0x0004  /*printer has capabilities of a laserjet 500*/
#define LOTRAY	    0x0008  /*lower tray is handled*/
#define NOSOFT	    0x0010  /*printer doesn't support d.l. fonts*/
#define NOMAN	    0x0020  /*manual feed is not supported*/
#define NOBITSTRIP  0x0040  /*print cannot support internal bit stripping*/
#define MANENVFEED  0x0080  /*printer supports manual envelope feed*/
#define HPEMUL	    0x0100  /*printer emulates an hplaserjet*/
#define NEWENVFEED  0x0200  /*printer supports new (LJ IID) envelope feed*/
#define HPIIDDUPLEX 0x0400  /*printer can print LJ IID duplex*/
#define HPLJIIP	    0x0800  /*Has HP LJ IIP white rules and compression */
#define PRINTDUPLEX 0x1000  /*printer can print duplex*/
#define AUTOSELECT  0x2000  /*printer selects bin based on paper size*/
#define BOTHORIENT  0x4000  /*printer does autorotation of fonts*/
#define HPSERIESII  0x8000  /*printer has capabilities of a series II*/

#define ANYENVFEED  (MANENVFEED | NEWENVFEED)
#define ANYDUPLEX   (PRINTDUPLEX | HPIIDDUPLEX)


/* bits for the options dialog
 *
 *  IF YOU ADD A CAP BIT HERE, ALSO PUT IT IN DEVICE.I
 */
#define OPTIONS_DPTEKCARD   0x0001  /* enable DP-TEK LaserPort */
#define OPTIONS_RESETJOB    0x0002  /* reset printer between jobs */
#define OPTIONS_FORCESOFT   0x0004  /* always do soft fonts */
#define OPTIONS_VERTCLIP    0x0008  /* allow vertical clip */

/*default caps = HP LaserJet*/

#define JETCAPS 	(NOSOFT | HPJET)
#define JETOPTS 	(OPTIONS_DPTEKCARD | OPTIONS_RESETJOB)
#define DEVMODE_MAXCART 8

typedef struct {
    DEVMODE dm; 		      /* standard Device Mode structure       */
    short prtResFac;		      /* printer resolution shift factor      */
    short prtCaps;		      /* bit field of printer capabilites     */
    short paperInd;		      /* idx in PaperList of supported papers */
    short prtIndex;		      /* index DEVNAME in string table	      */
    short cartIndex[DEVMODE_MAXCART]; /* index in string table for cart       */
    short numCartridges;	      /* number of cartridges selected	      */
    short cartind[DEVMODE_MAXCART];   /* index to cartridge font	      */
    short cartcount[DEVMODE_MAXCART]; /* number of cartridge fonts	      */
    short romind;		      /* index from ROM_ESC_BASE to rom fonts */
    short romcount;		      /* number of rom fonts		      */
    short availmem;		      /* available mem in Kbytes	      */
    short maxPgSoft;		      /* limit of soft fonts per page	      */
    short maxSoft;		      /* max softfonts that can be downloaded */
    short txwhite;		      /* white text intensity		      */
    short options;		      /* bit field for options dialog	      */
    short fsvers;		      /* fontSummary version number	      */
} PCLDEVMODE;

typedef PCLDEVMODE far *LPPCLDEVMODE;

/*scalefactors for GDI to go from 300dpi to printer res*/
#define SF75    2
#define SF150   1
#define SF300   0

/* dialog and device mode constants */

#define PRTBOX      10
#define MEMBOX      11
#define CARTBOX     12

#define PORTRAIT    15
#define LANDSCAPE   16
#define IDPORTLAND  17

#define SIZEBOX     20

#define DPI75       30
#define DPI150      31
#define DPI300      32

#define TRAYBOX     33

#define COPYBOX     40
#define NUMCARTS    41
#define NODUPLEX    42
#define VDUPLEX     43
#define HDUPLEX     44

#define IDSOFTFONT  50
#define IDOPTION    51
#define IDABOUT     52
#define IDHELP	    53

/* Icon controls for options dialog */

#define OPT_ICON	70

/* other options dialog constants */

#define OPTN_DLG_BASE   100
#define OPTN_IDDPTEK    (OPTN_DLG_BASE+OPTIONS_DPTEKCARD)


#ifdef DIALOG_MESSAGES
/* Loading lots of fonts dialog constants */
#define LOTSFONT_PCNT   200
#define LOTSFONT_FONT   201
#endif


/*actual memory values*/

#define JETMEM	    65	       /*available mem on laserjet*/
#define PLUSMEM     395        /*available mem on a laserjet plus*/
#define OPT2M	    1895       /*available mem on a 2M option*/
#define PlusMemStg  "512 KB"

/*default # fonts per page/job*/

#define MAXPGSOFT 16
#define MAXSOFT   32

#define MAX_COPIES  9999
#define MAX_MEM     9999

/*stuff for print dialog*/

#define MAX_PRINTERS	 72
#define MAX_CARTRIDGES	 50
#define MAX_PAPERLIST	 6
#define MAX_PAPERSOURCES 10
#define MAX_PAPERSIZES	 10
#define STR_LEN 	 100

/*number of cartridges visible in the cartridge list box*/

#define NUMVISIBLE_CARTS 5
