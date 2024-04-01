;
;
;	File:	COPYENCO.INC
;	Author: James Keller
;	Date:	8/15/89
;
;


createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg

extrn	copyenco_i1e1:near
extrn	copyenco_i1e4:near
extrn	copyenco_i1e8:near
extrn	copyenco_i1e24:near
extrn	copyenco_i8e1:near
extrn	copyenco_i8e4:near
extrn	copyenco_i8e8:near
extrn	copyenco_i8e24:near

sEnd	DIMapSeg

