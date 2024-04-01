;
;
;      File:   SETABS.INC
;      Author: James Keller
;      Date:   7/16/89
;

createSeg	_DIMAPS, DIMapSeg, word, public, CODE
sBegin		DIMapSeg

	assumes	cs, DIMapSeg

extrn	     copyabs_e1i1:near
extrn	     copyabs_e4i1:near
extrn	     copyabs_e8i1:near
extrn	     copyabs_e24i1:near
extrn	     copyabs_e1i8:near
extrn	     copyabs_e4i8:near
extrn	     copyabs_e8i8:near
extrn	     copyabs_e24i8:near


sEnd	DIMapSeg


