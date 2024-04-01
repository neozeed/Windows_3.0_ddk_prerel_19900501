/**[f******************************************************************
 * environ.h - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation
 * Copyright (C) 1988-1990 Microsoft Corporation.
 * All rights reserved.  Company confidential.
 *
 **f]*****************************************************************/

void FAR PASCAL MakeEnvironment(LPPCLDEVMODE, LPSTR, LPSTR, LPSTR);

#ifdef PRTCARTITEMS

#define DEV_NAME_LEN	42

typedef struct {			 
		char devname[DEV_NAME_LEN];
		short availmem;
		char realmem[20];
		short caps;
		short romind;
		short romcount;
		short maxpgsoft;
		short maxsoft;
		short numcart;
		short indlistbox;
		short indpaperlist;
	}PRTINFO;

typedef struct {			 
		char cartname[DEV_NAME_LEN];
		short iPCM;
		short cartind;
		short cartcount;
	}CARTINFO;

BOOL FAR PASCAL GetPrtItem(PRTINFO FAR *, short, HANDLE);

#if defined(CARTS_IN_RESOURCE)	/*--------------------------------------*/

BOOL FAR PASCAL GetCartItem(CARTINFO FAR *, short, HANDLE);

#endif	/*-- defined(CARTS_IN_RESOURCE) --------------------------------*/

#endif
