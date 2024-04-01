#include <stdio.h>

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
BOOL APDPass2(void);

BOOL APDPass2()
{
	int startLine;
	int stopLine;

	fprintf(stderr,"APD Compiler:  Pass 2.\n");
	printf("APD Compiler:  Pass 2.\n");

	/* remember that the 1st entry is the default entry */
	GetListLength(&gSourceList[1],&gNumSources);

	DBMSG(("Creating header file\n"));
	if(!CreateHeaderFile("idssource.h","IDS_SR_",IDS_SR_MIN,gNumSources)){
		APDError(E_MAKE,"idssource.h");
		goto ERROR;
	}
	if(!Append("idssource.h","#define IDS_MANUAL ",
		MANUAL+IDS_SR_MIN,"")
	){
		APDError(E_FIND,"idssource.h");
		goto ERROR;
	}
	if(!Append("idssource.h","#define IDS_AUTO ",
		AUTO+IDS_SR_MIN,"")
	){
		APDError(E_FIND,"idssource.h");
		goto ERROR;
	}

	DBMSG(("Making resource file\n"));
	if(!MakeFile("pass2.rc",&gSourceList[1],gNumSources,
		"IDS_SR_","",TRUE,TRUE)
	){
		APDError(E_MAKE,"pass2.rc");
		goto ERROR;
	}

#if 0
	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 2--begin */","/* Pass 2--end */",
		&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	if(!Replace("pscript.rc",startLine,stopLine,&gSourceList[1],gNumSources,
		"IDS_SR_","",TRUE,TRUE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}
#endif
	if((gDefSource=MapItemToList(gSourceList[0],&gSourceList[1]))<0){
		goto ERROR;
	}
	return(TRUE);

ERROR:
	return(FALSE);
}

