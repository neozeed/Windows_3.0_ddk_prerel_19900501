/**[f******************************************************************
 * version.h - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/*	PostScript driver version number
 *
 *  .90  87-1-27  10:45am  The first driver with a version number.
 *  .91  87-1-27   2:00pm  W/O debugging gunk. (oops)
 *  .92  87-1-27   5:25pm  "soft" font fix, lino def to 1270, better graying
 * 1.01  ???????   ?:??    lino name change, tabloid support, justification
 *			   fix, wang additions (with setbin), a4/b5 fix
 * 1.04		25Mar87	Windows 1.04 release.
 * 1.05		23Apr87	PageMaker 1.0A release.
 * 1.06a1	8Jun87	Windows 2.0.  1st Alpha.
 * 1.06a2	15Jun87	Windows 2.0.  2nd Alpha.
 * 1.06b1	19Jun87	Windows 2.0.  1st Beta.
 * 1.06b2	26Jul87	Windows 2.0.  2nd Beta.
 * 2.0b2.2	5Aug87	Windows 2.0.  2nd Beta--new number.
 * 2.0c?	20Aug87	Windows 2.0.  Post beta 2.
 * 2.00		20Aug87	Windows 2.0 release.
 * 2.01		10Sep87	Dynamic downloading of BS and Adobe fonts.
 * 2.02		11Sep87	Added 4 new printers.
 * 2.03		17Sep87	Fixed image areas, added timeout, mod. resolution.
 * 2.04		26Sep87	The "new" look.
 * 2.05		30Sep87	The options dialog.
 * 2.10		4Nov87	1st completed integration of APD stuff.
 * 2.20		20Nov87	Added 17 APDs.
 * 2.25		24Nov87	APD fixes.
 * 2.30		01Dec87	APD additions: Translation, Norm. Transfer fixes
 *			*.dir insertions into PSCRIPT.RC.
 * 2.35		03Dec87	Stage2 implementation of OPTIONS dialog.
 * 2.40		05Dec87	Stage3 implementation of OPTIONS dialog.
 * 2.50		05Dec87	Stage4(final) implementation of OPTIONS dialog
 *			and write-to-WIN.INI optimization.
 * 2.60		15Dec87	Swap tuned GetCharWidth().  1st stage
 *			implementation of incremental RC.
 * 2.70		15Dec87	Full implementation of incremental RC.
 * 2.80		05Jan88	APD fixes for NEC and DP and LPS40 (auto tray)
 *			and APD fixes for WANG printers (bins and papers).
 * 2.90		13Jan88
 * 2.91		20Jan88	Normalized transfer fixes, and image area fixes.
 * 2.92		21Jan88	Adobe download font fixes, Transverse papers.
 * 2.93		26Jan88	Transverse paper fixes, 1st text resource fix,
 *			PSOPTION.C: send to file, VT600 afm fixes.
 * 2.95		03Feb88	Alias stuff, and AppleTalk.
 * 2.96		12Feb88	AppleTalk dialogs, modeless incomplete.
 * 2.97		12Feb88	AppleTalk dialogs almost complete.
 * 2.98		21Feb88	First version AppleTalk stuff.
 * 2.99		25Feb88	Merged code of v2.93 and v2.98.
 * 3.00b0	03Mar88	Fixed bug in COMPACT, reduced status calls in
 *			AppleTalk stuff, fixed download stuff and
 *			added hourglass to download now stuff.
 * 3.00b1	11Mar88	Moved AppleTalk stuff to ATALK.DLL.
 *			Modified GetTechnology() to send BINARY caps.
 * 3.00b2	17Mar88	Minor APD fixes(Executive), minor ATALK stuff.
 * 3.1b1	27Oct88	msd: First Beta v3.1 release, includes new win.ini
 *			switch for downloading binary images (for IBM) and	first
 *			cut at support for Olivetti.
 * 3.????	xxDec88 cleaning up the code.  building proper include files.
 *			making this driver buildable under standard SDK/DDK
 *			enviornment.
 * 3.2		Feb89	win 2.21 release.  color support, bug fixes.
 *
 * 3.3		Feb89	started work on win 3.0 features
 */

#define	VERSION	"Version 3.3"

#define GDI_VERSION		0x300	/* windows version # returned by
					 * DevCaps */

#define DRIVER_VERSION		0x0330	/* used by ExtDevMode() */

#define PRINT_CAP_VERSION	0x0001
