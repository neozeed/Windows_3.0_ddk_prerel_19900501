/*****************************************************************
* This module of the APD compiler parses the APD file and collects
* information in the APD structure.
*
*****************************************************************
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <io.h>
#include <sys\stat.h>
#include <string.h>
#include <process.h>

#include "version.h"
#include "apd.h"
#define APDGLOBALS
#include "globals.h"

/****************************************************************************/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/

BOOL APDPass1(void);
BOOL APDPass2(void);
BOOL APDPass3(void);
BOOL APDPass4(char*);
BOOL APDPass5(void);


#if 0
char rgbLine[160];	    /* The current line of text being processed */
char *szLine;		    /* Ptr to the current location in the line */

/************************************************************
* Name: GetNumber()
*
* Action: This routine parses an ASCII decimal number from the
*	  input file stream and returns its value.
*
**************************************************************
*/
int GetNumber()
{
    int iVal;
    BOOL fNegative;

    fNegative = FALSE;

    iVal = 0;
    EatWhite();

    if (*szLine=='-')
	{
	fNegative = TRUE;
	++szLine;
	}
    if (*szLine<'0' || *szLine>'9')
	goto ERROR;
    while (*szLine>='0' && *szLine<='9')
	iVal = iVal * 10 + (*szLine++ - '0');
    if (fNegative)
	iVal = - iVal;

    if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';')
	return(iVal);

ERROR:
    printf("GetNumber: invalid number %s\n", szLine);
    printf("%s\n", rgbLine);
    exit(1);
}


/******************************************************************
* Name: GetFloat()
*
* Action: This routine parses an ASCII floating point decimal number
*	  from the input file stream and returns its value scaled
*	  by a specified amount.
*
********************************************************************
*/
int GetFloat(iScale)
	int iScale;	    /* The amount to scale the value by */
{
    long lVal;
    long lDivisor;
    BOOL fNegative;
    int iFraction;

    EatWhite();

    fNegative = FALSE;
    lVal = 0L;

    if (*szLine=='-')
	{
	fNegative = TRUE;
	++szLine;
	}
    if (*szLine<'0' || *szLine>'9')
	goto ERROR;
    while (*szLine>='0' && *szLine<='9')
	lVal = lVal * 10 + (*szLine++ - '0');

    lDivisor = 1L;
    if (*szLine=='.')
	{
	++szLine;
	while (*szLine>='0' && *szLine<='9')
	    {
	    lVal = lVal * 10 + (*szLine++ - '0');
	    lDivisor = lDivisor * 10;
	    }
	}
    lVal = (lVal * iScale) / lDivisor;

    if (fNegative)
	lVal = - lVal;

    if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';')
	return((short)lVal);

ERROR:
    printf("GetFloat: invalid number %s\n", szLine);
    printf("%s\n", rgbLine);
    exit(1);
}
#endif


main(argc, argv)
	int argc;
	char **argv;
{
	char APDListFile[13];

    if (argc != 2){
		fprintf(stderr,"USAGE: APD <file.lst>\n");
		exit(1);
	}
    ++argv;
    strncpy(APDListFile, *argv, sizeof(APDListFile));

	fprintf(stderr,"APD Compiler (");
	fprintf(stderr,VERSION);
	fprintf(stderr,")\n");

	printf("APD Compiler (");
	printf(VERSION);
	printf(")\n");

	if(!APDPass1()) goto ERROR1;
	if(!APDPass2()) goto ERROR2;
	if(!APDPass3()) goto ERROR3;
	if(!APDPass4(APDListFile)) goto ERROR4;
	if(!APDPass5()) goto ERROR5;
	fprintf(stderr,"APD Compiler finished.\n");
	exit(0);

ERROR1:
	fprintf(stderr,"APD Compiler:  Error in pass 1\n");
	exit(1);
ERROR2:
	fprintf(stderr,"APD Compiler:  Error in pass 2\n");
	exit(2);
ERROR3:
	fprintf(stderr,"APD Compiler:  Error in pass 3\n");
	exit(3);
ERROR4:
	fprintf(stderr,"APD Compiler:  Error in pass 4\n");
	exit(4);
ERROR5:
	fprintf(stderr,"APD Compiler:  Error in pass 5\n");
	exit(5);

	/* dummy return to satisfy warning level 2 */
	return(0);
}

