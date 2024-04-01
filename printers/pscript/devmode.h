/****************************************************************************
 * define the driver specific form of the DEVMODE structure
 *
 ***************************************************************************/


#include "printers.h"

/*
 * DEVMODE
 *
 * this structuer is used to save device data for a given port between
 * creations and deletions of PDEVICE data.  it is also used to communicate
 * the data in the DEVICEMODE dialog to the driver.  also note that some
 * escapes change this data.
 *
 */

typedef struct {
	DEVMODE dm;
	short iPrinter;		/* The printer type */
	short iRes; 		/* The device resolution */
	short iJobTimeout;	/* The job timeout in seconds */
	short marginState;	/* margin state: default, zero, tile */
	BOOL  fHeader;		/* TRUE=download header */
	BOOL  fDoEps;		/* output an EPS header with bbos */
	BOOL  fBinary;		/* is binary port */
	int   iScale;		/* scale paper size 100/iScale, iRes by iScale/100 */
	short rgiPaper[DMBIN_FIRST + NUMFEEDS]; /* 1 paper type per paper source */
	char  EpsFile[40];
} PSDEVMODE;
typedef PSDEVMODE FAR *LPPSDEVMODE;




