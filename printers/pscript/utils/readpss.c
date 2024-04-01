#include "..\win.h"
#include <stdio.h>
#include <string.h>

#include "..\printcap.h"

#define MAXLEN 1024

#define NULL 0
#define TRUE 1
#define FALSE 0

/****************************************************************************/
/*#define DEBUG_ON*/
#ifdef DEBUG_ON
#define DBMSG(msg) printf msg
#else
#define DBMSG(msg)
#endif
/****************************************************************************/

BOOL freadInt(FILE*, int*);
BOOL freadInt(fp, num)
FILE *fp;
int	*num;
{
	DBMSG((">freadInt()\n"));
	if (fread((char * )num, sizeof(int), 1, fp) != 1) 
		goto ERROR;
	DBMSG(("<freadInt(): num=%d\n", *num));
	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL freadLong(FILE*, long*);
BOOL freadLong(fp, offset)
FILE *fp;
long	*offset;
{
	DBMSG((">freadLong()\n"));
	if (fread((char * )offset, sizeof(long), 1, fp) != 1) 
		goto ERROR;
	DBMSG(("<freadLong(): offset=%ld\n", *offset));
	return(TRUE);

ERROR:
	return(FALSE);
}


BOOL ReadString(FILE*, long, char*, int*);
BOOL ReadString(fp, offset, string, len)
FILE *fp;
long	offset;
char	*string;
int	*len;
{
	int	numRead;

	DBMSG((">ReadString(): offset=%ld\n", offset));
	if (fseek(fp, offset, 0)) 
		goto ERROR;
	if (!freadInt(fp, len)) 
		goto ERROR;
	if (*len >= MAXLEN - 1) 
		*len = MAXLEN - 1;
	if ((numRead = fread(string, sizeof(char), *len, fp)) != *len) {
		string[numRead] = '\0';
		DBMSG(("ERROR: numRead=%d,%s\n", numRead, string));
		goto ERROR;
	}
	string[*len] = '\0';
	DBMSG(("<ReadString(): len=%d,string=%s\n", *len, string));

	return(TRUE);

ERROR:
	return(FALSE);
}

/*
 * PSS file
 *
 *	6 DWORDS	first 4 are offsets to directories of DWORDS
 *			last point to PS data
 *	4 directories lenghts depend on the number of feeders
 *	and number of papers.  these directories are DWORD offsets
 *	from the beginning of the file to the PS data.
 *
 *	PS data is first a WORD string length and then a null terminated
 *	string.
 *
 */


main(argc, argv)
int	argc;
char	**argv;
{
	char	fileName[13];
	char	string[MAXLEN];
	long	offset;
	long	listOffset;
	int	num[4];
	int	i;
	int	j;
	int	len;
	int	numRead;
	FILE * fp;

	if (argc != 2) {
		printf("USAGE: APD <file.pss>\n");
		exit(1);
	}
	++argv;
	strncpy(fileName, *argv, sizeof(fileName));

	/* these are the sizes of the first 4 PSS directories */

	num[0] = num[1] = NUMPAPERS;
	num[2] = 2;
	num[3] = NUMFEEDS;

	printf("file=%s\n", fileName);
	if (!(fp = fopen(fileName, "rb"))) {
		printf("can't open file");
		exit(1);
	}

	/* do first 4.  they all have a secondary directory */

	for (i = 0; i < 4; i++) {
		DBMSG(("\naccessing the PSS directory, entry #%d\n", i));
		/* access the PSS directory */
		if (fseek(fp, 0L, 0)) 
			goto ERROR;
		if (fseek(fp, (long)(i * sizeof(long)), 0)) 
			goto ERROR;
		if (!freadLong(fp, &offset)) 
			goto ERROR;
		if (!offset) 
			continue;

		listOffset = offset;
		for (j = 0; j < num[i]; j++) {
			DBMSG(("accessing list[%d], entry #%d\n", i, j));
			if (fseek(fp, listOffset + (long)(j * sizeof(long)), 0)) 
				goto ERROR;
			if (!freadLong(fp, &offset)) 
				goto ERROR;
			if (!offset) 
				continue;

			if (!ReadString(fp, offset, string, &len)) 
				goto ERROR;
			printf("[%d][%d](%d)%s\n", i, j, len, string);
		}
	}

	/* do the the last 2.  they point directly to the PS data */

	for (i = 4; i < 6; i++) {
		/* access the PSS directory */
		DBMSG(("\naccessing the PSS directory, entry #%d\n", i));
		if (fseek(fp, 0L, 0)) 
			goto ERROR;
		if (fseek(fp, (long)(i * sizeof(long)), 0)) 
			goto ERROR;
		if (!freadLong(fp, &offset)) 
			goto ERROR;
		if (!offset) 
			continue;

		if (!ReadString(fp, offset, string, &len)) 
			goto ERROR;
		printf("[%d](%d)%s\n", i, len, string);
	}
	if (fclose(fp) == EOF) 
		goto ERROR;

	exit(0);

ERROR:
	printf("invalid PSS file.\n");
	exit(1);

	/* dummy return for -W2 */
	return(0);
}


