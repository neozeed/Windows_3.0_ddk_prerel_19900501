/**[f******************************************************************
 * pserrors.c - 
 *
 * Copyright (C) 1988 Aldus Corporation.  All rights reserved.
 * Copyright (C) 1989 Microsoft Corporation.
 * Company confidential.
 *
 **f]*****************************************************************/

#include "pscript.h"
#include "pserrors.h"
#include "debug.h"
#include "psdata.h"

/****************************************************************************/

void FAR PASCAL PSDownloadError(short downloadErrorNum)
{
	short errorNum;

	switch (downloadErrorNum) {
	case 1:
		errorNum=PS_DOWNLOAD;
		break;

	case 2:
		errorNum=PS_PORT;
		break;

	case 3:
		errorNum=PS_CORRUPT;
		break;

	default:
		errorNum=PS_GENERAL;
		break;
	}
	PSError(errorNum);
}


/****************************************************************************/

void FAR PASCAL PSError(short errorNum)
{
	char	caption[24];
	char	message[80];
	int	idsCap;
	int	idsMsg;

	DBMSG((">PSError(): %d, hInst=%d\n",errorNum,hInst));

	switch (errorNum) {
	case PS_CORRUPT:
		idsCap = IDS_ERROR_CAPTION_GENERAL;
		idsMsg = IDS_ERROR_MESSAGE_CORRUPT;
		break;

	case PS_COPIES0:
		idsCap = IDS_ERROR_CAPTION_DATA;
		idsMsg = IDS_ERROR_MESSAGE_COPIES;
		break;

	case PS_JOBTIMEOUT0:
		idsCap = IDS_ERROR_CAPTION_DATA;
		idsMsg = IDS_ERROR_MESSAGE_JOBTIMEOUT;
		break;

	case PS_DOWNLOAD:
		idsCap = IDS_ERROR_CAPTION_GENERAL;
		idsMsg = IDS_ERROR_MESSAGE_DOWNLOAD;
		break;

	case PS_PORT:
		idsCap = IDS_ERROR_CAPTION_GENERAL;
		idsMsg = IDS_ERROR_MESSAGE_PORT;
		break;

	case PS_AT:
		idsCap = IDS_ERROR_CAPTION_GENERAL;
		idsMsg = IDS_ERROR_MESSAGE_AT;
		break;

	default:
		idsCap = IDS_ERROR_CAPTION_GENERAL;
		idsMsg = IDS_ERROR_MESSAGE_GENERAL;
		break;
	}
	LoadString(ghInst, idsCap, caption, sizeof(caption));
	LoadString(ghInst, idsMsg, message, sizeof(message));

	MessageBox(GetActiveWindow(), message, caption, MB_OK | MB_ICONEXCLAMATION);

	DBMSG(("<PSError()\n"));
}

