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

#define DEBUG1_ON
#ifdef DEBUG1_ON	/* used for CAPS and PSS display */
#define DBMSG1(msg) printf msg
#else
#define DBMSG1(msg)
#endif

/*#define DEBUG2_ON*/
#ifdef DEBUG2_ON	/* used for 'EndOfFile' */
#define DBMSG2(msg) printf msg
#else
#define DBMSG2(msg)
#endif
/****************************************************************************/

int Scale(int,int,int);
BOOL CreateHeaderFile(char*,char*,int,int);
BOOL ParseForList(char*,char*,char*,int*,int*);
BOOL Replace(char*,int,int,char**,int,char*,char*,BOOL,BOOL);
BOOL MakeFile(char*,char**,int,char*,char*,BOOL,BOOL);
int MapItemToList(char*,char**);

int MapToken(char*);
int GetToken(char*,char*,int,char);
int GetString(char*,char*,int);
int GetStringAgain(char*,FILE*);
int DoTransverse(char*,int,BOOL*);
BOOL GetTranslation(char*,int);
int FindChar(char*,int,char);
BOOL CreatePrCapsStructure(char*,int,int);
void InitPrCaps(void);
void InitPSS(void);
BOOL MakePrCapsFile(char*);
BOOL MakePSSFile(char*);
int GetLine(char*,int,FILE*);
BOOL IsLineIllegal(char*);
BOOL IsLineWhite(char *);
BOOL GetMargins(char*, RECT *, int);
BOOL BuildString(int,char**,char*);
void APDError(int,char*,...);

BOOL APDPass5(void);

BOOL APDPass5()
{
	FILE *fp;
	char fileName[80];
	char line[MAX_LINE_LENGTH+1];
	char token[MAX_TOKEN_LENGTH+1];
	char string[MAX_LINE_LENGTH+1];
	char *dirFileList[MAX_APDS];
	int i;
	int j;
	int rc;
	int temp1;
	int temp2;
	int tokenID;
	int ptr;
	int subPtr;
	int startLine;
	int stopLine;
	int paperNum;
	int defPaperNum;
	int manualNum;
	int defManualNum;
	int inputSlotNum;
	int defInputSlotNum;
	int transferNum;
	int defTransferNum;
	int defPaperStock;
	int marginX;
	int marginY;
	BOOL fatalError;
	BOOL fImageableArea;
	BOOL fPaperTray;
	BOOL fManualFalse;
	BOOL fManualTrue;
	BOOL fWindowsAuto;
	BOOL fInputSlot;
	BOOL fTransfer;
	BOOL fPageSize;
	BOOL fPageRegion;
	BOOL fTransverse;

	fprintf(stderr,"APD Compiler:  Pass 5.\n");
	printf("APD Compiler:  Pass 5.\n");

	/* create printer caps structure */
	if(!CreatePrCapsStructure(gCapsFile,gNumSources,gNumPapers)){
		APDError(E_MAKE,gCapsFile);
		goto ERROR;
	}
	/* create a list of .cap,.pss, and .dir files for the printer stuff */
	for(i=0;i<gNumPrinters+1;i++){
		if(!(gPrCapsFileList[i]=malloc(strlen(gAPDFileList[i])+1))){
			APDError(E_MEMORY,gPrCapsFileList[i]);
			goto ERROR;
		}
		strcpy(gPrCapsFileList[i],gAPDFileList[i]);

		if(!(gPSSFileList[i]=malloc(strlen(gAPDFileList[i])+1))){
			APDError(E_MEMORY,gPSSFileList[i]);
			goto ERROR;
		}
		strcpy(gPSSFileList[i],gAPDFileList[i]);

		if(!(dirFileList[i]=malloc(strlen(gAPDFileList[i])+1))){
			APDError(E_MEMORY,dirFileList[i]);
			goto ERROR;
		}
		strcpy(dirFileList[i],gAPDFileList[i]);

		if((ptr=GetToken(gAPDFileList[i],token,0,'.'))<0){
			APDError(E_ILLEGALFILENAME,gAPDFileList[i]);
			goto ERROR;
		}
		DBMSG(("%d,%s,%s,%s\n",ptr,gPrCapsFileList[i],gPSSFileList[i],token));
		gPrCapsFileList[i][++ptr]='c';
		gPSSFileList[i][ptr]='p';
		dirFileList[i][ptr]='d';
		gPrCapsFileList[i][++ptr]='a';
		gPSSFileList[i][ptr]='s';
		dirFileList[i][ptr]='i';
		gPrCapsFileList[i][++ptr]='p';
		gPSSFileList[i][ptr]='s';
		dirFileList[i][ptr]='r';
		DBMSG(("%s,%s,%s,%s,%d\n",
			gAPDFileList[i],gPrCapsFileList[i],gPSSFileList[i],
			dirFileList[i],token,ptr));
	}
	if(!(gPaperCaps=(BOOL *)malloc(gNumPapers*sizeof(BOOL)))) goto ERROR;
	if(!(gSourceCaps=(BOOL *)malloc(gNumSources*sizeof(BOOL)))) goto ERROR;
	if(!(gMarginCaps=(RECT *)malloc(gNumPapers*sizeof(RECT)))) goto ERROR;

	fatalError=FALSE;

	for(i=1;!fatalError && i<gNumPrinters+1;i++){
		printf("\tCompiling file #%d \"%s\".\n",i,gAPDFileList[i]);

		/* initialize capabilities for each printer being processed */
		InitPrCaps();
		InitPSS();

		if(!(fp=fopen(gAPDFileList[i],"r"))){
			APDError(E_OPEN,gAPDFileList[i]);
			goto ERROR;
		}

		/* for error handler use */
		gFileName=gAPDFileList[i];

		gLineNum=0;
		gReUseLineFlag=FALSE;
		fPaperTray=FALSE;
		fPageSize=FALSE;
		fPageRegion=FALSE;
		fManualFalse=fManualTrue=FALSE;
		fImageableArea=FALSE;
		gWindowsAutoPSS=0;
		fWindowsAuto=FALSE;
		gTransferPSS=0;
		fTransfer=FALSE;

		DBMSG((">1>BEFORE WHILE\n"));
		while(TRUE){

			/* did we get the line from the file O.K.? or EOF? */
			if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))) goto ERROR;
			else if(rc<0) break;

			/* if the line is illegal then QUIT */
			if(fatalError=IsLineIllegal(line)){
				APDError(E_ILLEGALCHARACTER,gFileName);
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
				case APD_DEFAULTRESOLUTION:
					DBMSG((">DEFAULTRESOLUTION: %s,%s\n",token,line));

					/* look after the ':' for the 'd' */
					if((ptr=GetToken(line,token,++ptr,'d'))<0){
						APDError(E_MISSING,gFileName,"dpi");
						goto ERROR;
					}
					gDefResolutionCaps=atoi(token);

					DBMSG(("<DEFAULTRESOLUTION: %s,%d\n",
						token,gDefResolutionCaps));
				break;

				case APD_DEFAULTPAPERTRAY:
					DBMSG((">DEFAULTPAPERTRAY: %s,%s\n",token,line));

					/* At this time it doesn't matter what the default
					 * actually is.  The important thing is the presence
					 * of "PaperTray" entries.
					 */
					DBMSG((">PAPERTRAY\n"));
					fPaperTray=FALSE;
					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORDAFTER,gFileName,
								"PaperTray");
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_PAPERTRAY){
							DBMSG(("<PAPERTRAY: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						/* If we've already found a PaperTray entry
						 * continue reading until the PaperTray entries
						 * are all read.
						 */
						if(fPaperTray) continue;

						/* At this point there is at lease 1 "PaperTray"
						 * entry.  This implies that AUTO is available.
						 */
						fPaperTray=TRUE;
					}
					if(fPaperTray){
						gSourceCaps[AUTO]=TRUE;
						DBMSG(("AUTO\n"));
					}
					DBMSG(("<PAPERTRAY\n"));
				break;

				case APD_DEFAULTPAGESIZE:
					DBMSG((">DEFAULTPAGESIZE: %s,%s\n",token,line));
					/* ignore default */
					DBMSG(("<DEFAULTPAGESIZE: %s,%s\n",token,line));
					DBMSG((">PAGESIZE\n"));
					fPageSize=FALSE;
					gNormalImagePSSList=
						(char**)malloc((gNumPapers+1)*sizeof(char*));
					for(j=0;j<gNumPapers+1;j++) gNormalImagePSSList[j]=0;

					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORDAFTER,gFileName,
								"PageSize");
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_PAGESIZE){
							DBMSG(("<PAGESIZE: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","PageSize");
							goto ERROR;
						}
						/* if the entry has a ".Transverse" entry...
						 * process it
						 */
						subPtr=0;
						if(subPtr=DoTransverse(token,subPtr,&fTransverse)<0){
							APDError(E_TRANSVERSE,gFileName);
							goto ERROR;
						}
						if(!GetTranslation(token,subPtr)){
							APDError(E_TRANSLATION,gFileName);
							goto ERROR;
						}
						if((paperNum=MapItemToList(token,&gPaperList[1]))<0){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"PageSize");
							goto ERROR;
						}
						DBMSG((" PAGESIZE: ID=%d,paperNum=%d,token=%s,%s\n",
							tokenID,paperNum,token,&line[1]));
						if((ptr=GetString(line,string,++ptr))<0){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"PageSize");
							goto ERROR;
						}
						gNormalImagePSSList[paperNum]=
							malloc(strlen(string)+1);
						if(strlen(string)){
							strcpy(gNormalImagePSSList[paperNum],string);
						}
						DBMSG((" PAGESIZE PS: %s,%s\n",
							gNormalImagePSSList[paperNum],string));
						/* At this point there is at lease 1 "PageSize"
						 * entry.
						 */
						fPageSize=TRUE;
					}
					DBMSG(("<PAGESIZE\n"));
				break;

				case APD_DEFAULTPAGEREGION:
					DBMSG((">DEFAULTPAGEREGION: %s,%s\n",token,line));
					/* ignore default */
					DBMSG(("<DEFAULTPAGEREGION: %s,%s\n",token,line));
					DBMSG((">PAGEREGION\n"));
					fPageRegion=FALSE;
					gManualImagePSSList=
						(char**)malloc((gNumPapers+1)*sizeof(char*));
					for(j=0;j<gNumPapers+1;j++) gManualImagePSSList[j]=0;

					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORDAFTER,gFileName,
								"PageRegion");
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_PAGEREGION){
							DBMSG(("<PAGEREGION: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","PageRegion");
							goto ERROR;
						}
						/* if the entry has a ".Transverse" entry...
						 * process it
						 */
						subPtr=0;
						if(subPtr=DoTransverse(token,subPtr,&fTransverse)<0){
							APDError(E_TRANSVERSE,gFileName);
							goto ERROR;
						}
						if(!GetTranslation(token,subPtr)){
							APDError(E_TRANSLATION,gFileName);
							goto ERROR;
						}
						if((paperNum=MapItemToList(token,&gPaperList[1]))<0){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"PageRegion");
							goto ERROR;
						}
						DBMSG((" PAGEREGION:ID=%d,paperNum=%d,token=%s,%s\n",
							tokenID,paperNum,token,&line[1]));
						if((ptr=GetString(line,string,++ptr))<0){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"PageRegion");
							goto ERROR;
						}
						gManualImagePSSList[paperNum]=
							malloc(strlen(string)+1);
						if(strlen(string)){
							strcpy(gManualImagePSSList[paperNum],string);
						}
						DBMSG((" PAGEREGION PS: %s,%s\n",
							gManualImagePSSList[paperNum],string));
						/* At this point there is at lease 1 "PageRegion"
						 * entry.
						 */
						fPageRegion=TRUE;
					}
					DBMSG(("<PAGEREGION\n"));
				break;

				case APD_DEFAULTINPUTSLOT:
					DBMSG((">DEFAULTINPUTSLOT: %s,%s\n",token,line));

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"DefaultInputSlot");
						goto ERROR;
					}
					if(MapToken(token)==APD_NONE) continue;

					if((defInputSlotNum=
						MapItemToList(token,&gSourceList[1]))<0
					){
						/* paper is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"DefaultInputSlot");
						goto ERROR;
					}
					/* set up the default source entry--this will be
					 * overridden by AUTO capabilities
					 */
					gDefSourceCaps=defInputSlotNum;
					gSourceCaps[gDefSourceCaps]=TRUE;

					gInputSlotPSSList=
						(char**)malloc((gNumSources+1)*sizeof(char*));
					for(j=0;j<gNumSources+1;j++) gInputSlotPSSList[j]=0;

					DBMSG(("<DEFAULTINPUTSLOT: defInputSlotNum=%d,%s\n",
						defInputSlotNum,token));

					DBMSG((">INPUTSLOT\n"));
					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORDAFTER,gFileName,
								"InputSlot");
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_INPUTSLOT){
							DBMSG(("<INPUTSLOT: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","InputSlot");
							goto ERROR;
						}
						if((inputSlotNum=
							MapItemToList(token,&gSourceList[1]))<0
						){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"InputSlot");
							goto ERROR;
						}
						gSourceCaps[inputSlotNum]=TRUE;
						fInputSlot=TRUE;

						DBMSG((
							" INPUTSLOT: ID=%d,inputSlotNum=%d,token=%s,%s\n",
							tokenID,inputSlotNum,token,&line[1]));
						if((ptr=GetString(line,string,++ptr))<0){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"InputSlot");
							goto ERROR;
						}
						gInputSlotPSSList[inputSlotNum]=
							malloc(strlen(string)+1);
						if(strlen(string)){
							strcpy(gInputSlotPSSList[inputSlotNum],string);
						}
						DBMSG((" INPUTSLOT PS: %s,%s\n",
							gInputSlotPSSList[inputSlotNum],string));
					}
					DBMSG(("<INPUTSLOT\n"));
				break;

				case APD_ENDOFFILE:
					DBMSG2(("<ENDOFFILE: %s,%s\n",token,line));

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"EndOfFile");
						goto ERROR;
					}
					if(MapToken(token)==APD_NONE){
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,"EndOfFile");
						goto ERROR;
					}
					if((gEndOfFileCaps=MapItemToList(token,gLogicalList))<0){
						/* paper is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"EndOfFile");
						goto ERROR;
					}
					DBMSG2(("<ENDOFFILE: gEndOfFileCaps=%d,%s\n",
						gEndOfFileCaps,token));
				break;

				case APD_COLOR:

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"Color");
						goto ERROR;
					}
					if(MapToken(token)==APD_NONE){
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,"EndOfFile");
						goto ERROR;
					}
					if((gColor=MapItemToList(token,gLogicalList))<0){
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"Color");
						goto ERROR;
					}
				break;


				case APD_DEFAULTMANUALFEED:
					DBMSG(("<DEFAULTMANUALFEED: %s,%s\n",token,line));

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"DefaultManualFeed");
						goto ERROR;
					}
					if(MapToken(token)==APD_NONE) continue;

					if((defManualNum=MapItemToList(token,gLogicalList))<0){
						/* paper is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"DefaultManualFeed");
						goto ERROR;
					}
					DBMSG(("<DEFAULTMANUALFEED: defManualNum=%d,%s\n",
						defManualNum,token));

					DBMSG((">MANUALFEED AREA\n"));
					fManualFalse=fManualTrue=FALSE;

					gManualSwitchPSSList=
						(char**)malloc(3*sizeof(char*));
					for(j=0;j<3;j++) gManualSwitchPSSList[j]=0;

					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORD,gFileName,
								"ManualFeed");
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_MANUALFEED){
							DBMSG(("<MANUALFEED: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","ManualFeed");
							goto ERROR;
						}
						if((manualNum=MapItemToList(token,gLogicalList))<0){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"ManualFeed");
							goto ERROR;
						}
						if(manualNum==TRUE) fManualTrue=TRUE;
						if(manualNum==FALSE) fManualFalse=TRUE;

						DBMSG((" MANUALFEED: manNum=%d,fT=%d,fF=%d\n",
							manualNum,fManualTrue,fManualFalse));

						if((ptr=GetString(line,string,++ptr))<0){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"ManualFeed");
							goto ERROR;
						}
						DBMSG((" MANUALFEED PS: %s\n",string));
						gManualSwitchPSSList[manualNum]=
							malloc(strlen(string)+1);
						if(strlen(string)){
							strcpy(gManualSwitchPSSList[manualNum],string);
						}
						DBMSG((" MANUALFEED PS: %s,%s\n",
							gManualSwitchPSSList[manualNum],string));
					}
					if((fManualTrue && !fManualFalse)
						|| (!fManualTrue && fManualFalse)
					){
						APDError(E_TRUEFALSE,gFileName);
						goto ERROR;
					}
					if(fManualTrue && fManualFalse){
						gSourceCaps[MANUAL]=TRUE;
					}
					DBMSG(("<MANUALFEED: AFTER WHILE\n"));
				break;

				case APD_DEFAULTPAPERSTOCK:
					DBMSG(("<DEFAULTPAPERSTOCK: %s,%s\n",token,line));

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"DefaultPaperStock");
						goto ERROR;
					}
					if((defPaperStock=MapItemToList(token,&gPaperList[1]))<0){
						/* paper is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"DefaultPaperStock");
						goto ERROR;
					}
					gDefPaper=defPaperStock;

					DBMSG(("<DEFAULTPAPERSTOCK: defPaperStock=%d,%s\n",
						defPaperStock,token));
				break;

				case APD_DEFAULTIMAGEABLEAREA:
					DBMSG(("<DEFAULTIMAGEABLEAREA: %s,%s\n",token,line));

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"DefaultImageableArea");
						goto ERROR;
					}
					if((defPaperNum=MapItemToList(token,&gPaperList[1]))<0){
						/* paper is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"DefaultImageableArea");
						goto ERROR;
					}
					DBMSG(("<DEFAULTIMAGEABLEAREA: defPaperNum=%d,%s\n",
						defPaperNum,token));

					DBMSG((">IMAGEABLEAREA\n"));
					fImageableArea=FALSE;
					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORD,gFileName);
							goto ERROR;
						}
						if((tokenID=MapToken(token))!=APD_IMAGEABLEAREA){
							DBMSG(("<IMAGEABLEAREA: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						/* At this point there is at lease 1 "ImageableArea"
						 * entry.
						 */
						fImageableArea=TRUE;

						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","ImageableArea");
							goto ERROR;
						}
						/* if the entry has a ".Transverse" entry...
						 * process it
						 */
						subPtr=0;
						if(subPtr=DoTransverse(token,subPtr,&fTransverse)<0){
							APDError(E_TRANSVERSE,gFileName);
							goto ERROR;
						}
						if(!GetTranslation(token,subPtr)){
							APDError(E_TRANSLATION,gFileName);
							goto ERROR;
						}
						if((paperNum=MapItemToList(token,&gPaperList[1]))<0){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"ImageableArea");
							goto ERROR;
						}
						DBMSG((" IMAGEABLEAREA: paperNum=%d\n",paperNum));
						gPaperCaps[paperNum]=TRUE;

						if((ptr=GetString(line,string,++ptr))<0){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"ImageableArea");
							goto ERROR;
						}
						DBMSG((" IMAGEABLEAREA PS: %s\n",string));

						if (!GetMargins(string, &gMarginCaps[paperNum], paperNum)) {

							APDError(E_INCOMPLETE,gFileName);
							goto ERROR;
						}

					}
					if(!fImageableArea) gPaperCaps[defPaperNum]=TRUE;
					DBMSG(("<IMAGEABLEAREA: AFTER WHILE\n"));
				break;

				case APD_WINDOWSAUTO:
					DBMSG((">WINDOWSAUTO: %s,%s\n",token,line));
					gWindowsAutoPSS=0;

					if((ptr=GetString(line,string,++ptr))>=0
						&& strlen(string)
					){
						if(!BuildString(NULL,&gWindowsAutoPSS,string)){
							goto ERROR;
						}
						DBMSG((" WINDOWSAUTO:1malloc:%s,%s\n",
							gWindowsAutoPSS,string));
					}else if(ptr==-1){
						APDError(E_ILLEGALSTRINGAFTER,gFileName,
							"WindowsAuto");
						goto ERROR;
					}else if(ptr==-2){
						if(!BuildString(NULL,&gWindowsAutoPSS,string)){
							goto ERROR;
						}
						DBMSG((" WINDOWSAUTO:2malloc:%s,%s\n",
							gWindowsAutoPSS,string));
						while((ptr=GetStringAgain(string,fp))==-2){
							if(!BuildString(1,&gWindowsAutoPSS,string)){
								goto ERROR;
							}
							DBMSG((" WINDOWSAUTO:3realloc:%s,%s\n",
								gWindowsAutoPSS,string));
						}
						if(ptr>=0){
							if(!BuildString(1,&gWindowsAutoPSS,string)){
								goto ERROR;
							}
							DBMSG((" WINDOWSAUTO:4realloc:%s,%s\n",
								gWindowsAutoPSS,string));
						}else if(ptr==-1){
							APDError(E_ILLEGALSTRINGAFTER,gFileName,
								"WindowsAuto");
							goto ERROR;
						}
					}
					fWindowsAuto=TRUE;
					DBMSG(("<WINDOWSAUTO: AFTER WHILE\n"));
				break;

				case APD_DEFAULTTRANSFER:
					DBMSG(("<DEFAULTTRANSFER: %s,%s\n",token,line));
					gTransferPSS=0;

					/* look after the ':' for the default page size */
					if((ptr=GetToken(line,token,++ptr,'\0'))<0){
						APDError(E_MISSINGKEYWORDAFTER,gFileName,
							"DefaultTransfer");
						goto ERROR;
					}
					if((defTransferNum=MapItemToList(token,gTransferList))<0){
						/* transfer is not in the master list */
						APDError(E_ILLEGALKEYWORDAFTER,gFileName,
							"DefaultTransfer");
						goto ERROR;
					}
					DBMSG(("<DEFAULTTRANSFER: defTransferNum=%d,%s\n",
						defTransferNum,token));

					DBMSG((">TRANSFER\n"));
					fTransfer=FALSE;
					while(TRUE){

						if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
							goto ERROR;
						}else if(rc<0) break;

						if(fatalError=IsLineIllegal(line)) break;

						if(IsLineWhite(line)) continue;

						if((ptr=GetToken(line,token,1,'\0'))<0){
							APDError(E_MISSINGKEYWORD,gFileName);
							goto ERROR;
						}
						DBMSG(("token=%s\n",token));
						if(MapToken(token)!=APD_TRANSFER
							&& (GetToken(line,token,1,':')<0
								|| MapToken(token)!=APD_END)
						){
							DBMSG(("<TRANSFER: new category\n"));
							gReUseLineFlag=TRUE;
							break;
						}
						/* If we've already found the transfer function
						 * continue reading until the transfer entries
						 * are all read.
						 */
						if(fTransfer) continue;

						if((ptr=GetToken(line,token,++ptr,':'))<0){
							APDError(E_MISSINGAFTER,gFileName,
								":","Transfer");
							goto ERROR;
						}
						/* if the entry has a ".Inverse" entry...
						 * ignore it
						 */
						if(FindChar(token,0,'.')>=0){
							APDError(E_INVERSE,gFileName);
							continue;
						}
						if((transferNum=
							MapItemToList(token,gTransferList))<0
						){
							/* paper is not in the master list */
							APDError(E_ILLEGALKEYWORDAFTER,gFileName,
								"Transfer");
							goto ERROR;
						}
						DBMSG((" TRANSFER: transferNum=%d\n",transferNum));
						if(transferNum==defTransferNum){
							/* found the transfer function entry */
							if((ptr=GetString(line,string,++ptr))>=0
								&& strlen(string)
							){
								if(!BuildString(NULL,&gTransferPSS,string)){
									goto ERROR;
								}
								DBMSG((" WINDOWSAUTO:1malloc:%s,%s\n",
									gTransferPSS,string));
							}else if(ptr==-1){
								APDError(E_ILLEGALSTRINGAFTER,gFileName,
									"Transfer");
								goto ERROR;
							}else if(ptr==-2){
								if(!BuildString(NULL,&gTransferPSS,string)){
									goto ERROR;
								}
								DBMSG((" WINDOWSAUTO:2malloc:%s,%s\n",
									gTransferPSS,string));
								while((ptr=GetStringAgain(string,fp))==-2){
									if(!BuildString(1,&gTransferPSS,string)){
										goto ERROR;
									}
									DBMSG((" WINDOWSAUTO:3realloc:%s,%s\n",
										gTransferPSS,string));
								}
								if(ptr>=0){
									if(!BuildString(1,&gTransferPSS,string)){
										goto ERROR;
									}
									DBMSG((" WINDOWSAUTO:4realloc:%s,%s\n",
										gTransferPSS,string));
								}else if(ptr==-1){
									APDError(E_ILLEGALSTRINGAFTER,
										gFileName,"Transfer");
									goto ERROR;
								}
							}
							fTransfer=TRUE;
						}
					}
					DBMSG(("<TRANSFER: AFTER WHILE\n"));
				break;

				case APD_END:
					DBMSG((">END: %s,%s\n",token,line));
					DBMSG(("<END: AFTER WHILE\n"));
				break;

				default:
				break;
			}
		}
		DBMSG(("<1<AFTER WHILE\n"));
		if(fclose(fp)==EOF){
			APDError(E_CLOSE,gFileName);
			goto ERROR;
 		}
		if(gSourceCaps[AUTO]) gDefSourceCaps=AUTO;

		gNormalImageFlag=fPageSize;
		gManualImageFlag=fPageRegion;
		gManualSwitchFlag=gSourceCaps[MANUAL];
		gInputSlotFlag=gSourceCaps[AUTO] || fInputSlot;
		gTransferFlag=fTransfer;
		gWindowsAutoFlag=fWindowsAuto;
#ifdef DEBUG1_ON
		DBMSG1(("-------------------------------------------------------\n"));
		DBMSG1(("Paper list:\n"));
		for(j=0;j<gNumPapers;j++){
			DBMSG1(("[%d]%s(%d) ",j,gPaperList[j+1],gPaperCaps[j]));
		}
		DBMSG1(("Source list:\n"));
		for(j=0;j<gNumSources;j++){
			DBMSG1(("[%d]%s(%d)\n",j,gSourceList[j+1],gSourceCaps[j]));
		}
		DBMSG1(("Defaults: source=%d,res=%d,jobtimeout=%d,EOF=%d\n",
			gDefSourceCaps,gDefResolution,gDefJobTimeout,gEndOfFileCaps));
		DBMSG1(("-------------------------------------------------------\n"));
		DBMSG1(("Normal Image=%d\n",gNormalImageFlag));
		if(gNormalImageFlag){
			for(j=0;j<gNumPapers;j++){
				if(gPaperCaps[j] && gNormalImagePSSList[j]){
					DBMSG1(("[%d]%s\n",j,gNormalImagePSSList[j]));
				}
			}
		}

		DBMSG1(("Manual Image=%d\n",gManualImageFlag));
		if(gManualImageFlag){
			for(j=0;j<gNumPapers;j++){
				if(gPaperCaps[j] && gManualImagePSSList[j]){
					DBMSG1(("[%d]%s\n",j,gManualImagePSSList[j]));
				}
			}
		}

		DBMSG1(("Manual Switch=%d\n",gManualSwitchFlag));
		if(gManualSwitchFlag){
			for(j=0;j<2;j++){
				if(gManualSwitchPSSList[j]){
					DBMSG1(("[%d]%s\n",j,gManualSwitchPSSList[j]));
				}
			}
		}

		DBMSG1(("InputSlot=%d\n",gInputSlotFlag));
		if(gInputSlotFlag){
			for(j=0;j<gNumSources;j++){
				if(gSourceCaps[j] && gInputSlotPSSList[j]){
					DBMSG1(("[%d]%s\n",j,gInputSlotPSSList[j]));
				}
			}
		}

		DBMSG1(("Transfer=%d\n",gTransferFlag));
		if(gTransferFlag && gTransferPSS)DBMSG1(("%s\n",gTransferPSS));

		DBMSG1(("Windows Auto=%d\n",gWindowsAutoFlag));
		if(gWindowsAutoFlag && gWindowsAutoPSS){
			DBMSG1(("%s\n",gWindowsAutoPSS));
		}
		DBMSG1(("-------------------------------------------------------\n"));
#endif
		printf("\tGenerating file \"%s\".\n",gPrCapsFileList[i]);
		if(!MakePrCapsFile(gPrCapsFileList[i])){
			APDError(E_MAKE,gPrCapsFileList[i]);
			goto ERROR;
		}
		printf("\tGenerating file \"%s\".\n",gPSSFileList[i]);
		if(!MakePSSFile(gPSSFileList[i])){
			APDError(E_MAKE,gPSSFileList[i]);
			goto ERROR;
		}
		printf("\n");
	}
	if(fatalError) goto ERROR;

	DBMSG(("Making resource file\n"));
	/* remember that the caps list includes the entry for the default */
	if(!MakeFile("pass5a.rc",&gPrCapsFileList[1],
		gNumPrinters,"","PR_CAPS	LOADONCALL MOVEABLE DISCARDABLE",
		FALSE,FALSE)
	){
		APDError(E_MAKE,"pass5a.rc");
		goto ERROR;
	}

	DBMSG(("Making resource file\n"));
	/* remember that the pss list includes the entry for the default */
	if(!MakeFile("pass5b.rc",&gPSSFileList[1],
		gNumPrinters,"","PR_PSS	LOADONCALL MOVEABLE DISCARDABLE",
		FALSE,FALSE)
	){
		APDError(E_MAKE,"pass5b.rc");
		goto ERROR;
	}

	DBMSG(("Making resource file\n"));
	/* remember that the dir list includes the entry for the default */
	if(!MakeFile("pass5c.rc",&dirFileList[1],
		gNumPrinters,"","MYFONTDIR	LOADONCALL",FALSE,FALSE)
	){
		APDError(E_MAKE,"pass5c.rc");
		goto ERROR;
	}

#if 0
	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 5A--begin */",
		"/* Pass 5A--end */",&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	/* remember that the caps list includes the entry for the default */
	if(!Replace("pscript.rc",startLine,stopLine,&gPrCapsFileList[1],
		gNumPrinters,"","PR_CAPS	LOADONCALL MOVEABLE DISCARDABLE",
		FALSE,FALSE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}

	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 5B--begin */",
		"/* Pass 5B--end */",&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	/* remember that the pss list includes the entry for the default */
	if(!Replace("pscript.rc",startLine,stopLine,&gPSSFileList[1],
		gNumPrinters,"","PR_PSS	LOADONCALL MOVEABLE DISCARDABLE",
		FALSE,FALSE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}

	DBMSG(("Locating list\n"));
	if(!ParseForList("pscript.rc","/* Pass 5C--begin */",
		"/* Pass 5C--end */",&startLine,&stopLine)
	){
		APDError(E_FIND,"pscript.rc");
		goto ERROR;
	}
	DBMSG(("Replacing list\n"));
	/* remember that the dir list includes the entry for the default */
	if(!Replace("pscript.rc",startLine,stopLine,&dirFileList[1],
		gNumPrinters,"","MYFONTDIR	LOADONCALL",FALSE,FALSE)
	){
		APDError(E_MAKE,"pscript.rc");
		goto ERROR;
	}
#endif
	return(TRUE);

ERROR:
	return(FALSE);
}

