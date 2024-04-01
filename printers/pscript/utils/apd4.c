#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

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

BOOL CreateHeaderFile(char*,char*,int,int);
BOOL ParseForList(char*,char*,char*,int*,int*);
BOOL Replace(char*,int,int,char**,int,char*,char*,BOOL,BOOL);
BOOL MakeFile(char*,char**,int,char*,char*,BOOL,BOOL);
BOOL Append(char*,char*,int,char*);
int MapItemToList(char*,char**);

int MapToken(char*);
int FindChar(char*,int,char);
int GetToken(char*,char*,int,char);
int GetString(char*,char*,int);
int GetLine(char*,int,FILE*);
BOOL IsLineIllegal(char*);
BOOL IsLineWhite(char *);
void APDError(int,char*,...);

BOOL APDPass4(char*);

BOOL APDPass4(APDListFile)
	char APDListFile[];
{
	FILE *fp;
	char fileName[80];
	char line[MAX_LINE_LENGTH+1];
	char token[MAX_TOKEN_LENGTH+1];
	int i;
	int rc;
	int printerCount;
	int tokenID;
	int ptr;
	int startLine;
	int stopLine;
	int defRes;
	BOOL fatalError;
	BOOL fParse;

	fprintf(stderr,"APD Compiler:  Pass 4.\n");
	printf("APD Compiler:  Pass 4.\n");

	if(!(fp=fopen(APDListFile,"r"))) {
		printf("can not open %s\n", APDListFile);
		goto ERROR;
	}

	gNumPrinters=0;
	gLineNum=0;

	while(TRUE){
		if(!fgets(fileName,80,fp)) {
			printf("error reading %s\n", APDListFile);
			goto ERROR;
		}
		/* get rid of the newline character and add it to 
		 * the list of APD files available*/
		fileName[strlen(fileName)-1]='\0';
		DBMSG(("%s\n",fileName));

		if(gNumPrinters >= MAX_APDS){
			APDError(E_TOOMANYAPDS,APDListFile);
			goto ERROR;
		}
		/* check to see if we've reached the end of the APD list */
		if(!strcmp(fileName,"end")) break;
		if(!(gAPDFileList[gNumPrinters]=malloc(strlen(fileName)+1))){
			APDError(E_MEMORY,APDListFile);
			goto ERROR;
		}
		strcpy(gAPDFileList[gNumPrinters],fileName);
		DBMSG(("[%d]=%s,%s\n",
			gNumPrinters,gAPDFileList[gNumPrinters],fileName));
		gNumPrinters++;
		gLineNum++;
	}
	/* compensate for the default entry
	 * ...should be the equivalent of
	 *    GetListLength(&gAPDFileList[1],gNumPrinters)
	 */
	gNumPrinters--;

	if(fclose(fp)==EOF){
		APDError(E_CLOSE,APDListFile);
		goto ERROR;
	}
	DBMSG(("Creating header file\n"));
	if(!CreateHeaderFile("idsprint.h","IDS_PR_",IDS_PR_MIN,gNumPrinters)){
		APDError(E_MAKE,"idsprint.h");
		goto ERROR;
	}
#ifdef DEBUG_ON
	DBMSG(("gNumPrinters=%d\n",gNumPrinters));
	for(i=0;i<gNumPrinters+1;i++) DBMSG(("%s\n",gAPDFileList[i]));
#endif

	fatalError=FALSE;
	printerCount=0;

	for(i=1;!fatalError && i<gNumPrinters+1;i++){
		DBMSG(("i=%d,file=%s\n",i,gAPDFileList[i]));
		if(!(fp=fopen(gAPDFileList[i],"r"))){
			APDError(E_OPEN,gAPDFileList[i]);
			goto ERROR;
		}
		gFileName=gAPDFileList[i];
		fParse=TRUE;
		gReUseLineFlag=FALSE;
		gLineNum=0;

		while(fParse){

			/* did we get the line from the file O.K.? or EOF? */
			if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))) goto ERROR;
			else if(rc<0) break;

			/* if the line is illegal then QUIT */
			if(fatalError=IsLineIllegal(line)){
				APDError(E_ILLEGALORMISSING,gFileName,"PrinterName");
				break;
			}
			/* is line to be ignored? */
			if(IsLineWhite(line)) continue;

			if((ptr=GetToken(line,token,1,':'))<0){
				APDError(E_MISSING,gFileName,":");
				goto ERROR;
			}
			if(!(tokenID=MapToken(token))) token[0]='\0';
			DBMSG(("ID=%d,token=%s,line=%s\n",tokenID,token,&line[1]));

			switch(tokenID){
				case APD_PRINTERNAME:
					DBMSG(("PRINTERNAME: %s,%s\n",token,line));

					/* look after the ':' for the string */
					if(GetString(line,token,++ptr)<0) goto ERROR;

					if(!(gPrinterList[i-1]=malloc(strlen(token)+1))){
						APDError(E_MEMORY,gFileName);
						goto ERROR;
					}
					strcpy(gPrinterList[i-1],token);

					DBMSG(("%s,%s\n",gPrinterList[i-1],token));
					printerCount++;
					fParse=FALSE;
				break;
#if 0
				case APD_PRODUCT:
					DBMSG(("PRODUCT: %s,%s\n",token,line));

					/* look after the ':' for the string */
					if(GetString(line,token,++ptr)<0) goto ERROR;

					/* ignore the '(' and ')' and save the string */
					token[strlen(token)-1]='\0';
					if(!(gPrinterList[i-1]=malloc(strlen(&token[1])+1))){
						APDError(E_MEMORY,gFileName);
						goto ERROR;
					}
					strcpy(gPrinterList[i-1],&token[1]);

					DBMSG(("%s,%s\n",gPrinterList[i-1],&token[1]));
					printerCount++;
					fParse=FALSE;
				break;
#endif
				default:
				break;
			}
		}
		if(fclose(fp)==EOF){
			APDError(E_CLOSE,gFileName);
			goto ERROR;
		}
	}
	if(fatalError) goto ERROR;

	DBMSG(("printerCount=%d\n",printerCount));
	if(printerCount!=gNumPrinters) goto ERROR;

	/* null terminate the list just to be sure
	 * remember the default entry!
	 */
	*gPrinterList[gNumPrinters]='\0';

	DBMSG(("Making resource file\n"));
	if(!MakeFile("pass4.rc",&gPrinterList[0],gNumPrinters,
		"IDS_PR_","",TRUE,FALSE)
	){
		APDError(E_MAKE,"pass4.rc");
		goto ERROR;
	}

#if 0
	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 4--begin */","/* Pass 4--end */",
		&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	/* remember that the # of APD's includes the entry for the default */
	if(!Replace("pscript.rc",startLine,stopLine,&gPrinterList[0],gNumPrinters,
		"IDS_PR_","",TRUE,FALSE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}
#endif
	if((gDefPrinter=MapItemToList(gAPDFileList[0],&gAPDFileList[1]))<0){
		goto ERROR;
	}
	if(!Append("idsprint.h","#define IDS_DEFAULTPRINTER ",
		gDefPrinter+IDS_PR_MIN,"")
	){
		APDError(E_FIND,"idsprint.h");
		goto ERROR;
	}
	return(TRUE);

ERROR:
	return(FALSE);
}

