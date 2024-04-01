/*---------------------------------------------------------------------------*\
| FILE I/O MODULE                                                             |
|   This module contains routines associated with File I/O.                   |
|                                                                             |
| STRUCTURE (----)                                                            |
|                                                                             |
| FUNCTION EXPORTS METHODS                                                    |
|   ReadFile()                                                                |
|   WriteFile()                                                               |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 16, 1990                                                   |
| SEGMENT: _TEXT                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"

/*---------------------------------------------------------------------------*\
| WRITE FILE                                                                  |
|   This routine writes out a buffer to a file.  This routine allows more     |
|   than 64K segments to be written to the file.                              |
|                                                                             |
|                                                                             |
| RETURNS                                                                     |
|   DWORD - The number of bytes written to the file.                          |
\*---------------------------------------------------------------------------*/
DWORD FAR PASCAL WriteFile(hFile,szBuffer,dwLength)
     int   hFile;
     LPSTR szBuffer;
     DWORD dwLength;
{
     WORD  wTimes,wRemain;
     int   iLoop;
     LPSTR lpBuf;
     DWORD dwBytes;

     dwBytes = 0l;

     /*-----------------------------------------*\
     | Write as many 64K chuncks of memory that  |
     | will go in this buffer.                   |
     \*-----------------------------------------*/
     lpBuf = szBuffer;
     wTimes = (WORD)((DWORD)dwLength >> 16);
     for(iLoop=0; iLoop < wTimes; iLoop++)
     {
          dwBytes += _lwrite(hFile,lpBuf,(WORD)65535);
          lpBuf += 65535l;
     }

     /*-----------------------------------------*\
     | If there are remaining bytes to write,    |
     | then they can be output in this loop.     |
     \*-----------------------------------------*/
     if(wRemain = (WORD)(dwLength & 0x0000FFFF))
          dwBytes += _lwrite(hFile,lpBuf,wRemain);

     return(dwBytes);
}


/*---------------------------------------------------------------------------*\
| READ FILE                                                                   |
|   This routine reads in a file to a buffer.  This routine allows more than  |
|   64K segments to be retrieved from the file.                               |
|                                                                             |
|                                                                             |
| RETURNS                                                                     |
|   DWORD - The number of bytes read from the file.                           |
\*---------------------------------------------------------------------------*/
DWORD FAR PASCAL ReadFile(hFile,szBuffer,dwLength)
     int   hFile;
     LPSTR szBuffer;
     DWORD dwLength;
{
     WORD  wTimes,wRemain;
     int   iLoop;
     LPSTR lpBuf;
     DWORD dwBytes;

     /*-----------------------------------------*\
     | Read as many 64K segments into the buffer.|
     \*-----------------------------------------*/
     lpBuf = szBuffer;
     wTimes = (WORD)((DWORD)dwLength >> 16);
     for(iLoop=0; iLoop < wTimes; iLoop++)
     {
          dwBytes += _lread(hFile,lpBuf,(WORD)65535);
          lpBuf += 65535l;
     }

     /*-----------------------------------------*\
     | Read in the remaining.  Use and logic     |
     | since % won't work on DWORD.              |
     \*-----------------------------------------*/
     if(wRemain = (WORD)(dwLength & 0x0000FFFF))
          dwBytes += _lread(hFile,lpBuf,wRemain);

     return(dwBytes);
}
