#include "devmode.h"

/*
 * these are the routines that PSCRIPT.DRV exports
 *
 */

BOOL	FAR PASCAL BitBlt(LPPDEVICE, int, int, LPBITMAP, int, int, int, int, long, LPBR, LPDRAWMODE);
LONG	FAR PASCAL StrBlt(LPPDEVICE, int, int, LPRECT, LPSTR, int, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM);
int	FAR PASCAL Output(LPPDEVICE, int, int, LPPOINT, LPPEN, LPBR, LPDRAWMODE, LPRECT);
int	FAR PASCAL RealizeObject(LPPDEVICE, int, LPSTR, LPSTR, LPTEXTXFORM);
int	FAR PASCAL Disable(LPPDEVICE);
int	FAR PASCAL Enable(LPPDEVICE, WORD, LPSTR, LPSTR, LPPSDEVMODE);
RGB	FAR PASCAL ColorInfo(LPPDEVICE, RGB, LPCO);
int	FAR PASCAL EnumDFonts(LPPDEVICE, LPSTR, FARPROC, LPSTR);
int	FAR PASCAL Control(LPPDEVICE, int, LPSTR, LPSTR);
BOOL	FAR PASCAL EnumObj(LPPDEVICE, int, FARPROC, LPSTR);
CO	FAR PASCAL Pixel(LPPDEVICE, int, int, CO, LPDRAWMODE);
int	FAR PASCAL ScanLR(LPPDEVICE, int, int, CO, int);
int	FAR PASCAL DeviceBitmap(LPPDEVICE, int, LPBITMAP, LPSTR);
int	FAR PASCAL FastBorder(LPRECT, WORD, WORD, DWORD, LPPDEVICE, LPSTR, LPDRAWMODE, LPRECT);
int	FAR PASCAL SetAttribute(LPPDEVICE, int, int, int);
DWORD	FAR PASCAL ExtTextOut(LPPDEVICE, int, int, LPRECT, LPSTR, int, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM, short FAR *, LPRECT, WORD);
int	FAR PASCAL DeviceMode(HWND, HANDLE, LPSTR, LPSTR);
short	FAR PASCAL GetCharWidth(LPPDEVICE, LPSHORT, BYTE, BYTE, LPFONTINFO, LPDRAWMODE, LPTEXTXFORM);


/* DIB routines */

BOOL	FAR PASCAL CreateBitmap(void);
int	FAR PASCAL DeviceBitmapBits(LPPDEVICE, WORD, WORD, WORD, LPSTR, LPSTR);
int	FAR PASCAL DIBToDevice(LPPDEVICE, WORD, WORD, WORD, WORD, LPRECT, LPDRAWMODE, LPSTR,
			       LPBITMAPINFO, LPSTR);


/* Device Initilization routines */

DWORD	FAR PASCAL DeviceCapabilities(LPSTR, LPSTR, WORD, LPSTR, LPPSDEVMODE);
short	FAR PASCAL ExtDeviceMode(HWND, HANDLE, LPPSDEVMODE, LPSTR, LPSTR,
				 LPPSDEVMODE, LPSTR, WORD);


int FAR PASCAL StretchBlt(
	LPPDEVICE lpdv,
	int DstX, int DstY, int DstXE, int DstYE,
	LPBITMAP lpbm,
	int SrcX, int SrcY, int SrcXE, int SrcYE,
	DWORD rop, LPBR lpbr, LPDRAWMODE lpdm,
	LPRECT lpClip);
