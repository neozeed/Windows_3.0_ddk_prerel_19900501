/*---------------------------------------------------------------------------*\
| LOGFILE MODULE                                                              |
|   This module contains routines associated with Logging of debug info.      |
|                                                                             |
| STRUCTURE (----)                                                            |
|                                                                             |
| FUNCTION EXPORTS METHODS                                                    |
|   CreateLogFile()                                                           |
|   WriteLogFile()                                                            |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 16, 1990                                                   |
| SEGMENT: _FILEIO                                                            |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"

int FAR PASCAL CreateLogFile(szFile)
     LPSTR szFile;
{
     int hFile;
     if(!(hFile = _lcreat(szFile,0)))
          return(-1);
     _lclose(hFile);
     return(0);
}


int FAR PASCAL WriteLogFile(szFile,szLine)
     LPSTR szFile;
     LPSTR szLine;
{
     int    hFile,iLineSize;
     LONG   lFile;
     HANDLE hBuffer,hTempBuffer;
     LPSTR  lpBuffer,lpTempBuffer;
     DATETIME dt;

     if((hFile = _lopen(szFile,OF_READWRITE)) < 0)
     {
          if((hFile = _lcreat(szFile,0)) < 0)
               return(-1);
     }
     lFile = _llseek(hFile,0l,2);

     iLineSize = lstrlen(szLine);
     if(!(hBuffer = LocalAlloc(LHND,(WORD)iLineSize+80)))
     {
          _lclose(hFile);
          return(-1);
     }
     if(!(lpBuffer = LocalLock(hBuffer)))
     {
          _lclose(hFile);
          LocalFree(hBuffer);
          return(-1);
     }
     if(!(hTempBuffer = LocalAlloc(LHND,(WORD)80)))
     {
          _lclose(hFile);
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          return(-1);
     }
     if(!(lpTempBuffer = LocalLock(hTempBuffer)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalFree(hTempBuffer);
          return(-1);
     }

     GetSystemDateTime(&dt);
     wsprintf(lpTempBuffer,(LPSTR)" - (%d:%d:%d)\015\012",dt.bHours,dt.bMinutes,dt.bSeconds);
     lstrcpy(lpBuffer,szLine);
     lstrcat(lpBuffer,lpTempBuffer);
     _lwrite(hFile,lpBuffer,lstrlen(lpBuffer));

     _lclose(hFile);
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     LocalUnlock(hTempBuffer);
     LocalFree(hTempBuffer);

     return(0);
}
