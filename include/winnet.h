
/*
 *	Windows/Network Interface
 *	Copyright (C) Microsoft 1989
 *
 *	Standard WINNET Driver Header File, spec version 0.59
 */


typedef WORD far * LPWORD;


/*
 *	SPOOLING - CONTROLLING JOBS
 */

#define WNJ_NULL_JOBID  0


WORD FAR PASCAL WNetOpenJob 	      ( LPSTR		szQueue,
					LPSTR		szJobTitle,
					WORD		nCopies,
					LPWORD		pfh		);

WORD FAR PASCAL WNetCloseJob 	      ( WORD		fh,
					LPWORD		pidJob,
					LPSTR		szQueue		);

WORD FAR PASCAL WNetAbortJob 	      ( WORD		fh,
					LPSTR		szPName		);

WORD FAR PASCAL WNetHoldJob 	      ( LPSTR		szQueue,
					WORD		JobID		);

WORD FAR PASCAL WNetReleaseJob	      ( LPSTR		szQueue,
					WORD		JobID		);

WORD FAR PASCAL WNetCancelJob 	      ( LPSTR		szQueue,
					WORD		JobID		);

WORD FAR PASCAL WNetSetJobCopies      ( LPSTR		szQueue,
					WORD		JobID,
					WORD		nCopies		);

/*
 *	SPOOLING - QUEUE AND JOB INFO
 */

typedef struct _queuestruct	{
	WORD	pqName;
	WORD	pqComment;
	WORD	pqStatus;
	WORD	pqJobcount;
	WORD	pqPrinters;
} QUEUESTRUCT;

typedef QUEUESTRUCT far * LPQUEUESTRUCT;

#define WNPRQ_ACTIVE	0x0
#define WNPRQ_PAUSE	0x1
#define WNPRQ_ERROR	0x2
#define WNPRQ_PENDING	0x3
#define WNPRQ_PROBLEM	0x4


typedef struct _jobstruct 	{
	WORD	pjId;
	WORD	pjUsername;
	WORD	pjParms;
	WORD	pjPosition;
	WORD	pjStatus;
	DWORD	pjSubmitted;
	DWORD	pjSize;
	WORD	pjCopies;
	WORD	pjComment;
} JOBSTRUCT;

typedef JOBSTRUCT far * LPJOBSTRUCT;

#define WNPRJ_QSTATUS		0x7
#define  WNPRJ_QS_QUEUED    		0x0
#define  WNPRJ_QS_PAUSED    		0x1
#define  WNPRJ_QS_SPOOLING  		0x2
#define  WNPRJ_QS_PRINTING  		0x3
#define WNPRJ_DEVSTATUS	   	0xff8
#define  WNPRJ_DS_COMPLETE      	0x8
#define  WNPRJ_DS_INTERV        	0x10
#define  WNPRJ_DS_ERROR        		0x20
#define  WNPRJ_DS_DESTOFFLINE  		0x40
#define  WNPRJ_DS_DESTPAUSED   		0x80
#define  WNPRJ_DS_NOTIFY	   	0x100
#define  WNPRJ_DS_DESTNOPAPER  		0x200
#define  WNPRJ_DS_DESTFORMCHG  		0x400
#define  WNPRJ_DS_DESTCRTCHG  		0x800
#define  WNPRJ_DS_DESTPENCHG  		0x1000

#define SP_QUEUECHANGED		0x500


WORD FAR PASCAL WNetWatchQueue 	      (	HWND		hwnd,
					LPSTR		szLocal,
					LPSTR		szUser,
					WORD		nQueue		);

WORD FAR PASCAL WNetUnwatchQueue      (	LPSTR		szLocal		);

WORD FAR PASCAL WNetLockQueueData     (	LPSTR		szQueue,
					LPSTR		szUsername,
					LPQUEUESTRUCT	far * lpQueue	);

WORD FAR PASCAL WNetUnlockQueueData   (	LPSTR		szQueue		);


/*
 *	CONNECTIONS
 */

WORD FAR PASCAL WNetAddConnection     ( LPSTR		szNetPath,
					LPSTR		szPassword,
					LPSTR		szLocalName	);

WORD FAR PASCAL WNetCancelConnection  ( LPSTR		szName,
					BOOL		fForce		);

WORD FAR PASCAL WNetGetConnection     ( LPSTR		szLocal,
					LPSTR		szRemote,
					LPWORD		lpcbRemote );

/*
 *	CAPABILITIES
 */

#define WNNC_SPEC_VERSION		0x1

#define WNNC_NET_TYPE			0x2
#define  WNNC_NET_NONE				0x000
#define	 WNNC_NET_MSNet				0x100
#define  WNNC_NET_LanMan			0x200
#define  WNNC_NET_NetWare			0x300
#define  WNNC_NET_Vines 			0x400

#define WNNC_DRIVER_VERSION		0x3

#define WNNC_USER			0x4
#define  WNNC_USR_GetUser			0x1

#define WNNC_CONNECTION			0x6
#define  WNNC_CON_AddConnection			0x1
#define  WNNC_CON_CancelConnection		0x2
#define  WNNC_CON_GetConnections		0x4
#define  WNNC_CON_AutoConnect			0x8
#define  WNNC_CON_BrowseDialog			0x10

#define WNNC_PRINTING			0x7
#define  WNNC_PRT_OpenJob			0x2
#define  WNNC_PRT_CloseJob			0x4
#define  WNNC_PRT_HoldJob			0x10
#define  WNNC_PRT_ReleaseJob			0x20
#define  WNNC_PRT_CancelJob			0x40
#define  WNNC_PRT_SetJobCopies			0x80
#define  WNNC_PRT_WatchQueue			0x100
#define  WNNC_PRT_UnwatchQueue			0x200
#define  WNNC_PRT_LockQueueData			0x400
#define  WNNC_PRT_UnlockQueueData		0x800
#define  WNNC_PRT_ChangeMsg			0x1000
#define  WNNC_PRT_AbortJob			0x2000
#define  WNNC_PRT_NoArbitraryLock		0x4000

#define WNNC_ERROR			0xa
#define  WNNC_ERR_GetError			0x1
#define  WNNC_ERR_GetErrorText			0x2


WORD FAR PASCAL WNetGetCaps 	      ( WORD		nIndex		);

/*
 *	OTHER
 */

WORD FAR PASCAL WNetDeviceMode 	      ( HWND		hParent		);

WORD FAR PASCAL WNetGetUser 	      ( LPSTR		szUser,
					LPWORD		nBuffferSize	);

/*
 *	BROWSE DIALOG
 */

#define WNBD_CONN_UNKNOWN	0x0
#define WNBD_CONN_DISKTREE	0x1
#define WNBD_CONN_PRINTQ	0x3

WORD FAR PASCAL WNetBrowseDialog      ( HWND		hParent,
					WORD		nFunction,
					LPSTR		lpBuffer	);

/*
 *	ERRORS
 */

WORD FAR PASCAL WNetGetError 	      ( LPWORD		nError		);

WORD FAR PASCAL WNetGetErrorText      ( WORD		nError,
					LPSTR		lpBuffer,
					LPWORD		nBufferSize	);


/*
 *	STATUS CODES
 */

/* General */

#define WN_SUCCESS			0x00
#define WN_NOT_SUPPORTED		0x01
#define WN_NET_ERROR			0x02
#define WN_MORE_DATA			0x03
#define WN_BAD_POINTER			0x04
#define WN_BAD_VALUE			0x05
#define WN_BAD_PASSWORD			0x06
#define WN_ACCESS_DENIED		0x07
#define WN_FUNCTION_BUSY		0x08
#define WN_WINDOWS_ERROR		0x09
#define WN_BAD_USER			0x0a
#define WN_OUT_OF_MEMORY		0x0b
#define WN_CANCEL			0x0c

/* Connection */

#define WN_NOT_CONNECTED		0x30
#define WN_OPEN_FILES			0x31
#define WN_BAD_NETNAME			0x32
#define WN_BAD_LOCALNAME		0x33

/* Printing */

#define WN_BAD_JOBID			0x40
#define WN_JOB_NOT_FOUND		0x41
#define WN_JOB_NOT_HELD			0x42
#define WN_BAD_QUEUE			0x43
#define WN_BAD_FILE_HANDLE		0x44
#define WN_CANT_SET_COPIES		0x45
#define WN_ALREADY_LOCKED		0x46
