/**[f******************************************************************
 * pclstub.c - 
 *
 * Copyright (C) 1988,1989 Aldus Corporation.  
 * Copyright (C) 1989-1990 Microsoft Corporation.
 * All rights reserved.
 * Company confidential.
 *
 **f]*****************************************************************/

/* 
 *
 *  PCLDOS.C - stub routine for HPPCL.DRV to report the version number
 *             if HPPCL.DRV is executed without running windows.
 * 
 *  $Revision:   1.2  $
 *  $Date:   28 Oct 1988 15:35:22  $
 *  $Author:   dar  $
 *
 *  $Log:   J:/source/pcl/src/vcs/pclstub.cv  $
 * 
 *    Rev 1.2   28 Oct 1988 15:35:22   dar
 * Added copyright notice
 * 
 *    Rev 1.1   23 Mar 1988 13:55:10   msd
 * Put in prototype for strlen so this file compiles under warning level 2.
 * 
 *    Rev 1.0   18 Feb 1988 15:11:16   steved
 * Initial revision.
 * 
 */
#include "printer.h"
#include "dosutils.h"
#include "hppcl.h"
#include "resource.h"
#include "pfm.h"
#include "fontpriv.h"
#include "version.h"

int strlen(char *);

#define STDOUT 1

LOCAL void blast(s)
	char *s;
	{
	WORD writ;
	dos_write(STDOUT, s, strlen(s), &writ);
	}

void main()
{
blast("Microsoft PCL/HP LaserJet printer driver\r\n");
blast(VNUM);
blast(VDATE);
blast("\r\n");
blast(
  "Copyright (C) Microsoft Corporation, 1989-1990. All rights reserved.\r\n");
blast(
  "Copyright (C) Aldus Corporation, 1987-1989.  All rights reserved.\r\n");
blast("\nThis file is used by Windows and is not an executable program.\r\n");
blast("\n");
}
