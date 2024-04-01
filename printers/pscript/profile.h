/*
 * profile.h
 *
 */

short FAR PASCAL GetDefaultPaper(void);

BOOL FAR PASCAL MakeEnvironment(LPSTR, LPSTR, LPPSDEVMODE, LPSTR);
void FAR PASCAL SaveEnvironment(LPSTR, LPSTR, LPPSDEVMODE, LPPSDEVMODE, LPSTR, BOOL, BOOL);

int	FAR PASCAL GetExternPrinter(int i);
int	FAR PASCAL MatchPrinter(LPSTR lpName);
