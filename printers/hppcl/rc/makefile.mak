#
#    $Revision:   1.5  $
#    $Author:   msd  $
#    $Date:   27 Sep 1988 14:32:38  $
#
#
.IGNORE

DRIVE=i:\pm2

COMPILER=cc

LIBRARIAN=lib

GET_OPTION=-q -t

OBJ_LOC=

INC_LOC=..\ink\\

SRC_LOC=

VCS_SRC=$(DRIVE)\RC\vcs\\

VCS_INC=$(DRIVE)\ink\vcs\\

.PATH.cv = $(VCS_SRC)

.PATH.hv = $(VCS_INC)

.PATH.obj = $(OBJ_LOC)

.PATH.c = $(SRC_LOC)

.PATH.h = $(INC_LOC)

SOURCES= hppcl.rc \
makeres.exe \
xfaces.exe

OBJECTS= hppcl.res \
makeres.obj \
xfaces.obj

srcbuild: $(OBJECTS)
	date > $(SRC_LOC)srcbuild < $(INC_LOC)dateans
	time >> $(SRC_LOC)srcbuild < $(INC_LOC)dateans

makeres.exe: makeres.c \
printer.h \
hppcl.h \
pfm.h \
trans.h \
paperfmt.h
	$(COMPILER) -W2 $(SRC_LOC)makeres.c
	makeres

xfaces.exe: xfaces.c
	$(COMPILER) -W2 $(SRC_LOC)xfaces.c
	xfaces

hppcl.res: hppcl.rc \
makeres.exe \
xfaces.exe \
resource.h \
strings.h \
version.h
	rc -r -e hppcl.rc
	copy hppcl.res ..\hppcl.res
