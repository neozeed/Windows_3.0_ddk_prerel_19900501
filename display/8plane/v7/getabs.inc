;
;
;      File:   GETABS.INC
;      Author: James Keller
;      Date:   7/16/89
;

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg

extrn	     copyabs_i1e1:near
extrn	     copyabs_i1e4:near
extrn	     copyabs_i1e8:near
extrn	     copyabs_i1e24:near
extrn	     copyabs_i8e1:near
extrn	     copyabs_i8e4:near
extrn	     copyabs_i8e8:near
extrn	     copyabs_i8e24:near

sEnd	DIMapSeg


