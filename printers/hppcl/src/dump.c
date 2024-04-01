//*********************************************************************
// dump.c -
//
// Copyright (C) 1988,1989 Aldus Corporation
// Copyright (C) 1988-1990 Microsoft Corporation.
//			   All rights reserved.
// Company confidential.
//
//*********************************************************************
//
// The purpose of this module is to convert a banding buffer bitmap
// into a series of PCL graphics and cursor-positioning escapes.
//
// This is based on the original Aldus DUMP.C, but much of the 'inner-loop'
// code has been moved to DUMPUTIL.ASM (which is in the SAME segment).
//
//********************************************************************

//**********************************************************************
//
// 01 dec 89	peterbe	#ifdef'd out DumpLaserPort() call for 3.0.
//
// 25 sep 89	peterbe	Temporary fix for basic LJ cursor bug: inhibit
//			bitstripping in landscape for HPJET.
//
// 19 sep 89	peterbe	Big move of LJ IIP ('Entris') code into this
//			file from includes.
//
// 15 sep 89	peterbe	Code cleanup.  Made ordering of positioning
//			escapes same in port. as in landscape.
//			Use SendGraphics() in NEWGRXF.H for SPUD.
//
// 14 sep 89	peterbe	Mainly, added SPUD code for portrait mode.
//
// 13 sep 89	peterbe	Removed start-graphics escape and y-pos init at
//			start of landscape. Corrected count for graphics
//			bitmap escape, landsc.  Landscape works.  Still
//			need to fix basic LJ cursor correction code.
//
// 12 sep 89	peterbe	Adding bitstripping support in landscape mode.
//			Y-positioning (top margin) needs work.
//			CurX(), CurY() need much change in handling
//			of epXerr, epYerr for basic HPPCL printer.
//
// 10 sep 89	peterbe	Fixed bug in processing bit strips. Train picture
//			prints OK now (so far bitstripping only in portrait
//			mode - WILL CHANGE SOON!).
//
// 22 aug 89	peterbe	Removed commented-out TransposeBitMap().
//
// 16 may 89	peterbe	Added () pairs in #defines of escapes
//
// 10 may 89	peterbe	Added DumpLaserPort() call.
//
// 27 apr 89	peterbe	Replaced code calling dmTranspose() with
//			code calling TransposeBitmap() in DUMPUTIL.A.
//
// 26 apr 89	peterbe	Made more variables local to inner blocks.
//			Debugged Landscape output.
//			Optimizing Portrait output.
//
// 25 apr 89	peterbe	Added some comments in landscape mode section.
//
// 21 apr 89	peterbe	Optimization of portrait-mode output for single-
//			-strip scanlines.
//
// 20 apr 89	peterbe	Make scanbits global for debug.
//
// 19 apr 89	peterbe	Now using DUMPUTIL.A: FindBitStrips().
//
// 14 apr 89	peterbe	Making variables for bitstripping more local.
//
// 13 apr 89	peterbe	Now use CompByteArray() for complementing buffer
//			(GDI-to-laser conversion of bitmap) and StripTRail()
//			for finding last nonwhite byte in a scanline.
//
// 12 apr 89	peterbe	Modifying indentation for portrait mode.
//
// 11 apr 89	peterbe	Modifying code to use SendGrEsc() in DUMPUTIL.A.
//
// 21 mar 89	peterbe	Checked in comment & cosmetic changes.
//
//   1-18-89	jimmat	Removed some static data items, eliminated a far
//			ptr to our data seg, and cleaned up some _really_
//			ugly code (lots more still here).
//   2-24-89	jimmat	LaserPort code now in lasport.a, no stubs here.
//
//**********************************************************************


#include "generic.h"
#include "resource.h"
#define FONTMAN_UTILS
#include "fontman.h"
#include "strings.h"
#include "memoman.h"
#include "dump.h"

#ifdef DEBUG
#define DBGdump(msg) DBMSG(msg)
#define DBGgrx(msg) DBMSG(msg)
#else
#define DBGdump(msg) /* DBMSG(msg) */
#define DBGgrx(msg) /*null*/
#endif

// definitions of escapes

// Begin graphics -- numeric value must be 1
#define W_START_GRX	('*'+256*'r')
#define B_START_GRX	'A'

// end graphics -- no numeric value, use NONUM = 0x8000 as flag
#define W_END_GRX	('*'+256*'r')
#define B_END_GRX	'B'

// SendGrEsc() recognizes this special value -- no numeric field.
#define NONUM		0x8000

// Set X position in dots
#define W_SETX		('*'+256*'p')
#define B_SETX		'X'

// Set Horizontal position in decipoints.
#define W_SETH		('&'+256*'a')
#define B_SETH		'H'

// relative vertical settings are always positive ('+') here.

// Set Y position in dots (relative)
#define W_SETY		('*'+256*'p')
#define B_SETY		'Y'

// Set Vertical position in decipoints (relative).
#define W_SETV		('&'+256*'a')
#define B_SETV		'V'

// Output graphics.  Numeric value is following byte count.
#define W_GRX		('*'+256*'b')
#define B_GRX		'W'

// graphics compression mode escape for Entris printer:
//
// Set Raster compression mode "* b <mode> M"
// mode 0 is normal LJ raster transfer
// mode 1 is compressed: count + data repeat blocks
// mode 2 is mixed repeated runs and literal runs.  We use THIS mode
// for Entris output.

#define W_COMPRESS_MODE	('*'+256*'b')
#define B_COMPRESS_MODE	'M'


// Type definition

#define POSSIZE 12

typedef struct
{
    short startpos;
    short endpos;
} PosArray[POSSIZE];

// functions defined in DUMPUTIL.A

// send an escape sequence, and possibly some graphics.
int NEAR PASCAL SendGrEsc(LPDEVICE,	// lpDevice
			WORD,		// 2 characters starting escape
			BOOL,		// num. value relative?
			int,		// numeric value
			char,		// terminating character (endEsc)
			LPSTR);		// --> graphics bytes if not NULL.

// complement bytes in a scanline, to turn GDI blackness into laser
// printer gloom.
void NEAR PASCAL CompByteArray(LPSTR,	// points to array.
			int);		// byte count

// Find last nonzero byte in a scanline (black = 0)
int NEAR PASCAL StripTrail(LPSTR, 	// points to beginning of scanline.
			int);		// width in bytes

// Find sequences of nonzero bytes in a scanline
// returns number of bit strips found.
int NEAR PASCAL FindBitStrips(LPSTR,	// points to beginning of scanline,
			PosArray FAR *,	// points to PosArray structure
			int,		// length of scanline,
			int);		// no. of entries in PosArray.

// Copy a byte column of a landscape-mode bitmap into a buffer,
// transposing the bit array:  in the buffer, the scanlines run
// 'vertically' relative to the landscape-mode page.
// The dimensions of the buffer are 8 scanlines of heightbytes bytes.

void NEAR PASCAL TransposeBitmap(LPSTR,	// top of column in bitmap.
				LPSTR,	// origin of buffer.
				int,	// width of band (bitmap)
				int);	// height in bytes of band.

// Dump graphics using LaserPort (or similar, compatible) hardware.
// CONTROL previously set up parameters via calls to functions in
// LASPORT.A.  This just calls an INT 2F function.

#ifdef VISUALEDGE
void NEAR PASCAL DumpLaserPort();
#endif

// compress scanline for IIP
int NEAR PASCAL LJ_IIP_Comp(LPSTR, LPSTR, int);

// Forward declarations -- these are declared here

void NEAR PASCAL SendGraphics(LPDEVICE, LPSTR, int);
void SetY(LPDEVICE, int, BOOL /* bRel */);
void SetX(LPDEVICE, int, BOOL /* bRel */);

int FAR PASCAL dump(lpDevice)
    LPDEVICE lpDevice;
{
    // NOTE: many variables used in this function are local to inner
    // blocks.  There is NO run-time disadvantage to this, and it saves
    // stack space and increases code readability.

    PosArray scanbits;			// start/end indices for bitstripping
    LPSTR buf, bmp;
    int widthbytes, height;
    int buflen;
    int err = SUCCESS;
    BOOL bStripbits;			// device handles bitstripping
    BOOL bIngraphics;			// flags graphic esc. has been sent

    BOOL bSpud;				// flags for HP LJ IIP
    BOOL bDidSpudEsc;


#ifdef VISUALEDGE
    // Are we going to let the INT 2F function on a laser printer
    // card do this?
    if (lpDevice->epOptions & OPTIONS_DPTEKCARD)
	{
	DumpLaserPort();
	SendGrEsc(lpDevice, W_END_GRX, FALSE, NONUM, B_END_GRX, NULL);
	return(SUCCESS);
	}
#endif

    //      Disable bit stripping at 75 dpi and for devices not capable
    //      of handling bit stripping.

    if (!(lpDevice->epCaps & NOBITSTRIP) && (lpDevice->epScaleFac != 2))
	bStripbits = TRUE;
    else
	bStripbits = FALSE;

    // set flag for HP LaserJet IIP printer.
    if (lpDevice->epCaps & HPLJIIP)
	{
	bSpud = TRUE;			// it's a HP LJ IIP
	bDidSpudEsc = FALSE;		// We've told it it's one: this is
	bStripbits = FALSE;		// better than bitstripping!
	}
    else
	bSpud = FALSE;


    // force graphics escape on first output...
    bIngraphics = FALSE;

    // get dimensions of band..
    widthbytes = lpDevice->epBmpHdr.bmWidthBytes;
    height = lpDevice->epBmpHdr.bmHeight;

    // Complement bitmap: black is 0 in GDI, 1 for the PCL laser printer.
    CompByteArray(lpDevice->epBmp, widthbytes * height);

// is it Landscape mode??

    if (lpDevice->epType == (short)DEV_LAND)
    {	// landscape

	// local variables for landscape.
	int heightbytes;
	int bytecolumn;
	int relxpos;
	int x;

	DBGdump(("dump(): (landscape)\n"));

	// reset blank-scanline counter
	relxpos = 0;

	// width of bitmap in bytes = height of band in bytes
        heightbytes = height / 8;

	// TEMPORARILY inhibit landscape-mode bitstripping for LASERJET
	// since cursor positioning (SetX relative) needs fixing.
	if (lpDevice->epCaps & HPJET)
	    bStripbits = FALSE;

	// move right-to-left across band bitmap, a byte column at a time.
	// The raster lines of GDI's band bitmap are at right angles to
	// what the printer needs, so we do a bit transposition.

        for (bytecolumn = widthbytes - 1; bytecolumn >= 0; bytecolumn--)
        {
	    // fill an (8 x heightbytes) buffer with transposed
	    // pixels from the original bitmap.

	    TransposeBitmap(((LPSTR) (lpDevice->epBmp)) + bytecolumn,
			((LPSTR) lpDevice) + lpDevice->epBuf,
			widthbytes, heightbytes);

	    // now output the 8 transposed scanlines from the buffer.

	    buf = ((LPSTR) lpDevice) + lpDevice->epBuf;

            for (x = 0; x < BYTESIZE; x++, buf += heightbytes)
            {
		// we're processing a scanline of the transposed buffer.
		int nstrips;		// number of bit strips in the scanline

		// Find the bit strips in a scanline (see comment under
		// portrait mode).

		nstrips = FindBitStrips((LPSTR)buf,
				(PosArray FAR *)scanbits,
				heightbytes,
				(bStripbits) ? POSSIZE : 1);

	        //  strip trailing blanks:  find the last nonzero byte.

                buflen = StripTrail(buf, heightbytes);

#ifdef DEBUG
		if (buflen)
		    {
		    int i;

		    if (bSpud)
			DBGdump(("HP LJ IID: "));

		    DBGdump(("nstrips = %d ",
			bStripbits ? nstrips : 1));
		    for (i = 0; i < nstrips; i++)
			DBGdump(("<%d, %d>",
				scanbits[i].startpos,
				scanbits[i].endpos));
		    DBGdump((", buflen = %d\n", buflen));
		    }
#endif


		// Handle special case of bStripbits == FALSE.

		if (!bStripbits)
		    {
		    nstrips = 1;
		    scanbits[0].endpos = buflen;
		    scanbits[1].endpos = -1;
		    }
		else if (buflen)
		    scanbits[nstrips-1].endpos = buflen;

// (more landscape)

                if (buflen)
		    {	// (vertical) scanline isn't blank
		    int stripno;
		    int striplen;
		    int y;

		    // output bit strips
		    for (stripno = 0; stripno < nstrips; stripno++)
		      if ((scanbits[stripno].startpos != -1) &&
			  (scanbits[stripno].endpos != -1))

			{	// output next strip
			striplen = scanbits[stripno].endpos -
				   scanbits[stripno].startpos;

			// get Y position of this strip.
			y = scanbits[stripno].startpos <<
				(lpDevice->epScaleFac + 3);

			// do we need to send an end-graphics escape?
			if (((stripno != 0) ||
			     (lpDevice->epCury != y) ||
			     (relxpos != 0)) &&
			    bIngraphics)
			    {
			    bIngraphics = FALSE;
			    SendGrEsc(lpDevice, W_END_GRX, FALSE,
				NONUM, B_END_GRX, NULL);
			    }

			// Adjust X for strips after the 'topmost' one
			// (the printer automatically decrements X after each
			// strip, we have to back up the cursor).
			if (stripno != 0)
			    SetX(lpDevice, 1 << lpDevice->epScaleFac, TRUE);

			// if top margin has changed, set Y
			if (lpDevice->epCury != y)
			    SetY(lpDevice, y, FALSE);

			// if there have been blank lines, adjust cursor.
			if (relxpos != 0)
			    {	// this is a relative motion.
			    SetX(lpDevice, -relxpos << lpDevice->epScaleFac,
				TRUE);
			    relxpos = 0;
			    }

			// This sets compression mode 2 if the printer is
			// an HP LaserJet IIP.
			// We do this the first time we do graphics for
			// each band now.

			if (bSpud && !bDidSpudEsc)
			    {
			    SendGrEsc(lpDevice, W_COMPRESS_MODE,
				    FALSE, 2, B_COMPRESS_MODE, NULL);
			    bDidSpudEsc = TRUE;
			    }

			// if necessary, send start graphics:  Esc * r 1 A
			if (!bIngraphics)
			    {
			    bIngraphics = TRUE;
			    SendGrEsc(lpDevice, W_START_GRX, FALSE, 1,
				B_START_GRX, NULL);
			    }

			// do data conversion before outputting (landscape mode)
			SendGraphics(lpDevice,
				    buf + scanbits[stripno].startpos,
				    striplen);
			}

		    }
                else
		    //empty raster line -- just increment blank line counter
                    relxpos++;

            }	// for (x...)

        }	// for (bytecolumn ...)


        if (relxpos != 0)
	    {
	    SetX(lpDevice, -(relxpos << lpDevice->epScaleFac), TRUE);
            relxpos = 0;
	    }

	// Send the end graphics escape: esc * r B
	SendGrEsc(lpDevice, W_END_GRX, FALSE, NONUM, B_END_GRX, NULL);

    }	// end of landscape

    // Handle PORTRAIT mode.

    else	// The mode is PORTRAIT
    {
	// local variables
	int y;
	int relypos;

	DBGdump(("dump() (portrait)\n"));

	// reset blank-scanline counter
	relypos = 0;

        bmp = lpDevice->epBmp;

        for (y = 0; y < height; y++)
        {
	    int nstrips;		// number of 'bit strips' in scanline

	    // find the 'bit strips' in the scanline.  This routine scans
	    // through a scanline, finds sequences of nonwhite (nonzero)
	    // bytes separated by > 32 0 bytes, and puts the starting and
	    // ending points in scanbits[].  The scanbits[] array must be
	    // defined large enough to handle all possible bit strips
	    // in a scanline.  If bStripbits is false, we just find the
	    // first strip -- we just want the starting position in this
	    // case.

	    nstrips = FindBitStrips((LPSTR)bmp,
				(PosArray FAR *)scanbits,
				widthbytes,
				(bStripbits) ? POSSIZE : 1);


	    //  strip trailing blanks
	    buflen = StripTrail(bmp, widthbytes);

#ifdef DEBUG
	    // portrait, remember
	    if (buflen)
		{
		int i;

		if (bSpud)
		    DBGdump(("HP LJ IIP: "));

		DBGdump(("nstrips = %d ", bStripbits) ? nstrips : 1);
		    for (i = 0; i < nstrips; i++)
			{
			DBGdump(("<%d, %d>",
				scanbits[i].startpos,
				scanbits[i].endpos));
			}
		DBGdump((", buflen = %d\n", buflen));
		}
#endif

	    // Handle special case of bStripbits == FALSE .. we now handle
	    // this as a single strip.
	    if (!bStripbits)
		{
		nstrips = 1;
		scanbits[0].endpos = buflen;
		scanbits[1].endpos = -1;
		}
	    // This fixes 'trainpic' bug.
	    else if (buflen)
		scanbits[nstrips-1].endpos = buflen;

// (more portrait) 

            if (buflen)
            { // scanline isn't blank

		int stripno;
		int striplen;
		int x;

		// Output bit strips.
		for (stripno = 0; stripno < nstrips; stripno++)
		    // start and end must both not be -1
		    if ((scanbits[stripno].startpos != -1) &&
			(scanbits[stripno].endpos != -1)) 
		{ // output next strip

		    // get length of strip.
		    striplen = scanbits[stripno].endpos -
			     scanbits[stripno].startpos;

		    // get X position.
		    x = scanbits[stripno].startpos <<
			    (lpDevice->epScaleFac + 3);

		    // do we need to send an end-graphics escape?
		    if (((stripno != 0) ||
			 (lpDevice->epCurx != x)||
			 (relypos != 0)) &&
			bIngraphics)
			{
			bIngraphics = FALSE;
			SendGrEsc(lpDevice, W_END_GRX, FALSE,
				    NONUM, B_END_GRX, NULL);
			}

		    // adjust Y for strips after the leftmost one..
		    // (the printer automatically increments Y after each
		    // strip, we have to back up the cursor).
		    if (stripno != 0)
			 SetY(lpDevice, -1 << lpDevice->epScaleFac, TRUE);

		    // if left margin has changed, set X (absolute)
		    if (lpDevice->epCurx != x)
			SetX(lpDevice, x, FALSE);

		    if (relypos != 0)
			{
			SetY(lpDevice,
			    relypos << lpDevice->epScaleFac,TRUE);
			relypos = 0;
			}

		    // This sets compression mode 2 if the printer is
		    // an HP LaserJet IIP.
		    // We do this the first time we do graphics for
		    // each band now.

		    if (bSpud && !bDidSpudEsc)
			{
			SendGrEsc(lpDevice, W_COMPRESS_MODE,
				FALSE, 2, B_COMPRESS_MODE, NULL);
			bDidSpudEsc = TRUE;
			}

		    // If necessary, send start Graphics: Esc * r 1 A
		    if (!bIngraphics)
			{
			bIngraphics = TRUE;
			SendGrEsc(lpDevice, W_START_GRX, FALSE, 1,
			    B_START_GRX, NULL);
			}

		    // do data conversion before outputting (landscape mode)
		    SendGraphics(lpDevice,
			    bmp + scanbits[stripno].startpos,
			    striplen);

		}  // end .. for (stripno...)
            } // end .. scanline isn't blank

            else
		{
		//empty raster line ..  increment blank line counter
                relypos++;
		}

            bmp += widthbytes;

        }	// for (y ..)

	// Send the end graphics escape: esc * r B
	if (bIngraphics)
	    SendGrEsc(lpDevice, W_END_GRX, FALSE, NONUM, B_END_GRX, NULL);

	// adjust Y position if there were any trailing blank scanlines.
        if (relypos != 0)
	    {
            SetY(lpDevice, relypos << lpDevice->epScaleFac, TRUE);
            relypos = 0;
	    }

    }	// portrait

    return (err);

}	// end dump()

// Function for setting relative or absolute Y position.

void SetY(lpDevice, y, bRel)
LPDEVICE lpDevice;
int y;
BOOL bRel;
{
    //  save where we last set Y
    if (bRel)
	lpDevice->epCury += y;
    else
	lpDevice->epCury = y;

    if (!(lpDevice->epCaps & HPJET))
	{
	// Most printers are this simple!
	// set Y position in points
	// esc * p <num> Y
	SendGrEsc(lpDevice, W_SETY, bRel, y, B_SETY, NULL);
	}
    else
	{
	// It's the BASIC HP LaserJet.
	// scale the delta-Y into decipoints.
	y *= 12;
	lpDevice->epYerr += y % 5;
	y /= 5;

	// calculate error, adjust for + or - overflow.
	if (bRel && (lpDevice->epYerr < 0))
	    {
	    y--;
	    lpDevice->epYerr += 5;
	    }
	else if (lpDevice->epYerr > 5)
	    {
	    y++;
	    lpDevice->epYerr -= 5;
	    }

	// set Vertical position, decipoints 
	// esc & a <num> V
	SendGrEsc(lpDevice, W_SETV, bRel, y, B_SETV, NULL);
	}
    
} // SetY

// Function for setting X position.

void SetX(lpDevice, x, bRel)
LPDEVICE lpDevice;
int x;
BOOL bRel;
{
    BOOL InLand = ((short)DEV_LAND == lpDevice->epType);

    //  save where we last set X
    if (bRel)
	lpDevice->epCurx += x;
    else
	lpDevice->epCurx = x;

    if (!(lpDevice->epCaps & HPJET))
	{
	// Most printers are this simple!
	// Set X position in dots (pixels)
	SendGrEsc(lpDevice, W_SETX, bRel, x,
		B_SETX, NULL);
	}
    else
	{
	// It's the BASIC HP LaserJet.

	short pos;

	if (InLand)
	    {	// do it this way in the Landscape code
	    x *= 12;
	    lpDevice->epXerr -= x % 5;
	    pos = x / 5;

	    if (lpDevice->epXerr < 0)
		{
		pos--;
		lpDevice->epXerr += 5;
		}
	    }
	else
	    {	// do it this way in the Portrait code
	    
	    x *= 12;
	    lpDevice->epXerr = x % 5;
	    pos = x / 5;

	    if (lpDevice->epXerr)
		{
		pos++;
		lpDevice->epXerr -= 5;
		}
	    }

	// Set Horiz. position in decipoints
	SendGrEsc(lpDevice, W_SETH, bRel, pos,
		B_SETH, NULL);
	}

} // SetX

//===========================================================================
// Output a (compressed) scanline to LJ IIP, or a bit strip to other printers.
// This is used in both landscape and portrait mode.
//===========================================================================

void NEAR PASCAL SendGraphics(lpDevice, bits, incount)
LPDEVICE lpDevice;
LPSTR bits;
int incount;
{
    // Compress and output scanline to Entris, or
    // output a bit strip to other printers.

    if (lpDevice->epCaps & HPLJIIP)
	{ // it's an Entris
	int complen;
	LPSTR compbuf;
	//BYTE compbuf[500];

	// get address of compression buffer.

	compbuf = ((LPSTR)lpDevice) + lpDevice->epLineBuf;

	// DBGgrx(("About to compress %d bytes", striplen));

	complen = LJ_IIP_Comp(
		(LPSTR) bits,
		(LPSTR) compbuf,
		incount);

	DBGgrx((" .. complen = %d", complen));

	SendGrEsc(lpDevice, W_GRX, FALSE, complen, B_GRX,
	    (LPSTR) compbuf);

	DBGgrx((".. Sent scanline.\n"));
	}
    else
	{ // Not Entris
	// Output graphics: escape * b <count> W <graphics>
	SendGrEsc(lpDevice, W_GRX, FALSE, incount, B_GRX, (LPSTR) bits);
	}

} // SendGraphics()
