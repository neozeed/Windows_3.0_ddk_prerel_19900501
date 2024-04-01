
/*---------------------------- getdata.c ---------------------------------*/

BOOL	FAR PASCAL GetImageRect(PPRINTER, int, LPRECT);
BOOL	FAR PASCAL PaperSupported(PPRINTER, int);
PPRINTER FAR PASCAL GetPrinter(short);

#define FreePrinter(pPrinter)	LocalFree((HANDLE)pPrinter)

LPSTR	FAR PASCAL GetResourceData(LPHANDLE,LPSTR,LPSTR);
BOOL	FAR PASCAL UnGetResourceData(HANDLE);
BOOL	FAR PASCAL DumpPSS(LPDV,short,short,short);
BOOL	FAR PASCAL DumpResourceString(LPDV,short,short);


