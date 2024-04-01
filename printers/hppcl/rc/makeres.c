/**[f******************************************************************
 * makeres.c -
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 *  All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

#include "printer.h"
#include "gdidefs.inc"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "hppcl.h"
#include "pfm.h"
#include "trans.h"
#define PAPER_DATA
#include "paperfmt.h"


main()
    {
    FILE *fp;
    char fname[32];
    BYTE *fdata;
    int fsize;
    int loop;

    for (loop = 0; loop < 6; ++loop)
	{
	switch (loop)
	    {
	    case 0:
		strcpy(fname, "USASCII.tbl");
		fdata = USASCII_Trans;
		fsize = sizeof(USASCII_Trans);
		break;
	    case 1:
		strcpy(fname, "Roman8.tbl");
		fdata = Roman8_Trans;
		fsize = sizeof(Roman8_Trans);
		break;
	    case 2:
		strcpy(fname, "GENERIC7.tbl");
		fdata = GENERIC7_Trans;
		fsize = sizeof(GENERIC7_Trans);
		break;
	    case 3:
		strcpy(fname, "GENERIC8.tbl");
		fdata = GENERIC8_Trans;
		fsize = sizeof(GENERIC8_Trans);
		break;
	    case 4:
		strcpy(fname, "ECMA94.tbl");
		fdata = ECMA94_Trans;
		fsize = sizeof(ECMA94_Trans);
		break;
	    case 5:
		strcpy(fname, "math8.tbl");
		fdata = MATH8_Trans;
		fsize = sizeof(MATH8_Trans);
	    }

	if (fp = fopen(fname, "wb"))
	    {
	    fprintf(stderr, "MAKERES: building %s\n", fname);
	    if (fwrite(fdata, 1, fsize, fp) != fsize)
		{
		fprintf(stderr, "MAKERES: ***failed to write %s\n", fname);
		exit(1);
		}
	    fclose(fp);
	    }
	else
	    {
	    fprintf(stderr, "MAKERES: ***failed to open %s\n", fname);
	    exit(1);
	    }
	}

    if (fp = fopen("paperfmt.bin", "wb"))
	{
	PAPERHEAD PaperHead;

	fprintf(stderr, "MAKERES: building paperfmt.bin\n");

	PaperHead.numLists = sizeof(PaperList) / sizeof(PAPERLIST);
	PaperHead.numFormats = sizeof(PaperFormat) / sizeof(PAPERFORMAT);
	PaperHead.offsLists = sizeof(PaperHead);
	PaperHead.offsFormats = PaperHead.offsLists + sizeof(PaperList);

	if (fwrite(&PaperHead,1,sizeof(PaperHead),fp) != sizeof(PaperHead) ||
	    fwrite(PaperList,1,sizeof(PaperList),fp) != sizeof(PaperList) ||
	    fwrite(PaperFormat,1,sizeof(PaperFormat),fp) != sizeof(PaperFormat))
	    {
	    fprintf(stderr, "MAKERES: ***failed to write paperfmt.bin\n");
	    exit(1);
	    }
	fclose(fp);
	}
    else
	{
	fprintf(stderr, "MAKERES: ***failed to open %s\n", fname);
	exit(1);
	}

    return (0);
    }
