/*
 * utils.h	defs of utility functions found in utils.c
 *
 */


int	FAR PASCAL Scale(int, int, int);	/* scale.asm */

long FAR PASCAL lmul(long, long);
long FAR PASCAL ldiv(long, long);
long FAR PASCAL lmod(long, long);

void FAR PASCAL lstrncat   (LPSTR, LPSTR, int);
void FAR PASCAL lmemcpy    (void FAR *, void FAR *, int);
BOOL FAR PASCAL lmemIsEqual(void FAR *, void FAR *, int);

void FAR PASCAL ClipBox(LPDV, LPRECT);

void FAR PASCAL lsfpfmcopy(LPSTR, LPSTR);      	/*** rb BitStream ***/
void FAR PASCAL lsfloadpathcopy(LPSTR, LPSTR);	/*** rb BitStream ***/
void FAR PASCAL lbitmapext(LPSTR);		/*** rb BitStream ***/
void FAR PASCAL GetProfileStringFromResource(short, short, short, short, LPSTR, short);
BOOL FAR PASCAL isUSA(void);

LPSTR FAR PASCAL SetKey(LPSTR lpKey, LPSTR lpFile);
