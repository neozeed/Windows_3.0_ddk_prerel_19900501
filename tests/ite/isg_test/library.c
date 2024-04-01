/*---------------------------------------------------------------------------*\
| TESTING MODULE                                                              |
|   This is an object oriented library for common routines used by testing    |
|   applications.  It is accompanied with it's own API routines.              |
|                                                                             |
| STRUCTURE (TEST)                                                            |
|                                                                             |
| FUNCTION EXPORTS                                                            |
|   MySRand()                                                                 |
|   MyRand()                                                                  |
|   WEP()                                                                     |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 08, 1990                                                   |
| SEGMENT: _TEXT                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"
#include "global.h"

void FAR PASCAL QuikSort(LPINT,int,int);

#define SWAP_INT(x,y) {int _t; _t=x; x=y; y=_t;}

void FAR PASCAL QuikSort(lpList,iLeft,iRight)
     LPINT lpList;
     int   iLeft;
     int   iRight;
{
     int iMiddle,idl,idr;

     /*-----------------------------------------*\
     | We have achieve convergence.              |
     \*-----------------------------------------*/
     if((iRight - iLeft) < 3)
     {
          if(lpList[iLeft] > lpList[iRight])
               SWAP_INT(lpList[iLeft],lpList[iRight]);
          return;
     }

     iMiddle = lpList[(iRight-iLeft)/2];

     lpList[(iRight-iLeft)/2] = lpList[iLeft];

     idl=iLeft+1; idr=iRight;

     while(idl < idr)
     {
          while(lpList[idl++] < iMiddle);
          while(lpList[idr--] >= iMiddle);
          if(idl < idr)
               SWAP_INT(lpList[idl++],lpList[idr--]);
     }
     lpList[iLeft] = iMiddle;

     if(iLeft < idr)
          QuikSort(lpList,iLeft,idr);
     if(idl < iRight)
          QuikSort(lpList,idl,iRight);
     return;
}

LPSTR lstrtok(lpString,lpDel)
     LPSTR lpString;
     LPSTR lpDel;
{
     unsigned char mDelArray[32];
     register int nIdx;
     static LPSTR lpNextTok;
     LPSTR lpTok;

     /*-----------------------------------------*\
     | Initialize 256 bit buffer to 0.  This     |
     | equates to a 32x8 byte buffer.            |
     \*-----------------------------------------*/
     for(nIdx=0; nIdx < 32; nIdx++)
          mDelArray[nIdx] = 0;

     /*-----------------------------------------*\
     | Fill in bitmap array with 1 flag to mark  |
     | each appearance of a deliminator.         |
     |   - requires translating a 8byte value to |
     |     a bit equivalent.                     |
     \*-----------------------------------------*/
     while(*lpDel)
     {
          mDelArray[*lpDel >> 3] |= (1 << (*lpDel & 0x07));
          lpDel++;
     }

     /*-----------------------------------------*\
     | If we are in the second or more pass,     |
     | then init string to beginning of next tok.|
     \*-----------------------------------------*/
     if(!*lpNextTok)
          return(NULL);
     if(!lpString)
          lpString = lpNextTok;

     /*-----------------------------------------*\
     | Search through string, looking for a bit  |
     | match in the deliminator array.           |
     \*-----------------------------------------*/
     lpTok = lpString;
     while(mDelArray[*lpString >> 3] & (1 << (*lpString & 0x07)) && *lpString)
          lpString++;

     if(*lpString)
     {
          *lpString++ = 0;
     }
     else
     {
          lpNextTok = NULL;
          return(lpTok);
     }

     lpTok = lpNextTok++;

     return(lpTok);
}


/*---------------------------------------------------------------------------*\
| WORD BIT COUNT ROUTINE                                           -chriswil- |
|   This routine counts the number of bits set in a word.                     |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes the variable passed is a valid integer.                           |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return the number of bits set.                                       |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   int - Count of bits set in the word.                                      |
\*---------------------------------------------------------------------------*/
int FAR PASCAL BitCountWord(wWord)
     WORD wWord;
{
     register int nCount;

     nCount=0;
     while(wWord)
     {
          wWord &= (wWord-1);
          nCount++;
     }
     return(nCount);
}


int FAR PASCAL BitCountDWord(dwWord)
     DWORD dwWord;
{
     register int  nCount;
     register WORD wWord;

     nCount=0;

     /*-----------------------------------------*\
     | For efficiency, mask the dword to word.   |
     \*-----------------------------------------*/
     wWord = (WORD)(dwWord >> 16) & 0xFFFF;
     while(wWord)
     {
          wWord &= (wWord-1);
          nCount++;
     }

     wWord = (WORD)dwWord & 0xFFFF;
     while(wWord)
     {
          wWord &= (wWord-1);
          nCount++;
     }
     return(nCount);
}


/*---------------------------------------------------------------------------*\
| LONG SET RANDOM SEED                                             -chriswil- |
|   This routine Sets a randome seed for use by lrand() function.  It uses    |
|   a long pointer to a variable declared in the callers DS/SS.               |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes a valid pointer to a unsigned int location.                       |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return the new seed number requested.                                |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   unsigned int - New seed number.                                           |
\*---------------------------------------------------------------------------*/
unsigned FAR PASCAL lsrand(nSeed,nSeedNum)
     LPINT    nSeed;
     unsigned nSeedNum;
{
     *nSeed = nSeedNum;
     return(*nSeed);
}


/*---------------------------------------------------------------------------*\
| LONG RANDOM NUMBER GENERATOR                                     -chriswil- |
|   This routine generates a Pseudo-random number based on the contents of    |
|   the seed number passed to the function.                                   |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes a valid pointer to a unsigned int location.                       |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return the new seed number requested.                                |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   unsigned int - New seed number.                                           |
\*---------------------------------------------------------------------------*/
unsigned FAR PASCAL lrand(nSeed)
     LPINT nSeed;
{
     *nSeed = (unsigned)((*nSeed * 25173+13849) & 0xFFFF);
     return(*nSeed);
}


/*---------------------------------------------------------------------------*\
| LONG INTEGER TO ASCII                                            -chriswil- |
|   This routine converts an integer to its ascii equivalent.  It converts    |
|   the string (reverse order), then reverses the string to represent the     |
|   correct ordering of digits.                                               |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes a valid buffer which can hold the length of an integer.           |
|   Assumes iBase is a none zero integer.                                     |
|   Assumes iNumb is a valid integer in the range (-32768 to 32767).          |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return a long pointer to string buffer passed to function.           |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   LPSTR - Long pointer to the beginning of the string.                      |
\*---------------------------------------------------------------------------*/
LPSTR FAR PASCAL litoa(iNumb,lpString,iBase)
     int   iNumb;
     LPSTR lpString;
     int   iBase;
{
     register char bDigit;
     register int  iSign;
     LPSTR         lpSave;

     /*-----------------------------------------*\
     | Check for sign.                           |
     \*-----------------------------------------*/
     if(iNumb < 0)
          iSign = -1;
     else
          iSign = 1;

     /*-----------------------------------------*\
     | Convert integer to string (reverse order).|
     \*-----------------------------------------*/
     lpSave = lpString;
     while(iNumb > 0)
     {
          *lpString++ = (char)((iNumb % iBase) + 48);
          iNumb /= iBase;
     }
     *lpString = 0;

     return(ReverseString(lpSave));
}


/*---------------------------------------------------------------------------*\
| LONG ASCII TO INTEGER                                            -chriswil- |
|   This routine returns an integer equivalent of the string passed to this   |
|   function.  The local variables are declared as REGISTER to facilitate     |
|   faster processing of function (optimized).                                |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes valid pointer to string (NULL Terminated).                        |
|   Assumes string is in the range ("-32768" to "[+]32767").                  |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return valid integer (-32768 to 32767).                              |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   int - Integer equivalent of string passed to function.                    |
\*---------------------------------------------------------------------------*/
int FAR PASCAL latoi(lpString)
     LPSTR lpString;
{
     register int iNumb,iSign;

     /*-----------------------------------------*\
     | Is it signed?                             |
     \*-----------------------------------------*/
     if(*lpString == '-')
     {
          iSign = -1;
          lpString++;
     }
     else
     {
          iSign = 1;
          if(*lpString == '+')
               lpString++;
     }

     /*-----------------------------------------*\
     | Convert me!  Show me the way!             |
     \*-----------------------------------------*/
     iNumb = 0;
     while(*lpString)
          iNumb = (iNumb*10) + (*lpString++ - 48);

     return(iNumb*iSign);
}


/*---------------------------------------------------------------------------*\
| REVERSE STRING                                                   -chriswil- |
|   This routine reverses the characters in a string.                         |
|                                                                             |
| PRECONDITION                                                                |
|   Assumes valid pointer to string (NULL Terminated).                        |
|                                                                             |
| POSTCONDITION                                                               |
|   Will return a long pointer to the start of the string.                    |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   LPSTR - Beginning of the text string.                                     |
\*---------------------------------------------------------------------------*/
LPSTR FAR PASCAL ReverseString(lpString)
     LPSTR lpString;
{
     register char cTmp;
     register int  nIdx,nCount;

     nCount = lstrlen(lpString);

     for(nIdx=0; nIdx < (nCount >> 1); nIdx++)
     {
          cTmp = *(lpString+nIdx);
          *(lpString+nIdx) = *(lpString+nCount-nIdx-1);
          *(lpString+nCount-nIdx-1) = cTmp;
     }

     return(lpString);
}

/*---------------------------------------------------------------------------*\
| LIBRARY MAIN ENTRY POINT                                                    |
|   This routine is called from the startup code in the ListDLL.asm routine.  |
|   The instance variable is setup for use throughout the DynaLink.           |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HANDLE hInstance   - Instance handle of library.                          |
|   WORD   wDataSeg    - Data Segment address.                                |
|   WORD   wHeapSize   - Size of local heap.                                  |
|   LPSTR  lpszCmdLine - Command line.                                        |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   HANDLE hLibInst - Instance variable of the library.                       |
|                                                                             |
| RETURNS                                                                     |
|   int - TRUE if everything is OK.                                           |
\*---------------------------------------------------------------------------*/
int FAR PASCAL LibMain(hInstance,wDataSeg,wHeapSize,lpszCmdLine)
   HANDLE hInstance;
   WORD   wDataSeg;
   WORD   wHeapSize;
   LPSTR  lpszCmdLine;
{
     extern HANDLE hLibInst;

     if(hLibInst = hInstance)
          return(TRUE);
     return(FALSE);
}


/*---------------------------------------------------------------------------*\
| WINDOWS EXIT PROCEDURE                                                      |
|   This routine is required for all DLL's.  It provides a means for cleaning |
|   up prior to closing the DLL.                                              |
|                                                                             |
| PRECONDITION                                                                |
|   -none-                                                                    |
|                                                                             |
| POSTCONDITION                                                               |
|   -none-                                                                    |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   -none-                                                                    |
\*---------------------------------------------------------------------------*/
void FAR PASCAL WEP(wParam)
     WORD wParam;
{
}
