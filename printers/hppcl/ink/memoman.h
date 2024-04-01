/**[f******************************************************************
 * memoman.h -
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

#define MINMEM	    50000	/*minimum KB to image a page, don't let
                                    soft fonts use more than this*/
#define MAXSOFTNUM  32		/*maximum number of fonts that may be
                                  downloaded*/
#define MAXSOFTPERPG 16		/*maximum number of fonts that may be used
                                  per page*/

#define CharMEM(i)  ((17*i)>>2)	    /*i= number of characters*/

/*number of bytes per raster line*/
#define RasterMEM(i) (i+10)	    /*i= number of bytes on raster line*/

#ifdef SEG_PHYSICAL
BOOL FAR PASCAL DownLoadSoft(LPDEVICE, short);
void FAR PASCAL UpdateSoftInfo(LPDEVICE, LPFONTSUMMARYHDR, short);
#endif
