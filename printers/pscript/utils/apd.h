#include "defaults.h"

#define MAX_APDS	100
#define NUM_DIR_ENTRIES 6

#define MAX_LINE_LENGTH		254
#define MAX_TOKEN_LENGTH	40

#define IDS_SR_MIN	200

#define IDS_PP_MIN	300

#define IDS_PR_MIN	400


#define MANUAL 0
#define AUTO 1



#define NULL 0
#define TRUE 1
#define FALSE 0

#define FAR far

typedef int BOOL;
typedef char BYTE;
typedef short int WORD;
typedef long int DWORD;


/* A lookup table structure for converting strings to tokens */
typedef struct{
	char *pString;	/* Ptr to the string */
	int id; 		/* The corresponding token value */
}TOKEN;


/* The APD tokens */
#define APD_UNDEFINED				0
#define APD_ENDOFFILE				1
#define APD_DEFAULTRESOLUTION		2
#define APD_DEFAULTTRANSFER			3
#define APD_TRANSFER				4
#define APD_DEFAULTPAGESIZE			5
#define APD_PAGESIZE				6
#define APD_DEFAULTPAGEREGION		7
#define APD_PAGEREGION				8
#define APD_DEFAULTPAPERTRAY		8
#define APD_PAPERTRAY				9
#define APD_DEFAULTIMAGEABLEAREA	10
#define APD_IMAGEABLEAREA			11
#define APD_DEFAULTINPUTSLOT		12
#define APD_INPUTSLOT				13
#define APD_DEFAULTMANUALFEED		14
#define APD_MANUALFEED				15
#define APD_PRODUCT					16
#define APD_NONE					17
#define APD_PRINTERNAME				18
#define APD_WINDOWSAUTO				19
#define APD_DEFAULTPAPERSTOCK		20
#define APD_END						21
#define APD_ENDAPD					22
#define APD_STARTAPD				23
#define APD_TRANSVERSE				24
#define APD_COLOR				25


typedef struct{
    int 	left;
    int 	top;
    int 	right;
    int 	bottom;
}RECT;


typedef struct{
    short x;
    short y;
}POINT;


typedef struct{
    double x;
    double y;
}FPOINT;


extern PutByte(short int);
extern PutWord(short int);
extern PutLong(long);

