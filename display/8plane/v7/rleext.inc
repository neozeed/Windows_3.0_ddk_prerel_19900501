;
;	File: RLEEXT.INC
;	Date: 7/24/89
;	Author: James Keller
;
;	This module has some extern definitions for items from other modules.
;

externFP        sum_RGB_alt_far     ;in ROBJECT.ASM

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg

extrn        i8e24_color_table:word

sEnd		DIMapSeg
