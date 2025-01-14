/**[f******************************************************************
 * sfutils.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/*	Dialog states.  The "state" struct is a WORD where the left BYTE
 *	holds the left listbox state and the right BYTE holds the right
 *	listbox state.
 *
 *	craigc:	liars.  one is in the lower four bits, and one is in the
 *	the next four bits, all in the same byte.
 */
#define SFDLG_INACTIVE	0x000		/* Listbox is inactive (no fonts) */
#define SFDLG_RESFONTS	0x001		/* Listbox is showing resident fonts */
#define SFDLG_DSKFONTS	0x002		/* Listbox is showing disk fonts */
#define SFDLG_SELECTED	0x004		/* This side is currently selected */
#define SFDLG_ALLOWEDIT	0x008		/* Selected fonts may be edited */

#define SFDLG_CARTRIDGE 0x100


#ifndef NOBLDDESCSTR
/*	In order for this proc to be used, sflb.h and sfutils.h must
 *	precede this header file.
 */
void FAR PASCAL buildDescStr(LPSTR, int, BOOL, LPSFLBENTRY, LPSFDIRFILE);
#endif

void FAR PASCAL InitLBstrings(HANDLE, LPSTR, LPSTR, LPSTR);
HANDLE FAR PASCAL FillListBox(HWND, HANDLE, WORD, LPSTR, LPSTR);
WORD FAR PASCAL UpdateStatusLine(HWND, WORD, WORD, HANDLE, BOOL);
WORD FAR PASCAL UpdatePermTemp(HWND, HANDLE, WORD, WORD, HANDLE);
void FAR PASCAL UpdateControls(HWND, WORD);
BOOL FAR PASCAL resetLB(HWND, HANDLE, WORD, HANDLE, WORD, int, BOOL);
int FAR PASCAL getUniqueID(HANDLE);
void FAR PASCAL NewFS(HANDLE, LPSTR);
LPSTR FAR PASCAL trimLBcaption(LPSTR);
void FAR PASCAL makeDesc(LPSTR, LPSTR, int, LPSTR, LPSTR, LPSTR);
BOOL FAR PASCAL CanReplace(HWND, HANDLE, int FAR *, LPSTR, int);
void FAR PASCAL PermFontAlert(HWND, HANDLE);
void FAR PASCAL MaxFontAlert(HWND, HANDLE);
BOOL FAR PASCAL alertDU(HWND, HANDLE, WORD);
