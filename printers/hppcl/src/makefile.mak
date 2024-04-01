# MAKEFILE.MAK - make file for the SRC subdirectory using POLYMAKE
# Created using AMMF (Aldus Make MakeFile) Version 3.04
# Created on Tue Aug 11 15:58:37 1987
#
#
#
.IGNORE

DRIVE=i:\pm2

COMPILER=cc

LINK=link

LIBRARIAN=lib

GET_OPTION=-q -t

OBJ_LOC=obj\\

INC_LOC=..\ink\\

RC_LOC=..\rc\\

DATE_LOC=..\\

SRC_LOC=

VCS_SRC=$(DRIVE)\SRC\vcs\\

VCS_INC=$(DRIVE)\inc\vcs\\

.PATH.cv = $(VCS_SRC)

.PATH.hv = $(VCS_INC)

.PATH.obj = $(OBJ_LOC)

.PATH.exe = $(OBJ_LOC)

.PATH.c = $(SRC_LOC)

.PATH.h = $(INC_LOC)

.PATH.i = $(INC_LOC)

SOURCES= heap.a \
lmemcpy.a \
lmemset.a \
offsrect.a \
dump.a \
_gtfree.a \
_opend.a \
_readd.a \
_delete.a \
_mkdir.a \
charwdth.c \
control.c \
debug.c \
devmode.c \
dlgutils.c \
environ.c \
escquery.c \
fntutils.c \
fontbld.c \
fontman.c \
fontutil.c \
memoman.c \
options.c \
paper.c \
physical.c \
qsort.c \
realize.c \
reset.c \
sfadd.c \
sfadd2.c \
sfcopy.c \
sfdir.c \
sfdownld.c \
sfedit.c \
sferase.c \
sffile.c \
sfinstal.c \
sflb.c \
sfpfm.c \
sfutils.c \
sfutils2.c \
stubs.c \
transtbl.c \
utils.c \
pclstub.c \
pclsf_yn.c

OBJECTS= heap.obj \
lmemcpy.obj \
lmemset.obj \
offsrect.obj \
dump.obj \
_gtfree.obj \
_opend.obj \
_readd.obj \
_delete.obj \
_mkdir.obj \
charwdth.obj \
control.obj \
debug.obj \
devmode.obj \
dlgutils.obj \
environ.obj \
escquery.obj \
fntutils.obj \
fontbld.obj \
fontman.obj \
fontutil.obj \
memoman.obj \
options.obj \
paper.obj \
physical.obj \
qsort.obj \
realize.obj \
reset.obj \
sfadd.obj \
sfadd2.obj \
sfcopy.obj \
sfdir.obj \
sfdownld.obj \
sfedit.obj \
sferase.obj \
sffile.obj \
sfinstal.obj \
sflb.obj \
sfpfm.obj \
sfutils.obj \
sfutils2.obj \
stubs.obj \
transtbl.obj \
utils.obj \
pclstub.exe \
pclsf_yn.exe

srcbuild: $(OBJECTS)
	date > $(SRC_LOC)srcbuild < $(DATE_LOC)dateans
	time >> $(SRC_LOC)srcbuild < $(DATE_LOC)dateans

heap.obj: heap.a
	MASM -I$(INC_LOC) heap.a,$(OBJ_LOC);

lmemcpy.obj: lmemcpy.a
	MASM -I$(INC_LOC) lmemcpy.a,$(OBJ_LOC);

lmemset.obj: lmemset.a
	MASM -I$(INC_LOC) lmemset.a,$(OBJ_LOC);

offsrect.obj: offsrect.a
	MASM -I$(INC_LOC) offsrect.a,$(OBJ_LOC);

dump.obj: dump.a device.i
	MASM -I$(INC_LOC) dump.a,$(OBJ_LOC);

_gtfree.obj: _gtfree.a
	MASM -I$(INC_LOC) _gtfree.a,$(OBJ_LOC);

_opend.obj: _opend.a
	MASM -I$(INC_LOC) _opend.a,$(OBJ_LOC);

_readd.obj: _readd.a
	MASM -I$(INC_LOC) _readd.a,$(OBJ_LOC);

_write.obj: _write.a
	MASM -I$(INC_LOC) _write.a,$(OBJ_LOC);

_delete.obj: _delete.a
	MASM -I$(INC_LOC) _delete.a,$(OBJ_LOC);

_mkdir.obj: _mkdir.a
	MASM -I$(INC_LOC) _mkdir.a,$(OBJ_LOC);

charwdth.obj: charwdth.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _CHARNESC -Fo$(OBJ_LOC) $(SRC_LOC)charwdth.c

control.obj: control.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
strings.h \
spool.h \
fontman.h \
memoman.h \
environ.h \
utils.h \
dump.h \
extescs.h \
paper.h \
message.c \
lockfont.c \
makefsnm.c \
loadfile.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _ESCAPE -Fo$(OBJ_LOC) $(SRC_LOC)control.c

debug.obj: debug.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -Fo$(OBJ_LOC) $(SRC_LOC)debug.c
	$(LIBRARIAN) $(OBJ_LOC)debug-+$(OBJ_LOC)debug.obj;

devmode.obj: devmode.c \
nocrap.h \
pfm.h \
hppcl.h \
resource.h \
fontman.h \
strings.h \
debug.h \
dlgutils.h \
environ.h \
utils.h \
paperfmt.h \
paper.h \
getint.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _DEVMODE -Fo$(OBJ_LOC) $(SRC_LOC)devmode.c

dlgutils.obj: dlgutils.c \
nocrap.h \
debug.h \
dlgutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _DEVMODE -Fo$(OBJ_LOC) $(SRC_LOC)dlgutils.c

environ.obj: environ.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
strings.h \
environ.h \
utils.h \
getint.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _ENVIRON -Fo$(OBJ_LOC) $(SRC_LOC)environ.c

escquery.obj: escquery.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
extescs.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _CHARNESC -Fo$(OBJ_LOC) $(SRC_LOC)escquery.c

fntutils.obj: fntutils.c \
nocrap.h \
fntutils.h \
neededh.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _FNTUTILS -Fo$(OBJ_LOC) $(SRC_LOC)fntutils.c

fontbld.obj: fontbld.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
fonts.h \
debug.h \
strings.h \
memoman.h \
fontpriv.h \
utils.h \
getint.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _FONTBLD -Fo$(OBJ_LOC) $(SRC_LOC)fontbld.c

fontman.obj: fontman.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
fonts.h \
debug.h \
strings.h \
version.h \
fontpriv.h \
environ.h \
utils.h \
lockfont.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _RESET -Fo$(OBJ_LOC) $(SRC_LOC)fontman.c

fontutil.obj: fontutil.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
fonts.h \
utils.h \
memoman.h \
debug.h \
lockfont.c \
makefsnm.c \
loadfile.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _CHARNESC -Fo$(OBJ_LOC) $(SRC_LOC)fontutil.c

memoman.obj: memoman.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
utils.h \
memoman.h \
lockfont.c \
makefsnm.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _PHYSICAL -Fo$(OBJ_LOC) $(SRC_LOC)memoman.c

options.obj: options.c \
nocrap.h \
hppcl.h \
resource.h \
dlgutils.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _OPTIONS -Fo$(OBJ_LOC) $(SRC_LOC)options.c

paper.obj: paper.c \
nocrap.h \
resource.h \
environ.h \
paperfmt.h \
paper.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _PAPER -Fo$(OBJ_LOC) $(SRC_LOC)paper.c

physical.obj: physical.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
strings.h \
utils.h \
memoman.h \
transtbl.h \
message.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _PHYSICAL -Fo$(OBJ_LOC) $(SRC_LOC)physical.c

qsort.obj: qsort.c \
pfm.h \
qsort.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _ESCAPE -Fo$(OBJ_LOC) $(SRC_LOC)qsort.c

realize.obj: realize.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
fonts.h \
lockfont.c
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _REALIZE -Fo$(OBJ_LOC) $(SRC_LOC)realize.c

reset.obj: reset.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h \
strings.h \
environ.h \
utils.h \
dump.h \
paper.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _RESET -Fo$(OBJ_LOC) $(SRC_LOC)reset.c

sfadd.obj: sfadd.c \
nocrap.h \
neededh.h \
device.h \
utils.h \
debug.h \
strings.h \
pfm.h \
sfadd.h \
sfdir.h \
sflb.h \
sfpfm.h \
sffile.h \
sfutils.h \
sfinstal.h \
dlgutils.h \
dosutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFADD -Fo$(OBJ_LOC) $(SRC_LOC)sfadd.c

sfadd2.obj: sfadd2.c \
nocrap.h \
neededh.h \
device.h \
utils.h \
debug.h \
strings.h \
pfm.h \
sfadd.h \
sfdir.h \
sflb.h \
sfpfm.h \
sfedit.h \
sfinstal.h \
sfutils.h \
dlgutils.h \
dosutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFADD2 -Fo$(OBJ_LOC) $(SRC_LOC)sfadd2.c

sfcopy.obj: sfcopy.c \
nocrap.h \
neededh.h \
strings.h \
device.h \
utils.h \
debug.h \
sfcopy.h \
strings.h \
dlgutils.h \
sfdir.h \
sflb.h \
sfutils.h \
sfinstal.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFCOPY -Fo$(OBJ_LOC) $(SRC_LOC)sfcopy.c

sfdir.obj: sfdir.c \
nocrap.h \
sfdir.h \
utils.h \
neededh.h \
debug.h \
dosutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFDIR -Fo$(OBJ_LOC) $(SRC_LOC)sfdir.c

sfdownld.obj: sfdownld.c \
nocrap.h \
neededh.h \
utils.h \
strings.h \
debug.h \
sfdownld.h \
sfdir.h \
sflb.h \
sfutils.h \
dlgutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFDOWNLD -Fo$(OBJ_LOC) $(SRC_LOC)sfdownld.c

sfedit.obj: sfedit.c \
nocrap.h \
neededh.h \
strings.h \
device.h \
utils.h \
debug.h \
sfedit.h \
dlgutils.h \
sfdir.h \
sflb.h \
sfutils.h \
pfm.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFEDIT -Fo$(OBJ_LOC) $(SRC_LOC)sfedit.c

sferase.obj: sferase.c \
nocrap.h \
neededh.h \
strings.h \
utils.h \
debug.h \
sferase.h \
sfdir.h \
sflb.h \
sfutils.h \
strings.h \
sfinstal.h \
dlgutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFERASE -Fo$(OBJ_LOC) $(SRC_LOC)sferase.c

sffile.obj: sffile.c \
nocrap.h \
neededh.h \
utils.h \
sffile.h \
sfdir.h \
sflb.h \
sfutils.h \
strings.h \
sfadd.h \
sfinstal.h \
dlgutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFFILE -Fo$(OBJ_LOC) $(SRC_LOC)sffile.c

sfinstal.obj: sfinstal.c \
nocrap.h \
utils.h \
sfinstal.h \
dlgutils.h \
neededh.h \
sfdir.h \
sfutils.h \
strings.h \
sffile.h \
sfadd.h \
sferase.h \
sfcopy.h \
sfedit.h \
sfdownld.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFINSTAL -Fo$(OBJ_LOC) $(SRC_LOC)sfinstal.c

sflb.obj: sflb.c \
nocrap.h \
neededh.h \
device.h \
utils.h \
debug.h \
sfdir.h \
sflb.h \
sfutils.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFLB -Fo$(OBJ_LOC) $(SRC_LOC)sflb.c

sfpfm.obj: sfpfm.c \
nocrap.h \
neededh.h \
pfm.h \
sfpfm.h \
transtbl.h \
memoman.h \
strings.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFPFM -Fo$(OBJ_LOC) $(SRC_LOC)sfpfm.c

sfutils.obj: sfutils.c \
nocrap.h \
utils.h \
fntutils.h \
neededh.h \
sfinstal.h \
dlgutils.h \
strings.h \
sfdir.h \
sflb.h \
sfutils.h \
pfm.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFUTILS -Fo$(OBJ_LOC) $(SRC_LOC)sfutils.c

sfutils2.obj: sfutils2.c \
nocrap.h \
sflb.h \
sfutils.h \
neededh.h \
dlgutils.h \
device.h \
utils.h \
debug.h \
sfinstal.h \
strings.h \
pfm.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _SFUTILS2 -Fo$(OBJ_LOC) $(SRC_LOC)sfutils2.c

stubs.obj: stubs.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
fontman.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _STUBS -Fo$(OBJ_LOC) $(SRC_LOC)stubs.c

transtbl.obj: transtbl.c \
nocrap.h \
pfm.h \
transtbl.h \
hppcl.h \
resource.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -NT _TRANSTBL -Fo$(OBJ_LOC) $(SRC_LOC)transtbl.c

utils.obj: utils.c \
generic.h \
printer.h \
hppcl.h \
debug.h \
pfm.h \
paperfmt.h \
device.h \
resource.h \
utils.h \
debug.h
	$(COMPILER) -W2 -u -c -Asnw -PLM -Gsw -Oas -Zpe -Fo$(OBJ_LOC) $(SRC_LOC)utils.c

pclstub.obj: pclstub.c \
printer.h \
dosutils.h \
fontpriv.h \
hppcl.h \
resource.h \
version.h \
pfm.h
	$(COMPILER) -W2 -c -Fo$(OBJ_LOC) $(SRC_LOC)pclstub.c

pclsf_yn.obj: pclsf_yn.c \
dosutils.h
	$(COMPILER) -W2 -c -Fo$(OBJ_LOC) $(SRC_LOC)pclsf_yn.c

pclstub.exe: pclstub.obj \
_write.obj
	$(LINK) $(OBJ_LOC)pclstub+$(OBJ_LOC)_write.obj,$(OBJ_LOC)pclstub;
	copy $(OBJ_LOC)pclstub.exe ..

pclsf_yn.exe: pclsf_yn.obj \
_write.obj
	$(LINK) $(OBJ_LOC)pclsf_yn+$(OBJ_LOC)_write.obj,$(OBJ_LOC)pclsf_yn;
	copy $(OBJ_LOC)pclsf_yn.exe $(RC_LOC)pclsf_yn.bin
