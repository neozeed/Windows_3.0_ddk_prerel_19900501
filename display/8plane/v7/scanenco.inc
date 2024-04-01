
;
;	File:	SCANENCO.INC
;	Date:	7/24/89
;	Author:	James Keller
;
;	This module handles the setup and rle encoding of a bitmap image.
;

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg

extrn	     scanline_encode_bitmap:near

sEnd	DIMapSeg

