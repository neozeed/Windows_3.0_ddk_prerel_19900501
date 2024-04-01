/**[f******************************************************************
 * utils.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/***************************************************************************/
/********************************   UTILS   ********************************/
/*
 *   1-13-89	jimmat	Reduced # of redundant strings by adding lclstr.h
 *   1-25-89	jimmat	Use hLibInst instead of calling GetModuleHandle()-
 *			lclstr.h no longer required by this file.
 *   1-30-89	jimmat	Changed MakeAppName to allow names for other app's
 *			to be built (like the Font Installer).
 */


#include "generic.h"
#include "resource.h"
#include "utils.h"


static int reverse(LPSTR);


#define DBGtrace(msg) DBMSG(msg)
#define DBGerr(msg) DBMSG(msg)


#define topofpath(s,lpFileName) \
    for (s = lpFileName + lstrlen(lpFileName); \
        (s > lpFileName) && (s[-1] != ':') && (s[-1] != '\\'); --s)

/*  labdivc
 *
 *  Long (A*B)/C:  this is an auxilary routine used to access the
 *  long arithmetic library functions as a FAR procedure.
 */
long FAR PASCAL labdivc(lval1, lval2, lval3)
long lval1;
long lval2;
long lval3;
    {
    return((lval1 * lval2) / lval3);
    }


/*  lmul
 *
 *  Long multiply:  this an auxilary routine used to access the
 *  long arithmetic library functions as a FAR procedure.
 */
long FAR PASCAL lmul(lval1, lval2)
long lval1;
long lval2;
    {
    return(lval1 * lval2);
    }


/*  ldiv 
 *
 *  Long divide:  this an auxilary routine used to access the
 *  long arithmetic library functions as a FAR procedure.
 */
long FAR PASCAL ldiv(lval1, lval2)
long lval1;
long lval2;
    {
    return(lval1/lval2);
    }


/*  FontMEM
 *
 *  Calculate the  number of bytes per font.
 */
long FAR PASCAL FontMEM(numchars, width, height)
    int numchars;
    long width;
    long height;
    {
    return((long)numchars * (((width + 7) >> 3) * height + 63));
    }

/*  itoa
 *
 *  Convert integer to ascii text.
 */
int FAR PASCAL itoa(n, s)
int n;
LPSTR s;
{
    int i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do          /* generate digits in reverse order */
    {
        s[i++] = (char)(n % 10 + '0');
    } while (n /= 10);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    return i;
}


/*  atoi
 *
 *  Convert ascii to integer.
 */
int FAR PASCAL atoi(s)
LPSTR s;
{
    short n, i;

    for (i = n = 0;; n *= 10)
        {
        n += s[i] - '0';
        if (!s[++i])
            break;
        }
    return n;
}

/*  myWriteSpool
 *
 *  Dump the contents of the epSpool buffer into the output channel.
 */
int FAR PASCAL myWriteSpool(lpDevice)
    register LPDEVICE lpDevice;
    {
    short n;

    DBGtrace(("IN myWriteSpool\n"));

    if (!lpDevice->epPtr || lpDevice->epDoc != TRUE)
        return 0;
    if ((n = WriteSpool(lpDevice->epJob, (LPSTR)lpDevice->epSpool,
            lpDevice->epPtr)) != lpDevice->epPtr)
        lpDevice->epDoc = n;

    lpDevice->epPtr = 0;
    return n;
    }

/*  myWrite
 *
 *  Copy the output string into our spool buffer, if the string overflows
 *  the spool buffer, then dump it first.
 */
int FAR PASCAL myWrite(lpDevice, str, n)
    LPDEVICE lpDevice;
    LPSTR str;                  /*string of character to write*/
    short n;                    /*length of string*/
    {
    LPSTR p;
    register short i;
    register short m;

    #ifdef DBGEscapes
    if (*str == '\033' && n < 20)
        {
        DBGtrace(("IN myWrite "));
        #ifdef DEBUG
        debugStr(str, n);
        #endif
        }
    else {
        DBGtrace(("myWrite(%lp,%lp,%d)\n", lpDevice, str, n));
        }
    #endif

    if (lpDevice->epDoc != TRUE)
        return ERROR;

    p = &lpDevice->epSpool[i = lpDevice->epPtr]; /*end of current spool buf*/

    do {
        if ((i += m = n) >= SPOOL_SIZE)
            {
            if (myWriteSpool(lpDevice) < 0) /*write out spool buffer*/
                return ERROR;

            p = lpDevice->epSpool;

            i = m = (n > SPOOL_SIZE) ? SPOOL_SIZE : n;
            }

        lpDevice->epPtr = i;                /*update spool count*/
        lmemcpy(p, str, (WORD)m);           /*add new characters*/

        n -= m;

        } while (n > 0);

    return SUCCESS;
    }

/*  MakeEscape
 *
 *  Build an escape string.
 */
int FAR PASCAL MakeEscape(lpEsc, start1, start2, end, n)
    lpESC lpEsc;
    char start1, start2, end;
    short n;
    {
    lpEsc->esc = '\033';
    lpEsc->start1 = start1;
    lpEsc->start2 = start2;
    n = itoa(n, lpEsc->num);
    lpEsc->num[n] = end;
    lpEsc->num[n + 1] = '\0';
    return n + 4;
    }


/*  xmoveto
 *
 *  x value is in 300 dpi and is converted to decipoints
 */
int FAR PASCAL xmoveto(lpDevice, x)
    LPDEVICE lpDevice;
    WORD x;
    {
    ESCtype escape;
    int err = SUCCESS;
    DBGtrace(("IN xmoveto,x=%d\n", x));

    if (lpDevice->epCurx != x)
        {
        lpDevice->epCurx = x;

        if (!(lpDevice->epCaps & HPJET))
            err = myWrite(lpDevice, (LPSTR) &escape,
                MakeEscape(&escape, DOT_HCP, x));
        else
            {
            short pos;

            x *= 12;
            lpDevice->epXerr = x % 5;
            pos = x / 5;

            if (lpDevice->epXerr)
                {
                pos++;
                lpDevice->epXerr -= 5;
                }

            err = myWrite(lpDevice, (LPSTR) &escape,
                MakeEscape(&escape, HP_HCP, pos));
            }
        }

    return (err);
    }

/*  ymoveto
 *
 *  y value is in 300dpi and is converted to decipoints
 */
int FAR PASCAL ymoveto(lpDevice, y)
    LPDEVICE lpDevice;
    WORD y;
    {
    ESCtype escape;
    int err = SUCCESS;
    DBGtrace(("IN ymoveto,y=%d\n", y));

    if (lpDevice->epCury != y)
        {
        lpDevice->epCury = y;

        if (!(lpDevice->epCaps & HPJET))
            err=myWrite(lpDevice,(LPSTR) &escape,
                MakeEscape((lpESC)&escape,DOT_VCP,y));
        else
            {
            short pos;

            y *= 12;
            lpDevice->epYerr = y % 5;
            pos = y / 5;

            if (lpDevice->epYerr)
                {
                pos++;
                lpDevice->epYerr -= 5;
                }

            err = myWrite(lpDevice, (LPSTR) &escape, MakeEscape(&escape,
                HP_VCP, pos));
            }
        }

    return (err);
    }

/*  _lopenp
 *
 *  Attempt to open a file at lpFileName.  If that fails, then try to 
 *  append the file name to the same path the driver is on and attempt
 *  to open the file again.
 */
int FAR PASCAL _lopenp(lpFileName, IOflag)
    LPSTR lpFileName;
    WORD IOflag;
    {
    OFSTRUCT ofStruct;
    int hFile;
    extern HANDLE hLibInst;

    if ((hFile = OpenFile(lpFileName, (LPOFSTRUCT)&ofStruct, IOflag)) > 0)
        return (hFile);
    else
        {
        char modFileName[128];
        LPSTR s, t;

	if (!GetModuleFileName(hLibInst,(LPSTR)modFileName,sizeof(modFileName)))
            {
            DBGerr(("_lopenp(): could not get module file name\n"));
            return (0);
            }

        /*  Merge the name of the file with the path of the
         *  driver executable file.
         */
        topofpath(s, ((LPSTR)modFileName));
        topofpath(t, lpFileName);

        *s = '\0';

        /*  Failure if name is too long.
         */
        if ((lstrlen(t) + lstrlen(modFileName)) > sizeof(modFileName))
            {
            DBGerr(("_lopenp(): path name too long, abort\n"));
            return (0);
            }

        /*  Merge path + name.
         */
        lstrcpy(s, t);

        DBGerr(("_lopenp(): could not open %ls, trying %ls\n",
            lpFileName, (LPSTR)modFileName));

        /*  Open file.
         */
        return (OpenFile((LPSTR)modFileName, (LPOFSTRUCT)&ofStruct, IOflag));
        }
    }

/*  MakeAppName
 *
 *  Build the application name that heads up the section for the printer
 *  driver in the win.ini file.
 */
void FAR PASCAL
MakeAppName(lpModNm, lpPortNm, lpAppNm, nmsz)
    LPSTR lpModNm;
    LPSTR lpPortNm;
    LPSTR lpAppNm;
    int nmsz;
    {
    LPSTR s;

#if defined(THE_WAY_IT_WAS)	    /*----------------------------------*/
    /*	If a module handle is passed in, get the file name for that module
     *	--otherwise assume lpAppNm already has the file name.
     */

    if (hMd) {
	GetModuleFileName(hMd, lpAppNm, nmsz);

	/*  Strip off leading path name.
	 */
	for (s = lpAppNm + lstrlen(lpAppNm);
	    (s > lpAppNm) && (s[-1] != ':') && (s[-1] != '\\'); --s)
	    ;

	/*  Shift the name back over the path.
	 */
	lstrcpy(lpAppNm, s);
    }
#else				    /*----------------------------------*/
    lstrcpy(lpAppNm,lpModNm);
#endif				    /*----------------------------------*/

    /*  Strip off the extension to the file name.
     */
    for (s = lpAppNm; *s && (*s != '.');  ++s)
        ;
    *s = '\0';

    if (lstrlen(lpPortNm) + lstrlen(lpAppNm) + 1 < nmsz)
        {
        lstrcat(lpAppNm, (LPSTR)",");
        lstrcat(lpAppNm, lpPortNm);

        /*  Remove colon from end of port name if there is one.
         */
        s = lpAppNm + lstrlen(lpAppNm);
        if (*(--s) == ':')
            *s = '\0';
        }
    }

/***************************************************************************/
/********************************   Local   ********************************/


/*  reverse() -
    s1 points to the end of the string and moves toward the beginning
    s points to the beginning and moves to the end until s >= s1
*/
static int reverse(s)
LPSTR s;
{
    register char c, far *s1;

    s1 = s;
    while (*s1)
        s1++;
    for (--s1; s < s1; s++, s1--)
    {
        c = *s;
        *s = *s1;
        *s1 = c;
    }
    return (0);
}
