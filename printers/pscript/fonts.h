/*
 *
 * fonts.h
 *
 */

BOOL	FAR PASCAL LoadFontDir(int, LPSTR);
LPSTR	FAR PASCAL LockFontDir(int);
void	FAR PASCAL UnlockFontDir(int);
void	FAR PASCAL DeleteFontDir(int);

LPFX	FAR PASCAL LockFont(LPFONTINFO);	/* from charwdth.c */

