/**[f******************************************************************
 * strings.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 *
 *  02 jan 90	peterbe	Added S_DATE
 *  27 sep 89	peterbe	Added NULL_CART.
 *  15 apr 89	peterbe	Removed IDS_ABOUT, SF_SYSABOUT.
 *  01 apr 89	peterbe	Added SF_SYSABOUT, IDS_ABOUT for About dialog,menu
 *   1-26-89	jimmat	Several changes caused by the split out of the
 *			font installer (FINSTALL).
 *   2-21-89	jimmat	Device Mode Dialog box changes for Windows 3.0.
 */

#define NULL_PORT   1

// Menu message no. for date
#define S_DATE		5

#define ERROR_BASE	10
#define WARNING_BASE	50
#define DEVNAME_BASE	100
#define CART_BASE	200
#define ROM_ESC_BASE    300
#define CART_ESC_BASE   400

/*  Strings read from the win.ini file.
 */
#define WININI_BASE         1000
#define WININI_PAPER        (WININI_BASE)
#define WININI_COPIES       (WININI_BASE+1)
#define WININI_ORIENT       (WININI_BASE+2)
#define WININI_PRTRESFAC    (WININI_BASE+3)
#define WININI_TRAY         (WININI_BASE+4)
#define WININI_PRTINDEX     (WININI_BASE+5)
#define WININI_NUMCART      (WININI_BASE+6)
#define WININI_DUPLEX       (WININI_BASE+7)
/* cartridge indices should be last in this list */
#define WININI_CARTINDEX    (WININI_BASE+8)
#define WININI_CARTINDEX1   (WININI_BASE+9)
#define WININI_CARTINDEX2   (WININI_BASE+10)
#define WININI_CARTINDEX3   (WININI_BASE+11)
#define WININI_CARTINDEX4   (WININI_BASE+12)
#define WININI_CARTINDEX5   (WININI_BASE+13)
#define WININI_CARTINDEX6   (WININI_BASE+14)
#define WININI_CARTINDEX7   (WININI_BASE+15)
#define WININI_TXWHITE      (WININI_BASE+16)
#define WININI_OPTIONS      (WININI_BASE+17)
#define WININI_FSVERS       (WININI_BASE+18)
#define WININI_PRTCAPS      (WININI_BASE+19)
#define WININI_PAPERIND     (WININI_BASE+20)
#define WININI_CARTRIDGE    (WININI_BASE+21)
#define WININI_LAST         (WININI_BASE+22)

#define FSUM_NAME           (WININI_LAST+1)
#define FSUM_MEMLIMIT       (WININI_LAST+2)
#define FSUM_FILEPREFIX     (WININI_LAST+3)
#define FSUM_FILEEXTENSION  (WININI_LAST+4)
#define FSUM_MESSAGE        (WININI_LAST+6)
#define FSUM_MSGLAST        (FSUM_MESSAGE+20)

#define IDS_NUMCARTS        (FSUM_MSGLAST+1)
#define IDS_NOCARTS	    (FSUM_MSGLAST+2)
#define IDS_WINDOWS	    (FSUM_MSGLAST+3)
#define IDS_SPOOLER	    (FSUM_MSGLAST+4)

#define IDS_UPPER	    (FSUM_MSGLAST+5)
#define IDS_LOWER	    (FSUM_MSGLAST+6)
#define IDS_MANUAL	    (FSUM_MSGLAST+7)
#define IDS_ENVELOPE	    (FSUM_MSGLAST+8)
#define IDS_AUTO	    (FSUM_MSGLAST+9)

#define IDS_POINT           (FSUM_MSGLAST+10)
#define IDS_BOLD            (FSUM_MSGLAST+11)
#define IDS_ITALIC	    (FSUM_MSGLAST+12)

#define IDS_DUPVBIND        (FSUM_MSGLAST+13)
#define IDS_DUPHBIND        (FSUM_MSGLAST+14)
#define IDS_DUPLBIND        (FSUM_MSGLAST+15)
#define IDS_DUPSBIND	    (FSUM_MSGLAST+16)

#define IDS_LETTER	    (FSUM_MSGLAST+17)
#define IDS_LEGAL	    (FSUM_MSGLAST+18)
#define IDS_LEDGER	    (FSUM_MSGLAST+19)
#define IDS_EXEC	    (FSUM_MSGLAST+20)
#define IDS_A3		    (FSUM_MSGLAST+21)
#define IDS_A4		    (FSUM_MSGLAST+22)
#define IDS_B5		    (FSUM_MSGLAST+23)

#define SF_SOFTFONTS	    (FSUM_MSGLAST+24)

#define NULL_CART	    (FSUM_MSGLAST+25)

/*error string constants*/
#define MAX_PERM_DL         (ERROR_BASE+1)

/*warning string constants*/
#define SOFT_LIMIT          (WARNING_BASE+1)
