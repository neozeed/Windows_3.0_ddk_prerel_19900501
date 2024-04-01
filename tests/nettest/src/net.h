/* Windows Network include file */

/* Net function - Get Net Capabilities */
WORD FAR PASCAL WNetGetCaps(WORD nIndex);

#define WNNC_SPEC_VERSION	0x0001

/* Query */
#define WNNC_NET_TYPE		0x0002

#define WNNC_NET_None		    0x0000   /* Masks */
#define WNNC_NET_MSNet		    0x0100
#define WNNC_NET_LanMan 	    0x0200
#define WNNC_NET_NetWare	    0x0300
#define WNNC_NET_Vines		    0x0400

#define WNNC_DRIVER_VERSION	0x0003

/* Query */
#define WNNC_USER		0x0004

#define WNNC_USR_GetUser	    0x0001   /* Masks */

/* Query */
#define WNNC_CONNECTION 	0x0006

#define WNNC_CON_AddConnection	    0x0001   /* Masks */
#define WNNC_CON_CancelConnection   0x0002
#define WNNC_CON_GetConnections     0x0004
#define WNNC_CON_AutoConnect	    0x0008
#define WNNC_CON_BrowseDialog	    0x0010

/* Query */
#define WNNC_PRINTING		0x0007

#define WNNC_PRT_OpenJob	    0x0002   /* Masks */
#define WNNC_PRT_CloseJob	    0x0004
#define WNNC_PRT_HoldJob	    0x0010
#define WNNC_PRT_ReleaseJob	    0x0020
#define WNNC_PRT_CancelJob	    0x0040
#define WNNC_PRT_SetJobCopies	    0x0080
#define WNNC_PRT_WatchQueue	    0x0100
#define WNNC_PRT_UnwatchQueue	    0x0200
#define WNNC_PRT_LockQueueData	    0x0400
#define WNNC_PRT_UnlockQueueData    0x0800
#define WNNC_PRT_ChangeMsg	    0x1000
#define WNNC_PRT_AbortJob	    0x2000

/* Query */
#define WNNC_DEVICEMODE 	0x0008

#define WNNC_DEVM_DeviceMode	    0x0001   /* Masks */

/* Query */
#define WNNC_ERROR		0x000A

#define WNNC_ERR_GetError	    0x0001
#define WNNC_ERR_GetErrorInfo	    0x0002
