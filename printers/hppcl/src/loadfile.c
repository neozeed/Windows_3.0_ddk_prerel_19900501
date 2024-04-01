/**[f******************************************************************
 * loadfile.c -
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

// history
// 07 aug 89	peterbe		after comment "Copy struct from .pfm file."
//				change code to "sizeofStruct = 0;" (around line
//				141).
// 27 apr 89	peterbe		Tabs at 8 spaces, other format cleanup.

#define DBGloadfile(msg) DBMSG(msg)

/*  loadStructFromFile
 */
static int loadStructFromFile(LPFONTSUMMARYHDR, LPFONTSUMMARY, LPSTR, WORD);
static int loadStructFromFile(lpFontSummary, lpSummary, lpDest, kind)
    LPFONTSUMMARYHDR lpFontSummary;
    LPFONTSUMMARY lpSummary;
    LPSTR lpDest;
    WORD kind;
    {
    int sizeofStruct = 0;
    long seek = 0L;
    char buf[128];
    int hFile;

    DBGloadfile(("loadStructFromFile(%lp,%lp,%lp,%d)\n",
	lpFontSummary, lpSummary, lpDest, (WORD)kind));

    /*  Open .pfm file.
     */
    if (MakeFontSumFNm(lpFontSummary,lpSummary,buf,sizeof(buf),TRUE) &&
	((hFile = _lopenp(buf,OF_READ)) > 0))
	{
	DBGloadfile(("Reading PFM source %ls\n",(LPSTR)buf));
	/*  Initially pick up location and size of width table.
         */
	seek = sizeof(PFMHEADER) - 2;
	sizeofStruct = (lpSummary->dfLastChar -lpSummary->dfFirstChar +2) *2;

	if (kind != FNTLD_WIDTHS)
	    {
	    /*  Move file pointer to after width table in file
             *  (for fixed pitch fonts the table does not exist).
             */
	    if (lpSummary->dfPitchAndFamily & 0x1)
		seek += sizeofStruct;
	    sizeofStruct = 0;				    /* reset to zero */
	    DBGloadfile(("_llseek(hFile,%ld (%ld+%ld),0)\n",
		seek+lpSummary->lPCMOffset,seek,lpSummary->lPCMOffset));
	    _llseek(hFile, seek+lpSummary->lPCMOffset, 0);

	    /*  Read pfmExtension.
             */
	    if (_lread(hFile, buf, sizeof(PFMEXTENSION)) ==
		    sizeof(PFMEXTENSION))
		{
		/*  Locate extended text metrics structure.
                 */
		long tseek = ((LPPFMEXTENSION)buf)->dfExtMetricsOffset;

		/*  Pick up the location of the struct.
                 */
		switch (kind)
		    {
		    case FNTLD_EXTMETRICS:
			seek = tseek;
			sizeofStruct = sizeof(EXTTEXTMETRIC);
			break;
		    case FNTLD_PAIRKERN:
			seek = ((LPPFMEXTENSION)buf)->dfPairKernTable;
			break;
		    case FNTLD_TRACKKERN:
			seek = ((LPPFMEXTENSION)buf)->dfTrackKernTable;
			break;
		    default:
			seek = 0L;
			sizeofStruct = 0;
			break;
		    }

		/*  Pick up the size of the struct.
                 */
		if ((kind != FNTLD_EXTMETRICS) && seek && tseek)
		    {
	    DBGloadfile(("_llseek(hFile,%ld (%ld+%ld),0)\n",
		tseek+lpSummary->lPCMOffset,tseek,lpSummary->lPCMOffset));
		    _llseek(hFile, tseek+lpSummary->lPCMOffset, 0);
		    if (_lread(hFile, buf, sizeof(EXTTEXTMETRIC)) ==
			    sizeof(EXTTEXTMETRIC))
			{
			switch(kind)
			    {
			    case FNTLD_PAIRKERN:
				sizeofStruct = sizeof(KERNPAIR) *
				    ((LPEXTTEXTMETRIC)buf)->emKernPairs;
				break;
			    case FNTLD_TRACKKERN:
				sizeofStruct = sizeof(KERNTRACK) *
				    ((LPEXTTEXTMETRIC)buf)->emKernTracks;
				break;
			    default:
				seek = 0L;
				sizeofStruct = 0;
				break;
			    }
			}
		    else
			{
			DBGloadfile(
		      ("loadStructFromFile(): read extTextMetric *failed*\n"));
			seek = 0L;
			sizeofStruct = 0;
			}
		    }
		}
	    else
		{
		DBGloadfile(
		  ("loadStructFromFile(): read pfmExtenstion *failed*\n"));
		seek = 0L;
		sizeofStruct = 0;
		}
	    }

	if ((seek > 0L) && (sizeofStruct > 0))
	    {
	    DBGloadfile(("_llseek(hFile,%ld (%ld+%ld),0)\n",
		seek+lpSummary->lPCMOffset,seek,lpSummary->lPCMOffset));
	    _llseek(hFile, seek+lpSummary->lPCMOffset, 0);

	    /*  Copy struct from .pfm file.
             */
	    if (_lread(hFile, lpDest, sizeofStruct) != sizeofStruct)
		{
		sizeofStruct = 0;
		}
	    #ifdef DEBUG
	    else
		{
		DBGloadfile(
		("loadStructFromFile(): did *not* successfully read struct\n"));
		}
	    #endif
	    }

	/*  Close file.
         */
	_lclose (hFile);
	}
    #ifdef DEBUG
    else
	{
	DBGloadfile(("loadStructFromFile(): could *not* open .pfm file\n"));
	}
    #endif

    return (kind == FNTLD_PAIRKERN ?
	(sizeofStruct / sizeof(KERNPAIR)) : sizeofStruct);
    }
