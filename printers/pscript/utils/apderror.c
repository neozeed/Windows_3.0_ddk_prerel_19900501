#include <stdio.h>
#include "apd.h"
#include "globals.h"
#include "apderror.h"


/* this used to work in C4.0, but not in C5.0
 * void APDError(int,char*,...);
 */
void APDError();
void APDError(e,f,i1,i2)
	int e;
	char f[];
	char i1[];
	char i2[];
{
	static char APD[]="APD Error:  ";
	static char File[]="File ";
	static char Line[]="line ";

	switch(e){
		case E_GENERAL:
		default:
			printf("%s%s\"%s\" %s%d: Error.\n",APD,File,f,Line,gLineNum);
		break;
		case E_TOOMANYAPDS:
			printf("%s%s\"%s\" %s%d: Too many APD's.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_MISSING:
			printf("%s%s\"%s\" %s%d: Missing '%s'.\n",
				APD,File,f,Line,gLineNum,i1);
		break;
		case E_ILLEGALCHARACTER:
			printf("%s%s\"%s\" %s%d: Illegal character.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_ILLEGALFILENAME:
			printf("%s%s\"%s\": Illegal file name.\n",
				APD,File,f);
		break;
		case E_MISSINGKEYWORDAFTER:
			printf("%s%s\"%s\" %s%d: Missing keyword after '%s'.\n",
				APD,File,f,Line,gLineNum,i1);
		break;
		case E_MISSINGAFTER:
			printf("%s%s\"%s\" %s%d: Missing '%s' after '%s'.\n",
				APD,File,f,Line,gLineNum,i1,i2);
		break;
		case E_ILLEGALKEYWORDAFTER:
			printf("%s%s\"%s\" %s%d: Illegal keyword after '%s'.\n",
				APD,File,f,Line,gLineNum,i1);
		break;
		case E_ILLEGALSTRINGAFTER:
			printf("%s%s\"%s\" %s%d: Illegal string after '%s'.\n",
				APD,File,f,Line,gLineNum,i1);
		break;
		case E_TRANSVERSE:
			printf("%s%s\"%s\" %s%d: Expected transverse modifier.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_TRANSLATION:
			printf("%s%s\"%s\" %s%d: Illegal translation string.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_MISSINGKEYWORD:
			printf("%s%s\"%s\" %s%d: Missing keyword.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_TRUEFALSE:
			printf("%s%s\"%s\" %s%d: True/False entry missing.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_INCOMPLETE:
			printf("%s%s\"%s\" %s%d: Incomplete imageable area info.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_INVERSE:
			printf("%s%s\"%s\" %s%d: Inverse modifier...ignored.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_FIND:
			printf("%s%s\"%s\": Can't locate '/* Pass xx */' string.\n",
				APD,File,f);
		break;
		case E_MAKE:
			printf("%s%s\"%s\": Can't make file.\n",
				APD,File,f);
		break;
		case E_OPEN:
			printf("%s%s\"%s\": Open open file.\n",
				APD,File,f);
		break;
		case E_CLOSE:
			printf("%s%s\"%s\": Can't close file.\n",
				APD,File,f);
		break;
		case E_WRITE:
			printf("%s%s\"%s\": Can't write to file.\n",
				APD,File,f);
		break;
		case E_READ:
			printf("%s%s\"%s\": Can't read from file.\n",
				APD,File,f);
		break;
		case E_READAPD:
			printf("%s%s\"%s\" %s%d: Read error...check APD file.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_TOKENTOOLONG:
			printf("%s%s\"%s\" %s%d: Token is too long.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_STRINGTOOLONG:
			printf("%s%s\"%s\" %s%d: String is too long for one line.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_LINETOOLONG:
			printf("%s%s\"%s\" %s%d: Line is too long > %d characters.\n",
				APD,File,f,Line,gLineNum,MAX_LINE_LENGTH);
		break;
		case E_EOF:
			printf("%s%s\"%s\" %s%d: EOF while string unterminated.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_END:
			printf("%s%s\"%s\" %s%d: Warning: '*End:' without closing quote.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_UNTERMINATEDSTRING:
			printf("%s%s\"%s\" %s%d: Unterminated string.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_MEMORY:
			printf("%s%s\"%s\": Memory allocation error.\n",
				APD,File,f);
		break;
		case E_MEMORY1:
			printf("%s: Memory allocation error.\n",APD);
		break;
		case E_ILLEGALORMISSING:
			printf("%s%s\"%s\" %s%d: Illegal character or missing keyword '%s'.\n",
				APD,File,f,Line,gLineNum,i1);
		break;
		case E_POSSIBLENONNUMBER:
			printf("%s%s\"%s\" %s%d: Warning: Possible non-number.\n",
				APD,File,f,Line,gLineNum);
		break;
		case E_NEGATIVEMARGIN:
			printf("%s%s\"%s\" %s%d: Value produces negative margin value.\n",
				APD,File,f,Line,gLineNum);
		break;
	}
}
