/* Printer capability structure--used to consolidate the individual
 * printer's capabilities and requirements.
 *
 * the size of this structure depends on the defs in drvinint.h
 */

#define NUMFEEDS (DMBIN_LAST - DMBIN_FIRST + 1)
#define NUMPAPERS (DMPAPER_LAST - DMPAPER_FIRST + 1)


typedef struct {
	int iPaperType;
	RECT rcImage;
} PAPER_REC;


typedef struct {
	int	version;		// version number
	char	Name[32];		/* the printer name */
	int	defFeed;		/* default feeder (DMBIN_*) */
	BOOL	feed[NUMFEEDS];
	int	defRes;
	int	defJobTimeout;
	BOOL	fEOF;
	BOOL	fColor;
	int	ScreenFreq;	// in 10ths of an inch
	int	ScreenAngle;	// in 10ths of a degree
	char	Reserved[12];	// reserved for future use
	int	iNumPapers;	/* # of paper sizes supported */
	PAPER_REC Paper[1];	/* variable size array of papers supported */
} PRINTER, FAR *LPPRINTER, *PPRINTER;


typedef struct { 
	long	cap_loc;
	int	cap_len;
	long	dir_loc;
	int	dir_len;
	long	pss_loc;
	int	pss_len;
} PS_RES_HEADER;
