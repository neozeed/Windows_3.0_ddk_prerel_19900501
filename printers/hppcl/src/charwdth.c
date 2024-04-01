/**[f******************************************************************
 * charwdth.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 *             All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/**********************************************************************
 *
 *  16 oct 89	peterbe	Minor change to debug #ifdef's to facilitate dumping
 *			of character widths.
 *  27 apr 89	peterbe	Tabs are 8 spaces now.
 *   1-17-89    jimmat  Added PCL_* entry points to lock/unlock data seg.
 */

#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#include "fontman.h"


#ifdef DEBUG
#define DBGdumpwidth
#else
#undef DBGdumpwidth
#endif

int far PASCAL GetCharWidth(LPDEVICE, short far *, WORD, WORD, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM);


/*  PCL_GetCharWidth
 *
 *  GetCharWidth entry point to lock/unlock data segment.
 */

int FAR PASCAL
PCL_GetCharWidth(lpDevice, lpBuffer, firstChar, lastChar, lpFont,
	 lpDrawMode, lpXform)
LPDEVICE lpDevice;
short far *lpBuffer;
WORD firstChar, lastChar;
LPFONTINFO lpFont;
LPDRAWMODE lpDrawMode;
LPTEXTXFORM lpXform;
{
    int rc;

    LockSegment(-1);

    rc = GetCharWidth(lpDevice, lpBuffer, firstChar, lastChar, lpFont,
		lpDrawMode, lpXform);

    UnlockSegment(-1);

    return rc;
}

/*  GetCharWidth
 *
 *  Return the character widths from firstChar to lastChar.  This procedure
 *  is the same as the width-getting code in str_out(), if you change it
 *  there, you should change it here.
 */
int far PASCAL GetCharWidth(lpDevice, lpBuffer, firstChar, lastChar, lpFont,
	lpDrawMode, lpXform)
    LPDEVICE lpDevice;
    short far *lpBuffer;
    WORD firstChar;
    WORD lastChar;
    LPFONTINFO lpFont;
    LPDRAWMODE lpDrawMode;
    LPTEXTXFORM lpXform;
    {
    short far *widthptr;
    short overhang;
    WORD dfFirstChar = lpFont->dfFirstChar;
    WORD dfLastChar = lpFont->dfLastChar;

    DBMSG(("GetCharWidth(%lp): first = %c%d, last = %c%d\n",
	lpBuffer, firstChar, (WORD)firstChar, lastChar, (WORD)lastChar));

    if (!lpBuffer)
	return FALSE;

    /*  Synthesized bold.
     */
    if (lpFont->dfWeight < lpXform->ftWeight)
	{
	overhang = lpXform->ftOverhang;
	DBMSG(("GetCharWidth(): overhang for synthesized bold=%d\n", overhang));
	}
    else
	overhang = 0;

    /*  Get width of string -- if it is variable pitch, then load
     *  the width table and build up the widths.  If it is fixed pitch,
     *  or we fail to load the width table, then use dfPixWidth.
     */
    if ((lpFont->dfPitchAndFamily & 0x1) &&
	(widthptr = (short far *)LoadWidthTable(lpDevice,
	    ((LPPRDFONTINFO)lpFont)->indFontSummary)))
	{
	for (; firstChar <= lastChar; ++firstChar, ++lpBuffer)
	    {
	    if (firstChar == 0xA0)
		{
		/*  Detect fixed space and return width of normal space.
                 */
		*lpBuffer = widthptr[' ' - dfFirstChar];
		}
	    else if ((firstChar >= dfFirstChar) &&
		    (firstChar <= dfLastChar))
		*lpBuffer = widthptr[firstChar - dfFirstChar];
	    else
		*lpBuffer = widthptr[lpFont->dfDefaultChar];

	    /*  Add in overhang for synthesized bold.
             */
	    *lpBuffer += overhang;

	    #ifdef DBGdumpwidth

	    if (firstChar < 127)
		DBMSG(("<'%c',%d,%d>", firstChar,
		    (WORD)firstChar, (short)*lpBuffer));
	    else
		DBMSG(("<%d,%d>", (WORD)firstChar, (short)*lpBuffer));

	    if (firstChar == lastChar)
		DBMSG(("\n"));
	    #endif
	    }

	UnloadWidthTable(lpDevice, ((LPPRDFONTINFO)lpFont)->indFontSummary);
	}
    else
	{
/* Overhang included in fixed pitch fonts.  5 Dec 1989  Clark R. Cyr */
	overhang += lpFont->dfPixWidth;
	for (; firstChar <= lastChar; ++firstChar, ++lpBuffer)
	    {
	    *lpBuffer = overhang;
	    }

	#ifdef DBGdumpwidth
	DBMSG(("GetCharWidth(): fixed-pitch width = %d\n",
	    lpFont->dfPixWidth));
	#endif
	}

    return TRUE;
    }
