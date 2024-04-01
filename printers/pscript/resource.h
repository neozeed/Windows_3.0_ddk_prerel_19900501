/*
 * resource.h
 *
 * this contains resource type identifiers and some constants
 * used to name resources.
 *
 */

/*
 * resource types defined by this driver
 *
 */

#define MYFONTDIR	257	/* font direcotyr .DIR */
#define MYFONT		258	/* PFM file */
#define MY_DATA		261	/* random data */
#define PS_DATA		259	/* PS code */
#define PR_CAPS		262	/* PRINTER structs */
#define PR_PSS		263	/* PSS file */


#define DMBIN_BASE	5000	/* resource base numbers for DM strings */
#define DMPAPER_BASE	6000


/*
 * IDs for MY_DATA type
 */

#define PAPERSIZES	1


/* PostScript resource ID's (type PS_DATA) */

#define PS_HEADER	1
#define PS_DL_PREFIX	2
#define PS_DL_SUFFIX	3
#define PS_1		4
#define PS_SOFTWARE	5
#define PS_HARDWARE	6
#define PS_EHANDLER	7
#define PS_2            8
#define PS_OLIVCHSET	9
#define PS_UNPACK	10
#define PS_FONTS	11
#define PS_CIMAGE	12


/* string IDs */

#define IDS_DEVICE		100	/* for "device=" entry in WIN.INI */
#define IDS_PAPERX		101	/* for "paperX=" entries */
#define IDS_ORIENTATION		102	/* for "orientation=" entry */
#define IDS_RESOLUTION		103	/* for "resolution=" entry */
#define IDS_PAPERSOURCE		104	/* for "papersource=" entry */
#define IDS_JOBTIMEOUT		105	/* for "jobtimeout=" entry */
#define IDS_HEADER		106	/* for "header=" entry */
#define IDS_MARGINS		107	/* for "Margins=" entry */
#define IDS_YES			109	/* "yes" */
#define IDS_NO			110	/* "no" */
#define IDS_USER		111
#define IDS_DEFAULT_USER	112
#define IDS_APPLETALK		113
#define IDS_NULL		114
#define IDS_PREPARE		115
#define IDS_STRUCTURED_COMMENTS	116
#define IDS_WINDOWS		117
#define IDS_ATMODULEFILE	118
#define IDS_DEFAULT_ATFILE	119
#define IDS_DEFAULT_ATMODNAME	120
#define IDS_EPT		  	121
#define IDS_BINARYIMAGE  	122
#define IDS_COLOR		123
#define IDS_EPSHEAD		124
#define IDS_EPSBBOX		125
#define IDS_PSHEAD		126
#define IDS_PSJOB		127
#define IDS_PSTIMEOUT		128
#define IDS_PSTITLE		129
#define IDS_EXTPRINTER		130
#define IDS_PRINTER		131
#define IDS_FILE		132
#define IDS_EXTPRINTERS		133
#define IDS_ALREADYINSTALLED    134
#define IDS_ADDPRINTER		135
#define	IDS_INSTSUCCESS		136
#define	IDS_INSTFAIL		137
#define IDS_SETSCREENANGLE	138
#define IDS_OLIV		139
#define IDS_LZLIB		140
#define IDS_LZCOPY		141
