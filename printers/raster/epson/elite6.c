#include "printer.h"
#include "gdidefs.inc"
#include "..\epson\epson.h"
#include "..\device.h"

char FntFile[] = "elite6.fnt";

char DeviceName[] = "Epson FX-80";	/* see dfDevice */

char FaceName[] = "Elite Expanded";	/* see dfFace */

FONTINFO FontInfo = {
	0x80,	/* dfType (0x0080 is device font) */
	15,	/* dfPoints (in 1/72") */
	72,	/* dfVertRes */
	120,	/* dfHorizRes */
	7,	/* dfAscent */
	0,	/* dfInternalLeading */
	3,	/* dfExternalLeading */
	0,	/* dfItalic */
	0,	/* dfUnderline */
	0,	/* dfStrikeOut */
	400,	/* dfWeight */
	0,	/* dfCharSet */
	20,	/* dfPixWidth */
	9,	/* dfPixHeight */
	0x30,	/* dfPitchAndFamily
		 * low bit is variable pitch flag
		 *    0 => FF_DONTCARE
		 *    1 => FF_ROMAN
		 *    2 => FF_SWISS
		 *    3 => FF_MODERN
		 *    4 => FF_SCRIPT
		 *    5 => FF_DECORATIVE
		 */
	20,	/* dfAvgWidth */
	20,	/* dfMaxWidth */
	0x20,	/* dfFirstChar */
	0xff,	/* dfLastChar */
	0x2e,	/* dfDefaultChar + dfFirstChar */
	0x20,	/* dfBreakChar + dfFirstChar */
	0,	/* dfWidthBytes ( = 0 ) */
	0x41,	/* dfDevice - see DeviceName */
	0x4d,	/* dfFace - see FaceName */
	0x0,	/* dfBitsPointer ( = 0 ) */
	0x0,	/* dfBitsOffset ( = 0 ) */
};

PRDFONTINFO PrdFontInfo = {	/* Driver Specific Info */
	"",	/* dfFont */
	0,	/* wheel */
		/* bfont: */
	110,		/* offset */
	00,		/* mod */
	05,		/* length */
		/* efont: */
	115,		/* offset */
	00,		/* mod */
	05,		/* length */
	0x0,	/* widthtable - see WidthTable */
};

/* No Width Table */
int bWidthTable = FALSE;
short WidthTable[] = {
	0,
};
unsigned char WidthFirstChar = 0;
unsigned char WidthLastChar = 0;
