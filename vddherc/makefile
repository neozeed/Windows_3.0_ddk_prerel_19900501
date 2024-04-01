# sample make file for building vddherc virtual device for Windows/386

# comment this definition out with a "#", if building a non-debugging version
Debug=-DDEBUG

.asm.obj:
        masm5 -p -w2 -Mx $(Debug) -I..\include $*;

.asm.lst:
        masm5 -l -p -w2 -Mx $(Debug) -I..\include $*;

all:    vddherc.386

hercinit.obj hercinit.lst: hercinit.asm ..\include\debug.inc \
	 ..\include\vdd.inc herc.inc ..\include\vmdaherc.inc \
	 ..\include\vmm.inc

hercint.obj hercint.lst: hercint.asm ..\include\debug.inc herc.inc \
	 ..\include\vmdaherc.inc ..\include\vdd.inc ..\include\vmm.inc

hercmem.obj hercmem.lst: hercmem.asm ..\include\debug.inc herc.inc \
	 ..\include\vmdaherc.inc ..\include\vdd.inc ..\include\vmm.inc

hercmsg.obj hercmsg.lst: hercmsg.asm ..\include\vmm.inc

hercproc.obj hercproc.lst: hercproc.asm ..\include\debug.inc \
	 ..\include\vddgrb.inc ..\include\shell.inc ..\include\shellfsc.inc \
	 herc.inc ..\include\vmdaherc.inc ..\include\vdd.inc \
	 ..\include\vmm.inc

hercsave.obj hercsave.lst: hercsave.asm ..\include\vdd.inc \
	 ..\include\debug.inc herc.inc ..\include\vmdaherc.inc \
	 ..\include\vmm.inc

hercstat.obj hercstat.lst: hercstat.asm ..\include\debug.inc herc.inc \
	 ..\include\vmdaherc.inc ..\include\vdd.inc ..\include\vmm.inc

hercsvc.obj hercsvc.lst: hercsvc.asm ..\include\debug.inc ..\include\shell.inc \
	 ..\include\shellfsc.inc herc.inc ..\include\vmdaherc.inc \
	 ..\include\vdd.inc ..\include\vmm.inc

herctio.obj herctio.lst: herctio.asm ..\include\debug.inc ..\include\vdd.inc \
	 herc.inc ..\include\vmdaherc.inc ..\include\vmm.inc


OBJS =  hercinit.obj hercmem.obj hercproc.obj hercstat.obj herctio.obj \
        hercint.obj hercmsg.obj hercsave.obj hercsvc.obj 

vddherc.386: vddherc.def $(OBJS)
        link386 @vddherc.lnk
        addhdr vddherc.386
        mapsym32 vddherc
