/*
 *  RESET.C
 *
 *  This files contains functions called by GDI when a printer DC is
 *  created or deleted.
 *
 */

#include "generic.h"
#include "resource.h"
#include "drivinit.h"

#ifndef DEVICEEXTRA
#define DEVICEEXTRA 0
#endif

#define DMPAPER_FANFOLD DMPAPER_USER+1

/*   Needed by the color libraries for CS aliasing in BitBlt and Output
 */
#if COLOR
WORD cSelector=0;
WORD ScratchSelector=0;

WORD FAR PASCAL AllocSelector(WORD);
WORD FAR PASCAL FreeSelector(WORD);
#endif

#ifndef BUF_SLOP	 /* i know, and i dont care */
#define BUF_SLOP(x) 0
#endif
#ifndef BAND_SLOP
#define BAND_SLOP(x) 0
#endif

/* things in other modules
 */
WORD FAR PASCAL ExtDeviceMode(HWND,HANDLE,LPDEVMODE,
			      LPSTR,LPSTR,LPDEVMODE,LPSTR,WORD);

/* forward declare things in this file
 */
void NEAR PASCAL SetDeviceMode(LPDEVICE, DEVMODE far *);
short NEAR PASCAL atoi(LPSTR);
BOOL FAR PASCAL isUSA();
short NEAR BuffInit(LPDEVICE);
short NEAR BuffFree(LPDEVICE);

/* externally defined structures used for setting up dimension information
 * (GDIINFO structures) during enable.
 */
extern  DEVICEHDR port_device;
extern  DEVICEHDR land_device;

#if DEVMODE_WIDEPAPER			     
extern  DEVICEHDR wide_port_device;
extern  DEVICEHDR wide_land_device;
#endif

extern  char land_infobase[];
extern  char port_infobase[];

#if DEVMODE_WIDEPAPER			     
extern  char wide_land_infobase[];
extern  char wide_port_infobase[];
#endif


/***************************************************************
 *  BuffInit() -
 *
 *  Sets up buffers for printing at Enable() (CreateDC()) time
 */

short NEAR BuffInit(lpDevice)
LPDEVICE lpDevice;
{    
    /* Assume we know band widths.
     * Allocate buffers for spooler and bitmaps:
     */
    if (lpDevice->epHBmp = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
	    (long) (BAND_SIZE(lpDevice->epPageWidth)
		   + BAND_SLOP(lpDevice->epPageWidth)) ))
      {
	if (lpDevice->epHBuf = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
	   (long) (BUF_SIZE(lpDevice->epPageWidth)
		  + BUF_SLOP(lpDevice->epPageWidth)) ))
	  {
	    if (lpDevice->epHSpool = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
	       (long) SPOOL_SIZE(lpDevice->epPageWidth)))
	      {
		/* all allocations worked
		 */
		goto no_deAllocations;
	      }
	    else
		goto dealloc1; /* get rid of one allocation */
	  }
	else
	    goto dealloc2;  /* get rid of two allocations */
      }

dealloc2:
	GlobalFree(lpDevice->epHBuf);
dealloc1:			
	GlobalFree(lpDevice->epHBmp);

    /* nullify handles and pointers;
     * return error to indicate insufficient memory
     */
    lpDevice->epHSpool = lpDevice->epHBuf = lpDevice->epHBmp = 0;
    lpDevice->epBmp = lpDevice->epBuf = lpDevice->epSpool =
	    (LPSTR) NULL;

    lpDevice->epBuffSet = FALSE;

    return(FALSE);
	
no_deAllocations:

    /* lock down the memory for the bands --> returns long pointers to memory
     * No way this could fail; Kernel returns [<Handle>:0000] as pointer and
     * sets lock count
     */
    lpDevice->epBmp = (LPSTR) GlobalLock(lpDevice->epHBmp);
    lpDevice->epBuf = (LPSTR) GlobalLock(lpDevice->epHBuf);
    lpDevice->epSpool = (LPSTR) GlobalLock(lpDevice->epHSpool);
	
    /* buffer memory is all set up.
    */
    lpDevice->epBuffSet = TRUE;
    return(TRUE);
}


/****************************************************************
 *  BuffFree() -
 *
 *  Free up the driver's various buffers at Disable() time
 */

short NEAR BuffFree(lpDevice)
LPDEVICE lpDevice;
{
/* 
 *  Now unlock & free bitmap & buffer memory.
 */

    if (lpDevice->epBuffSet)
    {
	    GlobalUnlock(lpDevice->epHBmp);
	    GlobalUnlock(lpDevice->epHBuf);		
   	    GlobalUnlock(lpDevice->epHSpool);		
		
	    GlobalFree(lpDevice->epHBmp);
	    GlobalFree(lpDevice->epHBuf);	
 	    GlobalFree(lpDevice->epHSpool);	
	    lpDevice->epHSpool = lpDevice->epHBuf = lpDevice->epHBmp = 0; 
	    lpDevice->epBmp = lpDevice->epBuf = lpDevice->epSpool =
		    (LPSTR) NULL;
	    
	    lpDevice->epBuffSet = FALSE;
    }
}


/*************************************************************
 *  Enable() -
 *
 *  This function is called by GDI twice when creating a device context
 *  for this printer, once to generate device independant dimension
 *  and capabilities information (GDIINFO) and once to initialize
 *  device specific state information (PDEVICE).
 */

WORD FAR PASCAL Enable(
    LPDEVICE	lpDevice,
    short	style,
    LPSTR	lpDeviceType,
    LPSTR	lpOutputFile,
    LPDEVMODE	lpStuff)
{
    LPDEVMODE lpDM;
    HANDLE hDM;

    hDM = GlobalAlloc(GMEM_FIXED,sizeof(DEVMODE)+DEVICEEXTRA);
    if (!hDM)
	return NULL;
    lpDM = (LPDEVMODE)MAKELONG(0,hDM);

    /* use extdevicemode to validate input device mode, or create
     * a new one if none passed in
     */
    ExtDeviceMode(NULL,NULL,lpDM,
	lpDeviceType,lpOutputFile,lpStuff,NULL,DM_COPY|DM_MODIFY);

    if (style & InquireInfo)
      {
	/* GDI wants a GDIINFO structure...
	 * figure out what the paper size is and copy the appropriate
	 * one
	 */
#if DEVMODE_WIDEPAPER
	if (lpDM->dmPaperWidth > 3000)
	    Copy((LPSTR)lpDevice,
		 (lpDM->dmOrientation == DMORIENT_LANDSCAPE
			    ? (LPSTR)wide_land_infobase
			    : (LPSTR)wide_port_infobase),
		 sizeof(GDIINFO));

	else
#endif
	    Copy((LPSTR)lpDevice,
		 (lpDM->dmOrientation == DMORIENT_LANDSCAPE
			    ? (LPSTR)land_infobase
			    : (LPSTR)port_infobase),
		 sizeof(GDIINFO));

	/* Tell GDI how big the lpDevice needs to be
	 */
	((GDIINFO far *)lpDevice)->dpDEVICEsize = sizeof(DEVICE);

	/* return the size of the GDIINFO structure we created
	 * (used as part of error/version checking)
	 */
	GlobalFree(hDM);
	return sizeof(GDIINFO);
      }

    /* GDI wants our PDEVICE
     */

    /* initialize various thangs
     */
    lpDevice->epHSpool = lpDevice->epHBuf = lpDevice->epHBmp = 0;
    lpDevice->epBmp = lpDevice->epBuf = lpDevice->epSpool = (LPSTR)NULL;
    lpDevice->epBuffSet = FALSE;  /* have not set up buffers */

#if DEVMODE_WIDEPAPER		     
    if (lpDM->dmPaperWidth > 3000)
      {
	lpDevice->epPageWidth = WIDE_PG_ACROSS;
	Copy((LPSTR)lpDevice, (lpDM->dmOrientation == DMORIENT_LANDSCAPE ?
		(LPSTR)&wide_land_device : (LPSTR)&wide_port_device),
		sizeof(DEVICEHDR));
      }
    else
#endif
      {
	lpDevice->epPageWidth = PG_ACROSS;
	Copy((LPSTR)lpDevice, (lpDM->dmOrientation == DMORIENT_LANDSCAPE ?
		(LPSTR)&land_device : (LPSTR)&port_device),
		sizeof(DEVICEHDR));
      }

    /* set up the other settings (why is this separate???)
     */
    SetDeviceMode(lpDevice, lpDM);
    lpDevice->epJob = PQERROR;
    lpDevice->epXcurwidth = CHARWIDTH;

#if defined(SPECIALDEVICECNT)
    FindDeviceMode(lpDevice, lpDeviceType);
#endif

    /* "if style is 0, initialize support module and graphics peripheral for
     * use by GDI routines" (do necessary heap allocations)
     */
    if (!(style & InfoContext))   /* Initialization for a DC */
      {
	lstrcpy(lpDevice->epPort, lpOutputFile);

	if (BuffInit(lpDevice) == FALSE)
	    goto NoCigar;

	if (lpDevice->epType == DEV_PORT)
	  {
	     if (heapinit(lpDevice))
	       {
		 if (!(lpDevice->epYPQ = CreatePQ(Y_INIT_ENT)))
		   {
		     GlobalFree(lpDevice->epHeap);
		     goto NoCigar2;
		   }
	       }
	     else
	       {
NoCigar2:
		 BuffFree(lpDevice);  /* get rid of buffers */
NoCigar:
		 GlobalFree(hDM);
		 return FALSE;
	       }
	   }
	}
    else
	lpDevice->epMode |= INFO;

#ifdef COLOR
    if (!cSelector++)
	ScratchSelector = AllocSelector(0);
#endif

    GlobalFree(hDM);
    return TRUE;
}


/**********************************************************
 *  Disable() -
 *
 *  Called by GDI when the application deletes the printer's DC.
 *  The driver frees up all the crap it allocated during Enable().
 */

WORD FAR PASCAL Disable(lpDevice)
LPDEVICE lpDevice;
{
    if (lpDevice->epBuffSet == TRUE)
	BuffFree(lpDevice);  /* discard print buffers (if they exist) */

    if (lpDevice->epType == DEV_PORT)
      {
	DeletePQ(lpDevice->epYPQ);
	GlobalFree(lpDevice->epHeap);
      }

#ifdef COLOR
    if (!--cSelector)
	FreeSelector(ScratchSelector);
#endif

    return TRUE;
}

/**********************************************************
 *  SetDeviceMode() -
 *
 *  Set up a PDEVICE structure according to the settings in
 *  a DEVMODE structure.
 */

void NEAR PASCAL SetDeviceMode(lpDevice, lpStuff)
LPDEVICE lpDevice;
DEVMODE far *lpStuff;
{
    WORD iPaper;

    switch (lpStuff->dmPaperSize)
      {
    case DMPAPER_A4:
	iPaper = DINA4;
	break;
    case DMPAPER_FANFOLD:
	iPaper = FANFOLD;
	break;
    default:
	iPaper = LETTER;
	break;
      }

    /* if you had any device modes set it up here --
	store the information in lpDevice */
    /* high quality means low speed and low quality will get you
       high speed */
    if (lpStuff->dmPrintQuality == DMRES_DRAFT)
	    lpDevice->epMode |= HIGHSPEED;

    lpDevice->epPF =
	&PaperFormat[iPaper - STANDARDPF +
#if DEVMODE_WIDEPAPER		     
	(lpStuff->dmPaperWidth > 3000 ? NSMALLPAPERFORMATS: 0) +
#endif
	(lpStuff->dmOrientation == DMORIENT_LANDSCAPE ? MAXPAPERFORMAT: 0)];

#if COLOR
    if (lpStuff->dmColor == DMCOLOR_MONOCHROME)
	lpDevice->epBmpHdr.bmPlanes = 1;

#ifdef IBMCOLOR
    /* get the ribbon type
     */
    lpDevice->epRibbon = *(LPINT)(lpStuff+1);
#endif
#endif
}

#if defined(SPECIALDEVICECNT)

/*********************************************************
 *  FindDeviceMode() -
 *
 *  If the driver supports more than one device, switched according
 *  to the input device name, figure out which.
 */

void NEAR PASCAL FindDeviceMode(lpDevice, lpDeviceType)
LPDEVICE lpDevice;
LPSTR lpDeviceType;
{
    register i;

    for (i = 0;i < SPECIALDEVICECNT; i++)
	if (!lstrcmpi((LPSTR)lpDeviceType, DeviceSpec[i].DeviceName))
	  {
	    lpDevice->epMode |= DeviceSpec[i].flag;
	    break;
	  }
}
#endif

/*******************************************
 *  IsUSA() -
 *
 *  Returns TRUE if the machine is set up for the United States
 *  (ie, default paper is US Letter) or FALSE if the machine one of
 *  those subversive foreign contraptions, which case we'll default
 *  to that infidel DIN A4 paper.
 */

BOOL FAR PASCAL isUSA()
{
        BYTE buf[LINE_LEN];

        if (GetProfileString((LPSTR)"intl", (LPSTR)"icountry", (LPSTR)"",(LPSTR) buf, LINE_LEN))
                /* 1 if the USA country code */
                if (atoi((LPSTR) buf) != USA_COUNTRYCODE)
                        return FALSE;
        return TRUE;
}

/******************************************************
 *  atoi() -
 */

short NEAR PASCAL atoi(s)
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
