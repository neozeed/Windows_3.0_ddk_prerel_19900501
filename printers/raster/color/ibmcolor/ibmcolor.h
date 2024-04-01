#define DEV_MEMORY  0

#define DEVICEEXTRA 2		    /* two extra bytes */

/******************** device dependent escape sequences **********************/

#define HYPOTENUSE     163          /* sq root of x^2 + y^2 */
#define YMAJOR         140          /* the direction with finer resolution has less */
#define XMAJOR          84          /* distance to travel */
#define MAXSTYLEERR     HYPOTENUSE*2


/* sends form feed */
#define EP_FF      "\014", 1

/* line feed */
#define EP_LF      "\012", 1
#define LF_CHAR     '\012'

/* carriage return */
#define EP_CR       "\015", 1
#define EP_NL       "\012\015", 2

/* escape code for x microspace justification */
#define EP_BLANK    " ", 1
#define SPACE       ' '

#define LETTER              20
#define DINA4               21
#define FANFOLD             22
#define STANDARDPF          LETTER  /* standard paper format */

#define RIBBON_OFFSET       7       /* must be less than VOFFSET */
#define MAXPAPERFORMAT      3       /* support 3 paper formats for now */

/* standard page sizes and offsets for letter sized paper */
#define HPAGESIZE    1190           /* 8 1/2" */
#define VPAGESIZE     924           /* 11" */
#define HOFFSET        35
#define VOFFSET        14           /* move down by 1/2" */

/* DIN A4 paper */
#define DINA4_VOFFSET        43     /* center the printable area down */
#define DINA4_HOFFSET        19     /* center the printable area horizontally */
#define DINA4_HPAGESIZE    1158     /* 210 mm */
#define DINA4_VPAGESIZE     982     /* 297 mm */

/* Fandfold paper */
#define FANF_VOFFSET        56      /* center the printable area vertically   */
#define FANF_HOFFSET        17      /* center the printable area horizontally */
#define FANF_HPAGESIZE    1155      /* 8 1/42" */
#define FANF_VPAGESIZE    1008      /* 12" */

#define HDPI         140            /* 140 dots per inch across the page */
#define VDPI          84            /* 84 DPI down the page */
#define HSIZE          8            /* 8 inches across the page*/
#define VSIZE         11            /* 10.67 inches down the page */

#define NPINS   7   /* ibm color printer fires eight pins at a time */
#define MAXBAND  8  /* number of bands */

/* PG_ACROSS must be a multiple of 16
   PG_ACROSS is the resolution of one scan line */
#define PG_ACROSS   1120             /* 140 * 8 */

/*
 * PG_DOWN must be a multiple of (t * 16), where t is the product of factor 2 of
 * MAXBAND.  This is to ensure word alignment of bitmaps.
 * It must also be divisible by (MAXBAND * NPINS).
 * Size of bitmap required for a band = (PG_ACROSS/8 * PG_DOWN/MAXBAND *NPLANES)
 * 					BYTES
 * PG_DOWN is the number of scan lines per page 
 */
#define PG_DOWN     896

/* must reset printer, select black band, pica print */
#define EP_ENABLE "\033@\000\033b\033C\000\013\033I\003\0336\022\033?\001", 18
#define EP_NORMAL "\033b\022", 3
#define SPECIAL_CHAR "\033^", 2

#define PICA_WIDTH   14
#define COMP_MODE_WIDTH 8	/* shoud be 8 1/6 */
/* if it is a varaible pitch font, we will try to be smart and use
   a fixed pitch font with a width that is 0 pixels less just to be save */
#define VAR_PITCH_KLUDGE 3

#define MAXDELY     255      /* max number of 1/144" a line feed covers */

#define CHARWIDTH   14      /* 12 dots per hardware font character */

#define XMS     'L'
#define FASTXMS 'Y'

/* Color Information */
#define NPLANES	3

#define CYAN	0x01
#define MAGENTA	0x02
#define YELLOW	0x04
#define WHITE	0x00
#define BLACK	0x07
#define MAXCOLOR 0x08

/******************* end of device dependent information ********************/

#define BAND_SIZE(k) ((PG_DOWN / MAXBAND) * (PG_ACROSS	/ 8) * NPLANES)   /* in bytes */
#define BUF_SIZE(k)  ((PG_ACROSS) * (NPLANES +1)) /* in bytes - (n+1)th plane needed for black and white - see line_out() */
#define SPOOL_SIZE(k) (BAND_SIZE(0) / 2 / NPLANES)

#define BAND_WIDTH  (PG_DOWN / MAXBAND) /* number of y-pixels in one band */
#define BAND_LINE   (BAND_WIDTH / NPINS)/* number of printing scan lines that can fit in a band */
#define BAND_HEIGHT (BAND_WIDTH)

#define Y_INIT_ENT   (10)   /* 10 entries, i.e 10 textout calls per band */
#define X_INIT_ENT   (4)
#define INIT_BUF    (512)   /* buffer space for textout calls */
#define MARG_BUF    (512)   /* increase by buffer space */

#define BAND_SLOP(k) (PG_ACROSS - PG_ACROSS/8*NPINS)
#define BUF_SLOP(k)  BAND_SLOP(k)
