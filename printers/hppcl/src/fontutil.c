/**[f******************************************************************
 * fontutil.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/*****************************   fontutil.c   ******************************/
/*
 *  Fontutil: This module contains utilities for accessing the fontSummary
 *  structure.
 *
 *  rev:
 *
 *  11-29-86    skk     made LoadFontString build escape string for
 *                          downloadable fonts
 *  11-26-86    msd     switched to getting escape sequences from the RC
 *  11-23-86    msd     added Load/UnloadWidthTable
 *  11-22-86    msd     module creation
 *
 *   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
 *   1-25-89	jimmat	Use global hLibInst instead of GetModuleName() -
 *			lclstr.h no longer required by this file.
 */


#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#define FONTMAN_ENABLE
#include "fontman.h"
#include "fonts.h"
#include "utils.h"
#define SEG_PHYSICAL
#include "memoman.h"

/*  lockfont utility
 */
#include "lockfont.c"
#include "makefsnm.c"
#include "loadfile.c"

#define LOCAL static

/*  Local debug structure.
 */
#ifdef DEBUG
    #undef LOCAL_DEBUG
    #undef DBGdumpwidths
    #undef DBGnewidthtable
#endif

#ifdef LOCAL_DEBUG
    #define DBGentry(msg) DBMSG(msg)
    #define DBGerr(msg) DBMSG(msg)
    #define DBGloadfontstring(msg) DBMSG(msg)
    #define DBGloadwidthtable(msg) /*DBMSG(msg)*/
#else
    #define DBGentry(msg) /*null*/
    #define DBGerr(msg) /*null*/
    #define DBGloadfontstring(msg) /*null*/
    #define DBGloadwidthtable(msg) /*null*/
#endif


LOCAL LPSTR loadWidths(LPFONTSUMMARYHDR, LPFONTSUMMARY);


/*  LoadFontString
 *
 *  Retrieve the font string from the font string table at the end of
 *  the fontSummary struct.
 */
BOOL far PASCAL LoadFontString(lpDevice, lpDest, len, sType, fontInd)
    LPDEVICE lpDevice;
    LPSTR lpDest;
    short len;
    stringType sType;
    short fontInd;
    {
    register LPFONTSUMMARYHDR lpFontSummary;
    LPSTR lpString;
    BOOL status = TRUE;
    short ind = -1, n;
    ESCtype escape;

    DBGentry(("LoadFontString(%lp,%d,%d,%d,%d)\n",
        lpDevice, lpDest, len, sType, fontInd));

    if (lpFontSummary = lockFontSummary(lpDevice))
        {
        if ((fontInd >= 0) && (fontInd < lpFontSummary->len))
            switch (sType)
                {
                case fontescape:
                    ind = lpFontSummary->f[fontInd].indEscape;
                    break;
                case fontpfmfile:
                    ind = lpFontSummary->f[fontInd].indPFMName;
                    break;
                case fontdlfile:
                    ind = lpFontSummary->f[fontInd].indDLName;
                    break;
                case fontname:
                default:
                    ind = lpFontSummary->f[fontInd].indName;
                    break;
                }

        if (ind > -1)
            {
            /*  Locate string table at end of fontSummary struct.
             */
            lpString = (LPSTR) & lpFontSummary->f[lpFontSummary->len];

            /*  Locate string inside of stringtable.
             */
            lpString = &lpString[ind];
            DBGloadfontstring(("LoadFontString(): string %lp, %ls\n",
                lpString, (lpString) ? lpString : (LPSTR)"{ null }"));

            /*  Copy into the caller's string.
             */
            lmemcpy(lpDest, lpString, len);
            lpDest[len-1] ='\0';
            }
        else if (sType == fontescape)
            {
            ind=lpFontSummary->f[fontInd].indDLName;

            /*  Handle downloadable fonts.
             */
            if ((ind==-1) || (lpFontSummary->f[fontInd].LRUcount!=-1))
                {
                /*  Font is downloaded so generate escape using ID.
                 */
                n = MakeEscape((lpESC)&escape, DES_FONT,
                    lpFontSummary->f[fontInd].fontID);
                if ((n + 1) > len)
                    n = len - 1;
                lmemcpy(lpDest, (LPSTR)&escape, n);
                lpDest[n] = '\0';
                UpdateSoftInfo(lpDevice, lpFontSummary, fontInd);
                ind = 0;
                }
            else
                ind = -1;
            }

        /*  String not loaded.
         */
        if (ind <= -1)
            {
            DBGloadfontstring(("LoadFontString(): string does *not* exist\n"));
            lpDest[0] = '\0';
            status = FALSE;
            }

        /*  Free fontSummary struct.
         */
        unlockFontSummary(lpDevice);
        }
    else
        {
        DBGerr(("LoadFontString(): could *not* lock hFontSummary\n"));
        lpDest[0] = '\0';
        status = FALSE;
        }

    DBGloadfontstring(("LoadFontString(): return %s\n",
        status ? "SUCCESS" : "FAILURE"));

    return (status);
    }


/*  LoadWidthTable
 *
 *  Load the character extents table corresponding to the fontSummary
 *  font at the passed in index.
 */
LPSTR far PASCAL LoadWidthTable(lpDevice, fontInd)
    LPDEVICE lpDevice;
    short fontInd;
    {
    LPFONTSUMMARY lpSummary;
    LPFONTSUMMARYHDR lpFontSummary;
    LPSTR lpWidthTable = 0L;
    BOOL status = TRUE;

    DBGloadwidthtable(("LoadWidthTable(%lp,%d)\n", lpDevice, fontInd));

    if (lpFontSummary = lockFontSummary(lpDevice))
        {
        /*  FontSummary successfully locked, make sure fontInd is in a
         *  valid range.
         */
        if ((fontInd >= 0) && (fontInd < lpFontSummary->len))
            {
            /*  fontInd is valid, get pointer to fontSummary info and
             *  attempt to lock down the widthTable if it has already once
             *  been loaded.
             */
            lpSummary = &lpFontSummary->f[fontInd];

            if (!lpSummary->hWidthTable ||
                !(lpWidthTable = GlobalLock(lpSummary->hWidthTable)))
                {
                if (!(lpWidthTable = loadWidths(lpFontSummary, lpSummary)))
                    {
                    DBGerr(("LoadWidthTable(): could *not* load width table\n"));
                    lpSummary->hWidthTable = 0;
                    unlockFontSummary(lpDevice);
                    }
                #ifdef DBGnewidthtable
                else {
                    DBMSG(("LoadWidthTable(): NEW width table created at fontInd %d\n", fontInd));
                    }
                #endif
                }
            #ifdef LOCAL_DEBUG
            else {
                DBGloadwidthtable(("LoadWithTable(): width table already exists, successfully locked\n"));
                }
            #endif
            }
        else
            {
            DBGerr(("LoadWidthTable(): received invalid fontInd (%d)\n",
                fontInd));
            unlockFontSummary(lpDevice);
            }
        }
    #ifdef LOCAL_DEBUG
    else {
        DBGerr(("LoadWidthTable(): could *not* lock hFontSummary\n"));
        }
    #endif

    DBGloadwidthtable(("...end of LoadWidthTable, return %lp\n",
        lpWidthTable));

    return (lpWidthTable);
    }


void far PASCAL UnloadWidthTable(lpDevice, fontInd)
    LPDEVICE lpDevice;
    short fontInd;
    {
    LPFONTSUMMARY lpSummary;
    LPFONTSUMMARYHDR lpFontSummary;

    DBGloadwidthtable(("UnloadWidthTable(%lp,%d)\n", lpDevice, fontInd));

    /*  Lock down (again) the fontSummary struct.  LoadWidthTable already
     *  locked it, we call lock again to pick up the pointer to the struct.
     */
    if (lpFontSummary = lockFontSummary(lpDevice))
        {
        /*  FontSummary successfully locked, make sure fontInd is in a
         *  valid range.
         */
        if ((fontInd >= 0) && (fontInd < lpFontSummary->len))
            {
            /*  fontInd is valid, get pointer to fontSummary info and
             *  if the handle to the width table exists, unlock the
             *  width table
             */
            lpSummary = &lpFontSummary->f[fontInd];

            if (lpSummary->hWidthTable)
                {
                DBGloadwidthtable((
                    "UnloadWidthTable(): unlocking width table\n"));
                GlobalUnlock(lpSummary->hWidthTable);
                }
            }
        #ifdef LOCAL_DEBUG
        else {
            DBGerr(("UnloadWidthTable(): received invalid fontInd (%d)\n",
                fontInd));
            }
        #endif
        }
    #ifdef LOCAL_DEBUG
    else {
        DBGerr(("UnloadWidthTable(): could *not* lock hFontSummary\n"));
        }
    #endif

    unlockFontSummary(lpDevice);
    }


/***************************************************************************/
/**************************   Local Procedures   ***************************/


/*  loadWidths
 *
 *  Load the width table from the resource or soft font info.
 */
LOCAL LPSTR loadWidths(lpFontSummary, lpSummary)
    LPFONTSUMMARYHDR lpFontSummary; 
    LPFONTSUMMARY lpSummary;
    {
    LPSTR lpWidthTable = 0L;
    unsigned sizeWidthTable;
    extern HANDLE hLibInst;

    DBGentry(("loadWidths(%lp,%lp)\n", lpFontSummary, lpSummary));

    /*  Calc size of width table.
     */
    sizeWidthTable =
        (lpSummary->dfLastChar - lpSummary->dfFirstChar + 2) * 2;

    /*  Free the handle to the width table if it exists.
     */
    if (lpSummary->hWidthTable)
        {
        GlobalFree(lpSummary->hWidthTable);
        lpSummary->hWidthTable = 0;
        }

    /*  Attempt to allocate the width table.
     */
    if (!(lpSummary->hWidthTable =
        GlobalAlloc(GMEM_MOVEABLE | GMEM_LOWER | GMEM_DDESHARE | GMEM_DISCARDABLE,
        (DWORD)sizeWidthTable)))
        {
        DBGerr(("loadWidths(): Could *not* alloc width table\n"));
        goto backout2;
        }

    /*  Attempt to lock down the width table.
     */
    if (!(lpWidthTable = GlobalLock(lpSummary->hWidthTable)))
        {
        DBGerr(("loadWidths(): Could *not* lock width table\n"));
        goto backout1;
        }

    if (lpSummary->indPFMName > -1)
        {
        /*  The fontSummary font came from a .pfm file.
         */
        if (!loadStructFromFile(lpFontSummary, lpSummary,
            lpWidthTable, FNTLD_WIDTHS))
            {
            goto backout0;
            }
        }
    else
        {
        /*  The fontSummary font came from a resource (ROM or cartridge
         *  font), load and lock the resource file.
         */
        LPPFMHEADER lpPFM;
        HANDLE hResData, hResInfo;

        /*  Find the font resource.
         */
	if (!(hResInfo = FindResource(hLibInst,
            (LPSTR)(long)(lpSummary->offset), (LPSTR)(long)MYFONT)))
            {
            DBGerr(("loadWidths(): Could not *find* resource\n"));
            goto backout0;
            }

        /*  Load (actually, only locate) font resource.
         */
	if (!(hResData = LoadResource(hLibInst, hResInfo)))
            {
            DBGerr(("loadWidths(): Could not *load* resource\n"));
            goto backout0;
            }

        /*  Lock (and load) font resource.
         */
        if (!(lpPFM = (LPPFMHEADER)LockResource(hResData)))
            {
            DBGerr(("loadWidths(): Could not *lock* resource\n"));
            FreeResource(hResData);
            goto backout0;
            }

        /*  Copy width table from resource file.
         */
        lmemcpy(lpWidthTable, (LPSTR)lpPFM->dfCharOffset, sizeWidthTable);

        /*  Free up resource.
         */
        GlobalUnlock(hResData);
        FreeResource(hResData);
        }

    #ifdef DBGdumpwidths
    {
    short ind;
    BYTE ch, last;
    short far * DBGwidth = (short far *)lpWidthTable;
    last = lpSummary->dfLastChar - lpSummary->dfFirstChar;

    DBMSG(("loadWidths(): dump of width table, size=%ld\n",
        GlobalSize(lpSummary->hWidthTable)));
    for (ind = 1, ch = 0; ch < last; ++ch, ++ind)
        {
        if ((ind % 10) == 0) {
            DBMSG(("\n"));
            }
        DBMSG(("%c%d  ", (char)(ch + lpSummary->dfFirstChar), DBGwidth[ch]));
        }
    DBMSG(("\n"));
    }
    #endif

    /*  Normal return.
     */
    goto done;

backout0:
    /*  Error return.
     */
    GlobalUnlock(lpSummary->hWidthTable);

backout1:
    GlobalFree(lpSummary->hWidthTable);

backout2:
    lpSummary->hWidthTable = 0L;
    lpWidthTable = 0L;

done:
    return (lpWidthTable);
    }
