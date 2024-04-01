#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <math.h>
#include <ctype.h>

#include "apd.h"
#include "globals.h"
#include "apderror.h"

void APDError(int,char*,...);

extern int errno;

/****************************************************************************/
/*#define DEBUG_ON*/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif

/*#define DEBUG1_ON*/
#ifdef DEBUG1_ON
#define DBMSG1(msg) printf msg
#else
#define DBMSG1(msg)
#endif

/*#define DEBUG2_ON*/
#ifdef DEBUG2_ON
#define DBMSG2(msg) printf msg
#else
#define DBMSG2(msg)
#endif

/* for GetTranslation() */
/*#define DEBUG3_ON*/
#ifdef DEBUG3_ON
#define DBMSG3(msg) printf msg
#else
#define DBMSG3(msg)
#endif

/* for DoTransverse() */
/*#define DEBUG4_ON*/
#ifdef DEBUG4_ON
#define DBMSG4(msg) printf msg
#else
#define DBMSG4(msg)
#endif
/****************************************************************************/

void GetListLength(char**,int*);
void GetListLength(list,numInList)
	char *list[];
	int *numInList;
{
	*numInList=0;
	while(*list[(*numInList)++]);
	(*numInList)--;
	DBMSG(("numInList=%d\n",*numInList));
}


int MapItemToList(char*,char**);
int MapItemToList(item,list)
	char *item;
	char **list;
{
	int i=0;
	while(list[i]){
		if(!strcmp(list[i],item)) break;
		i++;
	}
	/* default entry is not in the list? */
	if(!list[i]) i=-1;

	return(i);
}


BOOL CreateHeaderFile(char*,char*,int,int);
BOOL CreateHeaderFile(fileName,constantPrefix,minValue,numInList)
	char *fileName;
	char *constantPrefix;
	int minValue;
	int numInList;
{
	FILE *fp;
	int i;

	if(!(fp=fopen(fileName,"w"))) goto ERROR;

	fprintf(fp,"/* this files was created by APD.EXE.  do not edit */\n\n");
	fprintf(fp,"#define %sMIN\t%d\n",constantPrefix,minValue);
	for(i=0;i<numInList;i++){
		fprintf(fp,"#define %s%d\t%d\n",constantPrefix,i+1,minValue+i);
	}
	fprintf(fp,"#define %sMAX\t%d\n",constantPrefix,minValue+i-1);
	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL ParseForList(char*,char*,char*,int*,int*);
BOOL ParseForList(fileName,startString,stopString,startLine,stopLine)
	char *fileName;
	char *startString;
	char *stopString;
	int *startLine;
	int *stopLine;
{
	FILE *fp;
	char buffer[MAX_LINE_LENGTH];
	int i;

	*startLine=0;
	*stopLine=0;

	DBMSG(("%s: %s %s\n",fileName,startString,stopString));
	if(!(fp=fopen(fileName,"r"))) goto ERROR;
	DBMSG(("reading the file\n"));
	for(i=0;TRUE;i++){
		if(!fgets(buffer,MAX_LINE_LENGTH,fp)) goto ERROR;
		/* get rid of the newline character */
		buffer[strlen(buffer)-1]='\0';
		if(!(*startLine) && !strcmp(buffer,startString)) *startLine=i;
		if(!strcmp(buffer,stopString)) break;
	}
	DBMSG(("%s\n",buffer));
	*stopLine=i;
	DBMSG(("start=%d,stop=%d\n",*startLine,*stopLine));
	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


void Convert(char*,char*);
void Convert(name1,name2)
	char* name1;
	char* name2;
{
	int i;
	int j;

	name2[0]=name1[0];
	i=j=1;
	while(name1[i]){
		if(j>MAX_TOKEN_LENGTH) break;
		if(isupper(name1[i])){
			name2[j++]=' ';
			name2[j]=(char)tolower(name1[i]);
		}else name2[j]=name1[i];
		j++;
		i++;
	}
	name2[j]='\0';
}


BOOL MakeFile(char*,char**,int,char*,char*,BOOL,BOOL);
BOOL MakeFile(fileName,list,numInList,prefix,suffix,fStringTable,fConvert)
	char *fileName;
	char *list[];
	int numInList;
	char *prefix;
	char *suffix;
	BOOL fStringTable;
	BOOL fConvert;
{
	FILE *fp;
	char newStr[MAX_TOKEN_LENGTH+1];
	int i;

	DBMSG(("%s\n",fileName));
	if(!(fp=fopen(fileName,"w"))){
		goto ERROR;
	}
	for(i=0;i<numInList;i++){
		DBMSG(("%d=%s\n",i,list[i]));
		if(fStringTable){
			if(fConvert){
				Convert(list[i],newStr);
				fprintf(fp,"\t%s%d%s\t\t\"%s\"\n",prefix,i+1,suffix,newStr);
			}else{
				fprintf(fp,"\t%s%d%s\t\t\"%s\"\n",prefix,i+1,suffix,list[i]);
			}
		}else{
			fprintf(fp,"%s%d\t%s %s\n",prefix,i+1,suffix,list[i]);
		}
	}
	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL Replace(char*,int,int,char**,int,char*,char*,BOOL,BOOL);
BOOL Replace(fileName,startLine,stopLine,list,numInList,prefix,suffix,
fStringTable,fConvert)
	char *fileName;
	int startLine;
	int stopLine;
	char *list[];
	int numInList;
	char *prefix;
	char *suffix;
	BOOL fStringTable;
	BOOL fConvert;
{
	FILE *fp1;
	FILE *fp2;
	char buffer[MAX_LINE_LENGTH];
	char newStr[MAX_TOKEN_LENGTH+1];
	static char tempFileName1[]="temp1";
	static char tempFileName2[]="temp2";
	int i;

	DBMSG(("%s: %d,%d\n",fileName,startLine,stopLine));
	if(!(fp1=fopen(fileName,"r")) || !(fp2=fopen(tempFileName1,"w"))){
		goto ERROR;
	}
	for(i=0;i<=startLine;i++){
		if(!fgets(buffer,MAX_LINE_LENGTH,fp1)) goto ERROR;
		if(fputs(buffer,fp2)==EOF) goto ERROR;
	}
	for(i=0;i<stopLine-startLine-1;i++){
		if(!fgets(buffer,MAX_LINE_LENGTH,fp1)) goto ERROR;
	}
	for(i=0;i<numInList;i++){
		DBMSG(("%d=%s\n",i,list[i]));
		if(fStringTable){
			if(fConvert){
				Convert(list[i],newStr);
				fprintf(fp2,"\t%s%d%s\t\t\"%s\"\n",prefix,i+1,suffix,newStr);
			}else{
				fprintf(fp2,"\t%s%d%s\t\t\"%s\"\n",prefix,i+1,suffix,list[i]);
			}
		}else{
			fprintf(fp2,"%s%d\t%s %s\n",prefix,i+1,suffix,list[i]);
		}
	}
	while(TRUE){
		if(!fgets(buffer,MAX_LINE_LENGTH,fp1)){
			/* is this an error or the end of file? */
			if(!feof(fp1)){
				fclose(fp1);
				goto ERROR;
			}
			break;
		}
		if(fputs(buffer,fp2)==EOF) goto ERROR;
	}
	if((fclose(fp1)==EOF) || (fclose(fp2)==EOF)) goto ERROR;

	/* rename file to temporary file name */
	DBMSG(("renaming %s to %s\n",fileName,tempFileName2));
	if(rename(fileName,tempFileName2)){
		DBMSG(("ERROR #%d\n",errno));
		goto ERROR;
	}
	/* rename temporary file to old file name */
	DBMSG(("renaming %s to %s\n",tempFileName1,fileName));
	if(rename(tempFileName1,fileName)){
		DBMSG(("ERROR #%d\n",errno));
		goto ERROR;
	}
	/* delete temp files */
	DBMSG(("deleting %s\n",tempFileName2));
	if(unlink(tempFileName2)==-1) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL Append(char*,char*,int,char*);
BOOL Append(fileName,prefix,num,suffix)
	char *fileName;
	char *prefix;
	int num;
	char *suffix;
{
	FILE *fp;

	if(!(fp=fopen(fileName,"a+"))){
		goto ERROR;
	}
	fprintf(fp,"\n\n%s%d%s\n",prefix,num,suffix);
	if((fclose(fp)==EOF)) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}

int Scale(int,int,int);
int Scale(a,b,c)
	int a;
	int b;
	int c;
{
	return((int)(  ( ((long)a*(long)b) + (long)abs(c)/2L ) / (long)c  )   );
}


FILE * OpenFile(char *,char *);
FILE * OpenFile(pFileName,pModeString)
	char *pFileName;
	char *pModeString;
{
	FILE *pF,*fopen();

	if( !(pF=fopen(pFileName,pModeString)) ){
		APDError(E_OPEN,pFileName);
		pF=0;
	}
	DBMSG(("OpenFile pF=%d\n",pF));
	return(pF);
}

int CloseFile(FILE *,char *);
int CloseFile(pFile,pFileName)
	FILE *pFile;
	char *pFileName;
{
	int rc=1;

	if(fclose(pFile)==EOF){
		APDError(E_CLOSE,pFileName);
		rc=0;
	}
	DBMSG(("CloseFile rc=%d\n",rc));
	return(rc);
}


int WriteFile(char *,int,int,FILE *,char *);
int WriteFile(pData,iDataSize,iDataCount,pF,pFileName)
	char *pData;
	int iDataSize;
	int iDataCount;
	FILE *pF;
	char *pFileName;
{
	int iCount;
	int rc=1;

	if((iCount=fwrite(pData,iDataSize,iDataCount,pF))!=iDataCount){
		APDError(E_WRITE,pFileName);
		rc=0;
	}
	DBMSG(("WriteFile rc=%d, iCount=%d\n",rc,iCount));
	return(rc);
}


int Data2File(char *,char *,int,int);
int Data2File(pFileName,pData,iDataSize,iDataCount)
	char *pFileName;
	char *pData;
	int	iDataSize;
	int iDataCount;
{
	FILE *pF,*OpenFile();
	char *pMode;
	int fWriteOK=1;
	int rc=1;

	printf("\tConverting data into binary file...%s\n",pFileName);
	if(pF=OpenFile(pFileName,"wb")){
		fWriteOK=WriteFile(pData,iDataSize,iDataCount,pF,pFileName);
		if(!fWriteOK || !CloseFile(pF,pFileName)) rc=0;
	}else rc=0;

	DBMSG(("Data2File rc=%d\n",rc));
	return(rc);
}


BOOL IsWhite(char);
BOOL IsWhite(c)
	char c;
{
	BOOL flag=FALSE;

	if(c==' '||c=='\t'||c=='\n'||c=='\r') flag=TRUE;
	return(flag);
}


int EatWhite(char*,int);
int EatWhite(line,startPos)
	char *line;
	int startPos;
{
	int i=startPos;

	while(IsWhite(line[i])) i++;
	return(i);
}


int FindChar(char*,int,char);
int FindChar(line,startPos,theChar)
	char *line;
	int startPos;
	char theChar;
{
	int i=startPos;

	while(line[i]!=theChar)i++;
	if(i>strlen(line)) i=-1;

	return(i);
}


int GetLine(char*,int,FILE*);
int GetLine(line,maxLen,fp)
	char *line;
	int maxLen;
	FILE *fp;
{
	int rc=1;

/*	DBMSG((">GetLine(): reuse=%d\n",gReUseLineFlag));
 */
	if(gReUseLineFlag) gReUseLineFlag=FALSE;

	else{
		gLineNum++;
		if(!fgets(line,MAX_LINE_LENGTH,fp)){
			/* is this an error or the end of file? */
			rc=-1;
			if(!feof(fp)){
				fclose(fp);
				APDError(E_READAPD,gFileName);
				rc=0;
			}
		}
		if(FindChar(line,0,'\n')<0){
			APDError(E_LINETOOLONG,gFileName);
			rc=0;
		}
/*		DBMSG((" GetLine(): %s,%d\n",line,rc));
 */	}
/*	DBMSG(("<GetLine(): reuse=%d\n",gReUseLineFlag));
 */
	return(rc);
}


BOOL ExtractString(char*,char*,int,int);
BOOL ExtractString(string,line,startPos,stopPos)
	char *string;
	char *line;
	int startPos;
	int stopPos;
{
	int i;

/*	DBMSG((">ExtractString(): %s,%d,%d\n",line,startPos,stopPos));
 */
	string[0]='\0';
	if(startPos>stopPos) goto ERROR;

	for(i=startPos;i<=stopPos;i++){
		string[i-startPos]=line[i];
/*		DBMSG(("%d<%c>,",i,line[i]));
 */	}
	string[i-startPos]='\0';
/*	DBMSG(("\n<ExtractString(): i=%d,%s\n",i,string));
 */
	return(TRUE);

ERROR:
	return(FALSE);
}
/***************************************************************
* Name: MapToken()
*
* Action: This routine maps an ascii key word into an integer token.
*
* Returns: The token value.
*
*****************************************************************
*/
int MapToken(char*);
int MapToken(token)
	char *token;	    /* Ptr to the ascii keyword string */
{
	TOKEN *pToken;
	int theToken=APD_UNDEFINED;

	pToken = gTokenList;
	while (pToken->pString){
		if(!strcmp(token, pToken->pString)){
			theToken=pToken->id;
			break;
		}
		++pToken;
	}
	return(theToken);
}


int GetToken(char*,char*,int,char);
/* extracts a token from LINE with leading white space
 * that ends in white space or DELIMITER character
 */
int GetToken(line,token,startPos,delimiter)
	char *line;
	char *token;
	int startPos;
	char delimiter;
{
	int endPos;

	token[0]='\0';

/*	DBMSG((">GetToken(): line=%s,start=%d,delim=%c\n",
 *		line,startPos,delimiter));
 */
	endPos=startPos=EatWhite(line,startPos);

/*	DBMSG((" GetToken(): start=%d\n",startPos));
 */
	if(delimiter=='\0'){
		while(line[endPos] && !IsWhite(line[endPos])) endPos++;
	}else{
		if((endPos=FindChar(line,startPos,delimiter))<0) goto ERROR;
	}
/*	DBMSG((" GetToken(): stop=%d\n",endPos));
 */
	if(endPos-startPos+1 > MAX_TOKEN_LENGTH){
		APDError(E_TOKENTOOLONG,gFileName);
		goto ERROR;
	}
	if(!ExtractString(token,line,startPos,endPos-1)) goto ERROR;
/*	DBMSG(("<GetToken(): token=%s,start=%d,stop=%d\n",
 *		token,startPos,endPos-1));
 */
	return(endPos);

ERROR:
	return(-1);
}


int GetString(char*,char*,int);
int GetString(line,string,startPos)
	char *line;
	char *string;
	int startPos;
{
	int endPos;
	int len;
	BOOL fMore=FALSE;

/*	DBMSG((">GetString(): line=%s,start=%d\n",line,startPos));
 */
	startPos=EatWhite(line,startPos);
/*	DBMSG((" GetString(): start=%d\n",startPos));
 */
	/* find the 1st " */
	if((startPos=FindChar(line,startPos,'"'))<0) goto ERROR;
/*	DBMSG((" GetString(): start=%d\n",startPos));
 */
	/* find the 2nd " */
	if((endPos=FindChar(line,++startPos,'"'))<0){
		/* the string goes across multiple lines */
		fMore=TRUE;
		endPos=strlen(line);
	}
/*	DBMSG((" GetString(): stop=%d\n",endPos));
 */
	if(endPos-startPos+1 > MAX_LINE_LENGTH){
		APDError(E_STRINGTOOLONG,gFileName);
		goto ERROR;
	}
	/* Is there more stuff in the string on the next line? */

	ExtractString(string,line,startPos,endPos-1);
/*	DBMSG((" GetString: %s\n",string));
 */
	len=strlen(string);
	/* if there is a line feed convert to CRLF */
	if(string[len-1]=='\n'){
		if(len+1 > MAX_LINE_LENGTH){
			APDError(E_STRINGTOOLONG,gFileName);
			goto ERROR;
		}
		string[len-1]='\r';
		string[len]='\n';
		string[len+1]='\0';
	}
	if(fMore) endPos=-2;
	return(endPos);

ERROR:
	return(-1);
}


int GetStringAgain(char*,FILE*);
int GetStringAgain(string,fp)
	char *string;
	FILE *fp;
{
	int startPos=0;
	int endPos=0;
	int tokenID;
	int len;
	int rc;
	char token[MAX_TOKEN_LENGTH+1];
	char line[MAX_LINE_LENGTH+1];

	if(!(rc=GetLine(line,MAX_LINE_LENGTH,fp))){
		goto ERROR;
	}else if(rc<0){
		APDError(E_EOF,gFileName);
		goto ERROR;
	}
	if(line[0]=='*' && (rc=GetToken(line,token,1,':'))>0){
#ifdef DEBUG_ON
		tokenID=MapToken(token);
		DBMSG(("tokenID=%d,token=%s\n",tokenID,token));
#endif
		if((tokenID=MapToken(token))==APD_END){
			APDError(E_END,gFileName);
			goto ERROR;
		}else if(tokenID != APD_UNDEFINED){
			APDError(E_UNTERMINATEDSTRING,gFileName);
			goto ERROR;
		}
	}else if(rc<0){
		APDError(E_MISSING,gFileName,":");
		goto ERROR;
	}
	if((endPos=FindChar(line,startPos,'"'))>=0){
		ExtractString(string,line,startPos,endPos-1);
/*		DBMSG((" GetStringAgain: %s\n",string));
 */	}else{
		endPos=-2;
		strcpy(string,line);
/*		DBMSG((" GetStringAgain: %s\n",string));
 */	}
	len=strlen(string);
	/* if there is a line feed convert to CRLF */
	if(string[len-1]=='\n'){
		if(len+1 > MAX_LINE_LENGTH){
			APDError(E_STRINGTOOLONG,gFileName);
			goto ERROR;
		}
		string[len-1]='\r';
		string[len]='\n';
		string[len+1]='\0';
	}
	return(endPos);

ERROR:
	return(-1);
}


int DoTransverse(char*,int,BOOL*);
int DoTransverse(line,ptr,fTransverse)
	char *line;
	int ptr;
	BOOL *fTransverse;
{
	char delim=':';
	char token[MAX_TOKEN_LENGTH];
	int dotPosition;
	int slashPosition;

	DBMSG4((">DoTransverse():%s,ptr=%d\n",line,ptr));

	*fTransverse=FALSE;

	dotPosition=FindChar(line,0,'.');
	slashPosition=FindChar(line,0,'/');
	DBMSG4(("DoTransverse():dot=%d,slash=%d\n",dotPosition,slashPosition));

	if(dotPosition>=0){
		if(dotPosition<slashPosition) delim='/';

		if((ptr=GetToken(line,token,dotPosition+1,delim))<0
			|| MapToken(token)!=APD_TRANSVERSE
		){
			goto ERROR;
		}
		*fTransverse=TRUE;
	}
	DBMSG4(("<DoTransverse():%s,%c,%s,ptr=%d\n",token,delim,line,ptr));
	return(ptr);

ERROR:
	DBMSG4(("<DoTransverse():ERROR:%s,%c,%s,ptr=%d\n",token,delim,line,ptr));
	return(-1);
}


BOOL GetTranslation(char*,int);
BOOL GetTranslation(token,ptr)
	char *token;
	int ptr;
{
	int startPos;
	int endPos;
	char string[MAX_TOKEN_LENGTH];

	DBMSG3((">GetTranslation(): token=%s\n",token));
	if((startPos=FindChar(token,ptr,'/'))>=0){
		DBMSG3((" GetTranslation(): start=%d\n",startPos));

		endPos=strlen(token);
		DBMSG3((" GetTranslation(): stop=%d\n",endPos));

		if(endPos-startPos+1 > MAX_LINE_LENGTH){
			APDError(E_STRINGTOOLONG,gFileName);
			goto ERROR;
		}
		ExtractString(string,token,startPos+1,endPos);
		strcpy(token,string);
	}
	DBMSG3(("<GetTranslation(): token=%s\n",token));

	return(TRUE);

ERROR:
	DBMSG3(("<GetTranslation(): ERROR\n"));
	return(FALSE);
}

BOOL CreatePrCapsStructure(char*,int,int);
BOOL CreatePrCapsStructure(fileName,numSources,numPapers)
	char *fileName;
	int numSources;
	int numPapers;
{
	FILE *fp;

	if(!(fp=fopen("printcap.h","w"))) goto ERROR;
	fprintf(fp,"\n");
	fprintf(fp,"/* Printer capability structure--used to consolidate the individual\n");
	fprintf(fp," * printer's capabilities and requirements.\n");
	fprintf(fp," * ** This file was generated by APD.EXE **\n");
	fprintf(fp," */\n\n");

	fprintf(fp,"typedef struct {\n");
	fprintf(fp,"\tBOOL feed[%d];\n",numSources);
	fprintf(fp,"\tBOOL paper[%d]; /* map to DMPAPER_* */\n",numPapers);
	fprintf(fp,"\tRECT image[%d]; /* map to DMPAPER_* */\n",numPapers);
	fprintf(fp,"\tint defFeed;\n");
	fprintf(fp,"\tint defRes;\n");
	fprintf(fp,"\tint defJobTimeout;\n");
	fprintf(fp,"\tBOOL fEOF;\n");
	fprintf(fp,"\tBOOL fColor;\n");
	fprintf(fp,"} PRINTER;\n");
	fprintf(fp,"typedef PRINTER FAR *LPPRINTER;\n");
	fprintf(fp,"\n");
	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL MakePrCapsFile(char*);
BOOL MakePrCapsFile(fileName)
	char *fileName;
{
	FILE *fp;
	int tempDefSourceCaps=gDefSourceCaps+IDS_SR_MIN;

	if(!(fp=fopen(fileName,"w"))) goto ERROR;

	if(fwrite((char*)gSourceCaps,sizeof(BOOL),gNumSources,fp)!=gNumSources){
		goto ERROR;
	}
	if(fwrite((char*)gPaperCaps,sizeof(BOOL),gNumPapers,fp)!=gNumPapers){
		goto ERROR;	      
	}
	if(fwrite((char*)gMarginCaps,sizeof(RECT),gNumPapers,fp)!=gNumPapers){
		goto ERROR;
	}
	if(fwrite((char*)&tempDefSourceCaps,sizeof(int),1,fp)!=1) goto ERROR;
	if(fwrite((char*)&gDefResolutionCaps,sizeof(int),1,fp)!=1) goto ERROR;
	if(fwrite((char*)&gDefJobTimeoutCaps,sizeof(int),1,fp)!=1) goto ERROR;
	if(fwrite((char*)&gEndOfFileCaps,sizeof(BOOL),1,fp)!=1) goto ERROR;
	if(fwrite((char*)&gColor,sizeof(BOOL),1,fp)!=1) goto ERROR;

	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL fwriteZeroLong(FILE *,int);
BOOL fwriteZeroLong(fp,num)
	FILE *fp;
	int num;
{
	int i;
	long zeroL=0L;

	DBMSG1((">fwriteZeroLong(): numZeros=%d\n",num));
	for(i=0;i<num;i++){
		if((fwrite((char*)&zeroL,sizeof(long),1,fp))!=1) goto ERROR;
	}
	DBMSG1(("<fwriteZeroLong()\n"));
	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL fwriteLong(FILE *,long*);
BOOL fwriteLong(fp,num)
	FILE *fp;
	long *num;
{
	if((fwrite((char*)num,sizeof(long),1,fp))!=1) goto ERROR;
	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL fwriteInt(FILE *,int*);
BOOL fwriteInt(fp,num)
	FILE *fp;
	int *num;
{
	if((fwrite((char*)num,sizeof(int),1,fp))!=1) goto ERROR;
	return(TRUE);

ERROR:
	return(FALSE);
}


long DumpStringList(FILE*,int,BOOL,long,char**,BOOL*);
long DumpStringList(fp,num,flag,offset,list,caps)
	FILE *fp;
	int num;
	BOOL flag;
	long offset;
	char **list;
	BOOL *caps;
{
	int i;
	/* remember to start the string locations after the LIST area */
	long tempOffset=offset;

	DBMSG1((">DumpStringList(): num=%d,flag=%d,off=%d,list=%d,caps=%d\n",
		num,flag,offset,(int)list,(int)caps));
	if(flag){
		tempOffset+=(long)(num*sizeof(long));
		for(i=0;i<num;i++){
			DBMSG1((" DumpStringList(): offset=%ld\n",tempOffset));
			if(caps[i] && list[i]){
				if(!fwriteLong(fp,&tempOffset)) goto ERROR;
				tempOffset+=(long)(strlen(list[i])+sizeof(int));
			}else if(!fwriteZeroLong(fp,1)) goto ERROR;
		}
	}
	DBMSG1(("<DumpStringList(): offset=%ld\n",tempOffset));
	return(tempOffset);

ERROR:
	return(-1L);
}


BOOL DumpPSSList(FILE*,int,BOOL,char**,BOOL*);
BOOL DumpPSSList(fp,num,flag,list,caps)
	FILE *fp;
	int num;
	BOOL flag;
	char **list;
	BOOL *caps;
{
	int i;
	int len;

	DBMSG1((">DumpPSSList(): num=%d,flag=%d,list=%d,caps=%d\n",
		num,flag,(int)list,(int)caps));
	if(flag){
		for(i=0;i<num;i++){
			if(caps[i] && list[i]){
				len=strlen(list[i]);
				if(!fwriteInt(fp,&len)) goto ERROR;
				DBMSG1((" DumpPSSList(): [%d](%d)%s\n",i,len,list[i]));
				if(fputs(list[i],fp)==EOF) goto ERROR;
			}
		}
	}
	DBMSG1(("<DumpPSSList()\n"));
	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL MakePSSFile(char*);
BOOL MakePSSFile(fileName)
	char *fileName;
{
	int i;
	int len;
	FILE *fp;
	long offset=0L;
	long normalImageOffset;
	long manualImageOffset;
	long manualSwitchOffset;
	long inputSlotOffset;
	long transferOffset;
	long windowsAutoOffset;

	if(!(fp=fopen(fileName,"wb"))) goto ERROR;

	/* initialize the PSS directory */
	if(fseek(fp,offset,0)) goto ERROR;
	if(!fwriteZeroLong(fp,NUM_DIR_ENTRIES+2)) goto ERROR;

	offset+=(long)(sizeof(long)*(NUM_DIR_ENTRIES+2));

	normalImageOffset=offset;

	DBMSG1(("Normal Image=%d,%ld\n",gNormalImageFlag,normalImageOffset));
	if((manualImageOffset=DumpStringList(
		fp,gNumPapers,gNormalImageFlag,normalImageOffset,
		gNormalImagePSSList,gPaperCaps))<0
	){
		goto ERROR;
	}
	if(!DumpPSSList(fp,gNumPapers,gNormalImageFlag,gNormalImagePSSList,
		gPaperCaps)
	){
		goto ERROR;
	}

	DBMSG1(("Manual Image=%d,%ld\n",gManualImageFlag,manualImageOffset));
	if((manualSwitchOffset=DumpStringList(
		fp,gNumPapers,gManualImageFlag,manualImageOffset,
		gManualImagePSSList,gPaperCaps))<0
	){
		goto ERROR;
	}
	if(!DumpPSSList(fp,gNumPapers,gManualImageFlag,gManualImagePSSList,
		gPaperCaps)
	){
		goto ERROR;
	}

	DBMSG1(("Manual Switch=%d,%ld\n",gManualSwitchFlag,manualSwitchOffset));
	{
		BOOL manualCaps[2];

		manualCaps[0]=TRUE;
		manualCaps[1]=TRUE;
		if((inputSlotOffset=DumpStringList(
			fp,2,gManualSwitchFlag,manualSwitchOffset,
			gManualSwitchPSSList,manualCaps))<0
		){
			goto ERROR;
		}
		if(!DumpPSSList(fp,2,gManualSwitchFlag,gManualSwitchPSSList,
			manualCaps)
		){
			goto ERROR;
		}
	}

	DBMSG1(("InputSlot=%d,%ld\n",gInputSlotFlag,inputSlotOffset));
	if((transferOffset=DumpStringList(
		fp,gNumSources,gInputSlotFlag,inputSlotOffset,
		gInputSlotPSSList,gSourceCaps))<0
	){
		goto ERROR;
	}
	if(!DumpPSSList(fp,gNumSources,gInputSlotFlag,gInputSlotPSSList,
		gSourceCaps)
	){
		goto ERROR;
	}

	offset=0L;
	DBMSG1(("Transfer=%d,%ld\n",gTransferFlag,transferOffset));
	if(fseek(fp,transferOffset,0)) goto ERROR;
	if(gTransferFlag && gTransferPSS){
		len=strlen(gTransferPSS);
		offset=(long)(len+sizeof(int));
		if(!fwriteInt(fp,&len)) goto ERROR;
		if(fputs(gTransferPSS,fp)==EOF) goto ERROR;
		DBMSG1(("(%d)%s\n",strlen(gTransferPSS),gTransferPSS));
	}

	windowsAutoOffset=transferOffset+offset;
	DBMSG1(("Windows Auto=%d,%ld\n",gWindowsAutoFlag,windowsAutoOffset));
	if(gWindowsAutoFlag && gWindowsAutoPSS){
		len=strlen(gWindowsAutoPSS);
		if(!fwriteInt(fp,&len)) goto ERROR;
		if(fputs(gWindowsAutoPSS,fp)==EOF) goto ERROR;
		DBMSG1(("(%d)%s\n",strlen(gWindowsAutoPSS),gWindowsAutoPSS));
	}

	/* write the PSS directory */

	offset=0L;
	if(fseek(fp,offset,0)) goto ERROR;
	if(gNormalImageFlag && !fwriteLong(fp,&normalImageOffset)) goto ERROR;

	offset+=sizeof(long);
	if(fseek(fp,offset,0)) goto ERROR;
	if(gManualImageFlag && !fwriteLong(fp,&manualImageOffset)) goto ERROR;

	offset+=sizeof(long);
	if(fseek(fp,offset,0)) goto ERROR;
	if(gManualSwitchFlag && !fwriteLong(fp,&manualSwitchOffset)) goto ERROR;

	offset+=sizeof(long);
	if(fseek(fp,offset,0)) goto ERROR;
	if(gInputSlotFlag  && !fwriteLong(fp,&inputSlotOffset)) goto ERROR;

	offset+=sizeof(long);
	if(fseek(fp,offset,0)) goto ERROR;
	if(gTransferFlag && !fwriteLong(fp,&transferOffset)) goto ERROR;

	offset+=sizeof(long);
	if(fseek(fp,offset,0)) goto ERROR;
	if(gWindowsAutoFlag && !fwriteLong(fp,&windowsAutoOffset)) goto ERROR;

	if(fclose(fp)==EOF) goto ERROR;

	return(TRUE);

ERROR:
	return(FALSE);
}


void InitPrCaps(void);
void InitPrCaps()
{
	int i;

	/* initialize capabilities for each printer being processed */
	for(i=0;i<gNumPapers;i++) gPaperCaps[i]=FALSE;
	for(i=0;i<gNumSources;i++) gSourceCaps[i]=FALSE;
	for(i=0;i<gNumPapers;i++) {
		gMarginCaps[i].top = gMarginCaps[i].left = 0;
		gMarginCaps[i].bottom = gMarginCaps[i].right = 0;
	}
	gDefResolutionCaps=gDefResolution;
	gDefJobTimeoutCaps=gDefJobTimeout;
	gDefSourceCaps=gDefSource;
	gEndOfFileCaps=gDefEndOfFile;
	gColor=gDefColor;
}


void InitPSS(void);
void InitPSS()
{
	gNormalImagePSSList=0;
	gManualImagePSSList=0;
	gManualSwitchPSSList=0;
	gInputSlotPSSList=0;
	gTransferPSS=0;
	gWindowsAutoPSS=0;

	gNormalImageFlag=0;
	gManualImageFlag=0;
	gManualSwitchFlag=0;
	gTransferFlag=0;
	gWindowsAutoFlag=0;
}


BOOL IsLineIllegal(char*);
BOOL IsLineIllegal(line)
	char *line;
{
	BOOL fError=FALSE;

	/* a non-'*' or non white space character in 1st
	 * position --> illegal line
	 */
	if(line[0]!='*' && !IsWhite(line[0])){
		fError=TRUE;
	}
	return(fError);
}


BOOL IsLineWhite(char *);
BOOL IsLineWhite(line)
	char *line;
{
	BOOL fWhite=FALSE;

	/* if the line is a comment or starts with white space
	 * then ignore it
	 */
	if(line[1]=='%' || IsWhite(line[0])) fWhite=TRUE;

	return(fWhite);
}


BOOL GetMargins(char *token, RECT *rect, int paperNum)
{
	float x0, y0, x1, y1;

	printf("GetMargins() %s\n", token);

	if (sscanf(token, "%f %f %f %f", &x0, &y0, &x1, &y1) != 4) {
		printf("error reading margin data\n");
		exit(1);
	}

	/* paper data is saved at 100 dpi */
	/* map the imageable area to a rect that we will use */

	rect->left   = (int)((x0 * 100.0) / 72);
	rect->top    = (int)(((gPaperSizes[paperNum+1].y - y1) * 100.0) / 72);
	rect->right  = (int)((x1 * 100.0) / 72);
	rect->bottom = (int)(((gPaperSizes[paperNum+1].y - y0) * 100.0) / 72);


	return TRUE;

#if 0
	for(i=0;i<4;i++){
		if((ptr=GetToken(token,tempToken,ptr,'\0'))<0){
			goto ERROR;
		}
		tempMargins[i]=atof(tempToken);
		if(tempMargins[i]==0.0
			&& strcmp(tempToken,"0.00")
			&& strcmp(tempToken,"0.0")
			&& strcmp(tempToken,"0.")
			&& strcmp(tempToken,"0")
		){
			APDError(E_POSSIBLENONNUMBER,gFileName);
			goto ERROR;
		}
		DBMSG2(("GetMargins: num=%d, tM=%10.4f gPS.x=%10.4f gPS.y=%10.4f\n",
			paperNum,tempMargins[i],
			gPaperSizes[paperNum+1].x,gPaperSizes[paperNum+1].y));

		if(i==2) tempMargins[i]=gPaperSizes[paperNum+1].x-tempMargins[i];
		if(i==3) tempMargins[i]=gPaperSizes[paperNum+1].y-tempMargins[i];
		DBMSG2(("GetMargins: tM=%10.4f\n",tempMargins[i]));
		if(tempMargins[i]<0.0){
			APDError(E_NEGATIVEMARGIN,gFileName);
			goto ERROR;
		}
		marginTemp=(int)( ( (tempMargins[i]*100.0) / 72 ) + 0.5 );
/*OLD*	marginTemp=Scale(
 *OLD*		(int)((tempMargins[i]*100.0)+0.5),1,72);
 *OLD*/
		if(i==0 ||i==2){
			if(marginTemp>*marginX) *marginX=marginTemp;
		}
		else if(marginTemp>*marginY) *marginY=marginTemp;

		DBMSG2(("GetMargins:  margin[%d]=%s=%d=%10.4f x=%d,y=%d\n",
			i,tempToken,marginTemp,tempMargins[i],*marginX,*marginY));
	}
	return(TRUE);

ERROR:
	return(FALSE);
#endif

}


char* MyMalloc(int,int);
char* MyMalloc(length,qualifier)
	int length;
	int qualifier;
{
	unsigned int totalLength;

	totalLength=length*qualifier;
	DBMSG((" >MyAlloc(): totalLength=%d\n",totalLength));
	if(!totalLength) totalLength=1;
	DBMSG((" <MyAlloc(): totalLength=%d\n",totalLength));
	return(malloc(totalLength));
}

char* MyRealloc(char*,int,int,BOOL*);
char* MyRealloc(oldPtr,length,qualifier,fNew)
	char *oldPtr;
	int length;
	int qualifier;
	BOOL *fNew;
{
	unsigned int totalLength;
	char *newPtr;

	*fNew=FALSE;
	totalLength=length*qualifier;
	DBMSG((" *MyRealloc(): totalLength=%d\n",totalLength));
	if(!totalLength){
		newPtr=oldPtr;
	}else{
		newPtr=realloc(oldPtr,totalLength);
		*fNew=TRUE;
	}
	return(newPtr);
}


BOOL MyFree(char*);
BOOL MyFree(ptr)
	char *ptr;
{
	if(ptr) free(ptr);
	return(!(!ptr));
}


BOOL BuildString(int,char**,char*);
BOOL BuildString(mode,target,source)
	int mode;
	char **target;
	char *source;
{
	char *oldTarget;
	BOOL fNew=FALSE;

	DBMSG((" >BuildString(): %d,%d,target=%x\n",
		strlen(*target),strlen(source),*target));
	/* if the target string (the one being build) is a NULL
	 * pointer then start the build process
	 */
	if(!mode){
		DBMSG(("  BuildString(): NEW:%s,%s\n",*target,source));
		if(!(*target=MyMalloc(strlen(source)+1,sizeof(char)))) goto ERROR;
		DBMSG(("  BuildString(): NEW:target=%x\n",*target));
		strcpy(*target,source);
		DBMSG(("  BuildString(): NEW:%s,%s\n",*target,source));

	/* else...continue the building process */
	}else{
		DBMSG(("  BuildString(): AGAIN:%s,%s\n",*target,source));
		/* save the present string */
		if(!(oldTarget=MyMalloc(strlen(*target)+1,sizeof(char)))) goto ERROR;
		strcpy(oldTarget,*target);
		DBMSG(("  BuildString(): AGAIN:old=%x,%s\n",oldTarget,oldTarget));

		if(!(*target=MyRealloc(*target,strlen(*target)+strlen(source)+1,
			sizeof(char),&fNew))
		){
			goto ERROR;
		}
		DBMSG(("  BuildString(): AGAIN:target=%x\n",*target));
		/* if the target string newly allocated... */
		if(fNew){
			strcpy(*target,oldTarget);
			strcat(*target,source);
			DBMSG(("  BuildString(): AGAIN: NEW:target=%x,old=%x\n",
				*target,oldTarget));
			DBMSG(("  BuildString(): AGAIN: NEW: %s,%s\n",
				*target,source));
		}
#ifdef DEBUG_ON
		else DBMSG(("  BuildString(): AGAIN: new string is NULL\n"));
#endif
		if(!MyFree(oldTarget)) goto ERROR;
		DBMSG(("  BuildString(): AGAIN:%s,%s\n",*target,source));
	}
	DBMSG((" <BuildString()\n"));
	return(TRUE);

ERROR:
	APDError(E_MEMORY,gFileName);
	return(FALSE);
}

