/*---------------------------------------------------------------------------*\
| MISC FUNCTIONS                                                              |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : Aug 03, 1989                                                       |
| SEGMEN: _TEXT                                                               |
|                                                                             |
| HISTORY: Aug 03, 1989 - createed.                                           |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>                             /* Windows Header File      */
#include "DispTest.h"                            /* Program Header File      */

LOCALHANDLE FAR AllocTextBuffer(lpBuffer,wSize)
     LPSTR lpBuffer;
     WORD  wSize;
{
     LOCALHANDLE hBuffer;

     if(!(hBuffer = LocalAlloc(LHND,wSize)))
          return(NULL);

     if(!(lpBuffer = LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(NULL);
     }

     return(hBuffer);
}

BOOL FAR FreeTextBuffer(hBuffer)
     LOCALHANDLE hBuffer;
{
     if(!hBuffer)
          return(TRUE);
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     return(TRUE);
}
