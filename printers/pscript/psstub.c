/**[f******************************************************************
 * psstub.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

/* 
 *  PSSTUB.C - stub routine for PSCRIPT.DRV to report the version number
 *             if PSCRIPT.DRV is executed without running windows.
 * 
 *  $Revision:  1.1  $
 *  $Date:	21 Oct 1988 16:17:24  $
 *  $Author:	dar  $
 *
 *  $Log:   J:/source/pscript/inc/vcs/psstub.cv  $
 * 
 *    Rev 1.1   21 Oct 1988 16:17:24   dar
 *  Added
 * 
 *    Rev 1.0   19 Feb 1988 14:52:42   steved
 *  Initial revision.
 * 
 */
#include "stub.h"
#include "version.h"

LOCAL void blast(s)
	char *s;
	{
	WORD writ;
	dos_write(STDERR, s, strlen(s), &writ);
	}

void main()
{
    blast("Postscript Printer driver\r\n");
    blast(VERSION);
    blast("\r\n");
    blast("Copyright (C) Microsoft Corporation, 1986. All rights reserved.\r\n");
    blast("Copyright (C) Aldus Corporation, 1987-1988.  All rights reserved.\r\n");
    blast("\nThis file is used by Windows and is not an executable program.\r\n");
    blast("\n");
}
