# sample make file for building vhd virtual device for Windows/386

# comment this definition out with a "#", if building a non-debugging version
Debug=-DDEBUG

.asm.obj:
        masm5 -p -w2 -Mx $(Debug) -I..\include $*;

.asm.lst:
        masm5 -l -p -w2 -Mx $(Debug) -I..\include $*;

all:    vhd.386

vhd.obj vhd.lst: vhd.asm ..\include\vhd.inc ..\include\smartdrv.inc \
	 ..\include\vdmad.inc ..\include\sysinfo.inc ..\include\debug.inc \
	 ..\include\vtd.inc ..\include\vpicd.inc ..\include\vmm.inc

vhdtext.obj vhdtext.lst: vhdtext.asm ..\include\vmm.inc


OBJS =  vhd.obj vhdtext.obj 

vhd.386: vhd.def $(OBJS)
        link386 @vhd.lnk
        addhdr vhd.386
        mapsym32 vhd
