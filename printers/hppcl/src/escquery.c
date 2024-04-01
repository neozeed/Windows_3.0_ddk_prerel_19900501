/**[f******************************************************************
 * escquery.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 *
 **f]*****************************************************************/

/**********************************************************************
 *
 * 19 sep 89	peterbe	Moved LJ IIP code here from include file.
 *			See comments for values returned!
 * 27 apr 89	peterbe	Tabs are 8 spaces
 *   1-17-89    jimmat  Added PCL_* entry points to lock/unlock data seg.
 *   2-06-89    jimmat  Was returning FALSE for ENABLEDUPLEX in HP IID.
 */

#include "generic.h"
#include "resource.h"
#include "extescs.h"


int far PASCAL Control_II(LPDEVICE, short, LPSTR, LPPOINT);

int   FAR PASCAL Control(LPDEVICE,short,LPSTR,LPPOINT);


/*  PCL_Control
 *
 *  Control entry point to lock/unlock the data segment.
 */

int FAR PASCAL
PCL_Control(LPDEVICE lpDevice, short function, LPSTR lpInData,
    LPPOINT lpOutData) {

    int rc;

    LockSegment(-1);

    rc = Control(lpDevice,function,lpInData,lpOutData);

    UnlockSegment(-1);

    return rc;
}

/*  Control
 *
 *  Windows Escape() function QUERYESCSUPPORT.  The function is organized
 *  so the bulk of the work is done in control.c, this file supports only
 *  QUERYESCSUPPORT and some small speedups.
 */
int far PASCAL Control(lpDevice, function, lpInData, lpOutData)
    LPDEVICE lpDevice;
    short function;
    LPSTR lpInData;
    LPPOINT lpOutData;
    {
    if (function == QUERYESCSUPPORT)
	{
	short i = *(short far *)lpInData;

	DBMSG(("QUERYESCSUPPORT(%d)\n", i));

	switch (i)
	    {
	    case NEWFRAME:
	    case ABORTDOC:
	    case NEXTBAND:
	    case STARTDOC:
	    case ENDDOC:
	    case SETABORTPROC:
	    case QUERYESCSUPPORT:
	    case DRAFTMODE:
	    case GETPHYSPAGESIZE:
	    case GETPRINTINGOFFSET:
	    case GETSCALINGFACTOR:
	    case SETCOPYCOUNT:
	    case BANDINFO:
	    case DEVICEDATA:
	    case SETALLJUSTVALUES:
	    case GETEXTENDEDTEXTMETRICS:
	    case GETPAIRKERNTABLE:
	    case GETTRACKKERNTABLE:
	    case GETSETPRINTORIENT:
	    case GETSETPAPERBINS:
	    case ENUMPAPERBINS:
	    case GETTECHNOLOGY:
		return TRUE;

	    case DRAWPATTERNRECT:

		// We return 0, 1 or 2 here.
		//	0	means NO rules supported
		//	1	means rules supported on all but basic LJ
		//	2	means 'white rules' on IIP supported also.

		// Report extended DRAWPATTERNRECT (white rules)
		// on LJ IIP ('Entris')
		if (lpDevice->epCaps & HPLJIIP)
		    return 2;

		if (lpDevice->epCaps & HPJET)
		    return 0;	// Basic HPJET cannot do rules
		//  LaserPort is not currently supporting rules
		if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
		    return 0;
		// in all other cases, we have support of standard LJ rules.
		else
		    return 1;

	    case ENABLEDUPLEX:
		if (lpDevice->epCaps & ANYDUPLEX)
		    return TRUE;	/* Printer can print duplex */
		else
		    return FALSE;

	    default:
		return FALSE;
	    }
	}
    else
	{
	if (function == GETEXTENDEDTEXTMETRICS)
	    {
	    /*  Speedup for GETEXTENTEDTEXTMETRICS, if the font is
             *  from the resources (resident or cartridge font), then
             *  we know there are no extended text metrics.
             */
	    LPFONTINFO lpFont = ((LPEXTTEXTDATA)lpInData)->lpFont;

	    DBMSG(("Control(): quick GETEXTTEXTMETRICS\n"));

	    if (((LPPRDFONTINFO)lpFont)->isextpfm == FALSE)
		return FALSE;
	    }

	return (Control_II(lpDevice,function,lpInData,lpOutData));
	}
    }
