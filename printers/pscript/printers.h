/**[f******************************************************************
 * printers.h -
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/*********************************************************************
 * PRINTERS.H
 *
 * 14Apr87 sjp	Creation date.
 * 17Apr87 sjp	Added all printer specific constants, etc. so that
 *	   	all necessary info (except what is in PSCRIPT.RC)
 *	   	that is necessary to add, del, modify printers is
 *	   	in this file.  To modify logic, consult CONTROL.C.
 * 04Jun87 sjp	Added gPaperBins[][] array.
 * 11Sep87 sjp	Added new printers.
 * 03Nov87 sjp	Converted this file to use the stuff from APD compiler.
 *********************************************************************/

#include "printcap.h"

/*
 * this is set so that it is 20 inches in picas
 * and large enough to accomodate the Linotype at 1270 dpi
 *
 */

#define IRESMAX		1440

#define DEFAULTORIENTATION	DMORIENT_PORTRAIT	/* portrait */
#define DEFAULT_COLOR		DMCOLOR_COLOR		/* yes */

/* max length of the bin (aka: feed, source) names */

#define NUM_INT_PRINTERS	32
#define NUM_EXT_PRINTERS	5


#define INT_PRINTER_MIN 1
#define INT_PRINTER_MAX (INT_PRINTER_MIN + NUM_INT_PRINTERS - 1)
#define EXT_PRINTER_MIN (INT_PRINTER_MAX + 1)
#define EXT_PRINTER_MAX (EXT_PRINTER_MIN + NUM_EXT_PRINTERS - 1)
#define FIRST_PRINTER INT_PRINTER_MIN

#define BINSTRLEN	24

#define DEFAULT_PRINTER		2

#define NUMPRINTERS	(NUM_INT_PRINTERS + NUM_EXT_PRINTERS)

