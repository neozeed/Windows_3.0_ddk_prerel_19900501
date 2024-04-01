#include <stdio.h>
#include <string.h>

#include "apd.h"
#include "globals.h"
#include "apderror.h"

/****************************************************************************/
/*#define DEBUG_ON*/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/


void GetListLength(char**,int*);
BOOL CreateHeaderFile(char*,char*,int,int);
BOOL ParseForList(char*,char*,char*,int*,int*);
BOOL Replace(char*,int,int,char**,int,char*,char*,BOOL,BOOL);
BOOL MakeFile(char*,char**,int,char*,char*,BOOL,BOOL);
BOOL Append(char*,char*,int,char*);
int MapItemToList(char*,char**);
void APDError(int,char*,...);
BOOL APDPass3(void);

BOOL APDPass3()
{
	int startLine;
	int stopLine;
	int letterNum;
	int A4Num;

	fprintf(stderr,"APD Compiler:  Pass 3.\n");
	printf("APD Compiler:  Pass 3.\n");

	/* remember that the 1st entry is the default entry */
	GetListLength(&gPaperList[1],&gNumPapers);

	DBMSG(("Creating header file\n"));
	if(!CreateHeaderFile("idspaper.h","IDS_PP_",IDS_PP_MIN,gNumPapers)){
		APDError(E_MAKE,"idspaper.h");
		goto ERROR;
	}

	DBMSG(("Making resource file\n"));
	if(!MakeFile("pass3.rc",&gPaperList[1],gNumPapers,
		"IDS_PP_","",TRUE,FALSE)
	){
		APDError(E_MAKE,"pass3.rc");
		goto ERROR;
	}

#if 0
	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 3--begin */","/* Pass 3--end */",
		&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	if(!Replace("pscript.rc",startLine,stopLine,&gPaperList[1],gNumPapers,
		"IDS_PP_","",TRUE,TRUE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}
#endif
	if((gDefPaper=MapItemToList(gPaperList[0],&gPaperList[1]))<0) goto ERROR;

	/* include default constants for letter and a4 paper
	 * for US and international defaults.  This is to be backwards
	 * compatible with the PSCRIPT driver...Should be changed
	 * in the future?
	 */
	if((letterNum=MapItemToList("Letter",&gPaperList[1]))<0) goto ERROR;
	if(!Append("idspaper.h","#define IDS_LETTER ",
		letterNum+IDS_PP_MIN,"")
	){
		APDError(E_FIND,"idspaper.h");
		goto ERROR;
	}
	if((A4Num=MapItemToList("A4",&gPaperList[1]))<0) goto ERROR;
	if(!Append("idspaper.h","#define IDS_A4 ",
		A4Num+IDS_PP_MIN,"")
	){
		APDError(E_FIND,"idspaper.h");
		goto ERROR;
	}

	return(TRUE);

ERROR:
	return(FALSE);
}

